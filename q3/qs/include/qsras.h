/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSRAS_H__
#define __QSRAS_H__

#include <qs.h>
#include <qsstring.h>

#include <ras.h>
#include <raserror.h>

#if !defined _WIN32_WCE || defined _WIN32_WCE_EMULATION
	#define RASAPI APIENTRY
#endif

namespace qs {

class RasAPI;
class RasConnection;
class RasConnectionCallback;


/****************************************************************************
 *
 * RasAPI
 *
 */

class RasAPI
{
public:
#ifdef _WIN32_WCE
	typedef LPTSTR LPRASAPISTR;
#else
	typedef LPCTSTR LPRASAPISTR;
#endif
	
private:
	RasAPI();

public:
	~RasAPI();

public:
	static bool isInit();
	
	static DWORD rasDial(LPRASDIALEXTENSIONS lpRasDialExtensions,
		LPRASAPISTR lpszPhonebook, LPRASDIALPARAMS lpRasDialParams,
		DWORD dwNotifierType, LPVOID lpvNotifier, LPHRASCONN lphRasConn);
	static DWORD rasHangUp(HRASCONN hrasconn);
	static DWORD rasEnumEntries(LPRASAPISTR reserved, LPRASAPISTR lpszPhonebook,
		LPRASENTRYNAME lpRasEntryName, LPDWORD lpcb, LPDWORD lpcEntries);
	static DWORD rasEnumConnections(LPRASCONN lpRasConn,
		LPDWORD lpcb, LPDWORD lpConnections);
	static DWORD rasGetConnectStatus(HRASCONN hRasConn,
		LPRASCONNSTATUS lpRasConnStatus);
	static DWORD rasGetEntryDialParams(LPRASAPISTR pszPhonebook,
		LPRASDIALPARAMS lpRasDialParams, LPBOOL lpfPassword);
	static DWORD rasGetErrorString(UINT uErrorValue,
		LPTSTR pszErrorString, DWORD dwBufSize);

private:
	RasAPI(const RasAPI&);
	RasAPI& operator=(const RasAPI&);

private:
	typedef DWORD (RASAPI *PFN_RASDIAL)(LPRASDIALEXTENSIONS, LPRASAPISTR,
		LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN);
	typedef DWORD (RASAPI *PFN_RASHANGUP)(HRASCONN);
	typedef DWORD (RASAPI *PFN_RASENUMENTRIES)(LPRASAPISTR, LPRASAPISTR,
		LPRASENTRYNAME, LPDWORD, LPDWORD);
	typedef DWORD (RASAPI *PFN_RASENUMCONNECTIONS)(LPRASCONN, LPDWORD, LPDWORD);
	typedef DWORD (RASAPI *PFN_RASGETCONNECTSTATUS)(HRASCONN, LPRASCONNSTATUS);
	typedef DWORD (RASAPI *PFN_RASGETENTRYDIALPARAMS)(
		LPRASAPISTR, LPRASDIALPARAMS, LPBOOL);
#ifndef _WIN32_WCE
	typedef DWORD (RASAPI *PFN_RASGETERRORSTRING)(UINT, LPRASAPISTR, DWORD);
#endif

private:
	HINSTANCE hInst_;
	PFN_RASDIAL pfnRasDial_;
	PFN_RASHANGUP pfnRasHangUp_;
	PFN_RASENUMENTRIES pfnRasEnumEntries_;
	PFN_RASENUMCONNECTIONS pfnRasEnumConnections_;
	PFN_RASGETCONNECTSTATUS pfnRasGetConnectStatus_;
	PFN_RASGETENTRYDIALPARAMS pfnRasGetEntryDialParams_;
#ifndef _WIN32_WCE
	PFN_RASGETERRORSTRING pfnRasGetErrorString_;
#endif

private:
	static RasAPI api__;
};


/****************************************************************************
 *
 * RasConnection
 *
 */

class QSEXPORTCLASS RasConnection
{
public:
	enum Result {
		RAS_SUCCESS,
		RAS_FAIL,
		RAS_CANCEL,
		RAS_ALREADYCONNECTED
	};

public:
	typedef std::vector<WSTRING> EntryList;

public:
	RasConnection(unsigned int nDisconnectWait,
				  RasConnectionCallback* pCallback);
	explicit RasConnection(HRASCONN hrasconn);
	~RasConnection();

public:
	Result connect(const WCHAR* pwszEntry);
	Result disconnect(bool bWait);
	
public:
	static std::auto_ptr<RasConnection> getActiveConnection(size_t nIndex);
	static int getActiveConnectionCount();

public:
	static void getEntries(EntryList* pListEntry);
	static wstring_ptr getLocation();
	static bool setLocation(const WCHAR* pwszLocation);
	static bool selectLocation(HWND hwnd);
	static bool isNetworkConnected();

private:
	RasConnection(const RasConnection&);
	RasConnection& operator=(const RasConnection&);

private:
	struct RasConnectionImpl* pImpl_;
};


/****************************************************************************
 *
 * RasConnectionCallback
 *
 */

class QSEXPORTCLASS RasConnectionCallback
{
public:
	virtual ~RasConnectionCallback();

public:
	virtual bool isCanceled() = 0;
	virtual bool preConnect(RASDIALPARAMS* prdp) = 0;
	virtual void setMessage(const WCHAR* pwszMessage) = 0;
	virtual void error(const WCHAR* pwszMessage) = 0;
};

}

#endif // __QSRAS_H__
