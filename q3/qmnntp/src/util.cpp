/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmsecurity.h>

#include "main.h"
#include "nntp.h"
#include "resourceinc.h"
#include "util.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Util
 *
 */

void qmnntp::Util::reportError(Nntp* pNntp,
							   SessionCallback* pSessionCallback,
							   Account* pAccount,
							   SubAccount* pSubAccount)
{
	assert(pNntp);
	assert(pSessionCallback);
	assert(pAccount);
	assert(pSubAccount);
	
	struct
	{
		unsigned int nError_;
		UINT nId_;
	} maps[][12] = {
		{
			{ Nntp::NNTP_ERROR_GREETING,	IDS_ERROR_GREETING		},
			{ Nntp::NNTP_ERROR_GROUP,		IDS_ERROR_GROUP			},
			{ Nntp::NNTP_ERROR_AUTHINFO,	IDS_ERROR_AUTHINFO		},
			{ Nntp::NNTP_ERROR_ARTICLE,		IDS_ERROR_ARTICLE		},
			{ Nntp::NNTP_ERROR_HEAD,		IDS_ERROR_HEAD			},
			{ Nntp::NNTP_ERROR_BODY,		IDS_ERROR_BODY			},
			{ Nntp::NNTP_ERROR_XOVER,		IDS_ERROR_XOVER			},
			{ Nntp::NNTP_ERROR_MODEREADER,	IDS_ERROR_MODEREADER	},
			{ Nntp::NNTP_ERROR_POST,		IDS_ERROR_POST			}
		},
		{
			{ Nntp::NNTP_ERROR_INITIALIZE,		IDS_ERROR_INITIALIZE	},
			{ Nntp::NNTP_ERROR_CONNECT,			IDS_ERROR_CONNECT		},
			{ Nntp::NNTP_ERROR_RESPONSE,		IDS_ERROR_RESPONSE		},
			{ Nntp::NNTP_ERROR_INVALIDSOCKET,	IDS_ERROR_INVALIDSOCKET	},
			{ Nntp::NNTP_ERROR_OTHER,			IDS_ERROR_OTHER			},
			{ Nntp::NNTP_ERROR_SELECT,			IDS_ERROR_SELECT		},
			{ Nntp::NNTP_ERROR_TIMEOUT,			IDS_ERROR_TIMEOUT		},
			{ Nntp::NNTP_ERROR_RECEIVE,			IDS_ERROR_RECEIVE		},
			{ Nntp::NNTP_ERROR_DISCONNECT,		IDS_ERROR_DISCONNECT	},
			{ Nntp::NNTP_ERROR_SEND,			IDS_ERROR_SEND			},
			{ Nntp::NNTP_ERROR_SSL,				IDS_ERROR_SSL			}
		},
		{
			{ Socket::SOCKET_ERROR_SOCKET,			IDS_ERROR_SOCKET_SOCKET			},
			{ Socket::SOCKET_ERROR_CLOSESOCKET,		IDS_ERROR_SOCKET_CLOSESOCKET	},
			{ Socket::SOCKET_ERROR_LOOKUPNAME,		IDS_ERROR_SOCKET_LOOKUPNAME		},
			{ Socket::SOCKET_ERROR_CONNECT,			IDS_ERROR_SOCKET_CONNECT		},
			{ Socket::SOCKET_ERROR_CONNECTTIMEOUT,	IDS_ERROR_SOCKET_CONNECTTIMEOUT	},
			{ Socket::SOCKET_ERROR_RECV,			IDS_ERROR_SOCKET_RECV			},
			{ Socket::SOCKET_ERROR_RECVTIMEOUT,		IDS_ERROR_SOCKET_RECVTIMEOUT	},
			{ Socket::SOCKET_ERROR_SEND,			IDS_ERROR_SOCKET_SEND			},
			{ Socket::SOCKET_ERROR_SENDTIMEOUT,		IDS_ERROR_SOCKET_SENDTIMEOUT	},
			{ Socket::SOCKET_ERROR_CANCEL,			IDS_ERROR_SOCKET_CANCEL			},
			{ Socket::SOCKET_ERROR_UNKNOWN,			IDS_ERROR_SOCKET_UNKNOWN		}
		}
	};
	
	unsigned int nError = pNntp->getLastError();
	unsigned int nMasks[] = {
		Nntp::NNTP_ERROR_MASK_HIGHLEVEL,
		Nntp::NNTP_ERROR_MASK_LOWLEVEL,
		Socket::SOCKET_ERROR_MASK_SOCKET
	};
	wstring_ptr wstrDescriptions[countof(maps)];
	for (int n = 0; n < countof(maps); ++n) {
		for (int m = 0; m < countof(maps[n]) && !wstrDescriptions[n].get(); ++m) {
			if (maps[n][m].nError_ != 0 &&
				(nError & nMasks[n]) == maps[n][m].nError_)
				wstrDescriptions[n] = loadString(getResourceHandle(), maps[n][m].nId_);
		}
	}
	
	wstring_ptr wstrMessage(loadString(getResourceHandle(), IDS_ERROR_MESSAGE));
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrDescriptions[2].get(),
		pNntp->getLastErrorResponse()
	};
	SessionErrorInfo info(pAccount, pSubAccount, 0, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	pSessionCallback->addError(info);
}


/****************************************************************************
 *
 * AbstractCallback
 *
 */

qmnntp::AbstractCallback::AbstractCallback(SubAccount* pSubAccount,
										   const Security* pSecurity) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_RECEIVE, pSecurity),
	pSubAccount_(pSubAccount)
{
}

qmnntp::AbstractCallback::~AbstractCallback()
{
}

bool qmnntp::AbstractCallback::getUserInfo(wstring_ptr* pwstrUserName,
										   wstring_ptr* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	*pwstrUserName = allocWString(pSubAccount_->getUserName(Account::HOST_RECEIVE));
	*pwstrPassword = allocWString(pSubAccount_->getPassword(Account::HOST_RECEIVE));
	
	return true;
}

void qmnntp::AbstractCallback::setPassword(const WCHAR* pwszPassword)
{
	// TODO
}
