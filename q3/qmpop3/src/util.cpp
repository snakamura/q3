/*
 * $Id: util.cpp,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "main.h"
#include "pop3.h"
#include "resourceinc.h"
#include "util.h"

using namespace qmpop3;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Util
 *
 */

QSTATUS qmpop3::Util::reportError(Pop3* pPop3,
	SessionCallback* pSessionCallback,
	Account* pAccount, SubAccount* pSubAccount)
{
	assert(pPop3);
	assert(pSessionCallback);
	assert(pAccount);
	assert(pSubAccount);
	
	DECLARE_QSTATUS();
	
	struct
	{
		unsigned int nError_;
		UINT nId_;
	} maps[][12] = {
		{
			{ Pop3::POP3_ERROR_GREETING,	IDS_ERROR_GREETING	},
			{ Pop3::POP3_ERROR_APOP,		IDS_ERROR_APOP		},
			{ Pop3::POP3_ERROR_USER,		IDS_ERROR_USER		},
			{ Pop3::POP3_ERROR_PASS,		IDS_ERROR_PASS		},
			{ Pop3::POP3_ERROR_STAT,		IDS_ERROR_STAT		},
			{ Pop3::POP3_ERROR_LIST,		IDS_ERROR_LIST		},
			{ Pop3::POP3_ERROR_UIDL,		IDS_ERROR_UIDL		},
			{ Pop3::POP3_ERROR_RETR,		IDS_ERROR_RETR		},
			{ Pop3::POP3_ERROR_TOP,			IDS_ERROR_TOP		},
			{ Pop3::POP3_ERROR_DELE,		IDS_ERROR_DELE		},
			{ Pop3::POP3_ERROR_NOOP,		IDS_ERROR_NOOP		},
			{ Pop3::POP3_ERROR_XTNDXMIT,	IDS_ERROR_XTNDXMIT	}
		},
		{
			{ Pop3::POP3_ERROR_INITIALIZE,		IDS_ERROR_INITIALIZE		},
			{ Pop3::POP3_ERROR_CONNECT,			IDS_ERROR_CONNECT			},
			{ Pop3::POP3_ERROR_GENERATEDIGEST,	IDS_ERROR_GENERATEDIGEST	},
			{ Pop3::POP3_ERROR_PARSE,			IDS_ERROR_PARSE				},
			{ Pop3::POP3_ERROR_TIMEOUT,			IDS_ERROR_TIMEOUT			},
			{ Pop3::POP3_ERROR_SELECT,			IDS_ERROR_SELECT			},
			{ Pop3::POP3_ERROR_DISCONNECT,		IDS_ERROR_DISCONNECT		},
			{ Pop3::POP3_ERROR_RECEIVE,			IDS_ERROR_RECEIVE			},
			{ Pop3::POP3_ERROR_SEND,			IDS_ERROR_SEND				},
			{ Pop3::POP3_ERROR_INVALIDSOCKET,	IDS_ERROR_INVALIDSOCKET		},
			{ Pop3::POP3_ERROR_RESPONSE,		IDS_ERROR_RESPONSE			},
			{ Pop3::POP3_ERROR_OTHER,			IDS_ERROR_OTHER				}
		},
		{
			{ Socket::SOCKET_ERROR_SOCKET,			IDS_ERROR_SOCKET_SOCKET			},
			{ Socket::SOCKET_ERROR_CLOSESOCKET,		IDS_ERROR_SOCKET_CLOSESOCKET	},
			{ Socket::SOCKET_ERROR_SETSSL,			IDS_ERROR_SOCKET_SETSSL			},
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
	
	unsigned int nError = pPop3->getLastError();
	unsigned int nMasks[] = {
		Pop3::POP3_ERROR_MASK_HIGHLEVEL,
		Pop3::POP3_ERROR_MASK_LOWLEVEL,
		Socket::SOCKET_ERROR_MASK_SOCKET
	};
	string_ptr<WSTRING> wstrDescriptions[countof(maps)];
	for (int n = 0; n < countof(maps); ++n) {
		for (int m = 0; m < countof(maps[n]) && !wstrDescriptions[n].get(); ++m) {
			if (maps[n][m].nError_ != 0 &&
				(nError & nMasks[n]) == maps[n][m].nError_) {
				status = loadString(getResourceHandle(),
					maps[n][m].nId_, &wstrDescriptions[n]);
				CHECK_QSTATUS();
			}
		}
	}
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), IDS_ERROR_MESSAGE, &wstrMessage);
	CHECK_QSTATUS();
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrDescriptions[2].get(),
		pPop3->getLastErrorResponse()
	};
	SessionErrorInfo info(pAccount, pSubAccount, 0, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	status = pSessionCallback->addError(info);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
