/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __REGEXNFA_H__
#define __REGEXNFA_H__

#include <qs.h>
#include <qsregex.h>

#include <vector>


namespace qs {

class RegexAtom;
class RegexNode;
	class RegexRegexNode;
	class RegexBrunchNode;
	class RegexPieceNode;

class RegexNfa;
class RegexNfaState;
class RegexNfaCompiler;


/****************************************************************************
 *
 * RegexNfa
 *
 */

class RegexNfa
{
public:
	RegexNfa(RegexRegexNode* pNode, QSTATUS* pstatus);
	~RegexNfa();

public:
	unsigned int getStateCount() const;
	const RegexNfaState* getState(unsigned int n) const;

public:
	QSTATUS createState(unsigned int* pn);
	QSTATUS setTransition(unsigned int nFrom,
		unsigned int nTo, const RegexAtom* pAtom);
	QSTATUS pushGroup(unsigned int nGroup);
	void popGroup();

private:
	RegexNfa(const RegexNfa&);
	RegexNfa& operator=(const RegexNfa&);

private:
	typedef std::vector<std::pair<RegexNfaState*, RegexNfaState*> > StateList;
	typedef std::vector<unsigned int> GroupStack;

private:
	RegexNode* pNode_;
	StateList listState_;
	GroupStack stackGroup_;
};


/****************************************************************************
 *
 * RegexNfaState
 *
 */

class RegexNfaState
{
public:
	typedef std::vector<unsigned int> GroupList;

public:
	RegexNfaState(const RegexAtom* pAtom, unsigned int nTo,
		const GroupList& listGroup, RegexNfaState* pPrev, QSTATUS* pstatus);
	~RegexNfaState();

public:
	bool match(WCHAR c) const;
	bool isEpsilon() const;
	unsigned int getTo() const;
	const GroupList& getGroupList() const;
	RegexNfaState* getNext() const;

private:
	RegexNfaState(const RegexNfaState&);
	RegexNfaState& operator=(const RegexNfaState&);

private:
	const RegexAtom* pAtom_;
	unsigned int nTo_;
	GroupList listGroup_;
	RegexNfaState* pNext_;
};


/****************************************************************************
 *
 * RegexNfaCompiler
 *
 */

class RegexNfaCompiler
{
public:
	RegexNfaCompiler(QSTATUS* pstatus);
	~RegexNfaCompiler();

public:
	QSTATUS compile(RegexRegexNode* pNode, RegexNfa** ppNfa) const;

private:
	QSTATUS compileNode(const RegexNode* pNode,
		RegexNfa* pNfa, unsigned int nFrom, unsigned int nTo) const;
	QSTATUS compileRegexNode(const RegexRegexNode* pRegexNode,
		RegexNfa* pNfa, unsigned int nFrom, unsigned int nTo) const;
	QSTATUS compileBrunchNode(const RegexBrunchNode* pBrunchNode,
		RegexNfa* pNfa, unsigned int nFrom, unsigned int nTo) const;
	QSTATUS compilePieceNode(const RegexPieceNode* pPieceNode,
		RegexNfa* pNfa, unsigned int nFrom, unsigned int nTo) const;
	QSTATUS compileAtom(const RegexAtom* pAtom, unsigned int nCount,
		RegexNfa* pNfa, unsigned int nFrom, unsigned int nTo) const;

private:
	RegexNfaCompiler(const RegexNfaCompiler&);
	RegexNfaCompiler& operator=(const RegexNfaCompiler&);
};


/****************************************************************************
 *
 * RegexNfaMatcher
 *
 */

class RegexNfaMatcher
{
public:
	RegexNfaMatcher(const RegexNfa* pNfa);
	~RegexNfaMatcher();

public:
	QSTATUS match(const WCHAR* pwsz, size_t nLen,
		bool* pbMatch, RegexPattern::RangeList* pList);
	QSTATUS search(const WCHAR* pwsz, size_t nLen, const WCHAR** ppStart,
		const WCHAR** ppEnd, RegexPattern::RangeList* pList);

private:
	QSTATUS match(const WCHAR* pStart, const WCHAR* pEnd, bool bMatch,
		const WCHAR** ppEnd, RegexPattern::RangeList* pList);

private:
	RegexNfaMatcher(const RegexNfaMatcher&);
	RegexNfaMatcher& operator=(const RegexNfaMatcher&);

private:
	typedef std::vector<std::pair<const RegexNfaState*, const WCHAR*> > Stack;

private:
	const RegexNfa* pNfa_;
	Stack stackMatch_;
	Stack stackBackTrack_;
};

}

#endif // __REGEXNFA_H__
