/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	GoRoundCourseList();
	~GoRoundCourseList();

public:
	size_t getCount() const;
	GoRoundCourse* getCourse(size_t nIndex) const;
	GoRoundCourse* getCourse(const WCHAR* pwszCourse) const;

public:
	bool load(qs::InputStream* pStream);

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
				  unsigned int nFlags);
	~GoRoundCourse();

public:
	const WCHAR* getName() const;
	bool isFlag(Flag flag) const;
	Type getType() const;
	const GoRoundDialup* getDialup() const;
	const EntryList& getEntries() const;

public:
	void setDialup(std::auto_ptr<GoRoundDialup> pDialup);
	void setType(Type type);
	void addEntry(std::auto_ptr<GoRoundEntry> pEntry);

private:
	GoRoundCourse(const GoRoundCourse&);
	GoRoundCourse& operator=(const GoRoundCourse&);

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
	
	enum ConnectReceiveBeforeSend {
		CRBS_NONE,
		CRBS_TRUE,
		CRBS_FALSE
	};

public:
	GoRoundEntry(const WCHAR* pwszAccount,
				 const WCHAR* pwszSubAccount,
				 std::auto_ptr<qs::RegexPattern> pFolderName,
				 unsigned int nFlags,
				 const WCHAR* pwszFilterName,
				 ConnectReceiveBeforeSend crbs);
	~GoRoundEntry();

public:
	const WCHAR* getAccount() const;
	const WCHAR* getSubAccount() const;
	const qs::RegexPattern* getFolderNamePattern() const;
	bool isFlag(Flag flag) const;
	const WCHAR* getFilterName() const;
	ConnectReceiveBeforeSend getConnectReceiveBeforeSend() const;

private:
	GoRoundEntry(const GoRoundEntry&);
	GoRoundEntry& operator=(const GoRoundEntry&);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrSubAccount_;
	std::auto_ptr<qs::RegexPattern> pFolderName_;
	unsigned int nFlags_;
	qs::wstring_ptr wstrFilterName_;
	ConnectReceiveBeforeSend crbs_;
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
	GoRoundDialup(const WCHAR* pwszName,
				  unsigned int nFlags,
				  const WCHAR* pwszDialFrom,
				  unsigned int nDisconnectWait);
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
	typedef GoRoundCourseList::CourseList CourseList;

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

}

#endif // __GOROUND_H__
