/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsutil.h>
#include <qsstring.h>
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

qs::CommandLine::CommandLine(CommandLineHandler* pHandler) :
	pImpl_(0)
{
	assert(pHandler);
	
	pImpl_ = new CommandLineImpl();
	pImpl_->pHandler_ = pHandler;
}

qs::CommandLine::~CommandLine()
{
	delete pImpl_;
	pImpl_ = 0;
}

bool qs::CommandLine::parse(const WCHAR* pwszCommandLine)
{
	StringReader reader(pwszCommandLine, false);
	if (!reader)
		return false;
	return parse(&reader);
}

bool qs::CommandLine::parse(Reader* pReader)
{
	StringBuffer<WSTRING> buffer;
	
	bool bInQuote = false;
	bool bPrevSpace = true;
	WCHAR c = 0;
	while (true) {
		size_t nRead = pReader->read(&c, 1);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			c = L' ';
		
		bool bSpace = false;
		if (c == L'\"') {
			bInQuote = !bInQuote;
		}
		else if (c == L'^') {
			nRead = pReader->read(&c, 1);
			if (nRead == -1)
				return false;
			else if (nRead == 0)
				return false;
			buffer.append(c);
		}
		else if (c == L' ' && !bInQuote) {
			if (!bPrevSpace) {
				if (!pImpl_->pHandler_->process(buffer.getCharArray()))
					return false;
				buffer.remove(0, static_cast<size_t>(-1));
			}
			bSpace = true;
		}
		else {
			buffer.append(c);
		}
		bPrevSpace = bSpace;
		
		if (nRead == 0)
			break;
	}
	
	return true;
}


/****************************************************************************
 *
 * CommandLineHandler
 *
 */

qs::CommandLineHandler::~CommandLineHandler()
{
}
