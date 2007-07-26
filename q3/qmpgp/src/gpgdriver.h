/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __GPGDRIVER_H__
#define __GPGDRIVER_H__

#include <qsosutil.h>
#include <qsprofile.h>

#include "driver.h"


namespace qmpgp {

class GPGDriver;


/****************************************************************************
 *
 * GPGDriver
 *
 */

class GPGDriver : public Driver
{
public:
	explicit GPGDriver(qs::Profile* pProfile);
	virtual ~GPGDriver();

public:
	virtual qs::xstring_size_ptr sign(const CHAR* pszText,
									  size_t nLen,
									  SignFlag signFlag,
									  const WCHAR* pwszUserId,
									  const WCHAR* pwszPassphrase) const;
	virtual qs::xstring_size_ptr encrypt(const CHAR* pszText,
										 size_t nLen,
										 const UserIdList& listRecipient) const;
	virtual qs::xstring_size_ptr signAndEncrypt(const CHAR* pszText,
												size_t nLen,
												const WCHAR* pwszUserId,
												const WCHAR* pwszPassphrase,
												const UserIdList& listRecipient) const;
	virtual bool verify(const CHAR* pszContent,
						size_t nLen,
						const CHAR* pszSignature,
						const qs::AddressListParser* pFrom,
						const qs::AddressListParser* pSender,
						unsigned int* pnVerify,
						qs::wstring_ptr* pwstrUserId,
						qs::wstring_ptr* pwstrInfo) const;
	virtual qs::xstring_size_ptr decryptAndVerify(const CHAR* pszContent,
												  size_t nLen,
												  const WCHAR* pwszPassphrase,
												  const qs::AddressListParser* pFrom,
												  const qs::AddressListParser* pSender,
												  unsigned int* pnVerify,
												  qs::wstring_ptr* pwstrUserId,
												  qs::wstring_ptr* pwstrInfo) const;

private:
	qs::wstring_ptr getCommand() const;
	bool getUserIdFromFingerPrint(const WCHAR* pwszFingerPrint,
								  const qs::AddressListParser* pFrom,
								  const qs::AddressListParser* pSender,
								  qs::wstring_ptr* pwstrUserId,
								  bool* pbMatch) const;

private:
	static qs::wstring_ptr getAddressFromUserId(const CHAR* pszUserId);
	static const CHAR* getToken(CHAR** pp,
								CHAR c);
	static qs::wstring_ptr formatHandle(HANDLE h);

private:
	GPGDriver(const GPGDriver&);
	GPGDriver& operator=(const GPGDriver&);

private:
	class StatusHandler
	{
	public:
		StatusHandler(const GPGDriver* pDriver,
					  const WCHAR* pwszPassphrase);
		StatusHandler(const GPGDriver* pDriver,
					  const WCHAR* pwszPassphrase,
					  const qs::AddressListParser* pFrom,
					  const qs::AddressListParser* pSender);
		~StatusHandler();
	
	public:
		bool open();
		bool start();
		bool stop();
		qs::wstring_ptr getOption() const;
		unsigned int getVerify() const;
		qs::wstring_ptr getUserId() const;
	
	private:
		bool process();
		bool processBuffer(qs::XStringBuffer<qs::STRING>* pBuf);
	
	private:
		static qs::string_ptr fetchLine(qs::XStringBuffer<qs::STRING>* pBuf);
	
	private:
		static unsigned int __stdcall threadProc(void* pParam);
	
	private:
		StatusHandler(const StatusHandler&);
		StatusHandler& operator=(const StatusHandler&);
	
	private:
		const GPGDriver* pDriver_;
		const WCHAR* pwszPassphrase_;
		qs::AutoHandle hReadCommand_;
		qs::AutoHandle hWriteCommand_;
		qs::AutoHandle hReadStatus_;
		qs::AutoHandle hWriteStatus_;
		qs::AutoHandle hThread_;
		const qs::AddressListParser* pFrom_;
		const qs::AddressListParser* pSender_;
		unsigned int nVerify_;
		qs::wstring_ptr wstrUserId_;
	};

private:
	qs::Profile* pProfile_;
};

}

#endif // __GPGDRIVER_H__
