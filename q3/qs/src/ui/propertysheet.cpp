/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsdialog.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsconv.h>

#include <algorithm>

#include "dialog.h"

using namespace qs;


/****************************************************************************
 *
 * PropertyPageImpl
 *
 */

struct qs::PropertyPageImpl
{
	UINT nId_;
	PROPSHEETPAGE psp_;
	HPROPSHEETPAGE hpsp_;
	PropertySheetBase* pSheet_;
};


/****************************************************************************
 *
 * PropertySheetBaseImpl
 *
 */

class qs::PropertySheetBaseImpl
{
public:
	typedef ControllerMap<PropertySheetBase> PropertySheetMap;
	typedef std::vector<PropertySheetBase*> ModelessList;
	typedef std::vector<PropertyPage*> PageList;

public:
	static QSTATUS getPropertySheetMap(PropertySheetMap** ppMap);
	
	static QSTATUS getModelessList(const ModelessList** ppList);
	static QSTATUS addModelessPropertySheet(
		PropertySheetBase* pPropertySheetBase);
	static QSTATUS removeModelessPropertySheet(
		PropertySheetBase* pPropertySheetBase);

private:
	PropertySheetBase* pThis_;
	bool bDeleteThis_;
	PROPSHEETHEADER psh_;
	PageList listPage_;
	bool bInit_;

private:
	static PropertySheetMap* pMap__;
	static ThreadLocal* pModelessList__;
	static class InitializerImpl : public Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual QSTATUS init();
		virtual QSTATUS term();
		virtual QSTATUS initThread();
		virtual QSTATUS termThread();
	} init__;

friend class InitializerImpl;
friend class PropertySheetBase;
};

PropertySheetBaseImpl::PropertySheetMap* qs::PropertySheetBaseImpl::pMap__;
ThreadLocal* qs::PropertySheetBaseImpl::pModelessList__;
PropertySheetBaseImpl::InitializerImpl qs::PropertySheetBaseImpl::init__;

QSTATUS qs::PropertySheetBaseImpl::getPropertySheetMap(PropertySheetMap** ppMap)
{
	assert(ppMap);
	*ppMap = pMap__;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PropertySheetBaseImpl::getModelessList(const ModelessList** ppList)
{
	assert(ppList);
	
	DECLARE_QSTATUS();
	
	*ppList = 0;
	
	void* pValue = 0;
	status = pModelessList__->get(&pValue);
	CHECK_QSTATUS();
	
	*ppList = static_cast<ModelessList*>(pValue);
	assert(*ppList);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PropertySheetBaseImpl::addModelessPropertySheet(
	PropertySheetBase* pPropertySheetBase)
{
	DECLARE_QSTATUS();
	
	void* pValue = 0;
	status = pModelessList__->get(&pValue);
	CHECK_QSTATUS();
	
	ModelessList* pList = static_cast<ModelessList*>(pValue);
	assert(pList);
	
	return STLWrapper<ModelessList>(*pList).push_back(pPropertySheetBase);
}

QSTATUS qs::PropertySheetBaseImpl::removeModelessPropertySheet(
	PropertySheetBase* pPropertySheetBase)
{
	DECLARE_QSTATUS();
	
	void* pValue = 0;
	status = pModelessList__->get(&pValue);
	CHECK_QSTATUS();
	
	ModelessList* pList = static_cast<ModelessList*>(pValue);
	assert(pList);
	
	ModelessList::iterator it = std::remove(
		pList->begin(), pList->end(), pPropertySheetBase);
	pList->erase(it, pList->end());
	
	return QSTATUS_SUCCESS;
}

qs::PropertySheetBaseImpl::InitializerImpl::InitializerImpl()
{
}

qs::PropertySheetBaseImpl::InitializerImpl::~InitializerImpl()
{
}

QSTATUS qs::PropertySheetBaseImpl::InitializerImpl::init()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&PropertySheetBaseImpl::pMap__);
	CHECK_QSTATUS();
	
	status = newQsObject(&PropertySheetBaseImpl::pModelessList__);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PropertySheetBaseImpl::InitializerImpl::term()
{
	delete PropertySheetBaseImpl::pModelessList__;
	PropertySheetBaseImpl::pModelessList__ = 0;
	
	delete PropertySheetBaseImpl::pMap__;
	PropertySheetBaseImpl::pMap__ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PropertySheetBaseImpl::InitializerImpl::initThread()
{
	DECLARE_QSTATUS();
	
	status = PropertySheetBaseImpl::pMap__->initThread();
	CHECK_QSTATUS();
	
	ModelessList* pList = 0;
	status = newObject(&pList);
	CHECK_QSTATUS();
	assert(pList);
	std::auto_ptr<ModelessList> apList(pList);
	status = pModelessList__->set(pList);
	CHECK_QSTATUS();
	apList.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PropertySheetBaseImpl::InitializerImpl::termThread()
{
	void* pValue = 0;
	QSTATUS status = pModelessList__->get(&pValue);
	if (status == QSTATUS_SUCCESS)
		delete static_cast<ModelessList*>(pValue);
	
	PropertySheetBaseImpl::pMap__->termThread();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * PropertySheetBase
 *
 */

qs::PropertySheetBase::PropertySheetBase(HINSTANCE hInstResource,
	const WCHAR* pwszTitle, bool bDeleteThis, QSTATUS* pstatus) :
	Window(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	TSTRING tstrTitle = wcs2tcs(pwszTitle);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	assert(pImpl_);
	pImpl_->pThis_ = this;
	pImpl_->bDeleteThis_ = bDeleteThis;
	memset(&pImpl_->psh_, 0, sizeof(pImpl_->psh_));
	pImpl_->psh_.dwSize = sizeof(pImpl_->psh_);
	pImpl_->psh_.dwFlags = PSH_DEFAULT | PSH_USECALLBACK | PSH_NOAPPLYNOW;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	pImpl_->psh_.dwFlags |= PSH_MAXIMIZE;
#endif
	pImpl_->psh_.hInstance = hInstResource;
	pImpl_->psh_.pszCaption = tstrTitle;
	pImpl_->psh_.nPages = 0;
	pImpl_->psh_.nStartPage = 0;
	pImpl_->psh_.pfnCallback = propertySheetProc;
	pImpl_->bInit_ = false;
}

qs::PropertySheetBase::~PropertySheetBase()
{
	PropertySheetBaseImpl::PropertySheetMap* pMap = 0;
	PropertySheetBaseImpl::getPropertySheetMap(&pMap);
	pMap->removeController(getHandle());
	
	if (pImpl_) {
		freeTString(const_cast<TSTRING>(pImpl_->psh_.pszCaption));
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::PropertySheetBase::doModal(HWND hwndParent,
	ModalHandler* pModalHandler, int* pnRet)
{
	assert(pnRet);
	
	DECLARE_QSTATUS();
	
	if (pModalHandler)
		pModalHandler = getModalHandler();
	
	auto_ptr_array<HPROPSHEETPAGE> aphpsp;
	status = newArray(pImpl_->listPage_.size(), &aphpsp);
	CHECK_QSTATUS();
	
	pImpl_->psh_.hwndParent = hwndParent;
	pImpl_->psh_.phpage = aphpsp.get();
	
	int n = 0;
	PropertySheetBaseImpl::PageList::iterator it = pImpl_->listPage_.begin();
	while (it != pImpl_->listPage_.end())
		aphpsp[n++] = (*it++)->pImpl_->hpsp_;
	
	ModalHandlerInvoker(pModalHandler, hwndParent, &status);
	CHECK_QSTATUS();
	
	PropertySheetBaseImpl::PropertySheetMap* pMap = 0;
	status = PropertySheetBaseImpl::getPropertySheetMap(&pMap);
	CHECK_QSTATUS();
	pMap->setThis(this);
	*pnRet = ::PropertySheet(&pImpl_->psh_);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PropertySheetBase::add(PropertyPage* pPage)
{
	assert(pPage);
	
	DECLARE_QSTATUS();
	
	HPROPSHEETPAGE hpsp = 0;
	status = pPage->create(this, &hpsp);
	CHECK_QSTATUS();
	
	if (getHandle())
		PropSheet_AddPage(getHandle(), hpsp);
	++pImpl_->psh_.nPages;
	status = STLWrapper<PropertySheetBaseImpl::PageList>
		(pImpl_->listPage_).push_back(pPage);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PropertySheetBase::setStartPage(int nPage)
{
	pImpl_->psh_.nStartPage = nPage;
	return QSTATUS_SUCCESS;
}

PropertyPage* qs::PropertySheetBase::getPage(int nPage)
{
	assert(0 <= nPage && static_cast<unsigned int>(nPage) < pImpl_->listPage_.size());
	return pImpl_->listPage_[nPage];
}

void qs::PropertySheetBase::init()
{
	if (!pImpl_->bInit_) {
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
		centerWindow();
#endif
		pImpl_->bInit_ = true;
	}
}

bool qs::PropertySheetBase::isDialogMessage(MSG* pMsg)
{
	assert(getHandle());
	return sendMessage(PSM_ISDIALOGMESSAGE, 0, reinterpret_cast<LPARAM>(pMsg)) != 0;
}

QSTATUS qs::PropertySheetBase::processDialogMessage(
	MSG* pMsg, bool* pbProcessed)
{
	assert(pbProcessed);
	
	*pbProcessed = false;
	
	if ((pMsg->message < WM_KEYFIRST || WM_KEYLAST < pMsg->message) &&
		(pMsg->message < WM_MOUSEFIRST || WM_MOUSELAST < pMsg->message))
		return QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	const PropertySheetBaseImpl::ModelessList* pList = 0;
	status = PropertySheetBaseImpl::getModelessList(&pList);
	CHECK_QSTATUS();
	
	PropertySheetBaseImpl::ModelessList::const_iterator it = pList->begin();
	while (it != pList->end() && !(*it)->isDialogMessage(pMsg))
		++it;
	*pbProcessed = it != pList->end();
	
	return QSTATUS_SUCCESS;
}

int CALLBACK qs::propertySheetProc(HWND hwnd, UINT uMsg, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	switch (uMsg) {
	case PSCB_INITIALIZED:
		{
			PropertySheetBaseImpl::PropertySheetMap* pMap = 0;
			status = PropertySheetBaseImpl::getPropertySheetMap(&pMap);
			CHECK_QSTATUS_VALUE(0);
			PropertySheetBase* pThis = 0;
			status = pMap->getThis(&pThis);
			CHECK_QSTATUS_VALUE(0);
			assert(pThis);
			status = pMap->setController(hwnd, pThis);
			CHECK_QSTATUS_VALUE(0);
			pThis->setHandle(hwnd);
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
			pThis->centerWindow();
#endif
			status = pMap->setThis(0);
			CHECK_QSTATUS_VALUE(0);
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
			Window wndTab(::GetDlgItem(hwnd, 0x3020));
			wndTab.setStyle(TCS_BOTTOM, TCS_BOTTOM);
#endif
		}
		break;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	case PSCB_GETVERSION:
		return COMCTL32_VERSION;
#endif
	}
	return 0;
}


/****************************************************************************
 *
 * PropertyPage
 *
 */

qs::PropertyPage::PropertyPage(HINSTANCE hInstResource,
	UINT nId, bool bDeleteThis, QSTATUS* pstatus) :
	DialogBase(bDeleteThis, pstatus),
	pImpl_(0)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	assert(pImpl_);
	pImpl_->nId_ = nId;
	memset(&pImpl_->psp_, 0, sizeof(pImpl_->psp_));
	pImpl_->psp_.dwSize = sizeof(pImpl_->psp_);
	pImpl_->psp_.dwFlags = PSP_DEFAULT;
	pImpl_->psp_.hInstance = hInstResource;
	pImpl_->psp_.pszTemplate = MAKEINTRESOURCE(nId);
	pImpl_->psp_.pfnDlgProc = propertyPageProc;
	pImpl_->hpsp_ = 0;
	pImpl_->pSheet_ = 0;
}

qs::PropertyPage::~PropertyPage()
{
	delete pImpl_;
}

PropertySheetBase* qs::PropertyPage::getSheet() const
{
	return pImpl_->pSheet_;
}

QSTATUS qs::PropertyPage::create(
	PropertySheetBase* pSheet, HPROPSHEETPAGE* phpsp)
{
	assert(pImpl_);
	assert(!pImpl_->hpsp_);
	assert(pSheet);
	assert(phpsp);
	
	pImpl_->hpsp_ = ::CreatePropertySheetPage(&pImpl_->psp_);
	if (!pImpl_->hpsp_)
		return QSTATUS_FAIL;
	
	pImpl_->pSheet_ = pSheet;
	*phpsp = pImpl_->hpsp_;
	
	return QSTATUS_SUCCESS;
}

INT_PTR CALLBACK qs::propertyPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	DialogBaseImpl::DialogMap* pMap = 0;
	status = DialogBaseImpl::getDialogMap(&pMap);
	CHECK_QSTATUS();
	DialogBase* pThis = 0;
//	status = pMap->findController(hwnd, &pThis);
//	CHECK_QSTATUS_VALUE(0);
	status = pMap->getController(hwnd, &pThis);
	CHECK_QSTATUS_VALUE(0);
	if (!pThis) {
		HWND hwndSheet = ::GetParent(hwnd);
		assert(hwndSheet);
		PropertySheetBaseImpl::PropertySheetMap* pSheetMap = 0;
		status = PropertySheetBaseImpl::getPropertySheetMap(&pSheetMap);
		CHECK_QSTATUS_VALUE(0);
		PropertySheetBase* pSheet = 0;
		status = pSheetMap->getController(hwndSheet, &pSheet);
		CHECK_QSTATUS_VALUE(0);
		if (pSheet) {
			HWND hwndTab = PropSheet_GetTabControl(hwndSheet);
			int nPage = TabCtrl_GetCurSel(hwndTab);
			pThis = pSheet->getPage(nPage);
			if (pThis) {
				status = pMap->setController(hwnd, pThis);
				CHECK_QSTATUS_VALUE(0);
				pThis->setHandle(hwnd);
			}
		}
	}
	
	INT_PTR nResult = 0;
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndSheet = pThis->getParent();
			assert(hwndSheet);
			PropertySheetBaseImpl::PropertySheetMap* pSheetMap = 0;
			status = PropertySheetBaseImpl::getPropertySheetMap(&pSheetMap);
			CHECK_QSTATUS_VALUE(0);
			PropertySheetBase* pSheet = 0;
			status = pSheetMap->getController(hwndSheet, &pSheet);
			CHECK_QSTATUS_VALUE(0);
			if (pSheet)
				pSheet->init();
		}
		break;
	
	default:
		break;
	}
	
	return pThis->pImpl_->dialogProc(uMsg, wParam, lParam);
}


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

qs::DefaultPropertyPage::DefaultPropertyPage(
	HINSTANCE hInst, UINT nId, QSTATUS* pstatus) :
	PropertyPage(hInst, nId, false, pstatus),
	DefaultDialogHandler(pstatus),
	DefaultCommandHandler(pstatus)
{
	DECLARE_QSTATUS();
	
	status = addCommandHandler(this);
	CHECK_QSTATUS_SET(pstatus);
	status = addNotifyHandler(this);
	CHECK_QSTATUS_SET(pstatus);
	
	setDialogHandler(this, false);
}

qs::DefaultPropertyPage::~DefaultPropertyPage()
{
}

INT_PTR qs::DefaultPropertyPage::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qs::DefaultPropertyPage::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

LRESULT qs::DefaultPropertyPage::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qs::DefaultPropertyPage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qs::DefaultPropertyPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	return TRUE;
}

LRESULT qs::DefaultPropertyPage::onOk()
{
	return 0;
}

LRESULT qs::DefaultPropertyPage::onApply(NMHDR* pnmhdr, bool* pbHandled)
{
	return onOk();
}
