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
										size_t nLen,
										SignFlag signFlag,
										const WCHAR* pwszUserId,
										const WCHAR* pwszPassphrase) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
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
	command.append(L"\" --armor --batch --no-tty --passphrase-fd 0");
	
	log.debugf(L"Signing with commandline: %s", command.getCharArray());
	
	XStringBuffer<STRING> buf;
	string_ptr strPassphrase(wcs2mbs(pwszPassphrase));
	if (!buf.append(strPassphrase.get()) || !buf.append('\n') || !buf.append(pszText, nLen))
		return xstring_size_ptr();
	const unsigned char* p = reinterpret_cast<const unsigned char*>(buf.getCharArray());
	
	log.debug(L"Data into stdin", p, buf.getLength());
	
	ByteInputStream stdinStream(p, buf.getLength(), false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(), &stdinStream,
		&stdoutStream, log.isDebugEnabled() ? &stderrStream : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(
		stdoutStream.getBuffer()), stdoutStream.getLength()), stdoutStream.getLength());
}

xstring_size_ptr qmpgp::GPGDriver::encrypt(const CHAR* pszText,
										   size_t nLen,
										   const UserIdList& listRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --encrypt");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" --recipient \"");
		command.append(*it);
		command.append(L"\"");
	}
	command.append(L" --armor --batch --no-tty");
	
	log.debugf(L"Encrypting with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdinStream(p, nLen, false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(), &stdinStream,
		&stdoutStream, log.isDebugEnabled() ? &stderrStream : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(
		stdoutStream.getBuffer()), stdoutStream.getLength()), stdoutStream.getLength());
}

xstring_size_ptr qmpgp::GPGDriver::signAndEncrypt(const CHAR* pszText,
												  size_t nLen,
												  const WCHAR* pwszUserId,
												  const WCHAR* pwszPassphrase,
												  const UserIdList& listRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
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
	command.append(L" --armor --batch --no-tty --passphrase-fd 0");
	
	log.debugf(L"Signing and encrypting with commandline: %s", command.getCharArray());
	
	XStringBuffer<STRING> buf;
	string_ptr strPassphrase(wcs2mbs(pwszPassphrase));
	if (!buf.append(strPassphrase.get()) || !buf.append('\n') || !buf.append(pszText, nLen))
		return xstring_size_ptr();
	const unsigned char* p = reinterpret_cast<const unsigned char*>(buf.getCharArray());
	
	log.debug(L"Data into stdin", p, buf.getLength());
	
	ByteInputStream stdinStream(p, buf.getLength(), false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(), &stdinStream,
		&stdoutStream, log.isDebugEnabled() ? &stderrStream : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(
		stdoutStream.getBuffer()), stdoutStream.getLength()), stdoutStream.getLength());
}

bool qmpgp::GPGDriver::verify(const CHAR* pszContent,
							  size_t nLen,
							  const CHAR* pszSignature,
							  const AddressListParser* pFrom,
							  const AddressListParser* pSender,
							  unsigned int* pnVerify,
							  wstring_ptr* pwstrUserId,
							  wstring_ptr* pwstrInfo) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszContent);
	
	wstring_ptr wstrGPG(getCommand());
	
	size_t nSignatureLen = strlen(pszSignature);
	wstring_ptr wstrSignaturePath(Util::writeTemporaryFile(pszSignature, nSignatureLen));
	if (!wstrSignaturePath.get())
		return false;
	FileDeleter deleter(wstrSignaturePath.get());
	
	log.debugf(L"Creating a temporary file for verifying: %s", wstrSignaturePath.get());
	log.debug(L"Data in the temporary file",
		reinterpret_cast<const unsigned char*>(pszSignature), nSignatureLen);
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --verify --verify-options show-uid-validity");
	command.append(L" --batch --no-tty --status-fd 2");
	command.append(L" \"");
	command.append(wstrSignaturePath.get());
	command.append(L"\" -");
	
	log.debugf(L"Verifying with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszContent);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdinStream(p, nLen, false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(),
		&stdinStream, &stdoutStream, &stderrStream);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	*pnVerify = parseStatus(stderrStream.getBuffer(), stderrStream.getLength(),
		pFrom, pSender, pwstrUserId, pwstrInfo);
	
	return true;
}

xstring_size_ptr qmpgp::GPGDriver::decryptAndVerify(const CHAR* pszContent,
													size_t nLen,
													const WCHAR* pwszPassphrase,
													const AddressListParser* pFrom,
													const AddressListParser* pSender,
													unsigned int* pnVerify,
													wstring_ptr* pwstrUserId,
													wstring_ptr* pwstrInfo) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszContent);
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	if (pwszPassphrase)
		command.append(L" --passphrase-fd 0");
	command.append(L" --verify-options show-uid-validity");
	command.append(L" --batch --no-tty --status-fd 2");
	
	log.debugf(L"Decrypting and verifying with commandline: %s", command.getCharArray());
	
	const unsigned char* p = 0;
	XStringBuffer<STRING> buf;
	if (pwszPassphrase) {
		string_ptr strPassphrase(wcs2mbs(pwszPassphrase));
		if (!buf.append(strPassphrase.get()) || !buf.append('\n') || !buf.append(pszContent, nLen))
			return xstring_size_ptr();
		p = reinterpret_cast<const unsigned char*>(buf.getCharArray());
		nLen = buf.getLength();
	}
	else {
		p = reinterpret_cast<const unsigned char*>(pszContent);
	}
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdinStream(p, nLen, false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(),
		&stdinStream, &stdoutStream, &stderrStream);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	*pnVerify = parseStatus(stderrStream.getBuffer(),
		stderrStream.getLength(), pFrom, pSender, pwstrUserId, pwstrInfo);
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(
		stdoutStream.getBuffer()), stdoutStream.getLength()), stdoutStream.getLength());
}

wstring_ptr qmpgp::GPGDriver::getCommand() const
{
	return pProfile_->getString(L"GPG", L"Command");
}

unsigned int qmpgp::GPGDriver::parseStatus(const unsigned char* pBuf,
										   size_t nLen,
										   const AddressListParser* pFrom,
										   const AddressListParser* pSender,
										   wstring_ptr* pwstrUserId,
										   wstring_ptr* pwstrInfo) const
{
	unsigned int nVerify = PGPUtility::VERIFY_NONE;
	StringBuffer<STRING> bufInfo;
	
	const CHAR* pBegin = reinterpret_cast<const CHAR*>(pBuf);
	const CHAR* pEnd = pBegin + nLen;
	const CHAR* p = pBegin;
	while (pBegin < pEnd) {
		while (p < pEnd && *p != '\r' && *p != '\n')
			++p;
		
		if (p - pBegin > 9 && strncmp(pBegin, "[GNUPG:] ", 9) == 0) {
			if (p - pBegin > 18 && strncmp(pBegin + 9, "VALIDSIG ", 8) == 0) {
				nVerify = PGPUtility::VERIFY_OK;
				
				const CHAR* pFingerPrint = pBegin + 18;
				const CHAR* pFingerPrintEnd = pFingerPrint;
				while (pFingerPrintEnd < p && *pFingerPrintEnd != ' ')
					++pFingerPrintEnd;
				if (pFingerPrint != pFingerPrintEnd) {
					wstring_ptr wstrFingerPrint(mbs2wcs(pFingerPrint, pFingerPrintEnd - pFingerPrint));
					bool bMatch = false;
					if (!getUserIdFromFingerPrint(wstrFingerPrint.get(), pFrom, pSender, pwstrUserId, &bMatch))
						nVerify = PGPUtility::VERIFY_FAILED;
					else if (!bMatch)
						nVerify |= PGPUtility::VERIFY_ADDRESSNOTMATCH;
				}
				else {
					nVerify = PGPUtility::VERIFY_FAILED;
				}
			}
			else if (p - pBegin > 16 &&
				(strncmp(pBegin + 9, "BADSIG ", 7) == 0 ||
				strncmp(pBegin + 9, "ERRSIG ", 7) == 0)) {
				nVerify = PGPUtility::VERIFY_FAILED;
			}
		}
		else {
			bufInfo.append(pBegin, p - pBegin);
			bufInfo.append('\n');
		}
		
		if (p + 1 < pEnd && *p == '\r' && *(p + 1) == '\n')
			++p;
		++p;
		
		pBegin = p;
	}
	
	*pwstrInfo = mbs2wcs(bufInfo.getCharArray(), bufInfo.getLength());
	
	return nVerify;
}

bool qmpgp::GPGDriver::getUserIdFromFingerPrint(const WCHAR* pwszFingerPrint,
												const AddressListParser* pFrom,
												const AddressListParser* pSender,
												wstring_ptr* pwstrUserId,
												bool* pbMatch) const
{
	assert(pwszFingerPrint);
	assert(pwstrUserId);
	assert(pbMatch);
	
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --list-key --with-colon --fixed-list-mode --batch --no-tty ");
	command.append(pwszFingerPrint);
	
	log.debugf(L"Getting user info with commandline: %s", command.getCharArray());
	
	ByteOutputStream stdoutStream;
	int nCode = Process::exec(command.getCharArray(), 0, &stdoutStream, 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return false;
	}
	
	wstring_ptr wstrPrimaryUserId;
	
	const CHAR* pBegin = reinterpret_cast<const CHAR*>(stdoutStream.getBuffer());
	const CHAR* pEnd = pBegin + stdoutStream.getLength();
	const CHAR* p = pBegin;
	while (pBegin < pEnd) {
		while (p < pEnd && *p != '\r' && *p != '\n')
			++p;
		
		string_ptr strLine(allocString(pBegin, p - pBegin));
		CHAR* pNext = strLine.get();
		const CHAR* pToken = getToken(&pNext, ':');
		if (pToken && strcmp(pToken, "uid") == 0) {
			pToken = getToken(&pNext, ':');
			if (pToken) {
				if (*pToken == 'u' || *pToken == 'f') {
					for (int n = 0; n < 8 && pToken; ++n)
						pToken = getToken(&pNext, ':');
					if (pToken) {
						const CHAR* pszUserId = pToken;
						wstring_ptr wstrAddress(getAddressFromUserId(pszUserId));
						if (wstrAddress.get()) {
							if ((pFrom && pFrom->contains(wstrAddress.get())) ||
								(pSender && pSender->contains(wstrAddress.get()))) {
								*pwstrUserId = mbs2wcs(pszUserId);
								*pbMatch = true;
								return true;
							}
							else if (!wstrPrimaryUserId.get()) {
								wstrPrimaryUserId = mbs2wcs(pszUserId);
							}
						}
					}
				}
			}
		}
		
		if (p + 1 < pEnd && *p == '\r' && *(p + 1) == '\n')
			++p;
		++p;
		
		pBegin = p;
	}
	
	if (!wstrPrimaryUserId.get())
		return false;
	
	*pwstrUserId = wstrPrimaryUserId;
	*pbMatch = false;
	
	return true;
}

wstring_ptr qmpgp::GPGDriver::getAddressFromUserId(const CHAR* pszUserId)
{
	size_t nLen = strlen(pszUserId);
	if (*(pszUserId + nLen - 1) != '>')
		return 0;
	
	const CHAR* p = strrchr(pszUserId, '<');
	if (!p)
		return 0;
	
	return mbs2wcs(p + 1, nLen - (p - pszUserId) - 2);
}

const CHAR* qmpgp::GPGDriver::getToken(CHAR** pp,
									   CHAR c)
{
	if (!**pp)
		return 0;
	
	const CHAR* p = *pp;
	while (**pp != c)
		++*pp;
	**pp = '\0';
	++*pp;
	
	return p;
}
