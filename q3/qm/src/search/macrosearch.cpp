/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>

#include <qsnew.h>
#include <qsstl.h>

#include "macrosearch.h"
#include "../ui/resourceinc.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MacroSearchDriver
 *
 */

qm::MacroSearchDriver::MacroSearchDriver(Document* pDocument,
	Account* pAccount, HWND hwnd, Profile* pProfile, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MacroSearchDriver::~MacroSearchDriver()
{
}

QSTATUS qm::MacroSearchDriver::search(
	const SearchContext& context, MessageHolderList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	// TODO
	// TYPE_SEARCH?
	MacroParser parser(MacroParser::TYPE_RULE, &status);
	CHECK_QSTATUS();
	Macro* p = 0;
	status = parser.parse(context.getCondition(), &p);
	CHECK_QSTATUS();
	std::auto_ptr<Macro> pMacro(p);
	
	SearchContext::FolderList listFolder;
	status = context.getTargetFolders(pAccount_, &listFolder);
	CHECK_QSTATUS();
	
	MacroVariableHolder globalVariable(&status);
	CHECK_QSTATUS();
	
	SearchContext::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end()) {
		NormalFolder* pFolder = *it;
		
		status = pFolder->loadMessageHolders();
		CHECK_QSTATUS();
		
		unsigned int nCount = pFolder->getCount();
		CHECK_QSTATUS();
		for (unsigned int n = 0; n < nCount; ++n) {
			MessageHolder* pmh = pFolder->getMessage(n);
			
			Message msg(&status);
			CHECK_QSTATUS();
			MacroContext::Init init = {
				pmh,
				&msg,
				pAccount_,
				pDocument_,
				hwnd_,
				pProfile_,
				false,
				0,
				&globalVariable
			};
			MacroContext context(init, &status);
			CHECK_QSTATUS();
			MacroValuePtr pValue;
			status = pMacro->value(&context, &pValue);
			CHECK_QSTATUS();
			if (pValue->boolean()) {
				status = STLWrapper<MessageHolderList>(*pList).push_back(pmh);
				CHECK_QSTATUS();
			}
		}
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroSearchUI
 *
 */

qm::MacroSearchUI::MacroSearchUI(Profile* pProfile, QSTATUS* pstatus) :
	pProfile_(pProfile)
{
}

qm::MacroSearchUI::~MacroSearchUI()
{
}

int qm::MacroSearchUI::getIndex()
{
	return 10;
}

const WCHAR* qm::MacroSearchUI::getName()
{
	return L"macro";
}

QSTATUS qm::MacroSearchUI::getDisplayName(qs::WSTRING* pwstrName)
{
	return loadString(Application::getApplication().getResourceHandle(),
		IDS_MACROSEARCH, pwstrName);
}

QSTATUS qm::MacroSearchUI::createPropertyPage(
	bool bAllFolder, SearchPropertyPage** ppPage)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<MacroSearchPage> pPage;
	status = newQsObject(pProfile_, bAllFolder, &pPage);
	CHECK_QSTATUS();
	*ppPage = pPage.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroSearchPage
 *
 */

qm::MacroSearchPage::MacroSearchPage(
	Profile* pProfile, bool bAllFolder, QSTATUS* pstatus) :
	SearchPropertyPage(Application::getApplication().getResourceHandle(),
		IDD_MACROSEARCH, pstatus),
	pProfile_(pProfile),
	wstrCondition_(0),
	bAllFolder_(bAllFolder),
	bRecursive_(false)
{
}

qm::MacroSearchPage::~MacroSearchPage()
{
	freeWString(wstrCondition_);
}

const WCHAR* qm::MacroSearchPage::getDriver() const
{
	return L"macro";
}

const WCHAR* qm::MacroSearchPage::getCondition() const
{
	return wstrCondition_;
}

bool qm::MacroSearchPage::isAllFolder() const
{
	return bAllFolder_;
}

bool qm::MacroSearchPage::isRecursive() const
{
	return bRecursive_;
}

LRESULT qm::MacroSearchPage::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_MACRO, onMacro)
		HANDLE_COMMAND_ID(IDC_MATCHCASE, onMatchCase)
		HANDLE_COMMAND_ID(IDC_SEARCHBODY, onSearchBody)
	END_COMMAND_HANDLER()
	return SearchPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::MacroSearchPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrCondition;
	status = pProfile_->getString(L"Search", L"Condition", L"", &wstrCondition);
	CHECK_QSTATUS();
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
	} items[] = {
		{ IDC_MACRO,		L"Macro"		},
		{ IDC_MATCHCASE,	L"MatchCase",	},
		{ IDC_SEARCHBODY,	L"SearchBody",	}
	};
	for (int n = 0; n < countof(items); ++n) {
		int nValue = 0;
		status = pProfile_->getInt(L"MacroSearch", items[n].pwszKey_, 0, &nValue);
		CHECK_QSTATUS();
		if (nValue != 0)
			sendDlgItemMessage(items[n].nId_, BM_SETCHECK, BST_CHECKED);
	}
	
	int nFolder = 0;
	if (bAllFolder_) {
		Window(getDlgItem(IDC_CURRENT)).enableWindow(false);
		Window(getDlgItem(IDC_RECURSIVE)).enableWindow(false);
		nFolder = 2;
	}
	else {
		status = pProfile_->getInt(L"Search", L"Folder", 0, &nFolder);
		CHECK_QSTATUS();
		if (nFolder < 0 || 3 < nFolder)
			nFolder = 0;
	}
	sendDlgItemMessage(IDC_CURRENT + nFolder, BM_SETCHECK, BST_CHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::MacroSearchPage::onOk()
{
	DECLARE_QSTATUS();
	
	if (PropSheet_GetCurrentPageHwnd(getSheet()->getHandle()) == getHandle()) {
		string_ptr<WSTRING> wstrSearch(getDlgItemText(IDC_CONDITION));
		bool bMacro = sendDlgItemMessage(IDC_MACRO, BM_GETCHECK) == BST_CHECKED;
		bool bCase = sendDlgItemMessage(IDC_MATCHCASE, BM_GETCHECK) == BST_CHECKED;
		bool bSearchBody = sendDlgItemMessage(IDC_SEARCHBODY, BM_GETCHECK) == BST_CHECKED;
		if (bMacro) {
			wstrCondition_ = wstrSearch.release();
		}
		else {
			string_ptr<WSTRING> wstrLiteral;
			status = getLiteral(wstrSearch.get(), &wstrLiteral);
			CHECK_QSTATUS();
			
			StringBuffer<WSTRING> buf(&status);
			CHECK_QSTATUS();
			status = buf.append(L"@Or(");
			CHECK_QSTATUS();
			const WCHAR* pwszFields[] = {
				L"%Subject",
				L"%From",
				L"%To"
			};
			for (int n = 0; n < countof(pwszFields); ++n) {
				if (n != 0) {
					status = buf.append(L", ");
					CHECK_QSTATUS();
				}
				status = createMacro(&buf, pwszFields[n],
					wstrLiteral.get(), bCase);
				CHECK_QSTATUS();
			}
			if (bSearchBody) {
				status = createMacro(&buf, L"@Body('', @True())",
					wstrLiteral.get(), bCase);
				CHECK_QSTATUS();
			}
			status = buf.append(L")");
			CHECK_QSTATUS();
			
			wstrCondition_ = buf.getString();
		}
		
		bAllFolder_ = sendDlgItemMessage(IDC_ALLFOLDER, BM_GETCHECK) == BST_CHECKED;
		bRecursive_ = sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED;
		
		pProfile_->setString(L"Search", L"Condition",
			bMacro ? wstrCondition_ : wstrSearch.get());
		pProfile_->setInt(L"MacroSearch", L"Macro", bMacro);
		pProfile_->setInt(L"MacroSearch", L"MatchCase", bCase);
		pProfile_->setInt(L"MacroSearch", L"SearchBody", bSearchBody);
		pProfile_->setInt(L"Search", L"Folder",
			bAllFolder_ ? 2 : bRecursive_ ? 1 : 0);
	}
	return SearchPropertyPage::onOk();
}

LRESULT qm::MacroSearchPage::onMacro()
{
	updateState();
	return 0;
}

LRESULT qm::MacroSearchPage::onMatchCase()
{
	updateState();
	return 0;
}

LRESULT qm::MacroSearchPage::onSearchBody()
{
	updateState();
	return 0;
}

void qm::MacroSearchPage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_MACRO, BM_GETCHECK) != BST_CHECKED;
	Window(getDlgItem(IDC_MATCHCASE)).enableWindow(bEnable);
	Window(getDlgItem(IDC_SEARCHBODY)).enableWindow(bEnable);
}

QSTATUS qm::MacroSearchPage::getLiteral(const WCHAR* pwsz, WSTRING* pwstr)
{
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	status = buf.append(L'\'');
	CHECK_QSTATUS();
	for (const WCHAR* p = pwsz; *p; ++p) {
		if (*p == L'\'') {
			status = buf.append(L'\\');
			CHECK_QSTATUS();
		}
		status = buf.append(*p);
		CHECK_QSTATUS();
	}
	status = buf.append(L'\'');
	CHECK_QSTATUS();
	
	*pwstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroSearchPage::createMacro(StringBuffer<WSTRING>* pBuf,
	const WCHAR* pwszField, const WCHAR* pwszLiteral, bool bCase)
{
	DECLARE_QSTATUS();
	
	status = pBuf->append(L"@Contain(");
	CHECK_QSTATUS();
	status = pBuf->append(pwszField);
	CHECK_QSTATUS();
	status = pBuf->append(L", ");
	CHECK_QSTATUS();
	status = pBuf->append(pwszLiteral);
	CHECK_QSTATUS();
	status = pBuf->append(L", ");
	CHECK_QSTATUS();
	status = pBuf->append(bCase ? L"@True()" : L"@False()");
	CHECK_QSTATUS();
	status = pBuf->append(L')');
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroSearchDriverFactory
 *
 */

MacroSearchDriverFactory* qm::MacroSearchDriverFactory::pFactory__;
MacroSearchDriverFactory::InitializerImpl qm::MacroSearchDriverFactory::init__;

qm::MacroSearchDriverFactory::MacroSearchDriverFactory()
{
	regist(L"macro", this);
}

qm::MacroSearchDriverFactory::~MacroSearchDriverFactory()
{
	unregist(L"macro");
}

QSTATUS qm::MacroSearchDriverFactory::createDriver(Document* pDocument,
	Account* pAccount, HWND hwnd, Profile* pProfile, SearchDriver** ppDriver)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<MacroSearchDriver> pDriver;
	status = newQsObject(pDocument, pAccount, hwnd, pProfile, &pDriver);
	CHECK_QSTATUS();
	*ppDriver = pDriver.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroSearchDriverFactory::createUI(
	Account* pAccount, Profile* pProfile, SearchUI** ppUI)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<MacroSearchUI> pUI;
	status = newQsObject(pProfile, &pUI);
	CHECK_QSTATUS();
	*ppUI = pUI.release();
	
	return QSTATUS_SUCCESS;
}

qm::MacroSearchDriverFactory::InitializerImpl::InitializerImpl()
{
}

qm::MacroSearchDriverFactory::InitializerImpl::~InitializerImpl()
{
}

QSTATUS qm::MacroSearchDriverFactory::InitializerImpl::init()
{
	DECLARE_QSTATUS();
	
	status = newObject(&pFactory__);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroSearchDriverFactory::InitializerImpl::term()
{
	delete pFactory__;
	pFactory__ = 0;
	
	return QSTATUS_SUCCESS;
}
