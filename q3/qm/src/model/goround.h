/*
 * $Id: goround.h,v 1.2 2003/05/29 08:15:50 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __GOROUND_H__
#define __GOROUND_H__

#include <qm.h>

#include <qs.h>
#include <qsregex.h>
#include <qssax.h>
#include <qsstream.h>


namespace qm {

class GoRoundContentHandler;
class GoRoundCourse;
class GoRoundCourseList;
class GoRoundDialup;
class GoRoundEntry;


/****************************************************************************
 *
 * GoRoundCourseList
 *
 */

class GoRoundCourseList
{
public:
	typedef std::vector<GoRoundCourse*> CourseList;

public:
	GoRoundCourseList(qs::InputStream* pStream, qs::QSTATUS* pstatus);
	~GoRoundCourseList();

public:
	size_t getCount() const;
	GoRoundCourse* getCourse(size_t nIndex) const;
	GoRoundCourse* getCourse(const WCHAR* pwszCourse) const;

private:
	qs::QSTATUS load(qs::InputStream* pStream);

private:
	GoRoundCourseList(const GoRoundCourseList&);
	GoRoundCourseList& operator=(const GoRoundCourseList&);

private:
	CourseList listCourse_;
};


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
	GoRoundCourse(const WCHAR* pwszName,
		unsigned int nFlags, qs::QSTATUS* pstatus);
	~GoRoundCourse();

public:
	const WCHAR* getName() const;
	bool isFlag(Flag flag) const;
	Type getType() const;
	const GoRoundDialup* getDialup() const;
	const EntryList& getEntries() const;

public:
	void setDialup(GoRoundDialup* pDialup);
	void setType(Type type);
	qs::QSTATUS addEntry(GoRoundEntry* pEntry);

private:
	GoRoundCourse(const GoRoundCourse&);
	GoRoundCourse& operator=(const GoRoundCourse&);

private:
	qs::WSTRING wstrName_;
	unsigned int nFlags_;
	GoRoundDialup* pDialup_;
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
	GoRoundEntry(const WCHAR* pwszAccount, const WCHAR* pwszSubAccount,
		const WCHAR* pwszFolder, unsigned int nFlags,
		const WCHAR* pwszFilterName, qs::QSTATUS* pstatus);
	~GoRoundEntry();

public:
	const WCHAR* getAccount() const;
	const WCHAR* getSubAccount() const;
	const qs::RegexPattern* getFolderNamePattern() const;
	bool isFlag(Flag flag) const;
	const WCHAR* getFilterName() const;

private:
	GoRoundEntry(const GoRoundEntry&);
	GoRoundEntry& operator=(const GoRoundEntry&);

private:
	qs::WSTRING wstrAccount_;
	qs::WSTRING wstrSubAccount_;
	qs::RegexPattern* pFolderName_;
	unsigned int nFlags_;
	qs::WSTRING wstrFilterName_;
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
	GoRoundDialup(const WCHAR* pwszName, unsigned int nFlags,
		const WCHAR* pwszDialFrom, unsigned int nDisconnectWait,
		qs::QSTATUS* pstatus);
	~GoRoundDialup();

public:
	const WCHAR* getName() const;
	unsigned int getFlags() const;
	const WCHAR* getDialFrom() const;
	unsigned int getDisconnectWait() const;

private:
	GoRoundDialup(const GoRoundDialup&);
	GoRoundDialup& operator=(const GoRoundDialup&);

private:
	qs::WSTRING wstrName_;
	unsigned int nFlags_;
	qs::WSTRING wstrDialFrom_;
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
	typedef GoRoundCourseList::CourseList CourseList;

public:
	GoRoundContentHandler(CourseList* pListCourse, qs::QSTATUS* pstatus);
	virtual ~GoRoundContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

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

}

#endif // __GOROUND_H__
