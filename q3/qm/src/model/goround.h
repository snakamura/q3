/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __GOROUND_H__
#define __GOROUND_H__

#include <qm.h>
#include <qmgoround.h>

#include <qs.h>
#include <qsregex.h>
#include <qssax.h>
#include <qsstream.h>

#include "term.h"


namespace qm {

class GoRoundCourse;
class GoRoundDialup;
class GoRoundEntry;
class GoRoundContentHandler;
class GoRoundWriter;


/****************************************************************************
 *
 * GoRoundCourse
 *
 */

class GoRoundCourse
{
public:
	enum Type {
		TYPE_SEQUENTIAL,
		TYPE_PARALLEL
	};
	
	enum Flag {
		FLAG_CONFIRM	= 0x01
	};

public:
	typedef std::vector<GoRoundEntry*> EntryList;

public:
	GoRoundCourse();
	GoRoundCourse(const WCHAR* pwszName,
				  unsigned int nFlags);
	GoRoundCourse(const GoRoundCourse& course);
	~GoRoundCourse();

public:
	GoRoundCourse& operator=(const GoRoundCourse& course);

public:
	const WCHAR* getName() const;
	void setName(const WCHAR* pwszName);
	bool isFlag(Flag flag) const;
	void setFlags(unsigned int nFlags);
	Type getType() const;
	void setType(Type type);
	GoRoundDialup* getDialup() const;
	void setDialup(std::auto_ptr<GoRoundDialup> pDialup);
	const EntryList& getEntries() const;
	void setEntries(EntryList& listEntry);
	void addEntry(std::auto_ptr<GoRoundEntry> pEntry);

private:
	qs::wstring_ptr wstrName_;
	unsigned int nFlags_;
	std::auto_ptr<GoRoundDialup> pDialup_;
	Type type_;
	EntryList listEntry_;
};


/****************************************************************************
 *
 * GoRoundEntry
 *
 */

class GoRoundEntry
{
public:
	enum Flag {
		FLAG_SEND			= 0x01,
		FLAG_RECEIVE		= 0x02,
		FLAG_SELECTFOLDER	= 0x04
	};

public:
	GoRoundEntry();
	GoRoundEntry(const WCHAR* pwszAccount,
				 const WCHAR* pwszSubAccount,
				 Term& folder,
				 unsigned int nFlags,
				 const WCHAR* pwszFilter);
	GoRoundEntry(const GoRoundEntry& entry);
	~GoRoundEntry();

public:
	GoRoundEntry& operator=(const GoRoundEntry& entry);

public:
	const WCHAR* getAccount() const;
	void setAccount(const WCHAR* pwszAccount);
	const WCHAR* getSubAccount() const;
	void setSubAccount(const WCHAR* pwszSubAccount);
	const Term& getFolder() const;
	void setFolder(Term& folder);
	bool isFlag(Flag flag) const;
	void setFlags(unsigned int nFlags);
	const WCHAR* getFilter() const;
	void setFilter(const WCHAR* pwszFilter);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrSubAccount_;
	Term folder_;
	unsigned int nFlags_;
	qs::wstring_ptr wstrFilter_;
};


/****************************************************************************
 *
 * GoRoundDialup
 *
 */

class GoRoundDialup
{
public:
	enum Flag {
		FLAG_SHOWDIALOG				= 0x01,
		FLAG_WHENEVERNOTCONNECTED	= 0x02
	};

public:
	GoRoundDialup();
	GoRoundDialup(const WCHAR* pwszName,
				  unsigned int nFlags,
				  const WCHAR* pwszDialFrom,
				  unsigned int nDisconnectWait);
	GoRoundDialup(const GoRoundDialup& dialup);
	~GoRoundDialup();

public:
	GoRoundDialup& operator=(const GoRoundDialup& dialup);

public:
	const WCHAR* getName() const;
	void setName(const WCHAR* pwszName);
	unsigned int getFlags() const;
	bool isFlag(Flag flag) const;
	void setFlags(unsigned int nFlags);
	const WCHAR* getDialFrom() const;
	void setDialFrom(const WCHAR* pwszDialFrom);
	unsigned int getDisconnectWait() const;
	void setDisconnectWait(unsigned int nDisconnectWait);

private:
	qs::wstring_ptr wstrName_;
	unsigned int nFlags_;
	qs::wstring_ptr wstrDialFrom_;
	unsigned int nDisconnectWait_;
};


/****************************************************************************
 *
 * GoRoundContentHandler
 *
 */

class GoRoundContentHandler : public qs::DefaultHandler
{
public:
	typedef GoRound::CourseList CourseList;

public:
	explicit GoRoundContentHandler(CourseList* pListCourse);
	virtual ~GoRoundContentHandler();

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
	GoRoundContentHandler(const GoRoundContentHandler&);
	GoRoundContentHandler& operator=(const GoRoundContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_GOROUND,
		STATE_COURSE,
		STATE_DIALUP,
		STATE_TYPE,
		STATE_ENTRY
	};

private:
	CourseList* pListCourse_;
	State state_;
	GoRoundCourse* pCurrentCourse_;
	GoRoundEntry* pCurrentEntry_;
};


/****************************************************************************
 *
 * GoRoundWriter
 *
 */

class GoRoundWriter
{
public:
	explicit GoRoundWriter(qs::Writer* pWriter);
	~GoRoundWriter();

public:
	bool write(const GoRound* pGoRound);

private:
	bool write(const GoRoundCourse* pCourse);
	bool write(const GoRoundDialup* pDialup);
	bool write(const GoRoundEntry* pEntry);

private:
	GoRoundWriter(const GoRoundWriter&);
	GoRoundWriter& operator=(const GoRoundWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __GOROUND_H__
