/*
 * $Id: message.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	XQMAILAttachmentParser(qs::QSTATUS* pstatus);
	XQMAILAttachmentParser(const AttachmentList& listAttachment,
		qs::QSTATUS* pstatus);
	virtual ~XQMAILAttachmentParser();

public:
	const AttachmentList& getAttachments() const;

public:
	virtual qs::QSTATUS parse(const qs::Part& part,
		const WCHAR* pwszName, qs::Part::Field* pField);
	virtual qs::QSTATUS unparse(const qs::Part& part, qs::STRING* pstrValue) const;

public:
	static qs::QSTATUS format(const AttachmentList& l, qs::WSTRING* pwstr);

private:
	XQMAILAttachmentParser(const XQMAILAttachmentParser&);
	XQMAILAttachmentParser& operator=(const XQMAILAttachmentParser&);

private:
	AttachmentList listAttachment_;
};

}

#endif // __MESSAGE_H__
