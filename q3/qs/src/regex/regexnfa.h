/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __REGEXNFA_H__
#define __REGEXNFA_H__

#include <qs.h>
#include <qsregex.h>

#include <vector>

#include "regexparser.h"


namespace qs {

class RegexNfaCompiler;
class RegexNfaMatcher;
class RegexNfa;
class RegexNfaState;

class RegexAtom;
class RegexNode;
	class RegexRegexNode;
	class RegexBrunchNode;
	class RegexPieceNode;
class RegexMatchCallback;


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
					 unsigned int nTo,
					 bool bBackTrack) const;

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
	void match(const WCHAR* pStart,
			   const WCHAR* pEnd,
			   const WCHAR* pCurrent,
			   bool bMatchEnd,
			   const WCHAR** ppEnd,
			   MatchStack* pStackMatch) const;
	void search(const WCHAR* pStart,
				const WCHAR* pEnd,
				const WCHAR* pCurrent,
				bool bReverse,
				bool bMatchEnd,
				const WCHAR** ppStart,
				const WCHAR** ppEnd,
				MatchStack* pStackMatch) const;

public:
	static void getMatch(const WCHAR* pStart,
						 const WCHAR* pEnd,
						 const MatchStack& stackMatch,
						 RegexRangeList* pList);
	static void getMatch(const MatchStack& stackMatch,
						 RegexRangeList* pList);

private:
	RegexNfaMatcher(const RegexNfaMatcher&);
	RegexNfaMatcher& operator=(const RegexNfaMatcher&);

private:
	const RegexNfa* pNfa_;
};


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
					   const RegexAtom* pAtom,
					   bool bBackTrack);
	void setTransition(unsigned int nFrom,
					   unsigned int nTo,
					   std::auto_ptr<RegexNfa> pNfa,
					   RegexRegexNode::GroupType groupType);
	void pushGroup(unsigned int nGroup);
	void popGroup();

public:
	const WCHAR* getCandidate(const WCHAR* pStart,
							  const WCHAR* pEnd,
							  const WCHAR* p,
							  bool bReverse) const;

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
	RegexNfaState(unsigned int nTo,
				  const GroupList& listGroup,
				  RegexNfaState* pPrev);
	virtual ~RegexNfaState();

public:
	unsigned int getTo() const;
	const GroupList& getGroupList() const;
	RegexNfaState* getNext() const;

public:
	virtual const WCHAR* match(const WCHAR* pStart,
							   const WCHAR* pEnd,
							   const WCHAR* p,
							   RegexNfaMatcher::MatchStack* pStackMatch,
							   RegexMatchCallback* pCallback) const = 0;
	virtual bool isEpsilon() const = 0;
	virtual bool canBackTrack() const = 0;

private:
	RegexNfaState(const RegexNfaState&);
	RegexNfaState& operator=(const RegexNfaState&);

private:
	unsigned int nTo_;
	GroupList listGroup_;
	std::auto_ptr<RegexNfaState> pNext_;
};


/****************************************************************************
 *
 * RegexNfaAtomState
 *
 */

class RegexNfaAtomState : public RegexNfaState
{
public:
	RegexNfaAtomState(const RegexAtom* pAtom,
					  bool bBackTrack,
					  unsigned int nTo,
					  const GroupList& listGroup,
					  RegexNfaState* pPrev);
	virtual ~RegexNfaAtomState();

public:
	virtual const WCHAR* match(const WCHAR* pStart,
							   const WCHAR* pEnd,
							   const WCHAR* p,
							   RegexNfaMatcher::MatchStack* pStackMatch,
							   RegexMatchCallback* pCallback) const;
	virtual bool isEpsilon() const;
	virtual bool canBackTrack() const;

private:
	RegexNfaAtomState(const RegexNfaAtomState&);
	RegexNfaAtomState& operator=(const RegexNfaAtomState&);

private:
	const RegexAtom* pAtom_;
	bool bBackTrack_;
};


/****************************************************************************
 *
 * RegexNfaNfaState
 *
 */

class RegexNfaNfaState : public RegexNfaState
{
public:
	RegexNfaNfaState(std::auto_ptr<RegexNfa> pNfa,
					 RegexRegexNode::GroupType groupType,
					 unsigned int nTo,
					 const GroupList& listGroup,
					 RegexNfaState* pPrev);
	virtual ~RegexNfaNfaState();

public:
	virtual const WCHAR* match(const WCHAR* pStart,
							   const WCHAR* pEnd,
							   const WCHAR* p,
							   RegexNfaMatcher::MatchStack* pStackMatch,
							   RegexMatchCallback* pCallback) const;
	virtual bool isEpsilon() const;
	virtual bool canBackTrack() const;

private:
	RegexNfaNfaState(const RegexNfaNfaState&);
	RegexNfaNfaState& operator=(const RegexNfaNfaState&);

private:
	std::auto_ptr<RegexNfa> pNfa_;
	RegexRegexNode::GroupType groupType_;
};

}

#endif // __REGEXNFA_H__
