/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>

#include "regexnfa.h"
#include "regexparser.h"

using namespace qs;


/****************************************************************************
 *
 * RegexNfa
 *
 */

qs::RegexNfa::RegexNfa(RegexNode* pNode, QSTATUS* pstatus) :
	pNode_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = STLWrapper<StateList>(listState_).resize(2);
	CHECK_QSTATUS_SET(pstatus);
	pNode_ = pNode;
}

qs::RegexNfa::~RegexNfa()
{
	delete pNode_;
	std::for_each(listState_.begin(),
		listState_.end(), deleter<RegexNfaState>());
}

unsigned int qs::RegexNfa::getStateCount() const
{
	return listState_.size();
}

const RegexNfaState* qs::RegexNfa::getState(unsigned int n) const
{
	assert(n < listState_.size());
	return listState_[n];
}

QSTATUS qs::RegexNfa::createState(unsigned int* pn)
{
	assert(pn);
	
	DECLARE_QSTATUS();
	
	*pn = listState_.size();
	status = STLWrapper<StateList>(listState_).push_back(0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexNfa::setTransition(unsigned int nFrom,
	unsigned int nTo, const RegexAtom* pAtom)
{
	assert(listState_.size() > nFrom);
	assert(listState_.size() > nTo);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<RegexNfaState> pState;
	status = newQsObject(pAtom, nTo, listState_[nFrom], &pState);
	CHECK_QSTATUS();
	listState_[nFrom] = pState.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * RegexNfaState
 *
 */

qs::RegexNfaState::	RegexNfaState(const RegexAtom* pAtom,
	unsigned int nTo, RegexNfaState* pNext, QSTATUS* pstatus) :
	pAtom_(pAtom),
	nTo_(nTo),
	pNext_(pNext)
{
}

qs::RegexNfaState::~RegexNfaState()
{
	delete pNext_;
}

bool qs::RegexNfaState::match(WCHAR c) const
{
	return pAtom_ ? pAtom_->match(c) : false;
}

bool qs::RegexNfaState::isEpsilon() const
{
	return !pAtom_;
}

unsigned int qs::RegexNfaState::getTo() const
{
	return nTo_;
}

const RegexNfaState* qs::RegexNfaState::getNext() const
{
	return pNext_;
}


/****************************************************************************
 *
 * RegexNfaCompiler
 *
 */

qs::RegexNfaCompiler::RegexNfaCompiler(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::RegexNfaCompiler::~RegexNfaCompiler()
{
}

QSTATUS qs::RegexNfaCompiler::compile(
	RegexNode* pNode, RegexNfa** ppNfa) const
{
	assert(pNode);
	assert(ppNfa);
	
	DECLARE_QSTATUS();
	
	*ppNfa = 0;
	
	std::auto_ptr<RegexNfa> pNfa;
	status = newQsObject(pNode, &pNfa);
	CHECK_QSTATUS();
	
	status = compileNode(pNode, pNfa.get(), 0, 1);
	CHECK_QSTATUS();
	
	*ppNfa = pNfa.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexNfaCompiler::compileNode(const RegexNode* pNode,
	RegexNfa* pNfa, unsigned int nFrom, unsigned int nTo) const
{
	assert(pNode);
	assert(pNfa);
	
	DECLARE_QSTATUS();
	
	switch (pNode->getType()) {
	case RegexNode::TYPE_REGEX:
		status = compileRegexNode(static_cast<const RegexRegexNode*>(pNode),
			pNfa, nFrom, nTo);
		CHECK_QSTATUS();
		break;
	case RegexNode::TYPE_BRUNCH:
		status = compileBrunchNode(static_cast<const RegexBrunchNode*>(pNode),
			pNfa, nFrom, nTo);
		CHECK_QSTATUS();
		break;
	case RegexNode::TYPE_PIECE:
		status = compilePieceNode(static_cast<const RegexPieceNode*>(pNode),
			pNfa, nFrom, nTo);
		CHECK_QSTATUS();
		break;
	case RegexNode::TYPE_EMPTY:
		status = pNfa->setTransition(nFrom, nTo, 0);
		CHECK_QSTATUS();
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexNfaCompiler::compileRegexNode(
	const RegexRegexNode* pRegexNode, RegexNfa* pNfa,
	unsigned int nFrom, unsigned int nTo) const
{
	assert(pRegexNode);
	assert(pNfa);
	
	DECLARE_QSTATUS();
	
	const RegexRegexNode::NodeList& l = pRegexNode->getNodeList();
	RegexRegexNode::NodeList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = compileNode(*it, pNfa, nFrom, nTo);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexNfaCompiler::compileBrunchNode(
	const RegexBrunchNode* pBrunchNode, RegexNfa* pNfa,
	unsigned int nFrom, unsigned int nTo) const
{
	assert(pBrunchNode);
	assert(pNfa);
	
	DECLARE_QSTATUS();
	
	unsigned int nNodeFrom = nFrom;
	const RegexBrunchNode::NodeList& l = pBrunchNode->getNodeList();
	RegexBrunchNode::NodeList::const_iterator it = l.begin();
	while (it != l.end()) {
		unsigned int nNodeTo = nTo;
		if (it + 1 != l.end()) {
			status = pNfa->createState(&nNodeTo);
			CHECK_QSTATUS();
		}
		status = compileNode(*it, pNfa, nNodeFrom, nNodeTo);
		CHECK_QSTATUS();
		nNodeFrom = nNodeTo;
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexNfaCompiler::compilePieceNode(
	const RegexPieceNode* pPieceNode, RegexNfa* pNfa,
	unsigned int nFrom, unsigned int nTo) const
{
	assert(pPieceNode);
	assert(pNfa);
	
	DECLARE_QSTATUS();
	
	const RegexAtom* pAtom = pPieceNode->getAtom();
	const RegexQuantifier* pQuantifier = pPieceNode->getQuantifier();
	if (pQuantifier) {
		switch (pQuantifier->getType()) {
		case RegexQuantifier::TYPE_RANGE:
			{
				unsigned int n = pQuantifier->getMin();
				while (n <= pQuantifier->getMax()) {
					status = compileAtom(pAtom, n, pNfa, nFrom, nTo);
					CHECK_QSTATUS();
					++n;
				}
			}
			break;
		case RegexQuantifier::TYPE_MIN:
			{
				unsigned int nEpsilonFrom = 0;
				if (pQuantifier->getMin() != 0) {
					status = pNfa->createState(&nEpsilonFrom);
					CHECK_QSTATUS();
					status = compileAtom(pAtom, pQuantifier->getMin(),
						pNfa, nFrom, nEpsilonFrom);
					CHECK_QSTATUS();
				}
				else {
					nEpsilonFrom = nFrom;
				}
				
				status = pNfa->setTransition(nEpsilonFrom, nTo, 0);
				CHECK_QSTATUS();
				unsigned int n = 0;
				status = pNfa->createState(&n);
				CHECK_QSTATUS();
				status = compileAtom(pAtom, 1, pNfa, nEpsilonFrom, n);
				CHECK_QSTATUS();
				status = pNfa->setTransition(n, nEpsilonFrom, 0);
				CHECK_QSTATUS();
			}
			break;
		default:
			assert(false);
			return QSTATUS_FAIL;
		}
	}
	else {
		status = compileAtom(pAtom, 1, pNfa, nFrom, nTo);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexNfaCompiler::compileAtom(const RegexAtom* pAtom,
	unsigned int nCount, RegexNfa* pNfa,
	unsigned int nFrom, unsigned int nTo) const
{
	assert(pAtom);
	assert(pNfa);
	
	DECLARE_QSTATUS();
	
	const RegexNode* pNode = pAtom->getNode();
	unsigned int nAtomFrom = nFrom;
	for (unsigned int n = 0; n < nCount; ++n) {
		unsigned int nAtomTo = nTo;
		if (n != nCount - 1) {
			status = pNfa->createState(&nAtomTo);
			CHECK_QSTATUS();
		}
		if (pNode) {
			status = compileNode(pNode, pNfa, nAtomFrom, nAtomTo);
			CHECK_QSTATUS();
		}
		else {
			status = pNfa->setTransition(nAtomFrom, nAtomTo, pAtom);
			CHECK_QSTATUS();
		}
		nAtomFrom = nAtomTo;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * RegexNfaMatcher
 *
 */

qs::RegexNfaMatcher::RegexNfaMatcher(const RegexNfa* pNfa) :
	pNfa_(pNfa)
{
}

qs::RegexNfaMatcher::~RegexNfaMatcher()
{
}

QSTATUS qs::RegexNfaMatcher::match(const WCHAR* pwsz, bool* pbMatch)
{
	assert(pwsz);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	*pbMatch = false;
	
	status = STLWrapper<States>(states_).resize(pNfa_->getStateCount());
	CHECK_QSTATUS();
	std::fill(states_.begin(), states_.end(), 0);
	states_.front() = 1;
	
	epsilonTransition();
	
	while (*pwsz) {
		status = makeTransition(*pwsz);
		CHECK_QSTATUS();
		epsilonTransition();
		++pwsz;
	}

	*pbMatch = states_[1] != 0;
	
	return QSTATUS_SUCCESS;
}

void qs::RegexNfaMatcher::epsilonTransition()
{
	for (States::size_type n = 0; n < states_.size(); ++n)
		epsilonTransition(n);
}

void qs::RegexNfaMatcher::epsilonTransition(unsigned int n)
{
	if (states_[n]) {
		const RegexNfaState* pState = pNfa_->getState(n);
		while (pState) {
			if (pState->isEpsilon()) {
				unsigned int nTo = pState->getTo();
				if (!states_[nTo]) {
					states_[nTo] = 1;
					epsilonTransition(nTo);
				}
			}
			pState = pState->getNext();
		}
	}
}

QSTATUS qs::RegexNfaMatcher::makeTransition(WCHAR c)
{
	assert(c != L'\0');
	
	DECLARE_QSTATUS();
	
	States states;
	status = STLWrapper<States>(states).resize(states_.size());
	CHECK_QSTATUS();
	
	for (States::size_type n = 0; n < states_.size(); ++n) {
		if (states_[n]) {
			const RegexNfaState* pState = pNfa_->getState(n);
			while (pState) {
				if (pState->match(c))
					states[pState->getTo()] = 1;
				pState = pState->getNext();
			}
		}
	}
	std::copy(states.begin(), states.end(), states_.begin());
	
	return QSTATUS_SUCCESS;
}
