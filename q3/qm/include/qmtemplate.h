/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMTEMPLATE_H__
#define __QMTEMPLATE_H__

#include <qm.h>

#include <qsprofile.h>
#include <qsstream.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class Template;
class TemplateContext;
class TemplateParser;

class Message;
class MessageHolderBase;
class Account;
class Document;
class Macro;
class MacroErrorHandler;


/****************************************************************************
 *
 * Template
 *
 */

class Template
{
public:
	typedef std::vector<std::pair<qs::WSTRING, Macro*> > ValueList;

public:
	Template(const ValueList& l, qs::QSTATUS* pstatus);
	~Template();

public:
	qs::QSTATUS getValue(const TemplateContext& context,
		qs::WSTRING* pwstrValue) const;

private:
	Template(const Template&);
	Template& operator=(const Template&);

private:
	ValueList listValue_;
};


/****************************************************************************
 *
 * TemplateContext
 *
 */

class TemplateContext
{
public:
	struct Argument
	{
		const WCHAR* pwszName_;
		const WCHAR* pwszValue_;
	};

public:
	typedef std::vector<Argument> ArgumentList;

public:
	TemplateContext(MessageHolderBase* pmh, Message* pMessage,
		Account* pAccount, Document* pDocument, HWND hwnd,
		qs::Profile* pProfile, MacroErrorHandler* pErrorHandler,
		const ArgumentList& listArgument, qs::QSTATUS* pstatus);
	~TemplateContext();

public:
	MessageHolderBase* getMessageHolder() const;
	Message* getMessage() const;
	Account* getAccount() const;
	Document* getDocument() const;
	HWND getWindow() const;
	qs::Profile* getProfile() const;
	MacroErrorHandler* getErrorHandler() const;
	const ArgumentList& getArgumentList() const;

private:
	TemplateContext(const TemplateContext&);
	TemplateContext& operator=(const TemplateContext&);

private:
	MessageHolderBase* pmh_;
	Message* pMessage_;
	Account* pAccount_;
	Document* pDocument_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	MacroErrorHandler* pErrorHandler_;
	const ArgumentList& listArgument_;
};


/****************************************************************************
 *
 * TemplateParser
 *
 */

class TemplateParser
{
public:
	explicit TemplateParser(qs::QSTATUS* pstatus);
	~TemplateParser();

public:
	qs::QSTATUS parse(qs::Reader* pReader, Template** ppTemplate);

private:
	TemplateParser(const TemplateParser&);
	TemplateParser& operator=(const TemplateParser&);
};

}

#endif // __QMTEMPLATE_H__
