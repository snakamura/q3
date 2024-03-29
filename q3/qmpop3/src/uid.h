/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __UID_H__
#define __UID_H__

#include <qssax.h>
#include <qsstring.h>

#include <vector>


namespace qmpop3 {

/****************************************************************************
 *
 * UID
 *
 */

class UID
{
public:
	enum Flag {
		FLAG_NONE,
		FLAG_PARTIAL
	};

public:
	struct Date
	{
		short nYear_;
		short nMonth_;
		short nDay_;
	};

public:
	UID(const WCHAR* pwszUID,
		unsigned int nFlags,
		const Date& date);
	~UID();

public:
	const WCHAR* getUID() const;
	unsigned int getFlags() const;
	const Date& getDate() const;

public:
	void update(unsigned int nFlags,
				short nYear,
				short nMonth,
				short nDay);

private:
	UID(const UID&);
	UID& operator=(const UID&);

private:
	qs::wstring_ptr wstrUID_;
	unsigned int nFlags_;
	Date date_;
};


/****************************************************************************
 *
 * UIDList
 *
 */

class UIDList
{
public:
	typedef std::vector<unsigned int> IndexList;

public:
	UIDList();
	~UIDList();

public:
	unsigned int getCount() const;
	UID* getUID(unsigned int n) const;
	unsigned int getIndex(const WCHAR* pwszUID) const;
	unsigned int getIndex(const WCHAR* pwszUID,
						  unsigned int nStart) const;

public:
	bool load(const WCHAR* pwszPath);
	bool save(const WCHAR* pwszPath) const;
	void add(std::auto_ptr<UID> pUID);
	void remove(const IndexList& l);
	UID* remove(unsigned int n);
	void setModified(bool bModified);
	bool isModified() const;

private:
	UIDList(const UIDList&);
	UIDList& operator=(const UIDList&);

private:
	typedef std::vector<UID*> List;

private:
	List list_;
	mutable bool bModified_;
};


/****************************************************************************
 *
 * UIDListContentHandler
 *
 */

class UIDListContentHandler : public qs::DefaultHandler
{
public:
	explicit UIDListContentHandler(UIDList* pList);
	virtual ~UIDListContentHandler();

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
	UIDListContentHandler(const UIDListContentHandler&);
	UIDListContentHandler& operator=(const UIDListContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_UIDL,
		STATE_UID
	};

private:
	UIDList* pList_;
	State state_;
	unsigned int nFlags_;
	UID::Date date_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * UIDListWriter
 *
 */

class UIDListWriter
{
public:
	UIDListWriter(qs::Writer* pWriter,
				  const WCHAR* pwszEncoding);
	~UIDListWriter();

public:
	bool write(const UIDList& l);

private:
	UIDListWriter(const UIDListWriter&);
	UIDListWriter& operator=(const UIDListWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __UID_H__
