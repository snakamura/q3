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

#include <algorithm>

#include "regexparser.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * RegexNode
 *
 */

qs::RegexNode::~RegexNode()
{
}


/****************************************************************************
 *
 * RegexRegexNode
 *
 */

qs::RegexRegexNode::RegexRegexNode(RegexNode* pNode, QSTATUS* pstatus)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = addNode(pNode);
	CHECK_QSTATUS_SET(pstatus);
}

qs::RegexRegexNode::~RegexRegexNode()
{
	std::for_each(listNode_.begin(),
		listNode_.end(), deleter<RegexNode>());
}

const RegexRegexNode::NodeList& qs::RegexRegexNode::getNodeList() const
{
	return listNode_;
}

QSTATUS qs::RegexRegexNode::addNode(RegexNode* pNode)
{
	return STLWrapper<NodeList>(listNode_).push_back(pNode);
}

RegexNode::Type qs::RegexRegexNode::getType() const
{
	return TYPE_REGEX;
}


/****************************************************************************
 *
 * RegexBrunchNode
 *
 */

qs::RegexBrunchNode::RegexBrunchNode(
	RegexPieceNode* pPieceNode, QSTATUS* pstatus)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = addNode(pPieceNode);
	CHECK_QSTATUS_SET(pstatus);
}

qs::RegexBrunchNode::~RegexBrunchNode()
{
	std::for_each(listNode_.begin(),
		listNode_.end(), deleter<RegexPieceNode>());
}

const RegexBrunchNode::NodeList& qs::RegexBrunchNode::getNodeList() const
{
	return listNode_;
}

QSTATUS qs::RegexBrunchNode::addNode(RegexPieceNode* pPieceNode)
{
	return STLWrapper<NodeList>(listNode_).push_back(pPieceNode);
}

RegexNode::Type qs::RegexBrunchNode::getType() const
{
	return TYPE_BRUNCH;
}


/****************************************************************************
 *
 * RegexPieceNode
 *
 */

qs::RegexPieceNode::RegexPieceNode(RegexAtom* pAtom,
	RegexQuantifier* pQuantifier, QSTATUS* pstatus) :
	pAtom_(pAtom),
	pQuantifier_(pQuantifier)
{
}

qs::RegexPieceNode::~RegexPieceNode()
{
	delete pAtom_;
	delete pQuantifier_;
}

const RegexAtom* qs::RegexPieceNode::getAtom() const
{
	return pAtom_;
}

const RegexQuantifier* qs::RegexPieceNode::getQuantifier() const
{
	return pQuantifier_;
}

RegexNode::Type qs::RegexPieceNode::getType() const
{
	return TYPE_PIECE;
}


/****************************************************************************
 *
 * RegexEmptyNode
 *
 */

qs::RegexEmptyNode::RegexEmptyNode(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::RegexEmptyNode::~RegexEmptyNode()
{
}

RegexNode::Type qs::RegexEmptyNode::getType() const
{
	return TYPE_EMPTY;
}


/****************************************************************************
 *
 * RegexAtom
 *
 */

qs::RegexAtom::~RegexAtom()
{
}

const RegexNode* qs::RegexAtom::getNode() const
{
	return 0;
}


/****************************************************************************
 *
 * RegexCharAtom
 *
 */

qs::RegexCharAtom::RegexCharAtom(WCHAR c, QSTATUS* pstatus) :
	c_(c)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::RegexCharAtom::~RegexCharAtom()
{
}

bool qs::RegexCharAtom::match(WCHAR c) const
{
	return c == c_;
}


/****************************************************************************
 *
 * RegexMultiEscapeAtom
 *
 */

qs::RegexMultiEscapeAtom::RegexMultiEscapeAtom(
	Type type, bool bNegative, QSTATUS* pstatus) :
	type_(type),
	bNegative_(bNegative)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::RegexMultiEscapeAtom::~RegexMultiEscapeAtom()
{
}

bool qs::RegexMultiEscapeAtom::match(WCHAR c) const
{
	switch (type_) {
	case TYPE_DOT:
		return true;
	case TYPE_WHITESPACE:
		return bNegative_ ? c != L' ' && c != L'\t' && c != L'\r' && c != L'\n' :
			c == L' ' || c == L'\t' || c == L'\r' || c == L'\n';
	default:
		assert(false);
		return false;
	}
}


/****************************************************************************
 *
 * RegexCharGroupAtom
 *
 */

qs::RegexCharGroupAtom::CharGroup::~CharGroup()
{
}

qs::RegexCharGroupAtom::RangeCharGroup::RangeCharGroup(
	WCHAR cStart, WCHAR cEnd) :
	cStart_(cStart),
	cEnd_(cEnd)
{
}

qs::RegexCharGroupAtom::RangeCharGroup::~RangeCharGroup()
{
}

bool qs::RegexCharGroupAtom::RangeCharGroup::match(WCHAR c) const
{
	return cStart_ <= c && c <= cEnd_;
}

qs::RegexCharGroupAtom::AtomCharGroup::AtomCharGroup(RegexAtom* pAtom) :
	pAtom_(pAtom)
{
}

qs::RegexCharGroupAtom::AtomCharGroup::~AtomCharGroup()
{
	delete pAtom_;
}

bool qs::RegexCharGroupAtom::AtomCharGroup::match(WCHAR c) const
{
	return pAtom_->match(c);
}

qs::RegexCharGroupAtom::RegexCharGroupAtom(QSTATUS* pstatus) :
	bNegative_(false),
	pSubAtom_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::RegexCharGroupAtom::~RegexCharGroupAtom()
{
	std::for_each(listCharGroup_.begin(),
		listCharGroup_.end(), deleter<CharGroup>());
	delete pSubAtom_;
}

bool qs::RegexCharGroupAtom::match(WCHAR c) const
{
	bool bMatch = false;
	CharGroupList::const_iterator it = listCharGroup_.begin();
	while (it != listCharGroup_.end() && !bMatch) {
		bMatch = (*it)->match(c);
		++it;
	}
	if ((!bMatch && !bNegative_) || (bMatch && bNegative_))
		return false;
	else if (pSubAtom_)
		return !pSubAtom_->match(c);
	else
		return bNegative_ ? !bMatch : bMatch;
}

void qs::RegexCharGroupAtom::setNegative(bool bNegative)
{
	bNegative_ = bNegative;
}

QSTATUS qs::RegexCharGroupAtom::addRangeCharGroup(WCHAR cStart, WCHAR cEnd)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<RangeCharGroup> pCharGroup;
	status = newObject(cStart, cEnd, &pCharGroup);
	CHECK_QSTATUS();
	status = STLWrapper<CharGroupList>(
		listCharGroup_).push_back(pCharGroup.get());
	CHECK_QSTATUS();
	pCharGroup.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexCharGroupAtom::addAtomCharGroup(RegexAtom* pAtom)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<AtomCharGroup> pCharGroup;
	status = newObject(pAtom, &pCharGroup);
	CHECK_QSTATUS();
	status = STLWrapper<CharGroupList>(
		listCharGroup_).push_back(pCharGroup.get());
	CHECK_QSTATUS();
	pCharGroup.release();
	
	return QSTATUS_SUCCESS;
}

void qs::RegexCharGroupAtom::setSubAtom(RegexCharGroupAtom* pSubAtom)
{
	pSubAtom_ = pSubAtom;
}


/****************************************************************************
 *
 * RegexNodeAtom
 *
 */

qs::RegexNodeAtom::RegexNodeAtom(RegexNode* pNode, QSTATUS* pstatus) :
	pNode_(pNode)
{
}

qs::RegexNodeAtom::~RegexNodeAtom()
{
	delete pNode_;
}

const RegexNode* qs::RegexNodeAtom::getNode() const
{
	return pNode_;
}

bool qs::RegexNodeAtom::match(WCHAR c) const
{
	assert(false);
	return false;
}


/****************************************************************************
 *
 * RegexQuantifier
 *
 */

qs::RegexQuantifier::RegexQuantifier(Type type,
	unsigned int nMin, unsigned int nMax, QSTATUS* pstatus) :
	type_(type),
	nMin_(nMin),
	nMax_(nMax)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::RegexQuantifier::~RegexQuantifier()
{
}

RegexQuantifier::Type qs::RegexQuantifier::getType() const
{
	return type_;
}

unsigned int qs::RegexQuantifier::getMin() const
{
	return nMin_;
}

unsigned int qs::RegexQuantifier::getMax() const
{
	return nMax_;
}


/****************************************************************************
 *
 * RegexParser
 *
 */

const WCHAR qs::RegexParser::wszSingleEscapeChar__[] =
	L"nrt\\|.-^?*+{}()[]";

qs::RegexParser::RegexParser(const WCHAR* pwszPattern, QSTATUS* pstatus) :
	pwszPattern_(pwszPattern),
	p_(pwszPattern)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::RegexParser::~RegexParser()
{
}

QSTATUS qs::RegexParser::parse(RegexRegexNode** ppNode)
{
	assert(ppNode);
	
	DECLARE_QSTATUS();
	
	p_ = pwszPattern_;
	
	std::auto_ptr<RegexRegexNode> pNode;
	status = parseRegex(&pNode);
	CHECK_QSTATUS();
	*ppNode = pNode.release();
	
	return *p_ == L'\0' ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::RegexParser::parseRegex(std::auto_ptr<RegexRegexNode>* ppNode)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<RegexRegexNode> pRegexNode;
	while (true) {
		std::auto_ptr<RegexNode> pNode;
		status = parseBranch(&pNode);
		CHECK_QSTATUS();
		if (!pRegexNode.get()) {
			status = newQsObject(pNode.get(), &pRegexNode);
			CHECK_QSTATUS();
		}
		else {
			status = pRegexNode->addNode(pNode.get());
			CHECK_QSTATUS();
		}
		pNode.release();
		
		if (*p_ != L'|' || *p_ == L')')
			break;
		++p_;
	}
	ppNode->reset(pRegexNode.release());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexParser::parseBranch(std::auto_ptr<RegexNode>* ppNode)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<RegexBrunchNode> pBrunchNode;
	std::auto_ptr<RegexPieceNode> pPieceNode;
	while (*p_ != L'|' && *p_ != L')' && *p_ != L'\0') {
		if (pPieceNode.get()) {
			status = newQsObject(pPieceNode.get(), &pBrunchNode);
			CHECK_QSTATUS();
			pPieceNode.release();
		}
		status = parsePiece(&pPieceNode);
		CHECK_QSTATUS();
		if (pBrunchNode.get()) {
			status = pBrunchNode->addNode(pPieceNode.get());
			CHECK_QSTATUS();
			pPieceNode.release();
		}
	}
	
	if (pBrunchNode.get()) {
		ppNode->reset(pBrunchNode.release());
	}
	else if (pPieceNode.get()) {
		ppNode->reset(pPieceNode.release());
	}
	else {
		RegexEmptyNode* pEmptyNode;
		status = newQsObject(&pEmptyNode);
		CHECK_QSTATUS();
		ppNode->reset(pEmptyNode);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexParser::parsePiece(std::auto_ptr<RegexPieceNode>* ppPieceNode)
{
	assert(ppPieceNode);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<RegexAtom> pAtom;
	if (*p_ == L'(') {
		++p_;
		std::auto_ptr<RegexRegexNode> pNode;
		status = parseRegex(&pNode);
		CHECK_QSTATUS();
		if (*p_ != L')')
			return QSTATUS_FAIL;
		RegexNodeAtom* pNodeAtom = 0;
		status = newQsObject(pNode.get(), &pNodeAtom);
		CHECK_QSTATUS();
		pNode.release();
		pAtom.reset(pNodeAtom);
		++p_;
	}
	else if (*p_ == L'[') {
		std::auto_ptr<RegexCharGroupAtom> pCharGroupAtom;
		status = parseCharGroup(&pCharGroupAtom);
		CHECK_QSTATUS();
		pAtom.reset(pCharGroupAtom.release());
	}
	else if (*p_ == L'.') {
		RegexMultiEscapeAtom* pMultiEscapeAtom = 0;
		status = newQsObject(RegexMultiEscapeAtom::TYPE_DOT,
			false, &pMultiEscapeAtom);
		CHECK_QSTATUS();
		pAtom.reset(pMultiEscapeAtom);
		++p_;
	}
	else if (*p_ == L'\\') {
		++p_;
		if (wcschr(wszSingleEscapeChar__, *p_)) {
			RegexCharAtom* pCharAtom = 0;
			status = newQsObject(*p_, &pCharAtom);
			CHECK_QSTATUS();
			pAtom.reset(pCharAtom);
		}
		else if (*p_ == L's' || *p_ == L'S') {
			RegexMultiEscapeAtom* pMultiEscapeAtom = 0;
			status = newQsObject(RegexMultiEscapeAtom::TYPE_WHITESPACE,
				*p_ == L'S', &pMultiEscapeAtom);
			CHECK_QSTATUS();
			pAtom.reset(pMultiEscapeAtom);
		}
		else {
			return QSTATUS_FAIL;
		}
		++p_;
	}
	else if (*p_ == L'?' || *p_ == L'*' || *p_ == L'+' ||
		*p_ == L']' || *p_ == L'{' || *p_ == L'}') {
		return QSTATUS_FAIL;
	}
	else {
		RegexCharAtom* pCharAtom = 0;
		status = newQsObject(*p_, &pCharAtom);
		CHECK_QSTATUS();
		pAtom.reset(pCharAtom);
		++p_;
	}
	
	std::auto_ptr<RegexQuantifier> pQuantifier;
	if (*p_ == L'?') {
		status = newQsObject(RegexQuantifier::TYPE_RANGE, 0, 1, &pQuantifier);
		CHECK_QSTATUS();
		++p_;
	}
	else if (*p_ == L'*') {
		status = newQsObject(RegexQuantifier::TYPE_MIN, 0, 0, &pQuantifier);
		CHECK_QSTATUS();
		++p_;
	}
	else if (*p_ == L'+') {
		status = newQsObject(RegexQuantifier::TYPE_MIN, 1, 0, &pQuantifier);
		CHECK_QSTATUS();
		++p_;
	}
	else if (*p_ == L'{') {
		status = parseQuantity(&pQuantifier);
		CHECK_QSTATUS();
	}
	
	status = newQsObject(pAtom.get(), pQuantifier.get(), ppPieceNode);
	CHECK_QSTATUS();
	pAtom.release();
	pQuantifier.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexParser::parseCharGroup(
	std::auto_ptr<RegexCharGroupAtom>* ppCharGroupAtom)
{
	assert(ppCharGroupAtom);
	assert(*p_ == L'[');
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<RegexCharGroupAtom> pCharGroupAtom;
	status = newQsObject(&pCharGroupAtom);
	CHECK_QSTATUS();
	
	++p_;
	bool bNegative = *p_ == L'^';
	if (bNegative)
		++p_;
	pCharGroupAtom->setNegative(bNegative);
	
	if (*p_ == L'-') {
		status = pCharGroupAtom->addRangeCharGroup(L'-', L'-');
		CHECK_QSTATUS();
		++p_;
	}
	else if (*p_ == L'[' || *p_ == L']') {
		return QSTATUS_FAIL;
	}
	
	while (*p_ != L']') {
		WCHAR cStart = L'\0';
		bool bDash = false;
		if (*p_ == L'[') {
			return QSTATUS_FAIL;
		}
		else if (*p_ == L'\\') {
			++p_;
			if (wcschr(wszSingleEscapeChar__, *p_)) {
				cStart = *p_;
			}
			else if (*p_ == L's' || *p_ == L'S') {
				std::auto_ptr<RegexMultiEscapeAtom> pAtom;
				status = newQsObject(RegexMultiEscapeAtom::TYPE_WHITESPACE,
					*p_ == L'S', &pAtom);
				CHECK_QSTATUS();
				pCharGroupAtom->addAtomCharGroup(pAtom.get());
				CHECK_QSTATUS();
				pAtom.release();
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		else if (*p_ == L'-') {
			bDash = true;
		}
		else {
			cStart = *p_;
		}
		if (!bDash)
			++p_;
		if (cStart != L'\0' || bDash) {
			if (*p_ == L'-') {
				++p_;
				if (*p_ == L']') {
					if (cStart != L'\0') {
						status = pCharGroupAtom->addRangeCharGroup(cStart, cStart);
						CHECK_QSTATUS();
					}
					status = pCharGroupAtom->addRangeCharGroup(L'-', L'-');
					CHECK_QSTATUS();
				}
				else if (*p_ == L'[') {
					if (cStart != L'\0') {
						status = pCharGroupAtom->addRangeCharGroup(cStart, cStart);
						CHECK_QSTATUS();
					}
					std::auto_ptr<RegexCharGroupAtom> pSubAtom;
					status = parseCharGroup(&pSubAtom);
					CHECK_QSTATUS();
					pCharGroupAtom->setSubAtom(pSubAtom.release());
					if (*p_ != L']')
						return QSTATUS_FAIL;
				}
				else if (*p_ == L'\\' && !bDash) {
					++p_;
					if (wcschr(wszSingleEscapeChar__, *p_)) {
						status = pCharGroupAtom->addRangeCharGroup(cStart, *p_);
						CHECK_QSTATUS();
					}
					else {
						return QSTATUS_FAIL;
					}
					++p_;
				}
				else if (!bDash) {
					status = pCharGroupAtom->addRangeCharGroup(cStart, *p_);
					CHECK_QSTATUS();
					++p_;
				}
				else {
					return QSTATUS_FAIL;
				}
			}
			else {
				status = pCharGroupAtom->addRangeCharGroup(cStart, cStart);
				CHECK_QSTATUS();
			}
		}
	}
	++p_;
	
	*ppCharGroupAtom = pCharGroupAtom;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexParser::parseQuantity(
	std::auto_ptr<RegexQuantifier>* ppQuantifier)
{
	assert(ppQuantifier);
	assert(*p_ == L'{');
	
	DECLARE_QSTATUS();
	
	++p_;
	if (*p_ < L'0' || L'9' < *p_)
		return QSTATUS_FAIL;
	
	unsigned int nMin = 0;
	while (L'0' <= *p_ && *p_ <= L'9') {
		nMin = nMin*10 + (*p_ - L'0');
		++p_;
	}
	if (*p_ == L'}') {
		status = newQsObject(RegexQuantifier::TYPE_RANGE, nMin, nMin, ppQuantifier);
		CHECK_QSTATUS();
		++p_;
	}
	else if (*p_ == L',') {
		++p_;
		if (*p_ == L'}') {
			status = newQsObject(RegexQuantifier::TYPE_MIN, nMin, 0, ppQuantifier);
			CHECK_QSTATUS();
			++p_;
		}
		else if (L'0' <= *p_ && *p_ <= L'9') {
			unsigned int nMax = 0;
			while (L'0' <= *p_ && *p_ <= L'9') {
				nMax = nMax*10 + (*p_ - L'0');
				++p_;
			}
			if (nMin > nMax)
				return QSTATUS_FAIL;
			if (*p_ == L'}') {
				status = newQsObject(RegexQuantifier::TYPE_RANGE, nMin, nMax, ppQuantifier);
				CHECK_QSTATUS();
				++p_;
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		else {
			return QSTATUS_FAIL;
		}
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}
