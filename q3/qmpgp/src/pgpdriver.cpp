/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsinit.h>
#include <qslog.h>
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

xstring_size_ptr qmpgp::PGPDriver::sign(const CHAR* pszText,
										SignFlag signFlag,
										const WCHAR* pwszUserId,
										const WCHAR* pwszPassphrase) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
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
		return xstring_size_ptr();
	}
	command.append(L" -u \"");
	command.append(pwszUserId);
	command.append(L"\" -z \"");
	command.append(pwszPassphrase);
	command.append(L"\" -a -f");
	
	log.debugf(L"Signing with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
	size_t nLen = strlen(pszText);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdin(p, nLen, false);
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, log.isDebugEnabled() ? &stderr : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdout.getBuffer(), stdout.getLength());
	log.debug(L"Data from stderr", stderr.getBuffer(), stderr.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength()), stdout.getLength());
}

xstring_size_ptr qmpgp::PGPDriver::encrypt(const CHAR* pszText,
										   const UserIdList& listRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -e");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" \"");
		command.append(*it);
		command.append(L"\"");
	}
	command.append(L" -a -f");
	
	log.debugf(L"Encrypting with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
	size_t nLen = strlen(pszText);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdin(p, nLen, false);
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, log.isDebugEnabled() ? &stderr : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdout.getBuffer(), stdout.getLength());
	log.debug(L"Data from stderr", stderr.getBuffer(), stderr.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength()), stdout.getLength());
}

xstring_size_ptr qmpgp::PGPDriver::signAndEncrypt(const CHAR* pszText,
												  const WCHAR* pwszUserId,
												  const WCHAR* pwszPassphrase,
												  const UserIdList& listRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -es");
	command.append(L" -u \"");
	command.append(pwszUserId);
	command.append(L"\" -z \"");
	command.append(pwszPassphrase);
	command.append(L"\"");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" \"");
		command.append(*it);
		command.append(L"\"");
	}
	command.append(L" -a -f");
	
	log.debugf(L"Signing and encrypting with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
	size_t nLen = strlen(pszText);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdin(p, nLen, false);
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, log.isDebugEnabled() ? &stderr : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdout.getBuffer(), stdout.getLength());
	log.debug(L"Data from stderr", stderr.getBuffer(), stderr.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength()), stdout.getLength());
}

bool qmpgp::PGPDriver::verify(const CHAR* pszContent,
							  const CHAR* pszSignature,
							  wstring_ptr* pwstrUserId) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	wstring_ptr wstrPGP(getCommand());
	
	wstring_ptr wstrContentPath(Util::writeTemporaryFile(pszContent));
	if (!wstrContentPath.get())
		return false;
	FileDeleter deleter(wstrContentPath.get());
	
	log.debugf(L"Creating a temporary file for verifying: %s", wstrContentPath.get());
	log.debug(L"Data in the temporary file",
		reinterpret_cast<const unsigned char*>(pszContent), strlen(pszContent));
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -f");
	command.append(L" \"");
	command.append(wstrContentPath.get());
	command.append(L"\"");
	
	log.debugf(L"Verifying with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszSignature);
	size_t nLen = strlen(pszSignature);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdin(p, nLen, false);
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, &stderr);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdout.getBuffer(), stdout.getLength());
	log.debug(L"Data from stderr", stderr.getBuffer(), stderr.getLength());
	
	if (nCode != 0 && nCode != 1) {
		log.errorf(L"Command exited with: %d", nCode);
		return false;
	}
	
	return checkVerified(stderr.getBuffer(), stderr.getLength(), pwstrUserId) == PGPUtility::VERIFY_OK;
}

xstring_size_ptr qmpgp::PGPDriver::decryptAndVerify(const CHAR* pszContent,
													const WCHAR* pwszPassphrase,
													unsigned int* pnVerify,
													wstring_ptr* pwstrUserId) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	if (pwszPassphrase) {
		command.append(L" -z \"");
		command.append(pwszPassphrase);
		command.append(L"\"");
	}
	command.append(L" -f");
	
	log.debugf(L"Decrypting and verifying with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszContent);
	size_t nLen = strlen(pszContent);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdin(p, nLen, false);
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), &stdin, &stdout, &stderr);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdout.getBuffer(), stdout.getLength());
	log.debug(L"Data from stderr", stderr.getBuffer(), stderr.getLength());
	
	if (nCode != 0 && nCode != 1) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	*pnVerify = checkVerified(stderr.getBuffer(), stderr.getLength(), pwstrUserId);
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(stdout.getBuffer()), stdout.getLength()), stdout.getLength());
}

bool qmpgp::PGPDriver::getAlternatives(const WCHAR* pwszUserId,
									   UserIdList* pList) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -kv \"");
	command.append(pwszUserId);
	command.append(L"\"");
	
	log.debugf(L"Getting alternatives with commandline: %s", command.getCharArray());
	
	ByteOutputStream stdout;
	ByteOutputStream stderr;
	
	int nCode = Process::exec(command.getCharArray(), 0, &stdout, log.isDebugEnabled() ? &stderr : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdout.getBuffer(), stdout.getLength());
	log.debug(L"Data from stderr", stderr.getBuffer(), stderr.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return false;
	}
	
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
