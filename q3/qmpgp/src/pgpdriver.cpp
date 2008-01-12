/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
										size_t nLen,
										SignFlag signFlag,
										const WCHAR* pwszUserId,
										PGPPassphraseCallback* pPassphraseCallback) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
	wstring_ptr wstrPassphrase(pPassphraseCallback->getPassphrase(0));
	if (!wstrPassphrase.get())
		return xstring_size_ptr();
	
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
	command.append(wstrPassphrase.get());
	command.append(L"\" -a -f");
	
	log.debugf(L"Signing with commandline: %s", command.getCharArray());
	
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

xstring_size_ptr qmpgp::PGPDriver::encrypt(const CHAR* pszText,
										   size_t nLen,
										   const UserIdList& listRecipient,
										   const UserIdList& listHiddenRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -e");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" \"");
		command.append(*it);
		command.append(L"\"");
	}
	for (UserIdList::const_iterator it = listHiddenRecipient.begin(); it != listHiddenRecipient.end(); ++it) {
		command.append(L" \"");
		command.append(*it);
		command.append(L"\"");
	}
	command.append(L" -a -f");
	
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

xstring_size_ptr qmpgp::PGPDriver::signAndEncrypt(const CHAR* pszText,
												  size_t nLen,
												  const WCHAR* pwszUserId,
												  PGPPassphraseCallback* pPassphraseCallback,
												  const UserIdList& listRecipient,
												  const UserIdList& listHiddenRecipient) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
	wstring_ptr wstrPassphrase(pPassphraseCallback->getPassphrase(0));
	if (!wstrPassphrase.get())
		return xstring_size_ptr();
	
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	command.append(L" -es");
	command.append(L" -u \"");
	command.append(pwszUserId);
	command.append(L"\" -z \"");
	command.append(wstrPassphrase.get());
	command.append(L"\"");
	for (UserIdList::const_iterator it = listRecipient.begin(); it != listRecipient.end(); ++it) {
		command.append(L" \"");
		command.append(*it);
		command.append(L"\"");
	}
	for (UserIdList::const_iterator it = listHiddenRecipient.begin(); it != listHiddenRecipient.end(); ++it) {
		command.append(L" \"");
		command.append(*it);
		command.append(L"\"");
	}
	command.append(L" -a -f");
	
	log.debugf(L"Signing and encrypting with commandline: %s", command.getCharArray());
	
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

bool qmpgp::PGPDriver::verify(const CHAR* pszContent,
							  size_t nLen,
							  const CHAR* pszSignature,
							  const AddressListParser* pFrom,
							  const AddressListParser* pSender,
							  unsigned int* pnVerify,
							  wstring_ptr* pwstrUserId,
							  wstring_ptr* pwstrInfo) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	if (nLen == -1)
		nLen = strlen(pszContent);
	
	wstring_ptr wstrPGP(getCommand());
	
	wstring_ptr wstrContentPath(Util::writeTemporaryFile(pszContent, nLen));
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
	size_t nSignatureLen = strlen(pszSignature);
	
	log.debug(L"Data into stdin", p, nSignatureLen);
	
	ByteInputStream stdinStream(p, nSignatureLen, false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(),
		&stdinStream, &stdoutStream, &stderrStream);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	*pwstrInfo = mbs2wcs(reinterpret_cast<const CHAR*>(
		stderrStream.getBuffer()), stderrStream.getLength());
	
	if (nCode != 0 && nCode != 1) {
		log.errorf(L"Command exited with: %d", nCode);
		return false;
	}
	
	*pnVerify = checkVerified(stderrStream.getBuffer(), stderrStream.getLength(), pwstrUserId);
	
	return true;
}

xstring_size_ptr qmpgp::PGPDriver::decryptAndVerify(const CHAR* pszContent,
													size_t nLen,
													PGPPassphraseCallback* pPassphraseCallback,
													const AddressListParser* pFrom,
													const AddressListParser* pSender,
													unsigned int* pnVerify,
													wstring_ptr* pwstrUserId,
													wstring_ptr* pwstrInfo) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::PGPDriver");
	
	if (nLen == -1)
		nLen = strlen(pszContent);
	
	wstring_ptr wstrPGP(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrPGP.get());
	if (pPassphraseCallback) {
		wstring_ptr wstrPassphrase(pPassphraseCallback->getPassphrase(0));
		if (!wstrPassphrase.get())
			return xstring_size_ptr();
		command.append(L" -z \"");
		command.append(wstrPassphrase.get());
		command.append(L"\"");
	}
	command.append(L" -f");
	
	log.debugf(L"Decrypting and verifying with commandline: %s", command.getCharArray());
	
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
	
	*pwstrInfo = mbs2wcs(reinterpret_cast<const CHAR*>(
		stderrStream.getBuffer()), stderrStream.getLength());
	
	if (nCode != 0 && nCode != 1) {
		log.errorf(L"Command exited with: %d", nCode);
		return xstring_size_ptr();
	}
	
	*pnVerify = checkVerified(stderrStream.getBuffer(), stderrStream.getLength(), pwstrUserId);
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(
		stdoutStream.getBuffer()), stdoutStream.getLength()), stdoutStream.getLength());
}

#if 0
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
	
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(), 0,
		&stdoutStream, log.isDebugEnabled() ? &stderrStream : 0);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	if (nCode != 0) {
		log.errorf(L"Command exited with: %d", nCode);
		return false;
	}
	
	const CHAR* p = reinterpret_cast<const CHAR*>(stdoutStream.getBuffer());
	size_t nLen = stdoutStream.getLength();
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
#endif

wstring_ptr qmpgp::PGPDriver::getCommand() const
{
	return pProfile_->getString(L"PGP", L"Command");
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
