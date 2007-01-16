/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __AUTOPILOT_H__
#define __AUTOPILOT_H__

#include <qm.h>

#include <qsprofile.h>
#include <qssax.h>
#include <qsstream.h>
#include <qstimer.h>

#include "../util/confighelper.h"


namespace qm {

class AutoPilot;
class AutoPilotCallback;
class AutoPilotManager;
class AutoPilotEntry;
class AutoPilotContentHandler;
class AutoPilotWriter;

class AccountManager;
class Document;
class GoRound;
class SyncDialogManager;
class SyncManager;


/****************************************************************************
 *
 * AutoPilot
 *
 */

class AutoPilot : public qs::TimerHandler
{
public:
	enum {
		TIMER_CHECK	= 1000
	};

public:
	AutoPilot(AutoPilotManager* pAutoPilotManager,
			  qs::Profile* pProfile,
			  Document* pDocument,
			  GoRound* pGoRound,
			  SyncManager* pSyncManager,
			  SyncDialogManager* pSyncDialogManager,
			  AutoPilotCallback* pCallback);
	~AutoPilot();

public:
	AutoPilotManager* getAutoPilotManager() const;
	void start(HWND hwnd);
	bool isEnabled() const;
	void setEnabled(bool bEnabled);
	void save() const;

public:
	virtual void timerTimeout(qs::Timer::Id nId);

private:
	AutoPilot(const AutoPilot&);
	AutoPilot& operator=(const AutoPilot&);

#ifndef _WIN32_WCE
private:
	class UnseenCountUpdater
	{
	public:
		UnseenCountUpdater(AccountManager* pAccountManager,
						   qs::Profile* pProfile);
		~UnseenCountUpdater();
	
	public:
		void update();
	
	private:
		bool updateAccount(Account* pAccount);
	
	private:
		UnseenCountUpdater(const UnseenCountUpdater&);
		UnseenCountUpdater& operator=(const UnseenCountUpdater&);
	
	private:
		typedef HRESULT (STDAPICALLTYPE *PFN_SHSETUNREADMAILCOUNT)(LPCWSTR, DWORD, LPCWSTR);
	
	private:
		AccountManager* pAccountManager_;
		PFN_SHSETUNREADMAILCOUNT pfnSHSetUnreadMailCount_;
		qs::wstring_ptr wstrPath_;
	};
#endif

private:
	AutoPilotManager* pAutoPilotManager_;
	qs::Profile* pProfile_;
	Document* pDocument_;
	GoRound* pGoRound_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
	AutoPilotCallback* pCallback_;
	std::auto_ptr<qs::Timer> pTimer_;
	bool bTimer_;
	bool bEnabled_;
	unsigned int nCount_;
#ifndef _WIN32_WCE
	UnseenCountUpdater unseenCountUpdater_;
#endif
};


/****************************************************************************
 *
 * AutoPilotCallback
 *
 */

class AutoPilotCallback
{
public:
	virtual ~AutoPilotCallback();

public:
	virtual bool canAutoPilot() = 0;
};


/****************************************************************************
 *
 * AutoPilotManager
 *
 */

class AutoPilotManager
{
public:
	typedef std::vector<AutoPilotEntry*> EntryList;

public:
	explicit AutoPilotManager(const WCHAR* pwszPath);
	~AutoPilotManager();

public:
	const EntryList& getEntries();
	const EntryList& getEntries(bool bReload);
	void setEntries(EntryList& listEntry);
	bool save(bool bForce) const;
	void clear();

public:
	void addEntry(std::auto_ptr<AutoPilotEntry> pEntry);

private:
	bool load();

private:
	AutoPilotManager(const AutoPilotManager&);
	AutoPilotManager& operator=(const AutoPilotManager&);

private:
	EntryList listEntry_;
	ConfigHelper<AutoPilotManager, AutoPilotContentHandler, AutoPilotWriter> helper_;
};


/****************************************************************************
 *
 * AutoPilotEntry
 *
 */

class AutoPilotEntry
{
public:
	AutoPilotEntry();
	AutoPilotEntry(const WCHAR* pwszCourse,
				   int nInterval);
	AutoPilotEntry(const AutoPilotEntry& entry);
	~AutoPilotEntry();

public:
	const WCHAR* getCourse() const;
	void setCourse(const WCHAR* pwszCourse);
	int getInterval() const;
	void setInterval(int nInterval);

private:
	AutoPilotEntry& operator=(const AutoPilotEntry& entry);

private:
	qs::wstring_ptr wstrCourse_;
	int nInterval_;
};


/****************************************************************************
 *
 * AutoPilotContentHandler
 *
 */

class AutoPilotContentHandler : public qs::DefaultHandler
{
public:
	explicit AutoPilotContentHandler(AutoPilotManager* pManager);
	virtual ~AutoPilotContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	AutoPilotContentHandler(const AutoPilotContentHandler&);
	AutoPilotContentHandler& operator=(const AutoPilotContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_AUTOPILOT,
		STATE_ENTRY,
		STATE_COURSE,
		STATE_INTERVAL
	};

private:
	AutoPilotManager* pManager_;
	State state_;
	qs::wstring_ptr wstrCourse_;
	int nInterval_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * AutoPilotWriter
 *
 */

class AutoPilotWriter
{
public:
	AutoPilotWriter(qs::Writer* pWriter,
					const WCHAR* pwszEncoding);
	~AutoPilotWriter();

public:
	bool write(const AutoPilotManager* pManager);

private:
	bool write(const AutoPilotEntry* pEntry);

private:
	AutoPilotWriter(const AutoPilotWriter&);
	AutoPilotWriter& operator=(const AutoPilotWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __AUTOPILOT_H__
