/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsutil.h>
#include <qsstring.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsstream.h>

using namespace qs;


/****************************************************************************
 *
 * CommandLineImpl
 *
 */

struct qs::CommandLineImpl
{
	CommandLineHandler* pHandler_;
};


/****************************************************************************
 *
 * CommandLine
 *
 */

qs::CommandLine::CommandLine(CommandLineHandler* pHandler, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pHandler);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pHandler_ = pHandler;
}

qs::CommandLine::~CommandLine()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::CommandLine::parse(const WCHAR* pwszCommandLine)
{
	DECLARE_QSTATUS();
	
	StringReader reader(pwszCommandLine, &status);
	CHECK_QSTATUS();
	
	return parse(reader);
}

QSTATUS qs::CommandLine::parse(Reader& reader)
{
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buffer(&status);
	CHECK_QSTATUS();
	
	bool bInQuote = false;
	bool bPrevSpace = true;
	WCHAR c = 0;
	size_t nRead = 0;
	while (nRead != static_cast<size_t>(-1)) {
		status = reader.read(&c, 1, &nRead);
		CHECK_QSTATUS();
		if (nRead == static_cast<size_t>(-1))
			c = L' ';
		
		bool bSpace = false;
		if (c == L'\"') {
			bInQuote = !bInQuote;
		}
		else if (c == L'\\') {
			status = reader.read(&c, 1, &nRead);
			CHECK_QSTATUS();
			if (nRead == static_cast<size_t>(-1))
				return QSTATUS_FAIL;
			status = buffer.append(c);
			CHECK_QSTATUS();
		}
		else if (c == L' ' && !bInQuote) {
			if (!bPrevSpace) {
				status = pImpl_->pHandler_->process(buffer.getCharArray());
				CHECK_QSTATUS();
				status = buffer.remove(0, static_cast<size_t>(-1));
				CHECK_QSTATUS();
			}
			bSpace = true;
		}
		else {
			status = buffer.append(c);
			CHECK_QSTATUS();
		}
		bPrevSpace = bSpace;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * CommandLineHandler
 *
 */

qs::CommandLineHandler::~CommandLineHandler()
{
}
