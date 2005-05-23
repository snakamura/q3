/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMRECENT_H__
#define __QMRECENT_H__

#include <qm.h>

#include <qsprofile.h>
#include <qsregex.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class Recents;
class RecentsHandler;
class RecentsEvent;

class AccountManager;
class URI;


/****************************************************************************
 *
 * Recents
 *
 */

class Recents
{
public:
	Recents(AccountManager* pAccountManager,
			qs::Profile* pProfile);
	~Recents();

public:
	unsigned int getCount() const;
	const URI* get(unsigned int n) const;
	void add(std::auto_ptr<URI> pURI,
			 bool bAuto);
	void remove(const URI* pURI);
	void clear();
	void removeSeens();
	
	void lock() const;
	void unlock() const;
#ifndef NDEBUG
	bool isLocked() const;
#endif
	
	void addRecentsHandler(RecentsHandler* pHandler);
	void removeRecentsHandler(RecentsHandler* pHandler);

private:
	Recents(const Recents&);
	Recents& operator=(const Recents&);

private:
	struct RecentsImpl* pImpl_;
};


/****************************************************************************
 *
 * RecentsHandler
 *
 */

class RecentsHandler
{
public:
	virtual ~RecentsHandler();

public:
	virtual void recentsChanged(const RecentsEvent& event) = 0;
};


/****************************************************************************
 *
 * RecentsEvent
 *
 */

class RecentsEvent
{
public:
	explicit RecentsEvent(Recents* pRecents);
	~RecentsEvent();

public:
	Recents* getRecents() const;

private:
	RecentsEvent(const RecentsEvent&);
	RecentsEvent& operator=(const RecentsEvent&);

private:
	Recents* pRecents_;
};

}

#endif // __QMRECENT_H__
