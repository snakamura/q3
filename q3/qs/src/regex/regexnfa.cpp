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

qs::RegexNfa::RegexNfa(RegexRegexNode* pNode, QSTATUS* pstatus) :
	pNode_(0),
	nMaxGroup_(0)
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
	status = newQsObject(pAtom, nTo, stackGroup_, listState_[nFrom], &pState);
	CHECK_QSTATUS();
	listState_[nFrom] = pState.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexNfa::pushGroup()
{
	return STLWrapper<GroupStack>(stackGroup_).push_back(nMaxGroup_++);
}

void qs::RegexNfa::popGroup()
{
	stackGroup_.pop_back();
}


/****************************************************************************
 *
 * RegexNfaState
 *
 */

qs::RegexNfaState::	RegexNfaState(const RegexAtom* pAtom, unsigned int nTo,
	const GroupList& listGroup, RegexNfaState* pNext, QSTATUS* pstatus) :
	pAtom_(pAtom),
	nTo_(nTo),
	pNext_(pNext)
{
	DECLARE_QSTATUS();
	
	status = STLWrapper<GroupList>(listGroup_).resize(listGroup.size());
	CHECK_QSTATUS_SET(pstatus);
	std::copy(listGroup.begin(), listGroup.end(), listGroup_.begin());
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

const RegexNfaState::GroupList& qs::RegexNfaState::getGroupList() const
{
	return listGroup_;
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
	RegexRegexNode* pNode, RegexNfa** ppNfa) const
{
	assert(pNode);
	assert(ppNfa);
	
	DECLARE_QSTATUS();
	
	*ppNfa = 0;
	
	std::auto_ptr<RegexNfa> pNfa;
	status = newQsObject(pNode, &pNfa);
	CHECK_QSTATUS();
	
	status = compileRegexNode(pNode, pNfa.get(), 0, 1);
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
	
	status = pNfa->pushGroup();
	CHECK_QSTATUS();
	
	const RegexRegexNode::NodeList& l = pRegexNode->getNodeList();
	RegexRegexNode::NodeList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = compileNode(*it, pNfa, nFrom, nTo);
		CHECK_QSTATUS();
		++it;
	}
	
	pNfa->popGroup();
	
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
				if (n == 0) {
					status = pNfa->setTransition(nFrom, nTo, 0);
					CHECK_QSTATUS();
					n = 1;
				}
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
	assert(nCount != 0);
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

QSTATUS qs::RegexNfaMatcher::match(const WCHAR* pwsz,
	size_t nLen, bool* pbMatch, RegexPattern::RangeList* pList)
{
	assert(pwsz);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	*pbMatch = false;
	
	const WCHAR* p = pwsz;
	const WCHAR* pEnd = pwsz + nLen;
	const RegexNfaState* pState = pNfa_->getState(0);
	while (p <= pEnd) {
		const WCHAR* pMatch = 0;
		while (pState) {
			if (*p && pState->match(*p)) {
				pMatch = p;
				break;
			}
			else if (pState->isEpsilon()) {
				break;
			}
			else {
				pState = pState->getNext();
			}
		}
		if (pState) {
			if (pState->getNext()) {
				status = STLWrapper<Stack>(stackBackTrack_).push_back(
					std::make_pair(pState->getNext(), p));
				CHECK_QSTATUS();
			}
			status = STLWrapper<Stack>(stackMatch_).push_back(
				std::make_pair(pState, pMatch));
			CHECK_QSTATUS();
			pState = pNfa_->getState(pState->getTo());
			if (pMatch)
				++p;
			if (p == pEnd && !pState) {
				*pbMatch = true;
				break;
			}
		}
		else if (!stackBackTrack_.empty()) {
			pState = stackBackTrack_.back().first;
			p = stackBackTrack_.back().second;
			stackBackTrack_.pop_back();
			while (stackMatch_.back().second != p)
				stackMatch_.pop_back();
			stackMatch_.pop_back();
			if (pState->getNext()) {
				status = STLWrapper<Stack>(stackBackTrack_).push_back(
					std::make_pair(pState->getNext(), p));
				CHECK_QSTATUS();
			}
		}
		else {
			break;
		}
	}
	
	if (*pbMatch && pList) {
		Stack::const_iterator itM = stackMatch_.begin();
		while (itM != stackMatch_.end()) {
			if ((*itM).second) {
				const RegexNfaState* pState = (*itM).first;
				const RegexNfaState::GroupList& l = pState->getGroupList();
				RegexNfaState::GroupList::const_iterator itG = l.begin();
				while (itG != l.end()) {
					unsigned int nGroup = *itG;
					if (pList->size() <= nGroup) {
						status = STLWrapper<RegexPattern::RangeList>(
							*pList).resize(nGroup + 1);
						CHECK_QSTATUS();
					}
					RegexRange& range = (*pList)[nGroup];
					if (!range.pStart_)
						range.pStart_ = (*itM).second;
					range.pEnd_ = (*itM).second + 1;
					++itG;
				}
			}
			++itM;
		}
	}
	
	return QSTATUS_SUCCESS;
}
