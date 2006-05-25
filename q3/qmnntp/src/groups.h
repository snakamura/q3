/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __GROUPS_H__
#define __GROUPS_H__

#include <qs.h>
#include <qssax.h>
#include <qsstring.h>

#include <vector>


namespace qmnntp {

class Groups;
class Group;
class GroupsContentHandler;
class GroupsWriter;

class GroupsData;


/****************************************************************************
 *
 * Groups
 *
 */

class Groups
{
public:
	typedef std::vector<Group*> GroupList;

public:
	explicit Groups(const WCHAR* pwszPath);
	~Groups();

public:
	const GroupList& getGroupList() const;
	const WCHAR* getDate() const;
	const WCHAR* getTime() const;
	void add(const GroupsData* pData,
			 const WCHAR* pwszDate,
			 const WCHAR* pwszTime);
	bool save() const;

public:
	void setDateTime(const WCHAR* pwszDate,
					 const WCHAR* pwszTime);
	void add(std::auto_ptr<Group> pGroup);

private:
	bool load();
	void clear();

private:
	Groups(const Groups&);
	Groups& operator=(const Groups&);

private:
	qs::wstring_ptr wstrPath_;
	GroupList listGroup_;
	qs::wstring_ptr wstrDate_;
	qs::wstring_ptr wstrTime_;
	mutable bool bModified_;
};


/****************************************************************************
 *
 * Group
 *
 */

class Group
{
public:
	Group(const WCHAR* pwszName);
	~Group();

public:
	const WCHAR* getName() const;

private:
	Group(const Group&);
	Group& operator=(const Group&);

private:
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * GroupsContentHandler
 *
 */

class GroupsContentHandler : public qs::DefaultHandler
{
public:
	explicit GroupsContentHandler(Groups* pGroups);
	virtual ~GroupsContentHandler();

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
	GroupsContentHandler(const GroupsContentHandler&);
	GroupsContentHandler& operator=(const GroupsContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_GROUPS,
		STATE_GROUP
	};

private:
	Groups* pGroups_;
	State state_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * GroupsWriter
 *
 */

class GroupsWriter
{
public:
	explicit GroupsWriter(qs::Writer* pWriter);
	~GroupsWriter();

public:
	bool write(const Groups* pGroups);

private:
	GroupsWriter(const GroupsWriter&);
	GroupsWriter& operator=(const GroupsWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __GROUPS_H__
