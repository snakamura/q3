/*
 * $Id: regexnfa.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __REGEXNFA_H__
#define __REGEXNFA_H__

#include <qs.h>

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
	RegexNfa(RegexNode* pNode, QSTATUS* pstatus);
	~RegexNfa();

public:
	unsigned int getStateCount() const;
	const RegexNfaState* getState(unsigned int n) const;

public:
	QSTATUS createState(unsigned int* pn);
	QSTATUS setTransition(unsigned int nFrom,
		unsigned int nTo, const RegexAtom* pAtom);

private:
	RegexNfa(const RegexNfa&);
	RegexNfa& operator=(const RegexNfa&);

private:
	typedef std::vector<RegexNfaState*> StateList;

private:
	RegexNode* pNode_;
	StateList listState_;
};


/****************************************************************************
 *
 * RegexNfaState
 *
 */

class RegexNfaState
{
public:
	RegexNfaState(const RegexAtom* pAtom, unsigned int nTo,
		RegexNfaState* pNext, QSTATUS* pstatus);
	~RegexNfaState();

public:
	bool match(WCHAR c) const;
	bool isEpsilon() const;
	unsigned int getTo() const;
	const RegexNfaState* getNext() const;

private:
	RegexNfaState(const RegexNfaState&);
	RegexNfaState& operator=(const RegexNfaState&);

private:
	const RegexAtom* pAtom_;
	unsigned int nTo_;
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
	QSTATUS compile(RegexNode* pNode, RegexNfa** ppNfa) const;
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
	QSTATUS match(const WCHAR* pwsz, bool* pbMatch);

private:
	void epsilonTransition();
	void epsilonTransition(unsigned int n);
	QSTATUS makeTransition(WCHAR c);

private:
	RegexNfaMatcher(const RegexNfaMatcher&);
	RegexNfaMatcher& operator=(const RegexNfaMatcher&);

private:
	typedef std::vector<bool> States;

private:
	const RegexNfa* pNfa_;
	States states_;
};

}

#endif // __REGEXNFA_H__
