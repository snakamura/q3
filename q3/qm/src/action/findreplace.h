/*
 * $Id: findreplace.h,v 1.1 2003/05/22 08:05:17 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FINDREPLACE_H__
#define __FINDREPLACE_H__

#include <qm.h>

#include <qs.h>
#include <qsstring.h>


namespace qm {

/****************************************************************************
 *
 * FindReplaceData
 *
 */

class FindReplaceData
{
public:
	enum Flag {
		FLAG_MATCHCASE	= 0x01
	};

public:
	FindReplaceData(const WCHAR* pwszFind, const WCHAR* pwszReplace,
		unsigned int nFlags, qs::QSTATUS* pstatus);
	~FindReplaceData();

public:
	const WCHAR* getFind() const;
	const WCHAR* getReplace() const;
	unsigned int getFlags() const;

private:
	FindReplaceData(const FindReplaceData&);
	FindReplaceData& operator=(const FindReplaceData&);

private:
	qs::WSTRING wstrFind_;
	qs::WSTRING wstrReplace_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * FindReplaceManager
 *
 */

class FindReplaceManager
{
public:
	FindReplaceManager(qs::QSTATUS* pstatus);
	~FindReplaceManager();

public:
	const FindReplaceData* getData() const;
	qs::QSTATUS setData(const WCHAR* pwszFind, unsigned int nFlags);
	qs::QSTATUS setData(const WCHAR* pwszFind,
		const WCHAR* pwszReplace, unsigned int nFlags);

private:
	FindReplaceManager(const FindReplaceManager&);
	FindReplaceManager& operator=(const FindReplaceManager&);

private:
	FindReplaceData* pData_;
};

}

#endif // __FINDREPLACE_H__
