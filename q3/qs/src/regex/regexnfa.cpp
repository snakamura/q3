/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qsstl.h>

#include <algorithm>
#include <functional>

#include "regexnfa.h"
#include "regexparser.h"

using namespace qs;


/****************************************************************************
 *
 * RegexNfa
 *
 */

qs::RegexNfa::RegexNfa(std::auto_ptr<RegexRegexNode> pNode)
{
	listState_.resize(2);
	pNode_ = pNode;
}

qs::RegexNfa::~RegexNfa()
{
	std::for_each(listState_.begin(), listState_.end(),
		unary_compose_f_gx(
			deleter<RegexNfaState>(),
			std::select1st<StateList::value_type>()));
}

unsigned int qs::RegexNfa::getStateCount() const
{
	return listState_.size();
}

const RegexNfaState* qs::RegexNfa::getState(unsigned int n) const
{
	assert(n < listState_.size());
	return listState_[n].first;
}

unsigned int qs::RegexNfa::createState()
{
	unsigned int n = listState_.size();
	listState_.push_back(StateList::value_type(0, 0));
	return n;
}

void qs::RegexNfa::setTransition(unsigned int nFrom,
								 unsigned int nTo,
								 const RegexAtom* pAtom)
{
	assert(listState_.size() > nFrom);
	assert(listState_.size() > nTo);
	
	StateList::value_type& v = listState_[nFrom];
	
	std::auto_ptr<RegexNfaState> pState(new RegexNfaState(
		pAtom, nTo, stackGroup_, v.second));
	v.second = pState.release();
	if (!v.first)
		v.first = v.second;
}

void qs::RegexNfa::pushGroup(unsigned int nGroup)
{
	stackGroup_.push_back(nGroup);
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

qs::RegexNfaState::	RegexNfaState(const RegexAtom* pAtom,
								  unsigned int nTo,
								  const GroupList& listGroup,
								  RegexNfaState* pPrev) :
	pAtom_(pAtom),
	nTo_(nTo)
{
	listGroup_.resize(listGroup.size());
	std::copy(listGroup.begin(), listGroup.end(), listGroup_.begin());
	
	if (pPrev) {
		assert(!pPrev->pNext_.get());
		pPrev->pNext_.reset(this);
	}
}

qs::RegexNfaState::~RegexNfaState()
{
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

RegexNfaState* qs::RegexNfaState::getNext() const
{
	return pNext_.get();
}


/****************************************************************************
 *
 * RegexNfaCompiler
 *
 */

qs::RegexNfaCompiler::RegexNfaCompiler()
{
}

qs::RegexNfaCompiler::~RegexNfaCompiler()
{
}

std::auto_ptr<RegexNfa> qs::RegexNfaCompiler::compile(std::auto_ptr<RegexRegexNode> pNode) const
{
	assert(pNode.get());
	
	RegexRegexNode* p = pNode.get();
	std::auto_ptr<RegexNfa> pNfa(new RegexNfa(pNode));
	compileRegexNode(p, pNfa.get(), 0, 1);
	return pNfa;
}

void qs::RegexNfaCompiler::compileNode(const RegexNode* pNode,
									   RegexNfa* pNfa,
									   unsigned int nFrom,
									   unsigned int nTo) const
{
	assert(pNode);
	assert(pNfa);
	
	switch (pNode->getType()) {
	case RegexNode::TYPE_REGEX:
		compileRegexNode(static_cast<const RegexRegexNode*>(pNode), pNfa, nFrom, nTo);
		break;
	case RegexNode::TYPE_BRUNCH:
		compileBrunchNode(static_cast<const RegexBrunchNode*>(pNode), pNfa, nFrom, nTo);
		break;
	case RegexNode::TYPE_PIECE:
		compilePieceNode(static_cast<const RegexPieceNode*>(pNode), pNfa, nFrom, nTo);
		break;
	case RegexNode::TYPE_EMPTY:
		pNfa->setTransition(nFrom, nTo, 0);
		break;
	default:
		assert(false);
	}
}

void qs::RegexNfaCompiler::compileRegexNode(const RegexRegexNode* pRegexNode,
											RegexNfa* pNfa,
											unsigned int nFrom,
											unsigned int nTo) const
{
	assert(pRegexNode);
	assert(pNfa);
	
	pNfa->pushGroup(pRegexNode->getGroup());
	
	const RegexRegexNode::NodeList& l = pRegexNode->getNodeList();
	for (RegexRegexNode::NodeList::const_iterator it = l.begin(); it != l.end(); ++it)
		compileNode(*it, pNfa, nFrom, nTo);
	
	pNfa->popGroup();
}

void qs::RegexNfaCompiler::compileBrunchNode(const RegexBrunchNode* pBrunchNode,
											 RegexNfa* pNfa,
											 unsigned int nFrom,
											 unsigned int nTo) const
{
	assert(pBrunchNode);
	assert(pNfa);
	
	unsigned int nNodeFrom = nFrom;
	const RegexBrunchNode::NodeList& l = pBrunchNode->getNodeList();
	for (RegexBrunchNode::NodeList::const_iterator it = l.begin(); it != l.end(); ++it) {
		unsigned int nNodeTo = nTo;
		if (it + 1 != l.end())
			nNodeTo = pNfa->createState();
		compileNode(*it, pNfa, nNodeFrom, nNodeTo);
		nNodeFrom = nNodeTo;
	}
}

void qs::RegexNfaCompiler::compilePieceNode(const RegexPieceNode* pPieceNode,
											RegexNfa* pNfa,
											unsigned int nFrom,
											unsigned int nTo) const
{
	assert(pPieceNode);
	assert(pNfa);
	
	const RegexAtom* pAtom = pPieceNode->getAtom();
	const RegexQuantifier* pQuantifier = pPieceNode->getQuantifier();
	if (pQuantifier) {
		switch (pQuantifier->getType()) {
		case RegexQuantifier::TYPE_RANGE:
			{
				unsigned int n = pQuantifier->getMax();
				unsigned int nMin = pQuantifier->getMin();
				if (nMin != 0)
					--nMin;
				while (n > nMin) {
					compileAtom(pAtom, n, pNfa, nFrom, nTo);
					--n;
				}
				if (pQuantifier->getMin() == 0)
					pNfa->setTransition(nFrom, nTo, 0);
			}
			break;
		case RegexQuantifier::TYPE_MIN:
			{
				unsigned int nEpsilonFrom = 0;
				if (pQuantifier->getMin() != 0) {
					nEpsilonFrom = pNfa->createState();
					compileAtom(pAtom, pQuantifier->getMin(),
						pNfa, nFrom, nEpsilonFrom);
				}
				else {
					nEpsilonFrom = nFrom;
				}
				
				unsigned int n = pNfa->createState();
				compileAtom(pAtom, 1, pNfa, nEpsilonFrom, n);
				pNfa->setTransition(n, nEpsilonFrom, 0);
				pNfa->setTransition(nEpsilonFrom, nTo, 0);
			}
			break;
		default:
			assert(false);
		}
	}
	else {
		compileAtom(pAtom, 1, pNfa, nFrom, nTo);
	}
}

void qs::RegexNfaCompiler::compileAtom(const RegexAtom* pAtom,
									   unsigned int nCount,
									   RegexNfa* pNfa,
									   unsigned int nFrom,
									   unsigned int nTo) const
{
	assert(pAtom);
	assert(nCount != 0);
	assert(pNfa);
	
	const RegexRegexNode* pNode = pAtom->getNode();
	unsigned int nAtomFrom = nFrom;
	for (unsigned int n = 0; n < nCount; ++n) {
		unsigned int nAtomTo = nTo;
		if (n != nCount - 1)
			nAtomTo = pNfa->createState();
		if (pNode)
			compileRegexNode(pNode, pNfa, nAtomFrom, nAtomTo);
		else
			pNfa->setTransition(nAtomFrom, nAtomTo, pAtom);
		nAtomFrom = nAtomTo;
	}
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

bool qs::RegexNfaMatcher::match(const WCHAR* pwsz,
								size_t nLen,
								RegexRangeList* pList) const
{
	assert(pwsz);
	
	const WCHAR* pEnd = 0;
	match(pwsz, pwsz + nLen, true, &pEnd, pList);
	assert(!pEnd || pEnd == pwsz + nLen);
	return pEnd != 0;
}

void qs::RegexNfaMatcher::search(const WCHAR* pwsz,
								 size_t nLen,
								 const WCHAR* p,
								 bool bReverse,
								 const WCHAR** ppStart,
								 const WCHAR** ppEnd,
								 RegexRangeList* pList) const
{
	assert(pwsz);
	assert(p);
	assert(pwsz <= p && p <= pwsz + nLen);
	assert(ppStart);
	assert(ppEnd);
	
	*ppStart = 0;
	*ppEnd = 0;
	
	const WCHAR* pStart = pwsz;
	const WCHAR* pEnd = pwsz + nLen;
	if (bReverse)
		--p;
	while (bReverse ? p >= pStart : p < pEnd) {
		match(p, pEnd, false, ppEnd, pList);
		if (*ppEnd) {
			assert(*ppEnd != p);
			*ppStart = p;
			break;
		}
		bReverse ? --p : ++p;
	}
}

void qs::RegexNfaMatcher::match(const WCHAR* pStart,
								const WCHAR* pEnd,
								bool bMatch,
								const WCHAR** ppEnd,
								RegexRangeList* pList) const
{
	assert(pStart);
	assert(pEnd);
	assert(ppEnd);
	
	*ppEnd = 0;
	
	Stack stackMatch;
	Stack stackBackTrack;
	
	const WCHAR* p = pStart;
	const RegexNfaState* pState = pNfa_->getState(0);
	while (p <= pEnd) {
		const WCHAR* pMatch = 0;
		while (pState) {
			if (p != pEnd && pState->match(*p)) {
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
			if (pState->getNext())
				stackBackTrack.push_back(std::make_pair(pState->getNext(), p));
			stackMatch.push_back(std::make_pair(pState, pMatch));
			pState = pNfa_->getState(pState->getTo());
			if (pMatch)
				++p;
			if (!pState && ((bMatch && p == pEnd) || (!bMatch && p != pStart))) {
				*ppEnd = p;
				break;
			}
		}
		else if (!stackBackTrack.empty()) {
			pState = stackBackTrack.back().first;
			p = stackBackTrack.back().second;
			stackBackTrack.pop_back();
			
			while (stackMatch.size() > 1 && stackMatch.back().second != p)
				stackMatch.pop_back();
			stackMatch.pop_back();
			
			if (pState->getNext())
				stackBackTrack.push_back(std::make_pair(pState->getNext(), p));
		}
		else {
			break;
		}
	}
	
	if (*ppEnd && pList) {
		for (Stack::const_iterator itM = stackMatch.begin(); itM != stackMatch.end(); ++itM) {
			if ((*itM).second) {
				const RegexNfaState* pState = (*itM).first;
				const RegexNfaState::GroupList& l = pState->getGroupList();
				RegexNfaState::GroupList::const_iterator itG = l.begin();
				while (itG != l.end()) {
					unsigned int nGroup = *itG;
					if (pList->list_.size() <= nGroup)
						pList->list_.resize(nGroup + 1);
					RegexRange& range = pList->list_[nGroup];
					if (!range.pStart_)
						range.pStart_ = (*itM).second;
					range.pEnd_ = (*itM).second + 1;
					++itG;
				}
			}
		}
	}
}
