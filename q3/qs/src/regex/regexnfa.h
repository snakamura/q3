/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __REGEXNFA_H__
#define __REGEXNFA_H__

#include <qs.h>
#include <qsregex.h>

#include <vector>


namespace qs {

class RegexNfa;
class RegexNfaState;
class RegexNfaCompiler;

class RegexAtom;
class RegexNode;
	class RegexRegexNode;
	class RegexBrunchNode;
	class RegexPieceNode;
class RegexMatchCallback;


/****************************************************************************
 *
 * RegexNfa
 *
 */

class RegexNfa
{
public:
	RegexNfa(std::auto_ptr<RegexRegexNode> pNode);
	~RegexNfa();

public:
	unsigned int getStateCount() const;
	const RegexNfaState* getState(unsigned int n) const;

public:
	unsigned int createState();
	void setTransition(unsigned int nFrom,
					   unsigned int nTo,
					   const RegexAtom* pAtom);
	void pushGroup(unsigned int nGroup);
	void popGroup();

private:
	RegexNfa(const RegexNfa&);
	RegexNfa& operator=(const RegexNfa&);

private:
	typedef std::vector<std::pair<RegexNfaState*, RegexNfaState*> > StateList;
	typedef std::vector<unsigned int> GroupStack;

private:
	std::auto_ptr<RegexNode> pNode_;
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
	RegexNfaState(const RegexAtom* pAtom,
				  unsigned int nTo,
				  const GroupList& listGroup,
				  RegexNfaState* pPrev);
	~RegexNfaState();

public:
	const WCHAR* match(const WCHAR* pStart,
					   const WCHAR* pEnd,
					   RegexMatchCallback* pCallback) const;
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
	std::auto_ptr<RegexNfaState> pNext_;
};


/****************************************************************************
 *
 * RegexNfaCompiler
 *
 */

class RegexNfaCompiler
{
public:
	RegexNfaCompiler();
	~RegexNfaCompiler();

public:
	std::auto_ptr<RegexNfa> compile(std::auto_ptr<RegexRegexNode> pNode) const;

private:
	void compileNode(const RegexNode* pNode,
					 RegexNfa* pNfa,
					 unsigned int nFrom,
					 unsigned int nTo) const;
	void compileRegexNode(const RegexRegexNode* pRegexNode,
						  RegexNfa* pNfa,
						  unsigned int nFrom,
						  unsigned int nTo) const;
	void compileBrunchNode(const RegexBrunchNode* pBrunchNode,
						   RegexNfa* pNfa,
						   unsigned int nFrom,
						   unsigned int nTo) const;
	void compilePieceNode(const RegexPieceNode* pPieceNode,
						  RegexNfa* pNfa,
						  unsigned int nFrom,
						  unsigned int nTo) const;
	void compileAtom(const RegexAtom* pAtom,
					 unsigned int nCount,
					 RegexNfa* pNfa,
					 unsigned int nFrom,
					 unsigned int nTo) const;

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
	struct Match
	{
		const RegexNfaState* pState_;
		const WCHAR* pStart_;
		const WCHAR* pEnd_;
	};

public:
	typedef std::vector<std::pair<const RegexNfaState*, const WCHAR*> > Stack;
	typedef std::vector<Match> MatchStack;

public:
	explicit RegexNfaMatcher(const RegexNfa* pNfa);
	~RegexNfaMatcher();

public:
	bool match(const WCHAR* pwsz,
			   size_t nLen,
			   RegexRangeList* pList) const;
	void search(const WCHAR* pwsz,
				size_t nLen,
				const WCHAR* p,
				bool bReverse,
				const WCHAR** ppStart,
				const WCHAR** ppEnd,
				RegexRangeList* pList) const;

public:
	static void getMatch(const MatchStack& stackMatch,
						 RegexRangeList* pList);

private:
	void match(const WCHAR* pStart,
			   const WCHAR* pEnd,
			   bool bMatch,
			   const WCHAR** ppEnd,
			   RegexRangeList* pList) const;

private:
	RegexNfaMatcher(const RegexNfaMatcher&);
	RegexNfaMatcher& operator=(const RegexNfaMatcher&);

private:
	const RegexNfa* pNfa_;
};

}

#endif // __REGEXNFA_H__
