/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSREGEX_H__
#define __QSREGEX_H__

#include <qs.h>
#include <qsstring.h>

#include <vector>


namespace qs {

struct RegexRange;
class RegexPattern;
class RegexCompiler;

class RegexNfa;


/****************************************************************************
 *
 * RegexRange
 *
 */

struct QSEXPORTCLASS RegexRange
{
	RegexRange();
	
	const WCHAR* pStart_;
	const WCHAR* pEnd_;
};


/****************************************************************************
 *
 * RegexRangeList
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

struct QSEXPORTCLASS RegexRangeList
{
	typedef std::vector<RegexRange> List;
	
	/**
	 * Get replaced string.
	 *
	 * @param pwszReplace [in] Replace string.
	 * @return Replaced string. Can not be null.
	 * @exception std::bad_alloc Out of memory.
	 */
	wstring_ptr getReplace(const WCHAR* pwszReplace) const;
	
	/**
	 * Get replaced string.
	 *
	 * @param pwszReplace [in] Replace string.
	 * @param pBuf [in] StringBuffer which replaced string is written to.
	 * @exception std::bad_alloc Out of memory.
	 */
	void getReplace(const WCHAR* pwszReplace,
					StringBuffer<WSTRING>* pBuf) const;
	
	List list_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * RegexPattern
 *
 */

class QSEXPORTCLASS RegexPattern
{
public:
	explicit RegexPattern(std::auto_ptr<RegexNfa> pNfa);
	~RegexPattern();

public:
	/**
	 * Check if the specified string matches against the pattern.
	 *
	 * @param pwsz [in] String.
	 * @return true if match, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool match(const WCHAR* pwsz) const;
	
	/**
	 * Check if the specified string matches against the pattern.
	 *
	 * @param pwsz [in] String.
	 * @param nLen [in] String length.
	 * @param pList [in] Range list. Can be null.
	 * @return true if match, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool match(const WCHAR* pwsz,
			   size_t nLen,
			   RegexRangeList* pList) const;
	
	/**
	 * Search the pattern in the specified string.
	 *
	 * @param pwsz [in] String.
	 * @param nLen [in] String length;
	 * @param p [in] Pointer to string where search starts.
	 * @param bReverse [in] true if search in reverse order.
	 * @param ppStart [out] Start position. null if not found.
	 * @param ppEnd [out] End position.
	 * @param pList [in] Range list.
	 * @exception std::bad_alloc Out of memory.
	 */
	void search(const WCHAR* pwsz,
				size_t nLen,
				const WCHAR* p,
				bool bReverse,
				const WCHAR** ppStart,
				const WCHAR** ppEnd,
				RegexRangeList* pList) const;

private:
	RegexPattern(const RegexPattern&);
	RegexPattern& operator=(const RegexPattern&);

private:
	struct RegexPatternImpl* pImpl_;
};


/****************************************************************************
 *
 * RegexCompiler
 *
 */

class QSEXPORTCLASS RegexCompiler
{
public:
	enum Mode {
		MODE_MULTILINE	= 0x01,
		MODE_DOTALL		= 0x02
	};

public:
	/**
	 * Create instance.
	 */
	RegexCompiler();
	
	~RegexCompiler();

public:
	/**
	 * Compile regular expression.
	 *
	 * @param pwszPattern [in] Pattern.
	 * @return Compiled instance. null if error occured.
	 * @exception std::bad_alloc Out of memory;
	 */
	std::auto_ptr<RegexPattern> compile(const WCHAR* pwszPattern) const;
	
	/**
	 * Compile regular expression.
	 *
	 * @param pwszPattern [in] Pattern.
	 * @param nMode [in] Mode.
	 * @return Compiled instance. null if error occured.
	 * @exception std::bad_alloc Out of memory;
	 */
	std::auto_ptr<RegexPattern> compile(const WCHAR* pwszPattern,
										unsigned int nMode) const;

private:
	RegexCompiler(const RegexCompiler&);
	RegexCompiler& operator=(const RegexCompiler&);
};

}

#endif // __QSREGEX_H__
