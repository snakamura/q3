/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <qsmime.h>
#include <qsstring.h>

#include <vector>


namespace qm {

/****************************************************************************
 *
 * MessageSecurity
 *
 */

enum MessageSecurity
{
	MESSAGESECURITY_SMIMESIGN				= 0x0001,
	MESSAGESECURITY_SMIMEENCRYPT			= 0x0002,
	MESSAGESECURITY_SMIMEMULTIPARTSIGNED	= 0x0010,
	MESSAGESECURITY_SMIMEENCRYPTFORSELF		= 0x0020,
	MESSAGESECURITY_PGPSIGN					= 0x0100,
	MESSAGESECURITY_PGPENCRYPT				= 0x0200,
	MESSAGESECURITY_PGPMIME					= 0x1000
};


/****************************************************************************
 *
 * XQMAILAttachmentParser
 *
 */

class XQMAILAttachmentParser : public qs::FieldParser
{
public:
	typedef std::vector<qs::WSTRING> AttachmentList;

public:
	XQMAILAttachmentParser();
	virtual ~XQMAILAttachmentParser();

public:
	const AttachmentList& getAttachments() const;

public:
	virtual qs::Part::Field parse(const qs::Part& part,
								  const WCHAR* pwszName);
	virtual qs::string_ptr unparse(const qs::Part& part) const;

public:
	static qs::wstring_ptr format(const AttachmentList& l);

private:
	XQMAILAttachmentParser(const XQMAILAttachmentParser&);
	XQMAILAttachmentParser& operator=(const XQMAILAttachmentParser&);

private:
	AttachmentList listAttachment_;
};

}

#endif // __MESSAGE_H__
