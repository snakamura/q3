/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qsutil.h>

using namespace qs;


/****************************************************************************
 *
 * MessageFormat
 *
 */

qs::MessageFormat::MessageFormat(const WCHAR* pwszTemplate)
{
	wstrTemplate_ = allocWString(pwszTemplate);
}

qs::MessageFormat::~MessageFormat()
{
}

wstring_ptr qs::MessageFormat::format(const ArgumentList& listArgument) const
{
	StringBuffer<WSTRING> buf;
	for (const WCHAR* p = wstrTemplate_.get(); *p; ++p) {
		if (*p == L'%') {
			WCHAR c = *(p + 1);
			if (c == L'%') {
				buf.append(L'%');
				++p;
			}
			else if (L'0' <= c && c <= L'9') {
				size_t n = c - L'0';
				if (n < listArgument.size())
					buf.append(listArgument[n]);
				else
					buf.append(p, 2);
				++p;
			}
			else {
				buf.append(L'%');
			}
		}
		else {
			buf.append(*p);
		}
	}
	return buf.getString();
}

wstring_ptr qs::MessageFormat::format(const WCHAR* pwszTemplate,
									  const WCHAR* pwszArg0)
{
	return format(pwszTemplate, pwszArg0, 0);
}

wstring_ptr qs::MessageFormat::format(const WCHAR* pwszTemplate,
									  const WCHAR* pwszArg0,
									  const WCHAR* pwszArg1)
{
	assert(pwszArg0);
	
	ArgumentList l;
	l.push_back(pwszArg0);
	if (pwszArg1)
		l.push_back(pwszArg1);
	return format(pwszTemplate, l);
}

wstring_ptr qs::MessageFormat::format(const WCHAR* pwszTemplate,
									  const ArgumentList& listArgument)
{
	return MessageFormat(pwszTemplate).format(listArgument);
}
