/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmapplication.h>
#include <qmfolder.h>

#include <qswindow.h>

#include "actionutil.h"
#include "../model/messagecontext.h"
#include "../ui/dialogs.h"
#include "../uimodel/foldermodel.h"
#include "../uimodel/messagemodel.h"
#ifdef QMTABWINDOW
#	include "../uimodel/tabmodel.h"
#endif
#include "../uimodel/viewmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ActionUtil
 *
 */

void qm::ActionUtil::info(HWND hwnd,
						  UINT nMessage)
{
	messageBox(Application::getApplication().getResourceHandle(),
		nMessage, MB_OK | MB_ICONINFORMATION, hwnd);
}

void qm::ActionUtil::error(HWND hwnd,
						   UINT nMessage)
{
	messageBox(Application::getApplication().getResourceHandle(),
		nMessage, MB_OK | MB_ICONERROR, hwnd);
}

void qm::ActionUtil::error(HWND hwnd,
						   const WCHAR* pwszMessage)
{
	messageBox(pwszMessage, MB_OK | MB_ICONERROR, hwnd);
}


/****************************************************************************
 *
 * ActionParamUtil
 *
 */

const WCHAR* qm::ActionParamUtil::getString(const ActionParam* pParam,
											size_t n)
{
	if (!pParam || pParam->getCount() <= n)
		return 0;
	return pParam->getValue(n);
}

unsigned int qm::ActionParamUtil::getNumber(const ActionParam* pParam,
											size_t n)
{
	const WCHAR* pwszParam = getString(pParam, n);
	if (!pwszParam)
		return -1;
	
	WCHAR* pEnd = 0;
	long nParam = wcstol(pwszParam, &pEnd, 10);
	if (*pEnd)
		return -1;
	
	return static_cast<unsigned int>(nParam);
}

unsigned int qm::ActionParamUtil::getIndex(const ActionParam* pParam,
										   size_t n)
{
	const WCHAR* pwsz = getString(pParam, n);
	if (!pwsz || *pwsz != L'@')
		return -1;
	
	WCHAR* pEnd = 0;
	unsigned int nIndex = wcstol(pwsz + 1, &pEnd, 10);
	if (*pEnd)
		return -1;
	
	return nIndex;
}

std::pair<const WCHAR*, unsigned int> qm::ActionParamUtil::getStringOrIndex(const ActionParam* pParam,
																			size_t n)
{
	if (!pParam || pParam->getCount() <= n)
		return std::pair<const WCHAR*, unsigned int>(0, -1);
	
	const WCHAR* pwszParam = pParam->getValue(n);
	
	if (*pwszParam == L'@') {
		WCHAR* pEnd = 0;
		unsigned int nParam = wcstol(pwszParam + 1, &pEnd, 10);
		if (!*pEnd)
			return std::pair<const WCHAR*, unsigned int>(0, nParam);
	}
	
	return std::pair<const WCHAR*, unsigned int>(pwszParam, -1);
}


/****************************************************************************
 *
 * FolderActionUtil
 *
 */

std::pair<Account*, Folder*> qm::FolderActionUtil::getCurrent(const FolderModelBase* pModel)
{
	assert(pModel);
	
	std::pair<Account*, Folder*> p(pModel->getTemporary());
	if (!p.first && !p.second)
		p = pModel->getCurrent();
	return p;
}

Account* qm::FolderActionUtil::getAccount(const FolderModelBase* pModel)
{
	assert(pModel);
	
	std::pair<Account*, Folder*> p(getCurrent(pModel));
	return p.first ? p.first : p.second ? p.second->getAccount() : 0;
}

Folder* qm::FolderActionUtil::getFolder(const FolderModelBase* pModel)
{
	assert(pModel);
	return getCurrent(pModel).second;
}

Account* qm::FolderActionUtil::getAccount(const AccountManager* pAccountManager,
										  const FolderModelBase* pModel,
										  qs::Profile* pProfile,
										  const WCHAR* pwszClass)
{
	assert(pAccountManager);
	assert(pModel);
	assert(pProfile);
	assert(pwszClass);
	
	std::pair<Account*, Folder*> p(getCurrent(pModel));
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (!pAccount || wcscmp(pAccount->getClass(), pwszClass) != 0) {
		wstring_ptr wstrDefaultKey(getDefaultKey(pwszClass));
		wstring_ptr wstrAccount(pProfile->getString(L"Global", wstrDefaultKey.get()));
		if (*wstrAccount.get()) {
			pAccount = pAccountManager->getAccount(wstrAccount.get());
		}
		else {
			const AccountManager::AccountList& l = pAccountManager->getAccounts();
			AccountManager::AccountList::const_iterator it = std::find_if(l.begin(), l.end(),
				boost::bind(string_equal<WCHAR>(), boost::bind(&Account::getClass, _1), pwszClass));
			pAccount = it != l.end() ? *it : 0;
		}
	}
	return pAccount;
}

wstring_ptr qm::FolderActionUtil::getDefaultKey(const WCHAR* pwszClass)
{
	assert(pwszClass);
	
	StringBuffer<WSTRING> buf(L"Default");
	buf.append(toupper(*pwszClass));
	buf.append(pwszClass + 1);
	buf.append(L"Account");
	return buf.getString();
}


/****************************************************************************
 *
 * MessageActionUtil
 *
 */

void qm::MessageActionUtil::select(ViewModel* pViewModel,
								   unsigned int nIndex,
								   MessageModel* pMessageModel)
{
	assert(pViewModel);
	assert(pViewModel->isLocked());
	assert(nIndex < pViewModel->getCount());
	
	if (pMessageModel) {
		MessageHolder* pmh = pViewModel->getMessageHolder(nIndex);
		pMessageModel->setMessage(std::auto_ptr<MessageContext>(
			new MessagePtrMessageContext(pmh)));
	}
	
	select(pViewModel, nIndex, pMessageModel == 0);
}

void qm::MessageActionUtil::select(ViewModel* pViewModel,
								   unsigned int nIndex,
								   bool bDelay)
{
	assert(pViewModel);
	assert(pViewModel->isLocked());
	assert(nIndex < pViewModel->getCount());
	
	pViewModel->setFocused(nIndex, bDelay);
	pViewModel->setSelection(nIndex);
	pViewModel->setLastSelection(nIndex);
	pViewModel->payAttention(nIndex);
}

void qm::MessageActionUtil::selectNextUndeleted(ViewModel* pViewModel,
												unsigned int nIndex,
												const MessageHolderList& listExclude,
												MessageModel* pMessageModel)
{
	assert(pViewModel);
	assert(pViewModel->isLocked());
	assert(nIndex < pViewModel->getCount());
	
	unsigned int nCount = pViewModel->getCount();
	for (unsigned int n = nIndex + 1; n < nCount; ++n) {
		MessageHolder* pmh = pViewModel->getMessageHolder(n);
		if (!pmh->isFlag(MessageHolder::FLAG_DELETED) &&
			std::find(listExclude.begin(), listExclude.end(), pmh) == listExclude.end()) {
			select(pViewModel, n, pMessageModel);
			return;
		}
	}
	if (nIndex != 0) {
		unsigned int n = nIndex;
		do {
			--n;
			MessageHolder* pmh = pViewModel->getMessageHolder(n);
			if (!pmh->isFlag(MessageHolder::FLAG_DELETED) &&
				std::find(listExclude.begin(), listExclude.end(), pmh) == listExclude.end()) {
				select(pViewModel, n, pMessageModel);
				return;
			}
		} while (n != 0);
	}
}


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabActionUtil
 *
 */

int TabActionUtil::getCurrent(TabModel* pModel)
{
	int nItem = pModel->getTemporary();
	return nItem != -1 ? nItem : pModel->getCurrent();
}
#endif


/****************************************************************************
 *
 * MacroActionUtil
 *
 */

std::auto_ptr<Macro> MacroActionUtil::getMacro(const qs::ActionParam* pParam,
											   size_t n,
											   qs::Profile* pProfile,
											   HWND hwnd)
{
	assert(pProfile);
	
	const WCHAR* pwszMacro = ActionParamUtil::getString(pParam, n);
	wstring_ptr wstrMacro;
	if (!pwszMacro) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrTitle(loadString(hInst, IDS_EXECUTEMACRO));
		wstring_ptr wstrMessage(loadString(hInst, IDS_MACRO));
		wstring_ptr wstrPrevMacro(pProfile->getString(L"Global", L"Macro"));
		
		MultiLineInputBoxDialog dialog(wstrTitle.get(), wstrMessage.get(),
			wstrPrevMacro.get(), false, pProfile, L"MacroDialog");
		if (dialog.doModal(hwnd) != IDOK)
			return std::auto_ptr<Macro>();
		
		wstrMacro = allocWString(dialog.getValue());
		pwszMacro = wstrMacro.get();
		pProfile->setString(L"Global", L"Macro", pwszMacro);
	}
	
	std::auto_ptr<Macro> pMacro(MacroParser().parse(pwszMacro));
	if (!pMacro.get())
		ActionUtil::error(hwnd, IDS_ERROR_INVALIDMACRO);
	
	return pMacro;
}
