/*
 * $Id: fixedformtext.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FIXEDFORMTEXT_H__
#define __FIXEDFORMTEXT_H__

#include <qm.h>

#include <qs.h>
#include <qssax.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class FixedFormTextManager;
class FixedFormText;
class FixedFormTextContentHandler;


/****************************************************************************
 *
 * FixedFormTextManager
 *
 */

class FixedFormTextManager
{
public:
	typedef std::vector<FixedFormText*> TextList;

public:
	FixedFormTextManager(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~FixedFormTextManager();

public:
	const TextList& getTextList() const;

public:
	qs::QSTATUS addText(FixedFormText* pText);

private:
	qs::QSTATUS load(const WCHAR* pwszPath);

private:
	FixedFormTextManager(const FixedFormTextManager&);
	FixedFormTextManager& operator=(const FixedFormTextManager&);

private:
	TextList listText_;
};


/****************************************************************************
 *
 * FixedFormText
 *
 */

class FixedFormText
{
public:
	FixedFormText(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	~FixedFormText();

public:
	const WCHAR* getName() const;
	const WCHAR* getText() const;

public:
	void setText(qs::WSTRING wstrText);

private:
	FixedFormText(const FixedFormText&);
	FixedFormText& operator=(const FixedFormText&);

private:
	qs::WSTRING wstrName_;
	qs::WSTRING wstrText_;
};


/****************************************************************************
 *
 * FixedFormTextContentHandler
 *
 */

class FixedFormTextContentHandler : public qs::DefaultHandler
{
public:
	FixedFormTextContentHandler(FixedFormTextManager* pManager,
		qs::QSTATUS* pstatus);
	virtual ~FixedFormTextContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	FixedFormTextContentHandler(const FixedFormTextContentHandler&);
	FixedFormTextContentHandler& operator=(const FixedFormTextContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_TEXTS,
		STATE_TEXT
	};

private:
	FixedFormTextManager* pManager_;
	State state_;
	FixedFormText* pText_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
};

}

#endif // __FIXEDFORMTEXT_H__
