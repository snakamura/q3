/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "commandline.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MainCommandLineHandler
 *
 */

qm::MainCommandLineHandler::MainCommandLineHandler() :
	state_(STATE_NONE),
	wstrGoRound_(0),
	wstrMailFolder_(0),
	wstrProfile_(0),
	wstrURL_(0)
{
}

qm::MainCommandLineHandler::~MainCommandLineHandler()
{
	freeWString(wstrGoRound_);
	freeWString(wstrMailFolder_);
	freeWString(wstrProfile_);
	freeWString(wstrURL_);
}

const WCHAR* qm::MainCommandLineHandler::getGoRound() const
{
	return wstrGoRound_;
}

const WCHAR* qm::MainCommandLineHandler::getMailFolder() const
{
	return wstrMailFolder_;
}

const WCHAR* qm::MainCommandLineHandler::getProfile() const
{
	return wstrProfile_;
}

const WCHAR* qm::MainCommandLineHandler::getURL() const
{
	return wstrURL_;
}

QSTATUS qm::MainCommandLineHandler::process(const WCHAR* pwszOption)
{
	DECLARE_QSTATUS();
	
	struct {
		const WCHAR* pwsz_;
		State state_;
	} options[] = {
		{ L"g",	STATE_GOROUND		},
		{ L"d",	STATE_MAILFOLDER	},
		{ L"p",	STATE_PROFILE		},
		{ L"s",	STATE_URL			}
	};
	
	WSTRING* pwstr[] = {
		&wstrGoRound_,
		&wstrMailFolder_,
		&wstrProfile_,
		&wstrURL_
	};
	
	switch (state_) {
	case STATE_NONE:
		if (*pwszOption == L'-' || *pwszOption == L'/') {
			for (int n = 0; n < countof(options) && state_ == STATE_NONE; ++n) {
				if (wcscmp(pwszOption + 1, options[n].pwsz_) == 0)
					state_ = options[n].state_;
			}
		}
		break;
	case STATE_GOROUND:
	case STATE_MAILFOLDER:
	case STATE_PROFILE:
	case STATE_URL:
		{
			WSTRING& wstr = *pwstr[state_ - STATE_GOROUND];
			wstr = allocWString(pwszOption);
			if (!wstr)
				return QSTATUS_OUTOFMEMORY;
		}
		break;
	default:
		break;
	}
	
	return QSTATUS_SUCCESS;
}
