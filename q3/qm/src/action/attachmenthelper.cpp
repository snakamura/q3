/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmessage.h>

#include <qsconv.h>
#include <qsstl.h>
#include <qstextutil.h>
#include <qswindow.h>

#include <boost/bind.hpp>

#include <commdlg.h>
#include <tchar.h>

#include "attachmenthelper.h"
#include "../model/tempfilecleaner.h"
#include "../ui/dialogs.h"
#include "../ui/resourceinc.h"
#include "../uimodel/securitymodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DetachCallbackImpl
 *
 */

namespace {
class DetachCallbackImpl : public AttachmentParser::DetachCallback
{
public:
	DetachCallbackImpl(TempFileCleaner* pTempFileCleaner,
					   HWND hwnd);
	~DetachCallbackImpl();

public:
	virtual wstring_ptr confirmOverwrite(const WCHAR* pwszPath);

private:
	DetachCallbackImpl(const DetachCallbackImpl&);
	DetachCallbackImpl& operator=(const DetachCallbackImpl&);

private:
	TempFileCleaner* pTempFileCleaner_;
	HWND hwnd_;
};
}

DetachCallbackImpl::DetachCallbackImpl(TempFileCleaner* pTempFileCleaner,
									   HWND hwnd) :
	pTempFileCleaner_(pTempFileCleaner),
	hwnd_(hwnd)
{
}

DetachCallbackImpl::~DetachCallbackImpl()
{
}

wstring_ptr DetachCallbackImpl::confirmOverwrite(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	if (pTempFileCleaner_ && !pTempFileCleaner_->isModified(pwszPath))
		return allocWString(pwszPath);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstr(loadString(hInst, IDS_CONFIRM_OVERWRITE));
	wstring_ptr wstrMessage(concat(wstr.get(), pwszPath));
	
	wstring_ptr wstrPath;
	int nMsg = messageBox(wstrMessage.get(), MB_YESNOCANCEL, hwnd_, 0, 0);
	switch (nMsg) {
	case IDCANCEL:
		break;
	case IDYES:
		wstrPath = allocWString(pwszPath);
		break;
	case IDNO:
		{
			wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_ATTACHMENT));
			
			const WCHAR* pwszDir = 0;
			const WCHAR* pwszFileName = 0;
			wstring_ptr wstr(allocWString(pwszPath));
			WCHAR* p = wcsrchr(wstr.get(), L'\\');
			if (p) {
				*p = L'\0';
				pwszDir = wstr.get();
				pwszFileName = p + 1;
			}
			else {
				pwszFileName = wstr.get();
			}
			
			FileDialog dialog(false, wstrFilter.get(), pwszDir, 0, pwszFileName,
				OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT);
			
			if (dialog.doModal(hwnd_) == IDOK)
				wstrPath = allocWString(dialog.getPath());
		}
		break;
	default:
		break;
	}
	
	return wstrPath;
}


/****************************************************************************
 *
 * AttachmentHelper
 *
 */

qm::AttachmentHelper::AttachmentHelper(SecurityModel* pSecurityModel,
									   Profile* pProfile,
									   TempFileCleaner* pTempFileCleaner,
									   HWND hwnd) :
	pSecurityModel_(pSecurityModel),
	pProfile_(pProfile),
	pTempFileCleaner_(pTempFileCleaner),
	hwnd_(hwnd)
{
	assert(pProfile);
	assert(hwnd);
}

qm::AttachmentHelper::~AttachmentHelper()
{
}

AttachmentParser::Result qm::AttachmentHelper::detach(const MessageHolderList& listMessageHolder,
													  const NameList* pListName)
{
	DetachDialog::List list;
	struct Deleter
	{
		Deleter(DetachDialog::List& l) : l_(l) {}
		~Deleter()
		{
			std::for_each(l_.begin(), l_.end(),
				boost::bind(&freeWString,
					boost::bind(&DetachDialog::Item::wstrName_, _1)));
		}
		DetachDialog::List& l_;
	} deleter(list);
	
	for (MessageHolderList::const_iterator itM = listMessageHolder.begin(); itM != listMessageHolder.end(); ++itM) {
		MessageHolder* pmh = *itM;
		
		Message msg;
		if (!pmh->getMessage(Account::GETMESSAGEFLAG_TEXT, 0, pSecurityModel_->getSecurityMode(), &msg))
			return AttachmentParser::RESULT_FAIL;
		
		AttachmentParser parser(msg);
		AttachmentParser::AttachmentList l;
		AttachmentParser::AttachmentListFree free(l);
		parser.getAttachments(AttachmentParser::GAF_NONE, &l);
		for (AttachmentParser::AttachmentList::iterator itA = l.begin(); itA != l.end(); ++itA) {
			wstring_ptr wstrName(allocWString((*itA).first));
			
			bool bSelected = true;
			if (pListName) {
				NameList::const_iterator itN = std::find_if(
					pListName->begin(), pListName->end(),
					std::bind2nd(string_equal<WCHAR>(), wstrName.get()));
				bSelected = itN != pListName->end();
			}
			
			DetachDialog::Item item = {
				pmh,
				wstrName.get(),
				bSelected
			};
			list.push_back(item);
			wstrName.release();
		}
	}
	
	if (list.empty())
		return AttachmentParser::RESULT_OK;
	
	DetachDialog dialog(pProfile_, list);
	if (dialog.doModal(hwnd_) != IDOK)
		return AttachmentParser::RESULT_CANCEL;
	
	const WCHAR* pwszFolder = dialog.getFolder();
	
	MessageHolder* pmh = 0;
	Message msg;
	AttachmentParser::AttachmentList l;
	AttachmentParser::AttachmentListFree free(l);
	DetachCallbackImpl callback(0, hwnd_);
	unsigned int n = 0;
	for (DetachDialog::List::iterator it = list.begin(); it != list.end(); ++it) {
		if ((*it).pmh_ != pmh) {
			pmh = (*it).pmh_;
			n = 0;
			msg.clear();
			free.free();
		}
		else {
			++n;
		}
		if ((*it).wstrName_) {
			if (msg.getFlag() == Message::FLAG_EMPTY) {
				if (!(*it).pmh_->getMessage(Account::GETMESSAGEFLAG_ALL,
					0, pSecurityModel_->getSecurityMode(), &msg))
					return AttachmentParser::RESULT_FAIL;
			}
			if (l.empty())
				AttachmentParser(msg).getAttachments(AttachmentParser::GAF_NONE, &l);
			assert(n < l.size());
			const AttachmentParser::AttachmentList::value_type& v = l[n];
			if (AttachmentParser(*v.second).detach(pwszFolder, (*it).wstrName_,
				isAddZoneId(), &callback, 0) == AttachmentParser::RESULT_FAIL)
				return AttachmentParser::RESULT_FAIL;
		}
	}
	
	if (dialog.isOpenFolder()) {
		const WCHAR* pwszCommand = 0;
		WCHAR* pParam = 0;
		
		wstring_ptr wstrCommand(pProfile_->getString(L"Global", L"Filer"));
		if (*wstrCommand.get()) {
			wstrCommand = TextUtil::replace(wstrCommand.get(), L"%d", pwszFolder);
			
			pwszCommand = wstrCommand.get();
			if (*pwszCommand == L'\"') {
				++pwszCommand;
				pParam = wcschr(wstrCommand.get(), L'\"');
			}
			else {
				pParam = wcschr(wstrCommand.get(), L' ');
			}
			if (pParam) {
				*pParam = L'\0';
				++pParam;
				while (*pParam == L' ')
					++pParam;
			}
		}
		else {
			pwszCommand = pwszFolder;
		}
		
		W2T(pwszCommand, ptszCommand);
		W2T(pParam, ptszParam);
		SHELLEXECUTEINFO info = {
			sizeof(info),
			0,
			hwnd_,
#ifdef _WIN32_WCE
			_T("open"),
#else
			0,
#endif
			ptszCommand,
			ptszParam,
			0,
#ifdef _WIN32_WCE
		SW_SHOWNORMAL,
#else
		SW_SHOWDEFAULT,
#endif
		};
		::ShellExecuteEx(&info);
	}
	
	return AttachmentParser::RESULT_OK;
}

AttachmentParser::Result qm::AttachmentHelper::open(const Part* pPart,
													const WCHAR* pwszName,
													bool bOpenWithEditor)
{
	assert(pPart);
	assert(pwszName);
	assert(pTempFileCleaner_);
	
	AttachmentParser parser(*pPart);
	DetachCallbackImpl callback(pTempFileCleaner_, hwnd_);
	const WCHAR* pwszTempDir = Application::getApplication().getTemporaryFolder();
	wstring_ptr wstrPath;
	AttachmentParser::Result result = parser.detach(pwszTempDir,
		pwszName, isAddZoneId(), &callback, &wstrPath);
	if (result != AttachmentParser::RESULT_OK)
		return result;
	assert(wstrPath.get());
	
	pTempFileCleaner_->add(wstrPath.get());
	
	if (!bOpenWithEditor) {
		const WCHAR* p = wcsrchr(wstrPath.get(), L'.');
		if (p) {
			++p;
			
			wstring_ptr wstrExtensions(pProfile_->getString(L"Global", L"WarnExtensions"));
			if (wcsstr(wstrExtensions.get(), p)) {
				int nMsg = messageBox(
					Application::getApplication().getResourceHandle(),
					IDS_CONFIRM_EXECUTEATTACHMENT,
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING, hwnd_, 0, 0);
				if (nMsg != IDYES)
					return AttachmentParser::RESULT_CANCEL;
			}
		}
	}
	
	W2T(wstrPath.get(), ptszPath);
	
	tstring_ptr tstrEditor;
	if (bOpenWithEditor) {
		wstring_ptr wstrEditor(pProfile_->getString(L"Global", L"Editor"));
		tstrEditor = wcs2tcs(wstrEditor.get());
	}
	
	SHELLEXECUTEINFO sei = {
		sizeof(sei),
		0,
		hwnd_,
#ifdef _WIN32_WCE
		_T("open"),
#else
		0,
#endif
		0,
		0,
		0,
		SW_SHOW
	};
	if (bOpenWithEditor) {
		sei.lpFile = tstrEditor.get();
		sei.lpParameters = ptszPath;
	}
	else {
		sei.lpFile = ptszPath;
	}
	if (!::ShellExecuteEx(&sei))
		return AttachmentParser::RESULT_FAIL;
	
	return AttachmentParser::RESULT_OK;
}

bool qm::AttachmentHelper::isAddZoneId() const
{
	return pProfile_->getInt(L"Global", L"AddZoneId") != 0;
}
