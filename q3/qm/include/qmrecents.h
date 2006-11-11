/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
class Macro;
class URI;


/****************************************************************************
 *
 * Recents
 *
 */

class Recents
{
public:
	Recents(const AccountManager* pAccountManager,
			qs::Profile* pProfile);
	~Recents();

public:
	unsigned int getMax() const;
	void setMax(unsigned int nMax);
	const Macro* getFilter() const;
	void setFilter(std::auto_ptr<Macro> pFilter);
	unsigned int getCount() const;
	const std::pair<URI*, qs::Time>& get(unsigned int n) const;
	void add(std::auto_ptr<URI> pURI);
	void remove(const URI* pURI);
	void clear();
	void removeSeens();
	void save() const;
	
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
	enum Type {
		TYPE_ADDED,
		TYPE_REMOVED
	};

public:
	RecentsEvent(Recents* pRecents,
				 Type type);
	~RecentsEvent();

public:
	Recents* getRecents() const;
	Type getType() const;

private:
	RecentsEvent(const RecentsEvent&);
	RecentsEvent& operator=(const RecentsEvent&);

private:
	Recents* pRecents_;
	Type type_;
};

}

#endif // __QMRECENT_H__
