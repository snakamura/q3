/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmmessageholder.h>

#include "propertypages.h"
#include "resourceinc.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

qm::DefaultPropertyPage::DefaultPropertyPage(UINT nId) :
	qs::DefaultPropertyPage(Application::getApplication().getResourceHandle(), nId)
{
}

qm::DefaultPropertyPage::~DefaultPropertyPage()
{
}


/****************************************************************************
 *
 * MessagePropertyPage
 *
 */

namespace {
struct
{
	MessageHolderBase::Flag flag_;
	UINT nId_;
} flags[] = {
	{ MessageHolderBase::FLAG_SEEN,			IDC_SEEN			},
	{ MessageHolderBase::FLAG_REPLIED,		IDC_REPLIED			},
	{ MessageHolderBase::FLAG_FORWARDED,	IDC_FORWARDED		},
	{ MessageHolderBase::FLAG_SENT,			IDC_SENT			},
	{ MessageHolderBase::FLAG_DRAFT,		IDC_DRAFT			},
	{ MessageHolderBase::FLAG_MARKED,		IDC_MARKED			},
	{ MessageHolderBase::FLAG_DELETED,		IDC_DELETED			},
	{ MessageHolderBase::FLAG_DOWNLOAD,		IDC_DOWNLOAD		},
	{ MessageHolderBase::FLAG_DOWNLOADTEXT,	IDC_DOWNLOADTEXT	},
	{ MessageHolderBase::FLAG_TOME,			IDC_TOME			},
	{ MessageHolderBase::FLAG_CCME,			IDC_CCME			},
	{ MessageHolderBase::FLAG_USER1,		IDC_USER1			},
	{ MessageHolderBase::FLAG_USER2,		IDC_USER2			},
	{ MessageHolderBase::FLAG_USER3,		IDC_USER3			},
	{ MessageHolderBase::FLAG_USER4,		IDC_USER4			},
};
}

qm::MessagePropertyPage::MessagePropertyPage(const MessageHolderList& l) :
	DefaultPropertyPage(IDD_MESSAGEPROPERTY),
	listMessage_(l),
	nFlags_(0),
	nMask_(0)
{
	assert(!l.empty());
}

qm::MessagePropertyPage::~MessagePropertyPage()
{
}

unsigned int qm::MessagePropertyPage::getFlags() const
{
	return nFlags_;
}

unsigned int qm::MessagePropertyPage::getMask() const
{
	return nMask_;
}

LRESULT qm::MessagePropertyPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	if (listMessage_.size() == 1) {
		MessageHolder* pmh = listMessage_.front();
		
		struct
		{
			wstring_ptr (MessageHolder::*pfn_)() const;
			UINT nId_;
		} texts[] = {
			{ &MessageHolder::getFrom,		IDC_FROM	},
			{ &MessageHolder::getTo,		IDC_TO		},
			{ &MessageHolder::getSubject,	IDC_SUBJECT	}
		};
		for (int n = 0; n < countof(texts); ++n) {
			wstring_ptr wstr((pmh->*texts[n].pfn_)());
			setDlgItemText(texts[n].nId_, wstr.get());
		}
		
		struct
		{
			unsigned int (MessageHolder::*pfn_)() const;
			UINT nId_;
		} numbers[] = {
			{ &MessageHolder::getId,		IDC_ID			},
//			{ &MessageHolder::getSize,		IDC_MESSAGESIZE	}
		};
		for (int n = 0; n < countof(numbers); ++n)
			setDlgItemInt(numbers[n].nId_, (pmh->*numbers[n].pfn_)());
		
		wstring_ptr wstrSize = formatSize(pmh->getSize());
		setDlgItemText(IDC_MESSAGESIZE, wstrSize.get());
		
		wstring_ptr wstrFolder(pmh->getFolder()->getFullName());
		setDlgItemText(IDC_FOLDER, wstrFolder.get());
		
		Time time;
		pmh->getDate(&time);
		wstring_ptr wstrTime(time.format(Time::getDefaultFormat(), Time::FORMAT_LOCAL));
		setDlgItemText(IDC_DATE, wstrTime.get());
		
		unsigned int nFlags = pmh->getFlags();
		for (int n = 0; n < countof(flags); ++n) {
			sendDlgItemMessage(flags[n].nId_, BM_SETCHECK,
				nFlags & flags[n].flag_ ? BST_CHECKED : BST_UNCHECKED);
			Window(getDlgItem(flags[n].nId_)).setStyle(
				BS_AUTOCHECKBOX, BS_AUTOCHECKBOX | BS_AUTO3STATE);
		}
	}
	else {
		for (int n = 0; n < countof(flags); ++n) {
			unsigned int nCount = 0;
			for (MessageHolderList::const_iterator it = listMessage_.begin(); it != listMessage_.end(); ++it) {
				if ((*it)->getFlags() & flags[n].flag_)
					++nCount;
			}
			
			sendDlgItemMessage(flags[n].nId_, BM_SETCHECK,
				nCount == 0 ? BST_UNCHECKED :
				nCount == listMessage_.size() ? BST_CHECKED : BST_INDETERMINATE);
			if (nCount == 0 || nCount == listMessage_.size())
				Window(getDlgItem(flags[n].nId_)).setStyle(
					BS_AUTOCHECKBOX, BS_AUTOCHECKBOX | BS_AUTO3STATE);
		}
	}
	
	return TRUE;
}

LRESULT qm::MessagePropertyPage::onOk()
{
	for (int n = 0; n < countof(flags); ++n) {
		int nCheck = Button_GetCheck(getDlgItem(flags[n].nId_));
		switch (nCheck) {
		case BST_CHECKED:
			nFlags_ |= flags[n].flag_;
			nMask_ |= flags[n].flag_;
			break;
		case BST_UNCHECKED:
			nMask_ |= flags[n].flag_;
			break;
		case BST_INDETERMINATE:
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return DefaultPropertyPage::onOk();
}

wstring_ptr qm::MessagePropertyPage::formatSize(unsigned int nSize)
{
	unsigned int nKB = 0;
	if (nSize != 0)
		nKB = nSize/1024 + (nSize%1024 ? 1 : 0);
	
	StringBuffer<WSTRING> buf;
	formatDigits(nKB, &buf);
	buf.append(L"KB (");
	formatDigits(nSize, &buf);
	buf.append(L')');
	return buf.getString();
}

void qm::MessagePropertyPage::formatDigits(unsigned int n,
										   StringBuffer<WSTRING>* pBuf)
{
	assert(pBuf);
	
	WCHAR wsz[32];
	_snwprintf(wsz, countof(wsz), L"%u", n);
	size_t nLen = wcslen(wsz);
	for (const WCHAR* p = wsz; *p; ++p, --nLen) {
		pBuf->append(*p);
		if (nLen % 3 == 1 && nLen != 1)
			pBuf->append(L',');
	}
}
