/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmpgp.h>

#include <qsconv.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsstream.h>

#include <process.h>

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
										PGPPassphraseCallback* pPassphraseCallback) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
	StatusHandler statusHandler(this, pPassphraseCallback);
	if (!statusHandler.open())
		return xstring_size_ptr();
	
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
	command.append(L"\" --armor --no-tty");
	command.append(statusHandler.getOption().get());
	
	log.debugf(L"Signing with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdinStream(p, nLen, false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(), &stdinStream,
		&stdoutStream, log.isDebugEnabled() ? &stderrStream : 0,
		&StatusHandler::process, &statusHandler);
	
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
										   const UserIdList& listRecipient,
										   bool bThrowKeyId) const
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
	if (bThrowKeyId)
		command.append(L" --throw-keyid");
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
												  PGPPassphraseCallback* pPassphraseCallback,
												  const UserIdList& listRecipient,
												  bool bThrowKeyId) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszText);
	
	StatusHandler statusHandler(this, pPassphraseCallback);
	if (!statusHandler.open())
		return xstring_size_ptr();
	
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
	if (bThrowKeyId)
		command.append(L" --throw-keyid");
	command.append(L" --armor --no-tty");
	command.append(statusHandler.getOption().get());
	
	log.debugf(L"Signing and encrypting with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdinStream(p, nLen, false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(), &stdinStream,
		&stdoutStream, log.isDebugEnabled() ? &stderrStream : 0,
		&StatusHandler::process, &statusHandler);
	
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
	
	StatusHandler statusHandler(this, 0, pFrom, pSender);
	if (!statusHandler.open())
		return false;
	
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
	command.append(L" --no-tty");
	command.append(statusHandler.getOption().get());
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
		&stdinStream, &stdoutStream, &stderrStream,
		&StatusHandler::process, &statusHandler);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	*pnVerify = statusHandler.getVerify();
	*pwstrUserId = statusHandler.getUserId();
	*pwstrInfo = mbs2wcs(reinterpret_cast<const CHAR*>(stderrStream.getBuffer()), stderrStream.getLength());
	
	return true;
}

xstring_size_ptr qmpgp::GPGDriver::decryptAndVerify(const CHAR* pszContent,
													size_t nLen,
													PGPPassphraseCallback* pPassphraseCallback,
													const AddressListParser* pFrom,
													const AddressListParser* pSender,
													unsigned int* pnVerify,
													wstring_ptr* pwstrUserId,
													wstring_ptr* pwstrInfo) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmpgp::GPGDriver");
	
	if (nLen == -1)
		nLen = strlen(pszContent);
	
	StatusHandler statusHandler(this, pPassphraseCallback, pFrom, pSender);
	if (!statusHandler.open())
		return xstring_size_ptr();
	
	wstring_ptr wstrGPG(getCommand());
	
	StringBuffer<WSTRING> command;
	command.append(wstrGPG.get());
	command.append(L" --verify-options show-uid-validity");
	command.append(L" --no-tty");
	command.append(statusHandler.getOption().get());
	
	log.debugf(L"Decrypting and verifying with commandline: %s", command.getCharArray());
	
	const unsigned char* p = reinterpret_cast<const unsigned char*>(pszContent);
	
	log.debug(L"Data into stdin", p, nLen);
	
	ByteInputStream stdinStream(p, nLen, false);
	ByteOutputStream stdoutStream;
	ByteOutputStream stderrStream;
	
	int nCode = Process::exec(command.getCharArray(),
		&stdinStream, &stdoutStream, &stderrStream,
		&StatusHandler::process, &statusHandler);
	
	log.debugf(L"Command exited with: %d", nCode);
	log.debug(L"Data from stdout", stdoutStream.getBuffer(), stdoutStream.getLength());
	log.debug(L"Data from stderr", stderrStream.getBuffer(), stderrStream.getLength());
	
	*pnVerify = statusHandler.getVerify();
	*pwstrUserId = statusHandler.getUserId();
	*pwstrInfo = mbs2wcs(reinterpret_cast<const CHAR*>(stderrStream.getBuffer()), stderrStream.getLength());
	
	return xstring_size_ptr(allocXString(reinterpret_cast<const CHAR*>(
		stdoutStream.getBuffer()), stdoutStream.getLength()), stdoutStream.getLength());
}

wstring_ptr qmpgp::GPGDriver::getCommand() const
{
	return pProfile_->getString(L"GPG", L"Command");
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

wstring_ptr qmpgp::GPGDriver::formatHandle(HANDLE h)
{
	WCHAR wsz[32];
	wsprintf(wsz, L"%lu", reinterpret_cast<long>(h));
	return allocWString(wsz);
}


/****************************************************************************
 *
 * GPGDriver::StatusHandler
 *
 */

qmpgp::GPGDriver::StatusHandler::StatusHandler(const GPGDriver* pDriver,
											   PGPPassphraseCallback* pPassphraseCallback) :
	pDriver_(pDriver),
	pPassphraseCallback_(pPassphraseCallback),
	pFrom_(0),
	pSender_(0),
	nVerify_(PGPUtility::VERIFY_NONE)
{
}

qmpgp::GPGDriver::StatusHandler::StatusHandler(const GPGDriver* pDriver,
											   qm::PGPPassphraseCallback* pPassphraseCallback,
											   const AddressListParser* pFrom,
											   const AddressListParser* pSender) :
	pDriver_(pDriver),
	pPassphraseCallback_(pPassphraseCallback),
	pFrom_(pFrom),
	pSender_(pSender),
	nVerify_(PGPUtility::VERIFY_NONE)
{
}

qmpgp::GPGDriver::StatusHandler::~StatusHandler()
{
}

bool qmpgp::GPGDriver::StatusHandler::open()
{
	return Process::createInheritablePipe(&hReadCommand_, &hWriteCommand_, true) &&
		Process::createInheritablePipe(&hReadStatus_, &hWriteStatus_, false);
}

wstring_ptr qmpgp::GPGDriver::StatusHandler::getOption() const
{
	StringBuffer<WSTRING> buf;
	buf.append(L" --command-fd ");
	buf.append(GPGDriver::formatHandle(hReadCommand_.get()).get());
	buf.append(L" --status-fd ");
	buf.append(GPGDriver::formatHandle(hWriteStatus_.get()).get());
	return buf.getString();
}

unsigned int qmpgp::GPGDriver::StatusHandler::getVerify() const
{
	return nVerify_;
}

wstring_ptr qmpgp::GPGDriver::StatusHandler::getUserId() const
{
	if (!wstrUserId_.get())
		return 0;
	return allocWString(wstrUserId_.get());
}

bool qmpgp::GPGDriver::StatusHandler::process(const HANDLE* pHandles,
											  size_t n,
											  void* pParam)
{
	StatusHandler* p = static_cast<StatusHandler*>(pParam);
	return p->process();
}

bool qmpgp::GPGDriver::StatusHandler::process()
{
	hWriteStatus_.close();
	hReadCommand_.close();
	
	XStringBuffer<STRING> buf;
	const size_t nSize = 1024;
	while (true) {
		XStringBufferLock<STRING> lock(&buf, nSize);
		unsigned char* p = reinterpret_cast<unsigned char*>(lock.get());
		if (!p)
			return false;
		
		DWORD dwRead = 0;
		BOOL b = ::ReadFile(hReadStatus_.get(), p, nSize, &dwRead, 0);
		if (!b && ::GetLastError() != ERROR_BROKEN_PIPE)
			return false;
		else if (!b || dwRead == 0)
			break;
		
		lock.unlock(dwRead);
		
		if (!processBuffer(&buf))
			return false;
	}
	
	hWriteCommand_.close();
	hReadStatus_.close();
	
	return true;
}

bool qmpgp::GPGDriver::StatusHandler::processBuffer(XStringBuffer<STRING>* pBuf)
{
	assert(pBuf);
	
	while (true) {
		string_ptr strLine(fetchLine(pBuf));
		if (!strLine.get())
			break;
		
		size_t nLen = strlen(strLine.get());
		if (nLen <= 9 || strncmp(strLine.get(), "[GNUPG:] ", 9) != 0)
			continue;
		
		if (pPassphraseCallback_) {
			if (nLen > 21 && strncmp(strLine.get() + 9, "USERID_HINT ", 12) == 0) {
				const CHAR* pKeyId = strLine.get() + 21;
				const CHAR* pUserId = strchr(pKeyId, ' ');
				if (pUserId) {
					strHintKeyId_ = allocString(pKeyId, pUserId - pKeyId);
					wstrHintUserId_ = mbs2wcs(pUserId + 1);
				}
			}
			else if (nLen > 25 && strncmp(strLine.get() + 9, "NEED_PASSPHRASE ", 16) == 0) {
				const WCHAR* pwszUserId = 0;
				
				const CHAR* pKeyId = strLine.get() + 25;
				const CHAR* pKeyIdEnd = strchr(pKeyId, ' ');
				if (pKeyIdEnd &&
					pKeyIdEnd - pKeyId == strlen(strHintKeyId_.get()) &&
					strncmp(pKeyId, strHintKeyId_.get(), pKeyIdEnd - pKeyId) == 0)
					pwszUserId = wstrHintUserId_.get();
				
				wstring_ptr wstrPassphrase(pPassphraseCallback_->getPassphrase(pwszUserId));
				if (!wstrPassphrase.get()) {
					hWriteCommand_.close();
					return false;
				}
				string_ptr str(wcs2mbs(concat(wstrPassphrase.get(), L"\n").get()));
				size_t nLen = strlen(str.get());
				DWORD dwWritten = 0;
				if (!::WriteFile(hWriteCommand_.get(), str.get(), static_cast<DWORD>(nLen), &dwWritten, 0) ||
					dwWritten != nLen) {
					hWriteCommand_.close();
					return false;
				}
			}
			else if (nLen > 24 && strncmp(strLine.get() + 9, "BAD_PASSPHRASE ", 15) == 0) {
				pPassphraseCallback_->clear();
			}
		}
		if (nLen > 18 && strncmp(strLine.get() + 9, "VALIDSIG ", 9) == 0) {
			nVerify_ = PGPUtility::VERIFY_OK;
			
			const CHAR* pFingerPrint = strLine.get() + 18;
			const CHAR* pFingerPrintEnd = pFingerPrint;
			while (*pFingerPrintEnd && *pFingerPrintEnd != ' ')
				++pFingerPrintEnd;
			if (pFingerPrint != pFingerPrintEnd) {
				wstring_ptr wstrFingerPrint(mbs2wcs(pFingerPrint, pFingerPrintEnd - pFingerPrint));
				bool bMatch = false;
				if (!pDriver_->getUserIdFromFingerPrint(wstrFingerPrint.get(),
					pFrom_, pSender_, &wstrUserId_, &bMatch))
					nVerify_ = PGPUtility::VERIFY_FAILED;
				else if (!bMatch)
					nVerify_ |= PGPUtility::VERIFY_ADDRESSNOTMATCH;
			}
			else {
				nVerify_ = PGPUtility::VERIFY_FAILED;
			}
		}
		else if (nLen > 16 &&
			(strncmp(strLine.get() + 9, "BADSIG ", 7) == 0 ||
			strncmp(strLine.get() + 9, "ERRSIG ", 7) == 0)) {
			nVerify_ = PGPUtility::VERIFY_FAILED;
		}
	}
	
	return true;
}

string_ptr qmpgp::GPGDriver::StatusHandler::fetchLine(XStringBuffer<qs::STRING>* pBuf)
{
	assert(pBuf);
	
	const CHAR* pBegin = pBuf->getCharArray();
	const CHAR* p = pBegin;
	while (*p && *p != '\n' && *p != '\r')
		++p;
	if (!*p)
		return 0;
	
	string_ptr strLine(allocString(pBegin, p - pBegin));
	if (*p == '\r' && *(p + 1) == '\n')
		++p;
	pBuf->remove(0, p - pBegin + 1);
	
	return strLine;
}
