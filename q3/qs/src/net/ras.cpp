/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsras.h>
#include <qssocket.h>

#include <tapi.h>
#include <tchar.h>

#include "ras.h"
#include "../ui/resourceinc.h"

using namespace qs;


/****************************************************************************
 *
 * RasAPI
 *
 */

RasAPI qs::RasAPI::api__;

qs::RasAPI::RasAPI() :
	hInst_(0),
	pfnRasDial_(0),
	pfnRasHangUp_(0),
	pfnRasEnumEntries_(0),
	pfnRasEnumConnections_(0),
	pfnRasGetConnectStatus_(0),
	pfnRasGetEntryDialParams_(0)
{
#ifndef _WIN32_WCE
	hInst_ = ::LoadLibrary(_T("RASAPI32.DLL"));
	if (hInst_) {
#ifdef UNICODE
	#define FUNCTIONPOSTFIX "W"
#else
	#define FUNCTIONPOSTFIX "A"
#endif
		pfnRasDial_ = reinterpret_cast<PFN_RASDIAL>(
			::GetProcAddress(hInst_, "RasDial" FUNCTIONPOSTFIX));
		pfnRasHangUp_ = reinterpret_cast<PFN_RASHANGUP>(
			::GetProcAddress(hInst_, "RasHangUp" FUNCTIONPOSTFIX));
		pfnRasEnumEntries_ = reinterpret_cast<PFN_RASENUMENTRIES>(
			::GetProcAddress(hInst_, "RasEnumEntries" FUNCTIONPOSTFIX));
		pfnRasEnumConnections_ = reinterpret_cast<PFN_RASENUMCONNECTIONS>(
			::GetProcAddress(hInst_, "RasEnumConnections" FUNCTIONPOSTFIX));
		pfnRasGetConnectStatus_ = reinterpret_cast<PFN_RASGETCONNECTSTATUS>(
			::GetProcAddress(hInst_, "RasGetConnectStatus" FUNCTIONPOSTFIX));
		pfnRasGetEntryDialParams_ = reinterpret_cast<PFN_RASGETENTRYDIALPARAMS>(
			::GetProcAddress(hInst_, "RasGetEntryDialParams" FUNCTIONPOSTFIX));
		pfnRasGetErrorString_ = reinterpret_cast<PFN_RASGETERRORSTRING>(
			::GetProcAddress(hInst_, "RasGetErrorString" FUNCTIONPOSTFIX));
	}
#else
	pfnRasDial_ = RasDial;
	pfnRasHangUp_ = RasHangUp;
	pfnRasEnumEntries_ = RasEnumEntries;
	pfnRasEnumConnections_ = RasEnumConnections;
	pfnRasGetConnectStatus_ = RasGetConnectStatus;
	pfnRasGetEntryDialParams_ = RasGetEntryDialParams;
#endif
}

qs::RasAPI::~RasAPI()
{
#ifndef _WIN32_WCE
	if (hInst_)
		::FreeLibrary(hInst_);
#endif
}

bool RasAPI::isInit()
{
#ifdef _WIN32_WCE
	return true;
#else
	return api__.hInst_ != 0;
#endif
}

DWORD qs::RasAPI::rasDial(LPRASDIALEXTENSIONS lpRasDialExtensions,
	LPRASAPISTR lpszPhonebook, LPRASDIALPARAMS lpRasDialParams,
	DWORD dwNotifierType, LPVOID lpvNotifier, LPHRASCONN lphRasConn)
{
	return api__.pfnRasDial_ ?
		(*api__.pfnRasDial_)(lpRasDialExtensions, lpszPhonebook,
			lpRasDialParams, dwNotifierType, lpvNotifier, lphRasConn) :
		1;
}

DWORD RasAPI::rasHangUp(HRASCONN hrasconn)
{
	return api__.pfnRasHangUp_ ? (*api__.pfnRasHangUp_)(hrasconn) : 1;
}

DWORD RasAPI::rasEnumEntries(LPRASAPISTR reserved, LPRASAPISTR lpszPhonebook,
	LPRASENTRYNAME lpRasEntryName, LPDWORD lpcb, LPDWORD lpcEntries)
{
	return api__.pfnRasEnumEntries_ ?
		(*api__.pfnRasEnumEntries_)(reserved,
			lpszPhonebook, lpRasEntryName, lpcb, lpcEntries) :
		1;
}

DWORD RasAPI::rasEnumConnections(LPRASCONN lpRasConn, LPDWORD lpcb, LPDWORD lpConnections)
{
	return api__.pfnRasEnumConnections_ ?
		(*api__.pfnRasEnumConnections_)(lpRasConn, lpcb, lpConnections) : 1;
}

DWORD RasAPI::rasGetConnectStatus(HRASCONN hRasConn, LPRASCONNSTATUS lpRasConnStatus)
{
	return api__.pfnRasGetConnectStatus_ ?
		(*api__.pfnRasGetConnectStatus_)(hRasConn, lpRasConnStatus) : 1;
}

DWORD RasAPI::rasGetEntryDialParams(LPRASAPISTR pszPhonebook,
	LPRASDIALPARAMS lpRasDialParams, LPBOOL lpfPassword)
{
	return api__.pfnRasGetEntryDialParams_ ?
		(*api__.pfnRasGetEntryDialParams_)(pszPhonebook,
			lpRasDialParams, lpfPassword) :
		1;
}

DWORD RasAPI::rasGetErrorString(UINT uErrorValue, LPTSTR pszErrorString, DWORD dwBufSize)
{
#ifdef _WIN32_WCE
	static const LPCTSTR pszMessages[] = {
		_T("An operation is pending."),
		_T("An invalid port handle was detected."),
		_T("The specified port is already open."),
		_T("The caller's buffer is too small."),
		_T("Incorrect information was specified."),
		_T("The port information cannot be set."),
		_T("The specified port is not connected."),
		_T("An invalid event was detected."),
		_T("A device was specified that does not exist."),
		_T("A device type was specified that does not exist."),
		_T("An invalid buffer was specified."),
		_T("A route was specified that is not available."),
		_T("A route was specified that is not allocated."),
		_T("An invalid compression was specified."),
		_T("There were insufficient buffers available."),
		_T("The specified port was not found."),
		_T("An asynchronous request is pending."),
		_T("The modem (or other connecting device) is already disconnecting."),
		_T("The specified port is not open."),
		_T("The specified port is not connected."),
		_T("No endpoints could be determined."),
		_T("The system could not open the phone book file."),
		_T("The system could not load the phone book file."),
		_T("The system could not find the phone book entry for this connection."),
		_T("The system could not update the phone book file."),
		_T("The system found invalid information in the phone book file."),
		_T("A string could not be loaded."),
		_T("A key could not be found."),
		_T("The connection was closed."),
		_T("The connection was closed by the remote computer."),
		_T("The modem (or other connecting device) was disconnected due to hardware failure."),
		_T("The user disconnected the modem (or other connecting device)."),
		_T("An incorrect structure size was detected."),
		_T("The modem (or other connecting device) is already in use or is not configured properly."),
		_T("Your computer could not be registered on the remote network."),
		_T("There was an unknown error."),
		_T("The device attached to the port is not the one expected."),
		_T("A string was detected that could not be converted."),
		_T("The request has timed out."),
		_T("No asynchronous net is available."),
		_T("An error has occurred involving NetBIOS."),
		_T("The server cannot allocate NetBIOS resources needed to support the client."),
		_T("One of your computer's NetBIOS names is already registered on the remote network."),
		_T("A network adapter at the server failed."),
		_T("You will not receive network message popups."),
		_T("There was an internal authentication error."),
		_T("The account is not permitted to log on at this time of day."),
		_T("The account is disabled."),
		_T("The password for this account has expired."),
		_T("The account does not have permission to dial in."),
		_T("The remote access server is not responding."),
		_T("The modem (or other connecting device) has reported an error."),
		_T("There was an unrecognized response from the modem (or other connecting device)."),
		_T("A macro required by the modem (or other connecting device) was not found in the device.INF file."),
		_T("A command or response in the device.INF file section refers to an undefined macro."),
		_T("The <message> macro was not found in the device.INF file section."),
		_T("The <defaultoff> macro in the device.INF file section contains an undefined macro."),
		_T("The device.INF file could not be opened."),
		_T("The device name in the device.INF or media.INI file is too long."),
		_T("The media.INI file refers to an unknown device name."),
		_T("The device.INF file contains no responses for the command."),
		_T("The device.INF file is missing a command."),
		_T("There was an attempt to set a macro not listed in device.INF file section."),
		_T("The media.INI file refers to an unknown device type."),
		_T("The system has run out of memory."),
		_T("The modem (or other connecting device) is not properly configured."),
		_T("The modem (or other connecting device) is not functioning."),
		_T("The system was unable to read the media.INI file."),
		_T("The connection was terminated."),
		_T("The usage parameter in the media.INI file is invalid."),
		_T("The system was unable to read the section name from the media.INI file."),
		_T("The system was unable to read the device type from the media.INI file."),
		_T("The system was unable to read the device name from the media.INI file."),
		_T("The system was unable to read the usage from the media.INI file."),
		_T("The system was unable to read the maximum connection BPS rate from the media.INI file."),
		_T("The system was unable to read the maximum carrier connection speed from the media.INI file."),
		_T("The phone line is busy."),
		_T("A person answered instead of a modem (or other connecting device)."),
		_T("There was no answer."),
		_T("The system could not detect the carrier."),
		_T("There was no dial tone."),
		_T("The modem (or other connecting device) reported a general error."),
		_T("There was an error in writing the section name."),
		_T("There was an error in writing the device type."),
		_T("There was an error in writing the device name."),
		_T("There was an error in writing the maximum connection speed."),
		_T("There was an error in writing the maximum carrier speed."),
		_T("There was an error in writing the usage."),
		_T("There was an error in writing the default-off."),
		_T("There was an error in reading the default-off."),
		_T("ERROR_EMPTY_INI_FILE"),
		_T("Access was denied because the username and/or password was invalid on the domain."),
		_T("There was a hardware failure in the modem (or other connecting device)."),
		_T("ERROR_NOT_BINARY_MACRO"),
		_T("ERROR_DCB_NOT_FOUND"),
		_T("The state machines are not started."),
		_T("The state machines are already started."),
		_T("The response looping did not complete."),
		_T("A response keyname in the device.INF file is not in the expected format."),
		_T("The modem (or other connecting device) response caused a buffer overflow."),
		_T("The expanded command in the device.INF file is too long."),
		_T("The modem moved to a connection speed not supported by the COM driver."),
		_T("Device response received when none expected."),
		_T("The connection needs information from you, but the application does not allow user interaction."),
		_T("The callback number is invalid."),
		_T("The authorization state is invalid."),
		_T("ERROR_WRITING_INITBPS"),
		_T("There was an error related to the X.25 protocol."),
		_T("The account has expired."),
		_T("There was an error changing the password on the domain.  The password might have been too short or might have matched a previously used password."),
		_T("Serial overrun errors were detected while communicating with the modem."),
		_T("The Remote Access Service Manager could not start. Additional information is provided in the event log."),
		_T("The two-way port is initializing.  Wait a few seconds and redial."),
		_T("No active ISDN lines are available."),
		_T("No ISDN channels are available to make the call."),
		_T("Too many errors occurred because of poor phone line quality."),
		_T("The Remote Access Service IP configuration is unusable."),
		_T("No IP addresses are available in the static pool of Remote Access Service IP addresses."),
		_T("The connection timed out waiting for a valid response from the remote computer."),
		_T("The connection was terminated by the remote computer."),
		_T("The connection attempt failed because your computer and the remote computer could not agree on PPP control protocols."),
		_T("The remote computer is not responding."),
		_T("Invalid data was received from the remote computer. This data was ignored."),
		_T("The phone number, including prefix and suffix, is too long."),
		_T("The IPX protocol cannot dial out on the modem (or other connecting device) because this computer is not configured for dialing out (it is an IPX router)."),
		_T("The IPX protocol cannot dial in on the modem (or other connecting device) because this computer is not configured for dialing in (the IPX router is not installed)."),
		_T("The IPX protocol cannot be used for dialing out on more than one modem (or other connecting device) at a time."),
		_T("Cannot access TCPCFG.DLL."),
		_T("The system cannot find an IP adapter."),
		_T("SLIP cannot be used unless the IP protocol is installed."),
		_T("Computer registration is not complete."),
		_T("The protocol is not configured."),
		_T("Your computer and the remote computer could not agree on PPP control protocols."),
		_T("Your computer and the remote computer could not agree on PPP control protocols."),
		_T("The PPP link control protocol was terminated."),
		_T("The requested address was rejected by the server."),
		_T("The remote computer terminated the control protocol."),
		_T("Loopback was detected."),
		_T("The server did not assign an address."),
		_T("The authentication protocol required by the remote server cannot use the stored password.  Redial, entering the password explicitly."),
		_T("An invalid dialing rule was detected."),
		_T("The local computer does not support the required data encryption type."),
		_T("The remote computer does not support the required data encryption type."),
		_T("The remote computer requires data encryption."),
		_T("The system cannot use the IPX network number assigned by the remote computer.  Additional information is provided in the event log."),
		_T("ERROR_INVALID_SMM"),
		_T("ERROR_SMM_UNINITIALIZED"),
		_T("ERROR_NO_MAC_FOR_PORT"),
		_T("ERROR_SMM_TIMEOUT"),
		_T("ERROR_BAD_PHONE_NUMBER"),
		_T("ERROR_WRONG_MODULE"),
		_T("ERROR_PPP_MAC"),
		_T("ERROR_PPP_LCP"),
		_T("ERROR_PPP_AUTH"),
		_T("ERROR_PPP_NCP"),
		_T("ERROR_POWER_OFF"),
		_T("ERROR_POWER_OFF_CD")
	};
	if (uErrorValue < RASBASE || RASBASEEND < uErrorValue)
		return 1;
	_tcsncpy(pszErrorString, pszMessages[uErrorValue - RASBASE], dwBufSize);
	return 0;
#else
	return api__.pfnRasGetErrorString_ ? (*api__.pfnRasGetErrorString_)(
		uErrorValue, pszErrorString, dwBufSize) : 1;
#endif
}


/****************************************************************************
 *
 * RasConnectionImpl
 *
 */

struct qs::RasConnectionImpl
{
	static void setMessage(RasConnectionCallback* pCallback,
						   UINT nId);
	
	HRASCONN hrasconn_;
	RasConnectionCallback* pCallback_;
};

void qs::RasConnectionImpl::setMessage(RasConnectionCallback* pCallback,
									   UINT nId)
{
	assert(pCallback);
	wstring_ptr wstrMessage(loadString(getDllInstanceHandle(), nId));
	pCallback->setMessage(wstrMessage.get());
}


/****************************************************************************
 *
 * RasConnection
 *
 */

static void CALLBACK lineProc(DWORD hDevice,
							  DWORD dwMsg,
							  DWORD_PTR dwCallbackInstance,
							  DWORD_PTR dwParam1,
							  DWORD_PTR dwParam2,
							  DWORD_PTR dwParam3);

qs::RasConnection::RasConnection(RasConnectionCallback* pCallback) :
	pImpl_(0)
{
	pImpl_ = new RasConnectionImpl();
	pImpl_->hrasconn_ = 0;
	pImpl_->pCallback_ = pCallback;
}

qs::RasConnection::RasConnection(HRASCONN hrasconn) :
	pImpl_(0)
{
	assert(hrasconn);
	
	pImpl_ = new RasConnectionImpl();
	pImpl_->hrasconn_ = hrasconn;
	pImpl_->pCallback_ = 0;
}

qs::RasConnection::~RasConnection()
{
	delete pImpl_;
}

RasConnection::Result qs::RasConnection::connect(const WCHAR* pwszEntry)
{
	if (!RasAPI::isInit())
		return RAS_FAIL;
	
	RASCONN rasconn = { sizeof(rasconn) };
	DWORD dwSize = sizeof(rasconn);
	DWORD dwConnection = 0;
	if (!RasAPI::rasEnumConnections(&rasconn, &dwSize, &dwConnection)) {
		if (dwConnection != 0)
			return RAS_ALREADYCONNECTED;
	}
	
	W2T(pwszEntry, ptszEntry);
	RASDIALPARAMS rdp = { sizeof(rdp) };
	_tcsncpy(rdp.szEntryName, ptszEntry, countof(rdp.szEntryName));
	BOOL bPassword = TRUE;
	if (RasAPI::rasGetEntryDialParams(0, &rdp, &bPassword))
		return RAS_FAIL;
	
	if (!pImpl_->pCallback_->preConnect(&rdp))
		return RAS_CANCEL;
	
	std::auto_ptr<RasWindow> pRasWindow(new RasWindow(this, pImpl_->pCallback_));
	if (!pRasWindow->create(L"QsRasWindow",
		0, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0, 0))
		return RAS_FAIL;
	
	if (RasAPI::rasDial(0, 0, &rdp, 0xffffffff,
		pRasWindow->getHandle(), &pImpl_->hrasconn_)) {
		if (pImpl_->hrasconn_) {
			RasAPI::rasHangUp(pImpl_->hrasconn_);
			pImpl_->hrasconn_ = 0;
		}
		return RAS_FAIL;
	}
	
	while (!pRasWindow->isEnd()) {
		MSG msg;
		while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	pRasWindow->destroyWindow();
	
	if (pImpl_->hrasconn_) {
		RASCONNSTATUS rcs = { sizeof(rcs) };
		if (RasAPI::rasGetConnectStatus(pImpl_->hrasconn_, &rcs) != 0 ||
			rcs.dwError != 0 || rcs.rasconnstate == RASCS_Disconnected)
			return RAS_FAIL;
	}
	else {
		if (pImpl_->pCallback_->isCanceled())
			return RAS_CANCEL;
		else
			return RAS_FAIL;
	}
	
	return RAS_SUCCESS;
}

RasConnection::Result qs::RasConnection::disconnect(unsigned int nWait)
{
	if (!pImpl_->hrasconn_)
		return RAS_FAIL;
	
	if (pImpl_->pCallback_ && nWait != 0) {
		RasConnectionImpl::setMessage(pImpl_->pCallback_,
			IDS_RAS_WAITINGBEFOREDISCONNECTING);
		for (unsigned int n = 0; n < nWait*10; ++n) {
			if (pImpl_->pCallback_->isCanceled())
				break;
			::Sleep(100);
		}
	}
	
	if (pImpl_->pCallback_) {
		if (nWait != 0 && pImpl_->pCallback_->isCanceled())
			return RAS_CANCEL;
		RasConnectionImpl::setMessage(pImpl_->pCallback_, IDS_RAS_DISCONNECTING);
	}
	
	if (pImpl_->hrasconn_)
		RasAPI::rasHangUp(pImpl_->hrasconn_);
	
	RASCONNSTATUS rcs = { sizeof(rcs) };
	while (RasAPI::rasGetConnectStatus(pImpl_->hrasconn_, &rcs) == 0 &&
		rcs.rasconnstate == RASCS_Connected)
		::Sleep(0);
	pImpl_->hrasconn_ = 0;
	
	return RAS_SUCCESS;
}

std::auto_ptr<RasConnection> qs::RasConnection::getActiveConnection(size_t nIndex)
{
	if (!RasAPI::isInit())
		return std::auto_ptr<RasConnection>(0);
	
	HRASCONN hrasconn = 0;
	RASCONN rasconn = { sizeof(rasconn) };
	DWORD dwSize = sizeof(rasconn);
	DWORD dwConnection = 0;
	RasAPI::rasEnumConnections(&rasconn, &dwSize, &dwConnection);
	if (dwSize > sizeof(rasconn)) {
		typedef std::vector<RASCONN> RasConnList;
		RasConnList listRasConn;
		listRasConn.resize(dwSize/sizeof(RASCONN));
		if (RasAPI::rasEnumConnections(&listRasConn[0], &dwSize, &dwConnection) != 0)
			return std::auto_ptr<RasConnection>(0);
		if (nIndex == -1)
			nIndex = 0;
		if (nIndex >= dwConnection)
			return std::auto_ptr<RasConnection>(0);
		hrasconn = listRasConn[nIndex].hrasconn;
	}
	else if (dwConnection != 0) {
		assert(dwConnection == 1);
		hrasconn = rasconn.hrasconn;
	}
	else {
		return std::auto_ptr<RasConnection>(0);
	}
	
	return std::auto_ptr<RasConnection>(new RasConnection(hrasconn));
}

int qs::RasConnection::getActiveConnectionCount()
{
	if (!RasAPI::isInit())
		return -1;
	
	RASCONN rasconn = { sizeof(rasconn) };
	DWORD dwSize = sizeof(rasconn);
	DWORD dwConnection = 0;
	if (RasAPI::rasEnumConnections(&rasconn, &dwSize, &dwConnection) != 0)
		return -1;
	
	return dwSize/sizeof(RASCONN);
}

void qs::RasConnection::getEntries(EntryList* pListEntry)
{
	if (RasAPI::isInit()) {
		DWORD dwEntries = 1;
		DWORD dwSize = sizeof(RASENTRYNAME);
		RASENTRYNAME ren = { sizeof(ren) };
		RasAPI::rasEnumEntries(0, 0, &ren, &dwSize, &dwEntries);
		dwEntries = dwSize/sizeof(RASENTRYNAME);
		
		typedef std::vector<RASENTRYNAME> EntryNameList;
		EntryNameList listEntryName;
		listEntryName.resize(dwEntries);
		for (EntryNameList::size_type n = 0; n < listEntryName.size(); ++n)
			listEntryName[n].dwSize = sizeof(RASENTRYNAME);
		RasAPI::rasEnumEntries(0, 0, &listEntryName[0], &dwSize, &dwEntries);
		
		pListEntry->reserve(listEntryName.size());
		
		for (EntryNameList::size_type n = 0; n < listEntryName.size(); ++n) {
			if (listEntryName[n].szEntryName[0] != _T('`')) {
				wstring_ptr wstrEntryName(tcs2wcs(listEntryName[n].szEntryName));
				pListEntry->push_back(wstrEntryName.release());
			}
		}
	}
}

wstring_ptr qs::RasConnection::getLocation()
{
	const TCHAR* ptszLocation = 0;
	
	malloc_ptr<LINETRANSLATECAPS> ptc;
	
	HLINEAPP hLineApp = 0;
	DWORD dwNumDevs = 0;
	if (::lineInitialize(&hLineApp, getInstanceHandle(),
		lineProc, 0, &dwNumDevs) == 0) {
		DWORD dwVersion = 0;
#ifdef _WIN32_WCE
		::lineNegotiateAPIVersion(hLineApp, 0, MAKELONG(0, 1),
			MAKELONG(0, 2), &dwVersion, 0);
#else
		LINEEXTENSIONID lineExtId;
		::lineNegotiateAPIVersion(hLineApp, 0, MAKELONG(0, 1),
			MAKELONG(0, 4), &dwVersion, &lineExtId);
#endif
		
		ptc.reset(static_cast<LPLINETRANSLATECAPS>(
			::malloc(sizeof(LINETRANSLATECAPS) + 10240)));
		if (ptc.get()) {
			ptc->dwTotalSize = sizeof(LINETRANSLATECAPS) + 10240;
			if (::lineGetTranslateCaps(hLineApp, dwVersion, ptc.get()) == 0) {
				assert(ptc->dwNeededSize <= ptc->dwTotalSize);
#ifdef _WIN32_WCE
				int nSize = static_cast<int>(
					ptc->dwLocationListSize/sizeof(LINELOCATIONENTRY));
#else
				int nSize = ptc->dwNumLocations;
#endif
				for (int n = 0; n < nSize; ++n) {
					LPLINELOCATIONENTRY plle =
						reinterpret_cast<LPLINELOCATIONENTRY>(
							reinterpret_cast<BYTE*>(ptc.get()) +
								ptc->dwLocationListOffset +
								n*sizeof(LINELOCATIONENTRY));
#if !defined _WIN32_WCE || _WIN32_WCE >= 200
					if (plle->dwPermanentLocationID == ptc->dwCurrentLocationID) {
						ptszLocation = reinterpret_cast<LPCTSTR>(
							reinterpret_cast<BYTE*>(ptc.get()) +
							plle->dwLocationNameOffset);
						break;
					}
#else
					ptszLocation = reinterpret_cast<LPCTSTR>(
						reinterpret_cast<BYTE*>(ptc.get()) +
						plle->dwLocationNameOffset);
#endif
				}
			}
		}
		::lineShutdown(hLineApp);
	}
	
	if (!ptszLocation)
		return 0;
	
	return tcs2wcs(ptszLocation);
}

bool qs::RasConnection::setLocation(const WCHAR* pwszLocation)
{
	assert(pwszLocation);
	
#ifdef _WIN32_WCE
	W2T(pwszLocation, ptszLocation);
	
	HKEY hKey = 0;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER,
		_T("ControlPanel\\Dial\\Locations"),
		0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return false;
	
	int nLocation = -1;
	DWORD dw = 0;
	TCHAR szName[100];
	DWORD dwNameLen = countof(szName);
	TCHAR szValue[1000];
	DWORD dwValueLen = sizeof(szValue);
	DWORD dwType = 0;
	while (::RegEnumValue(hKey, dw, szName, &dwNameLen, 0, &dwType,
		reinterpret_cast<LPBYTE>(szValue), &dwValueLen) == ERROR_SUCCESS) {
		if (dwType == REG_MULTI_SZ) {
			unsigned int m = 0;
			while (m < _tcslen(szName)) {
				if (!_istdigit(szName[m]))
					break;
				++m;
			}
			if (m == _tcslen(szName)) {
				if (_tcscmp(ptszLocation, szValue) == 0) {
					nLocation = _ttoi(szName);
					break;
				}
			}
		}
		++dw;
		dwNameLen = countof(szName);
		dwValueLen = sizeof(szValue);
	}
	::RegCloseKey(hKey);
	if (nLocation == -1)
		return false;
	
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\Dial"),
		0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
		return false;
	::RegSetValueEx(hKey, _T("CurrentLoc"), 0, REG_DWORD,
		reinterpret_cast<LPBYTE>(&nLocation), sizeof(nLocation));
	::RegCloseKey(hKey);
	
	return true;
#else
	return false;
#endif
}

bool qs::RasConnection::selectLocation(HWND hwnd)
{
	LONG lRet = 0;
	HLINEAPP hLineApp = 0;
	DWORD dwNumDevs = 0;
	lRet = ::lineInitialize(&hLineApp,
		getInstanceHandle(), lineProc, 0, &dwNumDevs);
	if (lRet != 0)
		return false;
	
	DWORD dwVersion = 0;
#ifdef _WIN32_WCE
	lRet = ::lineNegotiateAPIVersion(hLineApp, 0, MAKELONG(0, 1),
		MAKELONG(0, 2), &dwVersion, 0);
#else
	LINEEXTENSIONID lineExtId;
	lRet = ::lineNegotiateAPIVersion(hLineApp, 0, MAKELONG(0, 1),
		MAKELONG(0, 4), &dwVersion, &lineExtId);
#endif
	if (lRet != 0)
		return false;
	
	::lineTranslateDialog(hLineApp, 0, dwVersion, hwnd, 0);
	
	::lineShutdown(hLineApp);
	
	return true;
}

bool qs::RasConnection::isNetworkConnected()
{
	Winsock winsock;
	
	char szHostName[256];
	if (::gethostname(szHostName, sizeof(szHostName)) != 0)
		return false;
	
	hostent* phe = ::gethostbyname(szHostName);
	if (!phe)
		return false;
	
	in_addr addr;
	memcpy(&addr, phe->h_addr, phe->h_length);
	const char* pszAddr = inet_ntoa(addr);
	if (strcmp(pszAddr, "0.0.0.0") == 0 ||
		strcmp(pszAddr, "127.0.0.1") == 0)
		return false;
	
	return true;
}

static void CALLBACK lineProc(DWORD hDevice,
							  DWORD dwMsg,
							  DWORD_PTR dwCallbackInstance,
							  DWORD_PTR dwParam1,
							  DWORD_PTR dwParam2,
							  DWORD_PTR dwParam3)
{
}


/****************************************************************************
 *
 * RasConnectionCallback
 *
 */

qs::RasConnectionCallback::~RasConnectionCallback()
{
}


/****************************************************************************
 *
 * RasWindow
 *
 */

#ifdef _WIN32_WCE
const UINT qs::RasWindow::nRasDialEventMessage__ = WM_RASDIALEVENT;
#else
const UINT qs::RasWindow::nRasDialEventMessage__ =
	::RegisterWindowMessageA(RASDIALEVENT);
#endif

qs::RasWindow::RasWindow(RasConnection* pConnection,
						 RasConnectionCallback* pCallback) :
	WindowBase(false),
	pConnection_(pConnection),
	pCallback_(pCallback),
	nTimerId_(0),
	bEnd_(false),
	bCanceled_(false)
{
	setWindowHandler(this, false);
}

qs::RasWindow::~RasWindow()
{
}

bool qs::RasWindow::isEnd() const
{
	return bEnd_;
}

LRESULT qs::RasWindow::windowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_TIMER()
	END_MESSAGE_HANDLER()
	BEGIN_REGISTERED_MESSAGE_HANDLER()
		HANDLE_REGISTERED_MESSAGE(nRasDialEventMessage__, onRasDialEvent)
	END_REGISTERED_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::RasWindow::onTimer(UINT_PTR nId)
{
	if (nId == nTimerId_) {
		if (pCallback_->isCanceled()) {
			bCanceled_ = true;
			end(true);
		}
		return 0;
	}
	
	return DefaultWindowHandler::onTimer(nId);
}

LRESULT qs::RasWindow::onRasDialEvent(WPARAM wParam,
									  LPARAM lParam)
{
	QTRY {
		RASCONNSTATE rcs = static_cast<RASCONNSTATE>(wParam);
		UINT nError = static_cast<UINT>(lParam);
		
		if (nTimerId_ == 0)
			nTimerId_ = setTimer(TIMER_RAS, TIMEOUT);
		
		bool bDisconnect = false;
		
		if (nError != 0) {
			if (!bCanceled_) {
				TCHAR szMessage[256];
				if (RasAPI::rasGetErrorString(nError, szMessage, countof(szMessage)) == 0) {
					T2W(szMessage, pwszMessage);
					pCallback_->error(pwszMessage);
				}
			}
			bDisconnect = true;
		}
		else if (pCallback_->isCanceled()) {
			bCanceled_ = true;
			bDisconnect = true;
		}
		else {
			struct {
				RASCONNSTATE rcs_;
				UINT nId_;
			} states[] = {
				{ RASCS_OpenPort,			IDS_RAS_OPENPORT		},
				{ RASCS_PortOpened,			IDS_RAS_PORTOPENED		},
				{ RASCS_ConnectDevice,		IDS_RAS_CONNECTDEVICE	},
				{ RASCS_DeviceConnected,	IDS_RAS_DEVICECONNECTED	},
				{ RASCS_Authenticate,		IDS_RAS_AUTHENTICATE	},
				{ RASCS_Authenticated,		IDS_RAS_AUTHENTICATED	},
				{ RASCS_Connected,			IDS_RAS_CONNECTED		}
			};
			for (int n = 0; n < countof(states); ++n) {
				if (rcs == states[n].rcs_) {
					RasConnectionImpl::setMessage(pCallback_, states[n].nId_);
					break;
				}
			}
			
			if (rcs == RASCS_Connected)
				end(false);
		}
		
		if (bDisconnect)
			end(true);
	}
	QCATCH_ALL() {
		;
	}
	
	return 0;
}

void qs::RasWindow::end(bool bDisconnect)
{
	if (bDisconnect)
		pConnection_->disconnect(0);
	bEnd_ = true;
	killTimer(nTimerId_);
	nTimerId_ = 0;
}
