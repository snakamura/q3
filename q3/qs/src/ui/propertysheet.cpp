/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsdialog.h>
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
	static PropertySheetMap* getPropertySheetMap();
	
	static const ModelessList* getModelessList();
	static void addModelessPropertySheet(PropertySheetBase* pPropertySheetBase);
	static void removeModelessPropertySheet(PropertySheetBase* pPropertySheetBase);

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
		virtual bool init();
		virtual void term();
		virtual bool initThread();
		virtual void termThread();
	} init__;

friend class InitializerImpl;
friend class PropertySheetBase;
};

PropertySheetBaseImpl::PropertySheetMap* qs::PropertySheetBaseImpl::pMap__;
ThreadLocal* qs::PropertySheetBaseImpl::pModelessList__;
PropertySheetBaseImpl::InitializerImpl qs::PropertySheetBaseImpl::init__;

PropertySheetBaseImpl::PropertySheetMap* qs::PropertySheetBaseImpl::getPropertySheetMap()
{
	return pMap__;
}

const PropertySheetBaseImpl::ModelessList* qs::PropertySheetBaseImpl::getModelessList()
{
	return static_cast<ModelessList*>(pModelessList__->get());
}

void qs::PropertySheetBaseImpl::addModelessPropertySheet(PropertySheetBase* pPropertySheetBase)
{
	ModelessList* pList = static_cast<ModelessList*>(pModelessList__->get());
	pList->push_back(pPropertySheetBase);
}

void qs::PropertySheetBaseImpl::removeModelessPropertySheet(PropertySheetBase* pPropertySheetBase)
{
	ModelessList* pList = static_cast<ModelessList*>(pModelessList__->get());
	ModelessList::iterator it = std::remove(
		pList->begin(), pList->end(), pPropertySheetBase);
	pList->erase(it, pList->end());
}

qs::PropertySheetBaseImpl::InitializerImpl::InitializerImpl()
{
}

qs::PropertySheetBaseImpl::InitializerImpl::~InitializerImpl()
{
}

bool qs::PropertySheetBaseImpl::InitializerImpl::init()
{
	PropertySheetBaseImpl::pMap__ = new PropertySheetBaseImpl::PropertySheetMap();
	PropertySheetBaseImpl::pModelessList__ = new ThreadLocal();
	return true;
}

void qs::PropertySheetBaseImpl::InitializerImpl::term()
{
	delete PropertySheetBaseImpl::pModelessList__;
	PropertySheetBaseImpl::pModelessList__ = 0;
	
	delete PropertySheetBaseImpl::pMap__;
	PropertySheetBaseImpl::pMap__ = 0;
}

bool qs::PropertySheetBaseImpl::InitializerImpl::initThread()
{
	if (!PropertySheetBaseImpl::pMap__->initThread())
		return false;
	
	pModelessList__->set(new ModelessList());
	
	return true;
}

void qs::PropertySheetBaseImpl::InitializerImpl::termThread()
{
	delete static_cast<ModelessList*>(pModelessList__->get());
	PropertySheetBaseImpl::pMap__->termThread();
}


/****************************************************************************
 *
 * PropertySheetBase
 *
 */

qs::PropertySheetBase::PropertySheetBase(HINSTANCE hInstResource,
										 const WCHAR* pwszTitle,
										 bool bDeleteThis) :
	Window(0)
{
	tstring_ptr tstrTitle(wcs2tcs(pwszTitle));
	
	pImpl_ = new PropertySheetBaseImpl();
	pImpl_->pThis_ = this;
	pImpl_->bDeleteThis_ = bDeleteThis;
	memset(&pImpl_->psh_, 0, sizeof(pImpl_->psh_));
	pImpl_->psh_.dwSize = sizeof(pImpl_->psh_);
	pImpl_->psh_.dwFlags = PSH_DEFAULT | PSH_USECALLBACK | PSH_NOAPPLYNOW;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	pImpl_->psh_.dwFlags |= PSH_MAXIMIZE;
#endif
	pImpl_->psh_.hInstance = hInstResource;
	pImpl_->psh_.pszCaption = tstrTitle.release();
	pImpl_->psh_.nPages = 0;
	pImpl_->psh_.nStartPage = 0;
	pImpl_->psh_.pfnCallback = propertySheetProc;
	pImpl_->bInit_ = false;
}

qs::PropertySheetBase::~PropertySheetBase()
{
	PropertySheetBaseImpl::PropertySheetMap* pMap =
		PropertySheetBaseImpl::getPropertySheetMap();
	pMap->removeController(getHandle());
	
	if (pImpl_) {
		freeTString(const_cast<TSTRING>(pImpl_->psh_.pszCaption));
		delete pImpl_;
		pImpl_ = 0;
	}
}

INT_PTR qs::PropertySheetBase::doModal(HWND hwndParent)
{
	return doModal(hwndParent, 0);
}

INT_PTR qs::PropertySheetBase::doModal(HWND hwndParent,
									   ModalHandler* pModalHandler)
{
	if (!pModalHandler)
		pModalHandler = getModalHandler();
	
	auto_ptr_array<HPROPSHEETPAGE> aphpsp(new HPROPSHEETPAGE[pImpl_->listPage_.size()]);
	
	pImpl_->psh_.hwndParent = hwndParent;
	pImpl_->psh_.phpage = aphpsp.get();
	
	int n = 0;
	PropertySheetBaseImpl::PageList::iterator it = pImpl_->listPage_.begin();
	while (it != pImpl_->listPage_.end())
		aphpsp[n++] = (*it++)->pImpl_->hpsp_;
	
	ModalHandlerInvoker invoker(pModalHandler, hwndParent);
	
	PropertySheetBaseImpl::PropertySheetMap* pMap = 
		PropertySheetBaseImpl::getPropertySheetMap();
	pMap->setThis(this);
	
	return ::PropertySheet(&pImpl_->psh_);
}

void qs::PropertySheetBase::add(PropertyPage* pPage)
{
	assert(pPage);
	
	HPROPSHEETPAGE hpsp = pPage->create(this);
	
	if (getHandle())
		PropSheet_AddPage(getHandle(), hpsp);
	++pImpl_->psh_.nPages;
	pImpl_->listPage_.push_back(pPage);
}

void qs::PropertySheetBase::setStartPage(int nPage)
{
	pImpl_->psh_.nStartPage = nPage;
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

bool qs::PropertySheetBase::processDialogMessage(MSG* pMsg)
{
	if ((pMsg->message < WM_KEYFIRST || WM_KEYLAST < pMsg->message) &&
		(pMsg->message < WM_MOUSEFIRST || WM_MOUSELAST < pMsg->message))
		return false;
	
	const PropertySheetBaseImpl::ModelessList* pList =
		PropertySheetBaseImpl::getModelessList();
	
	PropertySheetBaseImpl::ModelessList::const_iterator it = pList->begin();
	while (it != pList->end() && !(*it)->isDialogMessage(pMsg))
		++it;
	return it != pList->end();
}

int CALLBACK qs::propertySheetProc(HWND hwnd,
								   UINT uMsg,
								   LPARAM lParam)
{
	switch (uMsg) {
	case PSCB_INITIALIZED:
		{
			PropertySheetBaseImpl::PropertySheetMap* pMap =
				PropertySheetBaseImpl::getPropertySheetMap();
			PropertySheetBase* pThis = pMap->getThis();
			assert(pThis);
			pMap->setController(hwnd, pThis);
			pThis->setHandle(hwnd);
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
			pThis->centerWindow();
#endif
			pMap->setThis(0);
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
			Window wndTab(::GetDlgItem(hwnd, 0x3020));
			wndTab.setStyle(TCS_BOTTOM, TCS_BOTTOM);
#endif
		}
		break;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
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
							   UINT nId,
							   bool bDeleteThis) :
	DialogBase(bDeleteThis),
	pImpl_(0)
{
	pImpl_ = new PropertyPageImpl();
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

HPROPSHEETPAGE qs::PropertyPage::create(PropertySheetBase* pSheet)
{
	assert(pImpl_);
	assert(!pImpl_->hpsp_);
	assert(pSheet);
	
	pImpl_->hpsp_ = ::CreatePropertySheetPage(&pImpl_->psp_);
	if (!pImpl_->hpsp_)
		return 0;
	
	pImpl_->pSheet_ = pSheet;
	
	return pImpl_->hpsp_;
}

INT_PTR CALLBACK qs::propertyPageProc(HWND hwnd,
									  UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	DialogBaseImpl::DialogMap* pMap = DialogBaseImpl::getDialogMap();
	DialogBase* pThis = pMap->getController(hwnd);
	if (!pThis) {
		HWND hwndSheet = ::GetParent(hwnd);
		assert(hwndSheet);
		PropertySheetBaseImpl::PropertySheetMap* pSheetMap =
			PropertySheetBaseImpl::getPropertySheetMap();
		PropertySheetBase* pSheet = pSheetMap->getController(hwndSheet);
		if (pSheet) {
			HWND hwndTab = PropSheet_GetTabControl(hwndSheet);
			int nPage = TabCtrl_GetCurSel(hwndTab);
			pThis = pSheet->getPage(nPage);
			if (pThis) {
				pMap->setController(hwnd, pThis);
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
			PropertySheetBaseImpl::PropertySheetMap* pSheetMap =
				PropertySheetBaseImpl::getPropertySheetMap();
			PropertySheetBase* pSheet = pSheetMap->getController(hwndSheet);
			if (pSheet)
				pSheet->init();
		}
		break;
	
#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
	case WM_DESTROY:
		WindowDestroy::getWindowDestroy()->process(hwnd);
		break;
#endif
	
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

qs::DefaultPropertyPage::DefaultPropertyPage(HINSTANCE hInst,
											 UINT nId) :
	PropertyPage(hInst, nId, false)
{
	addCommandHandler(this);
	addNotifyHandler(this);
	setDialogHandler(this, false);
}

qs::DefaultPropertyPage::~DefaultPropertyPage()
{
}

INT_PTR qs::DefaultPropertyPage::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qs::DefaultPropertyPage::onCommand(WORD nCode,
										   WORD nId)
{
	return CommandHandler::onCommand(nCode, nId);
}

LRESULT qs::DefaultPropertyPage::onNotify(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qs::DefaultPropertyPage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qs::DefaultPropertyPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	return TRUE;
}

LRESULT qs::DefaultPropertyPage::onOk()
{
	return 0;
}

LRESULT qs::DefaultPropertyPage::onApply(NMHDR* pnmhdr,
										 bool* pbHandled)
{
	return onOk();
}
