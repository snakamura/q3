/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>
#include <qsstream.h>

#include <qmpgp.h>

#include "pgpdriver.h"
#include "util.h"

using namespace qmpgp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * PGPDriver
 *
 */

qmpgp::PGPDriver::PGPDriver(Profile* pProfile) :
	pProfile_(pProfile)
{
}

qmpgp::PGPDriver::~PGPDriver()
{
}

xstring_ptr qmpgp::PGPDriver::sign(const CHAR* pszText,
								   SignFlag signFlag,
								   const WCHAR* pwszUserId,
								   const WCHAR* pwszPassphrase) const
{
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" ");
	switch (signFlag) {
	case SIGNFLAG_NONE:
		command.append(L"-s");
		break;
	case SIGNFLAG_CLEARTEXT:
		command.append(L"-st");
		break;
	case SIGNFLAG_DETACH:
		command.append(L"-sb");
		break;
	default:
		assert(false);
		return 0;
	}
	command.append(L" -u ");
	command.append(pwszUserId);
	command.append(L" -z ");
	command.append(pwszPassphrase);
	command.append(L" -a -f");
	
	ByteInputStream stdin(reinterpret_cast<const unsigned char*>(pszText), strlen(pszText), false);
	ByteOutputStream stdout;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, 0);
	if (nCode != 0)
		return 0;
	
	return allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength());
}

xstring_ptr qmpgp::PGPDriver::encrypt(const CHAR* pszText,
									  const UserIdList& listRecipient) const
{
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -e");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" ");
		command.append(*it);
	}
	command.append(L" -a -f");
	
	ByteInputStream stdin(reinterpret_cast<const unsigned char*>(pszText), strlen(pszText), false);
	ByteOutputStream stdout;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, 0);
	if (nCode != 0)
		return 0;
	
	return allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength());
}

xstring_ptr qmpgp::PGPDriver::signAndEncrypt(const CHAR* pszText,
											 const WCHAR* pwszUserId,
											 const WCHAR* pwszPassphrase,
											 const UserIdList& listRecipient) const
{
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -es");
	command.append(L" -u ");
	command.append(pwszUserId);
	command.append(L" -z ");
	command.append(pwszPassphrase);
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" ");
		command.append(*it);
	}
	command.append(L" -a -f");
	
	ByteInputStream stdin(reinterpret_cast<const unsigned char*>(pszText), strlen(pszText), false);
	ByteOutputStream stdout;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, 0);
	if (nCode != 0)
		return 0;
	
	return allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength());
}

bool qmpgp::PGPDriver::verify(const CHAR* pszContent,
							  const CHAR* pszSignature,
							  wstring_ptr* pwstrUserId) const
{
	wstring_ptr wstrPGP(getCommand());
	
	wstring_ptr wstrContentPath(Util::writeTemporaryFile(pszContent));
	if (!wstrContentPath.get())
		return false;
	FileDeleter deleter(wstrContentPath.get());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -f");
	command.append(L" \"");
	command.append(wstrContentPath.get());
	command.append(L"\"");
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszSignature);
	ByteInputStream stdin(p, strlen(pszSignature), false);
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, &stderr);
	if (nCode != 0 && nCode != 1)
		return 0;
	
	return checkVerified(stderr.getBuffer(), stderr.getLength(), pwstrUserId) == PGPUtility::VERIFY_OK;
}

xstring_ptr qmpgp::PGPDriver::decryptAndVerify(const CHAR* pszContent,
											   const WCHAR* pwszPassphrase,
											   unsigned int* pnVerify,
											   wstring_ptr* pwstrUserId) const
{
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	if (pwszPassphrase) {
		command.append(L" -z ");
		command.append(pwszPassphrase);
	}
	command.append(L" -f");
	
	ByteInputStream stdin(reinterpret_cast<const unsigned char*>(pszContent), strlen(pszContent), false);
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, &stderr);
	if (nCode != 0 && nCode != 1)
		return 0;
	
	*pnVerify = checkVerified(stderr.getBuffer(), stderr.getLength(), pwstrUserId);
	
	return allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength());
}

bool qmpgp::PGPDriver::getAlternatives(const WCHAR* pwszUserId,
									   UserIdList* pList) const
{
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -kv \"");
	command.append(pwszUserId);
	command.append(L"\"");
	
	ByteOutputStream stdout;
	
	int nCode = Process::exec(command.getCharArray(), 0, &stdout, 0);
	if (nCode != 0)
		return false;
	
	const CHAR* p = reinterpret_cast<const CHAR*>(stdout.getBuffer());
	size_t nLen = stdout.getLength();
	while (nLen > 3) {
		if (*p == ' ') {
			const CHAR* pStart = p;
			while (*pStart == ' ')
				++pStart;
			const CHAR* pEnd = pStart;
			while (*pEnd != '\r' && *pEnd != '\n')
				++pEnd;
			wstring_ptr wstrUserId(mbs2wcs(pStart, pEnd - pStart));
			pList->push_back(wstrUserId.get());
			wstrUserId.release();
		}
		while (*p != '\n') {
			++p;
			--nLen;
		}
		++p;
		--nLen;
	}
	
	return true;
}

wstring_ptr qmpgp::PGPDriver::getCommand() const
{
	return pProfile_->getString(L"PGP", L"Command", L"pgp.exe");
}

unsigned int qmpgp::PGPDriver::checkVerified(const unsigned char* pBuf,
											 size_t nLen,
											 wstring_ptr* pwstrUserId)
{
	const CHAR* pData = reinterpret_cast<const CHAR*>(pBuf);
	
	const CHAR* p = 0;
	bool bGood = false;
	BMFindString<STRING> bmfsGood("Good signature from user \"");
	p = bmfsGood.find(pData, nLen);
	if (p) {
		bGood = true;
	}
	else {
		BMFindString<STRING> bmfsBad("Bad signature from user \"");
		p = bmfsBad.find(reinterpret_cast<const CHAR*>(pBuf), nLen);
	}
	if (!p)
		return PGPUtility::VERIFY_NONE;
	
	p += bGood ? 26 : 25;
	
	const CHAR* pStart = p;
	while (static_cast<size_t>(p - pData) < nLen &&
		*p != '\"' && *p != '\r' && *p != '\n')
		++p;
	
	*pwstrUserId = mbs2wcs(pStart, p - pStart);
	
	return bGood ? PGPUtility::VERIFY_OK : PGPUtility::VERIFY_FAILED;
}
