/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
