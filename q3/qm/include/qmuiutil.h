/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMUIUTIL_H__
#define __QMUIUTIL_H__

#include <qm.h>

namespace qm {

/****************************************************************************
 *
 * History
 *
 */

class QMEXPORTCLASS History
{
public:
	History(qs::Profile* pProfile,
			const WCHAR* pwszSection);
	~History();

public:
	qs::wstring_ptr getValue(unsigned int n) const;
	unsigned int getSize() const;
	void addValue(const WCHAR* pwszValue);

private:
	History(const History&);
	History& operator=(const History&);

private:
	qs::Profile* pProfile_;
	const WCHAR* pwszSection_;
	unsigned int nSize_;
};

}

#endif // __QMUIUTIL_H__
