/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmuiutil.h>

#include <qsstl.h>
#include <qsuiutil.h>

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
										 Account* pAccount,
										 HWND hwnd,
										 Profile* pProfile) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MacroSearchDriver::~MacroSearchDriver()
{
}

bool qm::MacroSearchDriver::search(const SearchContext& context,
								   MessageHolderList* pList)
{
	assert(pList);
	
	MacroParser parser;
	std::auto_ptr<Macro> pMacro(parser.parse(context.getCondition()));
	if (!pMacro.get())
		return false;
	
	SearchContext::FolderList listFolder;
	context.getTargetFolders(pAccount_, &listFolder);
	
	MacroVariableHolder globalVariable;
	
	for (SearchContext::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		NormalFolder* pFolder = *it;
		
		if (!pFolder->loadMessageHolders())
			return false;
		
		unsigned int nCount = pFolder->getCount();
		for (unsigned int n = 0; n < nCount; ++n) {
			MessageHolder* pmh = pFolder->getMessage(n);
			
			Message msg;
			MacroContext context(pmh, &msg, pAccount_,  MessageHolderList(),
				pFolder, pDocument_, hwnd_, pProfile_, 0,
				MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
				context.getSecurityMode(), 0, &globalVariable);
			MacroValuePtr pValue(pMacro->value(&context));
			if (pValue.get() && pValue->boolean())
				pList->push_back(pmh);
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * MacroSearchUI
 *
 */

qm::MacroSearchUI::MacroSearchUI(Profile* pProfile) :
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

wstring_ptr qm::MacroSearchUI::getDisplayName()
{
	return loadString(Application::getApplication().getResourceHandle(), IDS_TITLE_MACROSEARCH);
}

std::auto_ptr<SearchPropertyPage> qm::MacroSearchUI::createPropertyPage(bool bAllFolder,
																		SearchPropertyData* pData)
{
	return std::auto_ptr<SearchPropertyPage>(new MacroSearchPage(pProfile_, bAllFolder, pData));
}


/****************************************************************************
 *
 * MacroSearchPage
 *
 */

qm::MacroSearchPage::MacroSearchPage(Profile* pProfile,
									 bool bAllFolderOnly,
									 SearchPropertyData* pData) :
	SearchPropertyPage(Application::getApplication().getResourceHandle(), IDD_MACROSEARCH, pData),
	pProfile_(pProfile),
	bAllFolderOnly_(bAllFolderOnly)
{
}

qm::MacroSearchPage::~MacroSearchPage()
{
}

const WCHAR* qm::MacroSearchPage::getDriver() const
{
	return L"macro";
}

const WCHAR* qm::MacroSearchPage::getCondition() const
{
	return wstrCondition_.get();
}

void qm::MacroSearchPage::updateData(SearchPropertyData* pData)
{
	wstring_ptr wstrCondition = getDlgItemText(IDC_CONDITION);
	pData->set(wstrCondition.get(),
		sendDlgItemMessage(IDC_ALLFOLDER, BM_GETCHECK) == BST_CHECKED,
		sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED,
		sendDlgItemMessage(IDC_NEWFOLDER, BM_GETCHECK) == BST_CHECKED, getImeFlags());
}

void qm::MacroSearchPage::updateUI(const SearchPropertyData* pData)
{
	if (pData->getCondition()) {
		setDlgItemText(IDC_CONDITION, pData->getCondition());
		
		UINT nId = pData->isAllFolder() ? IDC_ALLFOLDER :
			pData->isRecursive() ? IDC_RECURSIVE : IDC_CURRENT;
		for (UINT n = IDC_CURRENT; n < IDC_CURRENT + 3; ++n)
			sendDlgItemMessage(n, BM_SETCHECK, n == nId ? BST_CHECKED : BST_UNCHECKED);
		
		sendDlgItemMessage(IDC_NEWFOLDER, BM_SETCHECK, pData->isNewFolder() ? BST_CHECKED : BST_UNCHECKED);
	}
	setImeFlags(pData->getImeFlags());
}

LRESULT qm::MacroSearchPage::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_MACRO, onMacro)
	END_COMMAND_HANDLER()
	return SearchPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::MacroSearchPage::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	History history(pProfile_, L"Search");
	for (unsigned int n = 0; n < history.getSize(); ++n) {
		wstring_ptr wstr(history.getValue(n));
		if (*wstr.get()) {
			W2T(wstr.get(), ptsz);
			sendDlgItemMessage(IDC_CONDITION, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptsz));
		}
	}
	if (sendDlgItemMessage(IDC_CONDITION, CB_GETCOUNT) != 0)
		sendDlgItemMessage(IDC_CONDITION, CB_SETCURSEL, 0);
	
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
	} items[] = {
		{ IDC_MACRO,		L"Macro"		},
		{ IDC_MATCHCASE,	L"MatchCase"	},
		{ IDC_SEARCHHEADER,	L"SearchHeader"	},
		{ IDC_SEARCHBODY,	L"SearchBody"	}
	};
	for (int n = 0; n < countof(items); ++n) {
		int nValue = pProfile_->getInt(L"MacroSearch", items[n].pwszKey_);
		if (nValue != 0)
			sendDlgItemMessage(items[n].nId_, BM_SETCHECK, BST_CHECKED);
	}
	
	if (bAllFolderOnly_) {
		Window(getDlgItem(IDC_CURRENT)).enableWindow(false);
		Window(getDlgItem(IDC_RECURSIVE)).enableWindow(false);
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::MacroSearchPage::onOk()
{
	if (PropSheet_GetCurrentPageHwnd(getSheet()->getHandle()) == getHandle()) {
		wstring_ptr wstrSearch = getDlgItemText(IDC_CONDITION);
		if (*wstrSearch.get()) {
			History(pProfile_, L"Search").addValue(wstrSearch.get());
			bool bMacro = sendDlgItemMessage(IDC_MACRO, BM_GETCHECK) == BST_CHECKED;
			bool bCase = sendDlgItemMessage(IDC_MATCHCASE, BM_GETCHECK) == BST_CHECKED;
			bool bSearchHeader = sendDlgItemMessage(IDC_SEARCHHEADER, BM_GETCHECK) == BST_CHECKED;
			bool bSearchBody = sendDlgItemMessage(IDC_SEARCHBODY, BM_GETCHECK) == BST_CHECKED;
			if (bMacro) {
				wstrCondition_ = wstrSearch;
			}
			else {
				wstring_ptr wstrMacro(pProfile_->getString(L"MacroSearch", L"SearchMacro"));
				wstring_ptr wstrLiteral(getLiteral(wstrSearch.get()));
				
				StringBuffer<WSTRING> buf;
				buf.append(L"@Progn(@Set('Search', ");
				buf.append(wstrLiteral.get());
				buf.append(L"), @Set('Case', ");
				buf.append(bCase ? L'1' : L'0');
				buf.append(L"), ");
				if (bSearchHeader || bSearchBody)
					buf.append(L"@Or(");
				buf.append(wstrMacro.get());
				if (bSearchHeader && bSearchBody)
					buf.append(L", @Contain(@Decode(@Header()), $Search, $Case), @Contain(@Body(), $Search, $Case))");
				else if (bSearchHeader)
					buf.append(L", @Contain(@Decode(@Header()), $Search, $Case))");
				else if (bSearchBody)
					buf.append(L", @Contain(@Body(), $Search, $Case))");
				buf.append(L")");
				
				wstrCondition_ = buf.getString();
			}
			
			pProfile_->setInt(L"MacroSearch", L"Macro", bMacro);
			pProfile_->setInt(L"MacroSearch", L"MatchCase", bCase);
			pProfile_->setInt(L"MacroSearch", L"SearchHeader", bSearchHeader);
			pProfile_->setInt(L"MacroSearch", L"SearchBody", bSearchBody);
		}
	}
	return SearchPropertyPage::onOk();
}

LRESULT qm::MacroSearchPage::onMacro()
{
	updateState();
	return 0;
}

void qm::MacroSearchPage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_MACRO, BM_GETCHECK) != BST_CHECKED;
	Window(getDlgItem(IDC_MATCHCASE)).enableWindow(bEnable);
	Window(getDlgItem(IDC_SEARCHHEADER)).enableWindow(bEnable);
	Window(getDlgItem(IDC_SEARCHBODY)).enableWindow(bEnable);
}

wstring_ptr qm::MacroSearchPage::getLiteral(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	
	buf.append(L'\'');
	for (const WCHAR* p = pwsz; *p; ++p) {
		if (*p == L'\'')
			buf.append(L'\\');
		buf.append(*p);
	}
	buf.append(L'\'');
	
	return buf.getString();
}

void qm::MacroSearchPage::createMacro(StringBuffer<WSTRING>* pBuf,
									  const WCHAR* pwszField,
									  const WCHAR* pwszLiteral,
									  bool bCase)
{
	pBuf->append(L"@Contain(");
	pBuf->append(pwszField);
	pBuf->append(L", ");
	pBuf->append(pwszLiteral);
	pBuf->append(L", ");
	pBuf->append(bCase ? L"@True()" : L"@False()");
	pBuf->append(L')');
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
	registerFactory(L"macro", this);
}

qm::MacroSearchDriverFactory::~MacroSearchDriverFactory()
{
	unregisterFactory(L"macro");
}

std::auto_ptr<SearchDriver> qm::MacroSearchDriverFactory::createDriver(Document* pDocument,
																	   Account* pAccount,
																	   HWND hwnd,
																	   Profile* pProfile)
{
	return std::auto_ptr<SearchDriver>(new MacroSearchDriver(
		pDocument, pAccount, hwnd, pProfile));
}

std::auto_ptr<SearchUI> qm::MacroSearchDriverFactory::createUI(Account* pAccount,
															   Profile* pProfile)
{
	return std::auto_ptr<SearchUI>(new MacroSearchUI(pProfile));
}

qm::MacroSearchDriverFactory::InitializerImpl::InitializerImpl()
{
}

qm::MacroSearchDriverFactory::InitializerImpl::~InitializerImpl()
{
}

bool qm::MacroSearchDriverFactory::InitializerImpl::init()
{
	pFactory__ = new MacroSearchDriverFactory();
	return true;
}

void qm::MacroSearchDriverFactory::InitializerImpl::term()
{
	delete pFactory__;
	pFactory__ = 0;
}
