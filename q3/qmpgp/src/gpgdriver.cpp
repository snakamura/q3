/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmpgp.h>

#include <qsconv.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsosutil.h>
#include <qsstream.h>

#include "gpgdriver.h"
#include "util.h"

using namespace qmpgp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * GPGDriver
 *
 */

qmpgp::GPGDriver::GPGDriver(Profile* pProfile) :
	pProfile_(pProfile)
{
}

qmpgp::GPGDriver::~GPGDriver()
{
}

xstring_size_ptr qmpgp::GPGDriver::sign(const CHAR* pszText,
										SignFlag signFlag,
										const WCHAR* pwszUserId,
										const WCHAR* pwszPassphrase) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" ");
	switch (signFlag) {
	case SIGNFLAG_NONE:
		command.append(L"--sign");
		break;
	case SIGNFLAG_CLEARTEXT:
		command.append(L"--clearsign");
		break;
	case SIGNFLAG_DETACH:
		command.append(L"--detach-sign");
		break;
	default:
		assert(false);
		return xstring_size_ptr();
	}
	command.append(L" --local-user \"");
	command.append(pwszUserId);
	command.append(L"\" --armor --batch --passphrase-fd 0");
	
	log.debugf(L"Signing with commandline: %s", command.getCharArray());
	
	XStringBuffer<STRING> buf;
	string_ptr strPassphrase(wcs2mbs(pwszPassphrase));
	if (!buf.append(strPassphrase.get()) || !buf.append('\n') || !buf.append(pszText))
		return xstring_size_ptr();
	const unsigned char* p = reinterpret_cast<const unsigned char*>(buf.getCharArray());
	
	log.debug(L"Data into stdin", p, buf.getLength());
	
	ByteInputStream stdin(p, buf.getLength(), false);
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

xstring_size_ptr qmpgp::GPGDriver::encrypt(const CHAR* pszText,
										   const UserIdList& listRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --encrypt");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" --recipient \"");
		command.append(*it);
		command.append(L"\"");
	}
	command.append(L" --armor --batch");
	
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

xstring_size_ptr qmpgp::GPGDriver::signAndEncrypt(const CHAR* pszText,
												  const WCHAR* pwszUserId,
												  const WCHAR* pwszPassphrase,
												  const UserIdList& listRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --sign");
	command.append(L" --local-user \"");
	command.append(pwszUserId);
	command.append(L"\" --encrypt");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" --recipient \"");
		command.append(*it);
		command.append(L"\"");
	}
	command.append(L" --armor --batch --passphrase-fd 0");
	
	log.debugf(L"Signing and encrypting with commandline: %s", command.getCharArray());
	
	XStringBuffer<STRING> buf;
	string_ptr strPassphrase(wcs2mbs(pwszPassphrase));
	if (!buf.append(strPassphrase.get()) || !buf.append('\n') || !buf.append(pszText))
		return xstring_size_ptr();
	const unsigned char* p = reinterpret_cast<const unsigned char*>(buf.getCharArray());
	
	log.debug(L"Data into stdin", p, buf.getLength());
	
	ByteInputStream stdin(p, buf.getLength(), false);
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

bool qmpgp::GPGDriver::verify(const CHAR* pszContent,
							  const CHAR* pszSignature,
							  wstring_ptr* pwstrUserId) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	wstring_ptr wstrGPG(getCommand());
	
	wstring_ptr wstrSignaturePath(Util::writeTemporaryFile(pszSignature));
	if (!wstrSignaturePath.get())
		return false;
	FileDeleter deleter(wstrSignaturePath.get());
	
	log.debugf(L"Creating a temporary file for verifying: %s", wstrSignaturePath.get());
	log.debug(L"Data in the temporary file",
		reinterpret_cast<const unsigned char*>(pszSignature), strlen(pszSignature));
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --verify");
	command.append(L" --batch --status-fd 2");
	command.append(L" \"");
	command.append(wstrSignaturePath.get());
	command.append(L"\" -");
	
	log.debugf(L"Verifying with commandline: %s", command.getCharArray());
	
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
		return false;
	}
	
	return checkVerified(stderr.getBuffer(), stderr.getLength(), pwstrUserId) == PGPUtility::VERIFY_OK;
}

xstring_size_ptr qmpgp::GPGDriver::decryptAndVerify(const CHAR* pszContent,
													const WCHAR* pwszPassphrase,
													unsigned int* pnVerify,
													wstring_ptr* pwstrUserId) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	if (pwszPassphrase)
		command.append(L" --passphrase-fd 0");
	command.append(L" --batch --status-fd 2");
	
	log.debugf(L"Decrypting and verifying with commandline: %s", command.getCharArray());
	
	const unsigned char* p = 0;
	size_t nLen = 0;
	XStringBuffer<STRING> buf;
	if (pwszPassphrase) {
		string_ptr strPassphrase(wcs2mbs(pwszPassphrase));
		if (!buf.append(strPassphrase.get()) || !buf.append('\n') || !buf.append(pszContent))
			return xstring_size_ptr();
		p = reinterpret_cast<const unsigned char*>(buf.getCharArray());
		nLen = buf.getLength();
	}
	else {
		p = reinterpret_cast<const unsigned char*>(pszContent);
		nLen = strlen(pszContent);
	}
	
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

bool qmpgp::GPGDriver::getAlternatives(const WCHAR* pwszUserId,
									   UserIdList* pList) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --list-keys \"=");
	command.append(pwszUserId);
	command.append(L"\" --batch");
	
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
		if (strncmp(p, "uid", 3) == 0) {
			const CHAR* pStart = p + 3;
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

wstring_ptr qmpgp::GPGDriver::getCommand() const
{
	return pProfile_->getString(L"GPG", L"Command", L"gpg.exe");
}

unsigned int qmpgp::GPGDriver::checkVerified(const unsigned char* pBuf,
											 size_t nLen,
											 wstring_ptr* pwstrUserId)
{
	const CHAR* pData = reinterpret_cast<const CHAR*>(pBuf);
	
	const CHAR* p = 0;
	bool bGood = false;
	BMFindString<STRING> bmfsGood("[GNUPG:] GOODSIG ");
	p = bmfsGood.find(pData, nLen);
	if (p) {
		bGood = true;
	}
	else {
		BMFindString<STRING> bmfsBad("[GNUPG:] BADSIG ");
		p = bmfsBad.find(reinterpret_cast<const CHAR*>(pBuf), nLen);
	}
	if (!p)
		return PGPUtility::VERIFY_NONE;
	
	p += bGood ? 17 : 16;
	while (*p != ' ')
		++p;
	++p;
	
	const CHAR* pStart = p;
	while (static_cast<size_t>(p - pData) < nLen &&
		*p != '\r' && *p != '\n')
		++p;
	
	*pwstrUserId = mbs2wcs(pStart, p - pStart);
	
	return bGood ? PGPUtility::VERIFY_OK : PGPUtility::VERIFY_FAILED;
}
