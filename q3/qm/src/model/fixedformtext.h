/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	FixedFormTextManager();
	~FixedFormTextManager();

public:
	const TextList& getTextList() const;

public:
	void addText(std::auto_ptr<FixedFormText> pText);

private:
	bool load();

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
	explicit FixedFormText(const WCHAR* pwszName);
	~FixedFormText();

public:
	const WCHAR* getName() const;
	const WCHAR* getText() const;

public:
	void setText(qs::wstring_ptr wstrText);

private:
	FixedFormText(const FixedFormText&);
	FixedFormText& operator=(const FixedFormText&);

private:
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrText_;
};


/****************************************************************************
 *
 * FixedFormTextContentHandler
 *
 */

class FixedFormTextContentHandler : public qs::DefaultHandler
{
public:
	explicit FixedFormTextContentHandler(FixedFormTextManager* pManager);
	virtual ~FixedFormTextContentHandler();

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
	qs::StringBuffer<qs::WSTRING> buffer_;
};

}

#endif // __FIXEDFORMTEXT_H__
