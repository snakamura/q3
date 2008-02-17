/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qsassert.h>
#include <qsstl.h>

#include <algorithm>
#include <functional>

#include <boost/bind.hpp>

#include "regexnfa.h"
#include "regexparser.h"

using namespace qs;


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
		pNfa->setTransition(nFrom, nTo, 0, true);
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
	
	unsigned int nGroup = pRegexNode->getGroup();
	if (nGroup != -1)
		pNfa->pushGroup(nGroup);
	
	const RegexRegexNode::NodeList& l = pRegexNode->getNodeList();
	for (RegexRegexNode::NodeList::const_iterator it = l.begin(); it != l.end(); ++it)
		compileNode(*it, pNfa, nFrom, nTo);
	
	if (nGroup != -1)
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
	RegexQuantifier* pQuantifier = pPieceNode->getQuantifier();
	RegexRegexNode* pNode = pAtom->getNode();
	RegexRegexNode::GroupType groupType = RegexRegexNode::GROUPTYPE_NORMAL;
	if (pNode) {
		groupType = pNode->getGroupType();
		if (groupType == RegexRegexNode::GROUPTYPE_NORMAL) {
			if (pQuantifier && pQuantifier->getOption() == RegexQuantifier::OPTION_POSSESSIVE) {
				groupType = RegexRegexNode::GROUPTYPE_INDEPENDENT;
				pQuantifier->resetOption();
			}
		}
	}
	if (!pNode || groupType == RegexRegexNode::GROUPTYPE_NORMAL) {
		if (pQuantifier) {
			switch (pQuantifier->getType()) {
			case RegexQuantifier::TYPE_RANGE:
				{
					unsigned int n = pQuantifier->getMax();
					unsigned int nMin = pQuantifier->getMin();
					if (nMin != 0)
						--nMin;
					while (n > nMin) {
						compileAtom(pAtom, n, pNfa, nFrom, nTo, true);
						--n;
					}
					if (pQuantifier->getMin() == 0)
						pNfa->setTransition(nFrom, nTo, 0, true);
				}
				break;
			case RegexQuantifier::TYPE_MIN:
				{
					unsigned int nEpsilonFrom = 0;
					if (pQuantifier->getMin() != 0) {
						nEpsilonFrom = pNfa->createState();
						compileAtom(pAtom, pQuantifier->getMin(),
							pNfa, nFrom, nEpsilonFrom, true);
					}
					else {
						nEpsilonFrom = nFrom;
					}
					
					unsigned int n = pNfa->createState();
					switch (pQuantifier->getOption()) {
					case RegexQuantifier::OPTION_GREEDY:
						compileAtom(pAtom, 1, pNfa, nEpsilonFrom, n, true);
						pNfa->setTransition(n, nEpsilonFrom, 0, true);
						pNfa->setTransition(nEpsilonFrom, nTo, 0, true);
						break;
					case RegexQuantifier::OPTION_RELUCTANT:
						pNfa->setTransition(n, nEpsilonFrom, 0, true);
						pNfa->setTransition(nEpsilonFrom, nTo, 0, true);
						compileAtom(pAtom, 1, pNfa, nEpsilonFrom, n, true);
						break;
					case RegexQuantifier::OPTION_POSSESSIVE:
						compileAtom(pAtom, 1, pNfa, nEpsilonFrom, n, false);
						pNfa->setTransition(n, nEpsilonFrom, 0, true);
						pNfa->setTransition(nEpsilonFrom, nTo, 0, true);
						break;
					default:
						assert(false);
						break;
					}
				}
				break;
			default:
				assert(false);
			}
		}
		else {
			compileAtom(pAtom, 1, pNfa, nFrom, nTo, true);
		}
	}
	else if (groupType != RegexRegexNode::GROUPTYPE_NORMAL) {
		std::auto_ptr<RegexRegexNode> pWrapNode(new RegexRegexNode(
			-1, RegexRegexNode::GROUPTYPE_NORMAL));
		pWrapNode->addNode(pPieceNode);
		pNode->resetGroupType();
		std::auto_ptr<RegexNfa> pIndependentNfa(compile(pWrapNode));
		pNfa->setTransition(nFrom, nTo, pIndependentNfa, groupType);
	}
	else {
		assert(false);
	}
}

void qs::RegexNfaCompiler::compileAtom(const RegexAtom* pAtom,
									   unsigned int nCount,
									   RegexNfa* pNfa,
									   unsigned int nFrom,
									   unsigned int nTo,
									   bool bBackTrack) const
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
		if (pNode) {
			assert(bBackTrack);
			compileRegexNode(pNode, pNfa, nAtomFrom, nAtomTo);
		}
		else {
			pNfa->setTransition(nAtomFrom, nAtomTo, pAtom, bBackTrack);
		}
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
	MatchStack stackMatch;
	match(pwsz, pwsz + nLen, pwsz, true, &pEnd, &stackMatch);
	assert(!pEnd || pEnd == pwsz + nLen);
	
	if (pEnd && pList)
		getMatch(pwsz, pwsz + nLen, stackMatch, pList);
	
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
	
	MatchStack stackMatch;
	search(pwsz, pwsz + nLen, p, bReverse, false, ppStart, ppEnd, &stackMatch);
	if (*ppStart && pList)
		getMatch(*ppStart, *ppEnd, stackMatch, pList);
}

void qs::RegexNfaMatcher::match(const WCHAR* pStart,
								const WCHAR* pEnd,
								const WCHAR* pCurrent,
								bool bMatchEnd,
								const WCHAR** ppEnd,
								MatchStack* pStackMatch) const
{
	assert(pStart);
	assert(pEnd);
	assert(pCurrent);
	assert(ppEnd);
	assert(pStackMatch);
	
	*ppEnd = 0;
	
	Stack stackBackTrack;
	
	class CallbackImpl : public RegexMatchCallback
	{
	public:
		CallbackImpl(const MatchStack& stackMatch) :
			stackMatch_(stackMatch)
		{
		}
		
		virtual std::pair<const WCHAR*, const WCHAR*> getReference(unsigned int n)
		{
			RegexRangeList l;
			RegexNfaMatcher::getMatch(stackMatch_, &l);
			assert(0 < n && n < l.list_.size());
			const RegexRange& range = l.list_[n];
			return std::make_pair(range.pStart_, range.pEnd_);
		}
	
	private:
		const MatchStack& stackMatch_;
	} callback(*pStackMatch);
	
	const WCHAR* p = pCurrent;
	const RegexNfaState* pState = pNfa_->getState(0);
	while (p <= pEnd) {
		const WCHAR* pMatch = 0;
		const WCHAR* pMatchEnd = 0;
		while (pState) {
			if ((pMatchEnd = pState->match(pStart, pEnd, p, pStackMatch, &callback)) != 0) {
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
			if (pState->getNext() && pState->canBackTrack())
				stackBackTrack.push_back(std::make_pair(pState->getNext(), p));
			Match match = {
				pState,
				pMatch ? pMatch : p,
				pMatch ? pMatchEnd : p
			};
			pStackMatch->push_back(match);
			pState = pNfa_->getState(pState->getTo());
			if (pMatch)
				p = pMatchEnd;
			if (!pState && (!bMatchEnd || p == pEnd)) {
				*ppEnd = p;
				break;
			}
		}
		else if (!stackBackTrack.empty()) {
			pState = stackBackTrack.back().first;
			p = stackBackTrack.back().second;
			stackBackTrack.pop_back();
			
			while (pStackMatch->size() > 0 && pStackMatch->back().pStart_ >= p)
				pStackMatch->pop_back();
			
			if (pState->getNext())
				stackBackTrack.push_back(std::make_pair(pState->getNext(), p));
		}
		else {
			break;
		}
	}
}

void qs::RegexNfaMatcher::search(const WCHAR* pStart,
								 const WCHAR* pEnd,
								 const WCHAR* pCurrent,
								 bool bReverse,
								 bool bMatchEnd,
								 const WCHAR** ppStart,
								 const WCHAR** ppEnd,
								 MatchStack* pStackMatch) const
{
	assert(pStart);
	assert(pEnd);
	assert(pCurrent);
	assert(ppStart);
	assert(ppEnd);
	assert(pStackMatch);
	
	*ppStart = 0;
	*ppEnd = 0;
	
	const WCHAR* p = pCurrent - (bReverse ? 1 : 0);
	while (bReverse ? p >= pStart : p <= pEnd) {
		p = pNfa_->getCandidate(pStart, pEnd, p, bReverse);
		if (!p)
			break;
		
		match(pStart, pEnd, p, bMatchEnd, ppEnd, pStackMatch);
		if (*ppEnd) {
			*ppStart = p;
			break;
		}
		bReverse ? --p : ++p;
		pStackMatch->clear();
	}
}

void qs::RegexNfaMatcher::getMatch(const WCHAR* pStart,
								   const WCHAR* pEnd,
								   const MatchStack& stackMatch,
								   RegexRangeList* pList)
{
	RegexRangeList::List& listRange = pList->list_;
	listRange.resize(1);
	listRange[0].pStart_ = pStart;
	listRange[0].pEnd_ = pEnd;
	getMatch(stackMatch, pList);
}

void qs::RegexNfaMatcher::getMatch(const MatchStack& stackMatch,
								   RegexRangeList* pList)
{
	RegexRangeList::List& listRange = pList->list_;
	for (MatchStack::const_iterator itM = stackMatch.begin(); itM != stackMatch.end(); ++itM) {
		const RegexNfaState* pState = (*itM).pState_;
		const RegexNfaState::GroupList& l = pState->getGroupList();
		for (RegexNfaState::GroupList::const_iterator itG = l.begin(); itG != l.end(); ++itG) {
			unsigned int nGroup = *itG;
			if (listRange.size() <= nGroup)
				listRange.resize(nGroup + 1);
			RegexRange& range = listRange[nGroup];
			if (!range.pStart_)
				range.pStart_ = (*itM).pStart_;
			range.pEnd_ = (*itM).pEnd_;
		}
	}
}


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
		boost::bind(boost::checked_deleter<RegexNfaState>(),
			boost::bind(&StateList::value_type::first, _1)));
}

unsigned int qs::RegexNfa::getStateCount() const
{
	return static_cast<unsigned int>(listState_.size());
}

const RegexNfaState* qs::RegexNfa::getState(unsigned int n) const
{
	assert(n < listState_.size());
	return listState_[n].first;
}

unsigned int qs::RegexNfa::createState()
{
	unsigned int n = static_cast<unsigned int>(listState_.size());
	listState_.push_back(StateList::value_type(0, 0));
	return n;
}

void qs::RegexNfa::setTransition(unsigned int nFrom,
								 unsigned int nTo,
								 const RegexAtom* pAtom,
								 bool bBackTrack)
{
	assert(listState_.size() > nFrom);
	assert(listState_.size() > nTo);
	
	StateList::value_type& v = listState_[nFrom];
	
	std::auto_ptr<RegexNfaAtomState> pState(new RegexNfaAtomState(
		pAtom, bBackTrack, nTo, stackGroup_, v.second));
	v.second = pState.release();
	if (!v.first)
		v.first = v.second;
}

void qs::RegexNfa::setTransition(unsigned int nFrom,
								 unsigned int nTo,
								 std::auto_ptr<RegexNfa> pNfa,
								 RegexRegexNode::GroupType groupType)
{
	assert(listState_.size() > nFrom);
	assert(listState_.size() > nTo);
	
	StateList::value_type& v = listState_[nFrom];
	
	std::auto_ptr<RegexNfaNfaState> pState(new RegexNfaNfaState(
		pNfa, groupType, nTo, stackGroup_, v.second));
	v.second = pState.release();
	if (!v.first)
		v.first = v.second;
}

void qs::RegexNfa::pushGroup(unsigned int nGroup)
{
	assert(nGroup != -1);
	assert(stackGroup_.empty() || stackGroup_.back() != nGroup);
	stackGroup_.push_back(nGroup);
}

void qs::RegexNfa::popGroup()
{
	assert(!stackGroup_.empty());
	stackGroup_.pop_back();
}

const WCHAR* qs::RegexNfa::getCandidate(const WCHAR* pStart,
										const WCHAR* pEnd,
										const WCHAR* p,
										bool bReverse) const
{
	assert(pStart);
	assert(pEnd);
	assert(p);
	assert(pStart <= p && p <= pEnd);
	
	return pNode_->getCandidate(pStart, pEnd, p, bReverse);
}


/****************************************************************************
 *
 * RegexNfaState
 *
 */

qs::RegexNfaState::	RegexNfaState(unsigned int nTo,
								  const GroupList& listGroup,
								  RegexNfaState* pPrev) :
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
 * RegexNfaAtomState
 *
 */

qs::RegexNfaAtomState::RegexNfaAtomState(const RegexAtom* pAtom,
										 bool bBackTrack,
										 unsigned int nTo,
										 const GroupList& listGroup,
										 RegexNfaState* pPrev) :
	RegexNfaState(nTo, listGroup, pPrev),
	pAtom_(pAtom),
	bBackTrack_(bBackTrack)
{
}

qs::RegexNfaAtomState::~RegexNfaAtomState()
{
}

const WCHAR* qs::RegexNfaAtomState::match(const WCHAR* pStart,
										  const WCHAR* pEnd,
										  const WCHAR* p,
										  RegexNfaMatcher::MatchStack* pStackMatch,
										  RegexMatchCallback* pCallback) const
{
	return pAtom_ ? pAtom_->match(pStart, pEnd, p, pCallback) : false;
}

bool qs::RegexNfaAtomState::isEpsilon() const
{
	return !pAtom_;
}

bool qs::RegexNfaAtomState::canBackTrack() const
{
	return bBackTrack_;
}


/****************************************************************************
 *
 * RegexNfaNfaState
 *
 */

qs::RegexNfaNfaState::RegexNfaNfaState(std::auto_ptr<RegexNfa> pNfa,
									   RegexRegexNode::GroupType groupType,
									   unsigned int nTo,
									   const GroupList& listGroup,
									   RegexNfaState* pPrev) :
	RegexNfaState(nTo, listGroup, pPrev),
	pNfa_(pNfa),
	groupType_(groupType)
{
}

qs::RegexNfaNfaState::~RegexNfaNfaState()
{
}

const WCHAR* qs::RegexNfaNfaState::match(const WCHAR* pStart,
										 const WCHAR* pEnd,
										 const WCHAR* p,
										 RegexNfaMatcher::MatchStack* pStackMatch,
										 RegexMatchCallback* pCallback) const
{
	RegexNfaMatcher matcher(pNfa_.get());
	
	switch (groupType_) {
	case RegexRegexNode::GROUPTYPE_POSITIVELOOKAHEAD:
	case RegexRegexNode::GROUPTYPE_NEGATIVELOOKAHEAD:
	case RegexRegexNode::GROUPTYPE_INDEPENDENT:
		{
			const WCHAR* pMatchEnd = 0;
			matcher.match(pStart, pEnd, p, false, &pMatchEnd, pStackMatch);
			switch (groupType_) {
			case RegexRegexNode::GROUPTYPE_POSITIVELOOKAHEAD:
				return pMatchEnd ? p : 0;
			case RegexRegexNode::GROUPTYPE_NEGATIVELOOKAHEAD:
				return pMatchEnd ? 0 : p;
			case RegexRegexNode::GROUPTYPE_INDEPENDENT:
				return pMatchEnd ? pMatchEnd : 0;
			default:
				assert(false);
				return 0;
			}
		}
		break;
	case RegexRegexNode::GROUPTYPE_POSITIVELOOKBEHIND:
	case RegexRegexNode::GROUPTYPE_NEGATIVELOOKBEHIND:
		{
			const WCHAR* pMatchStart = 0;
			const WCHAR* pMatchEnd = 0;
			matcher.search(pStart, p, p, true, true, &pMatchStart, &pMatchEnd, pStackMatch);
			switch (groupType_) {
			case RegexRegexNode::GROUPTYPE_POSITIVELOOKBEHIND:
				return pMatchStart ? p : 0;
			case RegexRegexNode::GROUPTYPE_NEGATIVELOOKBEHIND:
				return pMatchStart ? 0 : p;
			default:
				assert(false);
				return 0;
			}
		}
		break;
	default:
		assert(false);
		return 0;
	}
}

bool qs::RegexNfaNfaState::isEpsilon() const
{
	return false;
}

bool qs::RegexNfaNfaState::canBackTrack() const
{
	return true;
}
