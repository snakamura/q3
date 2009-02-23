/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

class Account;
class ActionInvoker;
class Document;
class Folder;
class Macro;
class MacroErrorHandler;
class Message;
class MessageHolderBase;
class SubAccount;


/****************************************************************************
 *
 * Template
 *
 */

class Template
{
public:
	enum Result {
		RESULT_SUCCESS,
		RESULT_ERROR,
		RESULT_CANCEL
	};

public:
	typedef std::vector<std::pair<qs::WSTRING, Macro*> > ValueList;

public:
	explicit Template(const ValueList& l);
	~Template();

public:
	Result getValue(const TemplateContext& context,
					qs::wstring_ptr* pwstrValue) const;
	Result getValue(const TemplateContext& context,
					qs::wxstring_size_ptr* pwstrValue) const;

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
	TemplateContext(MessageHolderBase* pmh,
					Message* pMessage,
					const MessageHolderList& listSelected,
					Folder* pFolder,
					Account* pAccount,
					SubAccount* pSubAccount,
					Document* pDocument,
					const ActionInvoker* pActionInvoker,
					HWND hwnd,
					const WCHAR* pwszBodyCharset,
					unsigned int nSecurityMode,
					unsigned int nMacroFlags,
					qs::Profile* pProfile,
					MacroErrorHandler* pErrorHandler,
					const ArgumentList& listArgument);
	~TemplateContext();

public:
	MessageHolderBase* getMessageHolder() const;
	Message* getMessage() const;
	const MessageHolderList& getSelectedMessageHolders() const;
	Folder* getFolder() const;
	Account* getAccount() const;
	SubAccount* getSubAccount() const;
	Document* getDocument() const;
	const ActionInvoker* getActionInvoker() const;
	HWND getWindow() const;
	const WCHAR* getBodyCharset() const;
	unsigned int getMacroFlags() const;
	unsigned int getSecurityMode() const;
	qs::Profile* getProfile() const;
	MacroErrorHandler* getErrorHandler() const;
	const ArgumentList& getArgumentList() const;

private:
	TemplateContext(const TemplateContext&);
	TemplateContext& operator=(const TemplateContext&);

private:
	MessageHolderBase* pmh_;
	Message* pMessage_;
	const MessageHolderList& listSelected_;
	Folder* pFolder_;
	Account* pAccount_;
	SubAccount* pSubAccount_;
	Document* pDocument_;
	const ActionInvoker* pActionInvoker_;
	HWND hwnd_;
	qs::wstring_ptr wstrBodyCharset_;
	unsigned int nMacroFlags_;
	unsigned int nSecurityMode_;
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
	TemplateParser();
	~TemplateParser();

public:
	std::auto_ptr<Template> parse(qs::Reader* pReader,
								  const WCHAR* pwszName) const;

private:
	std::auto_ptr<Template> error(const WCHAR* pwszLog,
								  const WCHAR* pwszArg) const;

private:
	TemplateParser(const TemplateParser&);
	TemplateParser& operator=(const TemplateParser&);
};

}

#endif // __QMTEMPLATE_H__
