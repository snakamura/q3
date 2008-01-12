/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include "main.h"
#include "pop3.h"
#include "pop3error.h"
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

void qmpop3::Util::reportError(Pop3* pPop3,
							   ErrorCallback* pCallback,
							   Account* pAccount,
							   SubAccount* pSubAccount,
							   NormalFolder* pFolder,
							   unsigned int nPop3Error)
{
	assert(pCallback);
	
	struct
	{
		unsigned int nError_;
		UINT nId_;
	} maps[][13] = {
		{
			{ POP3ERROR_APPLYRULES,	IDS_ERROR_APPLYRULES	},
			{ POP3ERROR_MANAGEJUNK,	IDS_ERROR_MANAGEJUNK	},
			{ POP3ERROR_FILTERJUNK,	IDS_ERROR_FILTERJUNK	}
		},
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
			{ Pop3::POP3_ERROR_XTNDXMIT,	IDS_ERROR_XTNDXMIT	},
			{ Pop3::POP3_ERROR_STLS,		IDS_ERROR_STLS		}
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
			{ Pop3::POP3_ERROR_SSL,				IDS_ERROR_SSL				},
			{ Pop3::POP3_ERROR_OTHER,			IDS_ERROR_OTHER				}
		}
	};
	
	unsigned int nError = (pPop3 ? pPop3->getLastError() : 0) | nPop3Error;
	unsigned int nMasks[] = {
		POP3ERROR_MASK,
		Pop3::POP3_ERROR_MASK_HIGHLEVEL,
		Pop3::POP3_ERROR_MASK_LOWLEVEL
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
	wstring_ptr wstrSocketDescription(SocketBase::getErrorDescription(
		static_cast<SocketBase::Error>(nError & SocketBase::SOCKET_ERROR_MASK_SOCKET)));
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrDescriptions[2].get(),
		wstrSocketDescription.get(),
		pPop3 ? pPop3->getLastErrorResponse() : 0
	};
	SessionErrorInfo info(pAccount, pSubAccount, pFolder, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	pCallback->addError(info);
}

Pop3::Secure qmpop3::Util::getSecure(SubAccount* pSubAccount,
									 qm::Account::Host host)
{
	assert(pSubAccount);
	return getSecure(pSubAccount->getSecure(host));
}

Pop3::Secure qmpop3::Util::getSecure(SubAccount::Secure secure)
{
	switch (secure) {
	case SubAccount::SECURE_SSL:
		return Pop3::SECURE_SSL;
	case SubAccount::SECURE_STARTTLS:
		return Pop3::SECURE_STARTTLS;
	default:
		return Pop3::SECURE_NONE;
	}
}

PasswordState qmpop3::Util::getUserInfo(SubAccount* pSubAccount,
										Account::Host host,
										PasswordCallback* pPasswordCallback,
										wstring_ptr* pwstrUserName,
										wstring_ptr* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	*pwstrUserName = allocWString(pSubAccount->getUserName(host));
	return pPasswordCallback->getPassword(pSubAccount, host, pwstrPassword);
}

void qmpop3::Util::setPassword(SubAccount* pSubAccount,
							   Account::Host host,
							   PasswordState state,
							   PasswordCallback* pPasswordCallback,
							   const WCHAR* pwszPassword)
{
	if (state == PASSWORDSTATE_SESSION || state == PASSWORDSTATE_SAVE)
		pPasswordCallback->setPassword(pSubAccount, host,
			pwszPassword, state == PASSWORDSTATE_SAVE);
}
