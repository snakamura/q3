/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmessage.h>

#include <qsconv.h>
#include <qsstl.h>
#include <qswindow.h>

#include "attachmenthelper.h"
#include "../model/tempfilecleaner.h"
#include "../ui/dialogs.h"
#include "../ui/resourceinc.h"

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
	DetachCallbackImpl(HWND hwnd);
	~DetachCallbackImpl();

public:
	virtual QSTATUS confirmOverwrite(
		const WCHAR* pwszPath, WSTRING* pwstrPath);

private:
	DetachCallbackImpl(const DetachCallbackImpl&);
	DetachCallbackImpl& operator=(const DetachCallbackImpl&);

private:
	HWND hwnd_;
};
}

DetachCallbackImpl::DetachCallbackImpl(HWND hwnd) :
	hwnd_(hwnd)
{
}

DetachCallbackImpl::~DetachCallbackImpl()
{
}

QSTATUS DetachCallbackImpl::confirmOverwrite(
	const WCHAR* pwszPath, WSTRING* pwstrPath)
{
	assert(pwszPath);
	assert(pwstrPath);
	
	DECLARE_QSTATUS();
	
	*pwstrPath = 0;
	
	string_ptr<WSTRING> wstr;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_CONFIRMOVERWRITE, &wstr);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrMessage(concat(wstr.get(), pwszPath));
	if (!wstrMessage.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<WSTRING> wstrPath;
	int nMsg = 0;
	status = messageBox(wstrMessage.get(), MB_YESNOCANCEL, hwnd_, 0, 0, &nMsg);
	CHECK_QSTATUS();
	switch (nMsg) {
	case IDCANCEL:
		break;
	case IDYES:
		wstrPath.reset(allocWString(pwszPath));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		break;
	case IDNO:
		// TODO
		// Open file dialog and pick new file name up.
		break;
	default:
		break;
	}
	
	*pwstrPath = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AttachmentHelper
 *
 */

qm::AttachmentHelper::AttachmentHelper(Profile* pProfile,
	TempFileCleaner* pTempFileCleaner, HWND hwnd) :
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

QSTATUS qm::AttachmentHelper::detach(
	const MessageHolderList& listMessageHolder, const NameList* pListName)
{
	DECLARE_QSTATUS();
	
	DetachDialog::List list;
	struct Deleter
	{
		Deleter(DetachDialog::List& l) : l_(l) {}
		~Deleter()
		{
			std::for_each(l_.begin(), l_.end(),
				unary_compose_f_gx(
					string_free<WSTRING>(),
					mem_data_ref(&DetachDialog::Item::wstrName_)));
		}
		DetachDialog::List& l_;
	} deleter(list);
	
	MessageHolderList::const_iterator itM = listMessageHolder.begin();
	while (itM != listMessageHolder.end()) {
		MessageHolder* pmh = *itM;
		
		Message msg(&status);
		CHECK_QSTATUS();
		status = pmh->getMessage(Account::GETMESSAGEFLAG_TEXT, 0, &msg);
		CHECK_QSTATUS();
		AttachmentParser parser(msg);
		AttachmentParser::AttachmentList l;
		AttachmentParser::AttachmentListFree free(l);
		status = parser.getAttachments(false, &l);
		CHECK_QSTATUS();
		AttachmentParser::AttachmentList::iterator itA = l.begin();
		while (itA != l.end()) {
			string_ptr<WSTRING> wstrName(allocWString((*itA).first));
			if (!wstrName.get())
				return QSTATUS_OUTOFMEMORY;
			
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
			status = STLWrapper<DetachDialog::List>(list).push_back(item);
			CHECK_QSTATUS();
			status = pmh->getMessage(Account::GETMESSAGEFLAG_TEXT, 0, &msg);
			CHECK_QSTATUS();
			AttachmentParser parser(msg);
			AttachmentParser::AttachmentList l;
			AttachmentParser::AttachmentListFree free(l);
			status = parser.getAttachments(false, &l);
			CHECK_QSTATUS();
			AttachmentParser::AttachmentList::iterator itA = l.begin();
			while (itA != l.end()) {
				string_ptr<WSTRING> wstrName(allocWString((*itA).first));
				if (!wstrName.get())
					return QSTATUS_OUTOFMEMORY;
				
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
				status = STLWrapper<DetachDialog::List>(list).push_back(item);
				CHECK_QSTATUS();
				wstrName.release();
				++itA;
			}
		}
		
		++itM;
	}
	
	if (!list.empty()) {
		DetachDialog dialog(pProfile_, list, &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwnd_, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
			const WCHAR* pwszFolder = dialog.getFolder();
			
			MessageHolder* pmh = 0;
			Message msg(&status);
			AttachmentParser::AttachmentList l;
			AttachmentParser::AttachmentListFree free(l);
			DetachCallbackImpl callback(hwnd_);
			unsigned int n = 0;
			DetachDialog::List::iterator it = list.begin();
			while (it != list.end()) {
				if ((*it).pmh_ != pmh) {
					pmh = (*it).pmh_;
					n = 0;
					status = msg.clear();
					CHECK_QSTATUS();
					free.free();
				}
				else {
					++n;
				}
				if ((*it).wstrName_) {
					if (msg.getFlag() == Message::FLAG_EMPTY) {
						status = (*it).pmh_->getMessage(
							Account::GETMESSAGEFLAG_ALL, 0, &msg);
						CHECK_QSTATUS();
					}
					if (l.empty()) {
						status = AttachmentParser(msg).getAttachments(false, &l);
						CHECK_QSTATUS();
					}
					assert(n < l.size());
					const AttachmentParser::AttachmentList::value_type& v = l[n];
					string_ptr<WSTRING> wstrPath;
					status = AttachmentParser(*v.second).detach(
						pwszFolder, (*it).wstrName_, &callback, &wstrPath);
					CHECK_QSTATUS();
				}
				++it;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHelper::open(const Part* pPart,
	const WCHAR* pwszName, bool bOpenWithEditor)
{
	assert(pPart);
	assert(pwszName);
	assert(pTempFileCleaner_);
	
	DECLARE_QSTATUS();
	
	AttachmentParser parser(*pPart);
	DetachCallbackImpl callback(hwnd_);
	const WCHAR* pwszTempDir =
		Application::getApplication().getTemporaryFolder();
	string_ptr<WSTRING> wstrPath;
	status = parser.detach(pwszTempDir, pwszName, &callback, &wstrPath);
	CHECK_QSTATUS();
	if (!wstrPath.get())
		return QSTATUS_SUCCESS;
	
	status = pTempFileCleaner_->add(wstrPath.get());
	CHECK_QSTATUS();
	
	if (!bOpenWithEditor) {
		const WCHAR* p = wcsrchr(wstrPath.get(), L'.');
		if (p) {
			++p;
			
			string_ptr<WSTRING> wstrExtensions;
			status = pProfile_->getString(L"Global", L"WarnExtensions",
				L"exe com pif bat scr htm html hta vbs js", &wstrExtensions);
			CHECK_QSTATUS();
			if (wcsstr(wstrExtensions.get(), p)) {
				int nMsg = 0;
				status = messageBox(
					Application::getApplication().getResourceHandle(),
					IDS_CONFIRMEXECUTEATTACHMENT,
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING, hwnd_, 0, 0, &nMsg);
				CHECK_QSTATUS();
				if (nMsg != IDYES)
					return QSTATUS_SUCCESS;
			}
		}
	}
	
	W2T(wstrPath.get(), ptszPath);
	
	string_ptr<TSTRING> tstrEditor;
	if (bOpenWithEditor) {
		string_ptr<WSTRING> wstrEditor;
		status = pProfile_->getString(L"Global", L"Editor", L"", &wstrEditor);
		CHECK_QSTATUS();
		tstrEditor.reset(wcs2tcs(wstrEditor.get()));
		if (!tstrEditor.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	SHELLEXECUTEINFO sei = {
		sizeof(sei),
		0,
		hwnd_,
		0,
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
	if (!::ShellExecuteEx(&sei)) {
		status = messageBox(Application::getApplication().getResourceHandle(),
			IDS_ERROR_EXECUTEATTACHMENT, hwnd_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}
