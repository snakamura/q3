/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SAX_H__
#define __SAX_H__

#include <qs.h>
#include <qssax.h>


namespace qs {

/****************************************************************************
 *
 * Attribute
 *
 */

struct Attribute
{
	WSTRING wstrQName_;
	const WCHAR* pwszNamespaceURI_;
	const WCHAR* pwszLocalName_;
	WSTRING wstrValue_;
	bool bNamespaceDecl_;
};


/****************************************************************************
 *
 * AttributesImpl
 *
 */

class AttributesImpl : public Attributes2
{
public:
	typedef std::vector<Attribute> AttributeList;

public:
	AttributesImpl(const AttributeList& l);
	virtual ~AttributesImpl();

public:
	virtual int getLength() const;
	virtual const WCHAR* getURI(int nIndex) const;
	virtual const WCHAR* getLocalName(int nIndex) const;
	virtual const WCHAR* getQName(int nIndex) const;
	virtual const WCHAR* getType(int nIndex) const;
	virtual const WCHAR* getValue(int nIndex) const;
	virtual int getIndex(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual int getIndex(const WCHAR* pwszQName) const;
	virtual const WCHAR* getType(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual const WCHAR* getType(const WCHAR* pwszQName) const;
	virtual const WCHAR* getValue(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual const WCHAR* getValue(const WCHAR* pwszQName) const;

public:
	virtual bool isDeclared(int index) const;
	virtual bool isDeclared(const WCHAR* pwszQName) const;
	virtual bool isDeclared(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual bool isSpecified(int index) const;
	virtual bool isSpecified(const WCHAR* pwszQName) const;
	virtual bool isSpecified(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;

private:
	AttributesImpl(const AttributesImpl&);
	AttributesImpl& operator=(const AttributesImpl&);

private:
	const AttributeList& listAttribute_;
};

}

#endif // __SAX_H__
