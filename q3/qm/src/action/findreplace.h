/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
		FLAG_MATCHCASE	= 0x01,
		FLAG_REGEX		= 0x02
	};

public:
	FindReplaceData(const WCHAR* pwszFind,
					const WCHAR* pwszReplace,
					unsigned int nFlags);
	~FindReplaceData();

public:
	const WCHAR* getFind() const;
	const WCHAR* getReplace() const;
	unsigned int getFlags() const;

private:
	FindReplaceData(const FindReplaceData&);
	FindReplaceData& operator=(const FindReplaceData&);

private:
	qs::wstring_ptr wstrFind_;
	qs::wstring_ptr wstrReplace_;
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
	FindReplaceManager();
	~FindReplaceManager();

public:
	const FindReplaceData* getData() const;
	void setData(const WCHAR* pwszFind,
				 unsigned int nFlags);
	void setData(const WCHAR* pwszFind,
				 const WCHAR* pwszReplace,
				 unsigned int nFlags);

private:
	FindReplaceManager(const FindReplaceManager&);
	FindReplaceManager& operator=(const FindReplaceManager&);

private:
	std::auto_ptr<FindReplaceData> pData_;
};

}

#endif // __FINDREPLACE_H__
