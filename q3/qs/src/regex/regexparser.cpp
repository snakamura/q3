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

qs::RegexRegexNode::RegexRegexNode(unsigned int nGroup) :
	nGroup_(nGroup)
{
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

unsigned int qs::RegexRegexNode::getGroup() const
{
	return nGroup_;
}

void qs::RegexRegexNode::addNode(std::auto_ptr<RegexNode> pNode)
{
	assert(pNode.get());
	listNode_.push_back(pNode.get());
	pNode.release();
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

qs::RegexBrunchNode::RegexBrunchNode(std::auto_ptr<RegexPieceNode> pPieceNode)
{
	addNode(pPieceNode);
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

void qs::RegexBrunchNode::addNode(std::auto_ptr<RegexPieceNode> pPieceNode)
{
	assert(pPieceNode.get());
	listNode_.push_back(pPieceNode.get());
	pPieceNode.release();
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

qs::RegexPieceNode::RegexPieceNode(std::auto_ptr<RegexAtom> pAtom,
								   std::auto_ptr<RegexQuantifier> pQuantifier) :
	pAtom_(pAtom),
	pQuantifier_(pQuantifier)
{
}

qs::RegexPieceNode::~RegexPieceNode()
{
}

const RegexAtom* qs::RegexPieceNode::getAtom() const
{
	return pAtom_.get();
}

const RegexQuantifier* qs::RegexPieceNode::getQuantifier() const
{
	return pQuantifier_.get();
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

qs::RegexEmptyNode::RegexEmptyNode()
{
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

const RegexRegexNode* qs::RegexAtom::getNode() const
{
	return 0;
}

const WCHAR* qs::RegexAtom::match(const WCHAR* pStart,
								  const WCHAR* pEnd,
								  RegexMatchCallback* pCallback) const
{
	assert(pStart < pEnd);
	return match(*pStart) ? pStart + 1 : 0;
}

bool qs::RegexAtom::match(WCHAR c) const
{
	assert(false);
	return false;
}


/****************************************************************************
 *
 * RegexCharAtom
 *
 */

qs::RegexCharAtom::RegexCharAtom(WCHAR c) :
	c_(c)
{
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

qs::RegexMultiEscapeAtom::RegexMultiEscapeAtom(Type type,
											   bool bNegative) :
	type_(type),
	bNegative_(bNegative)
{
}

qs::RegexMultiEscapeAtom::~RegexMultiEscapeAtom()
{
}

bool qs::RegexMultiEscapeAtom::match(WCHAR c) const
{
	bool bMatch = false;
	switch (type_) {
	case TYPE_DOT:
		bMatch = true;
		break;
	case TYPE_WHITESPACE:
		bMatch = c == L' ' || c == L'\t' || c == L'\r' || c == L'\n';
		break;
	case TYPE_WORD:
		bMatch = (L'a' <= c && c <= L'z') || (L'A' <= c && c <= L'Z') || (L'0' <= c && c <= L'9') || c == L'_';
		break;
	case TYPE_NUMBER:
		bMatch = L'0' <= c && c <= L'9';
		break;
	default:
		assert(false);
		return false;
	}
	return bNegative_ ? !bMatch : bMatch;
}


/****************************************************************************
 *
 * RegexCharGroupAtom
 *
 */

qs::RegexCharGroupAtom::CharGroup::~CharGroup()
{
}

qs::RegexCharGroupAtom::RangeCharGroup::RangeCharGroup(WCHAR cStart,
													   WCHAR cEnd) :
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

qs::RegexCharGroupAtom::AtomCharGroup::AtomCharGroup(std::auto_ptr<RegexAtom> pAtom) :
	pAtom_(pAtom)
{
}

qs::RegexCharGroupAtom::AtomCharGroup::~AtomCharGroup()
{
}

bool qs::RegexCharGroupAtom::AtomCharGroup::match(WCHAR c) const
{
	return pAtom_->match(&c, &c + 1, 0) != 0;
}

qs::RegexCharGroupAtom::RegexCharGroupAtom() :
	bNegative_(false),
	pSubAtom_(0)
{
}

qs::RegexCharGroupAtom::~RegexCharGroupAtom()
{
	std::for_each(listCharGroup_.begin(),
		listCharGroup_.end(), deleter<CharGroup>());
}

bool qs::RegexCharGroupAtom::match(WCHAR c) const
{
	bool bMatch = false;
	for (CharGroupList::const_iterator it = listCharGroup_.begin(); it != listCharGroup_.end() && !bMatch; ++it)
		bMatch = (*it)->match(c);
	if ((!bMatch && !bNegative_) || (bMatch && bNegative_))
		return false;
	else if (pSubAtom_.get())
		return !pSubAtom_->match(c);
	else
		return bNegative_ ? !bMatch : bMatch;
}

void qs::RegexCharGroupAtom::setNegative(bool bNegative)
{
	bNegative_ = bNegative;
}

void qs::RegexCharGroupAtom::addRangeCharGroup(WCHAR cStart,
											   WCHAR cEnd)
{
	std::auto_ptr<RangeCharGroup> pCharGroup(new RangeCharGroup(cStart, cEnd));
	listCharGroup_.push_back(pCharGroup.get());
	pCharGroup.release();
}

void qs::RegexCharGroupAtom::addAtomCharGroup(std::auto_ptr<RegexAtom> pAtom)
{
	std::auto_ptr<AtomCharGroup> pCharGroup(new AtomCharGroup(pAtom));
	listCharGroup_.push_back(pCharGroup.get());
	pCharGroup.release();
}

void qs::RegexCharGroupAtom::setSubAtom(std::auto_ptr<RegexCharGroupAtom> pSubAtom)
{
	pSubAtom_ = pSubAtom;
}


/****************************************************************************
 *
 * RegexNodeAtom
 *
 */

qs::RegexNodeAtom::RegexNodeAtom(std::auto_ptr<RegexRegexNode> pNode) :
	pNode_(pNode)
{
}

qs::RegexNodeAtom::~RegexNodeAtom()
{
}

const RegexRegexNode* qs::RegexNodeAtom::getNode() const
{
	return pNode_.get();
}


/****************************************************************************
 *
 * RegexReferenceAtom
 *
 */

qs::RegexReferenceAtom::RegexReferenceAtom(unsigned int n) :
	n_(n)
{
}

qs::RegexReferenceAtom::~RegexReferenceAtom()
{
}

const WCHAR* qs::RegexReferenceAtom::match(const WCHAR* pStart,
										   const WCHAR* pEnd,
										   RegexMatchCallback* pCallback) const
{
	std::pair<const WCHAR*, const WCHAR*> reference = pCallback->getReference(n_);
	assert(reference.first && reference.second);
	size_t nLen = reference.second - reference.first;
	if (static_cast<size_t>(pEnd - pStart) >= nLen &&
		wcsncmp(pStart, reference.first, nLen) == 0)
		return pStart + nLen;
	else
		return 0;
}


/****************************************************************************
 *
 * RegexQuantifier
 *
 */

qs::RegexQuantifier::RegexQuantifier(Type type,
									 unsigned int nMin,
									 unsigned int nMax) :
	type_(type),
	nMin_(nMin),
	nMax_(nMax)
{
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
 * RegexMatchCallback
 *
 */

qs::RegexMatchCallback::~RegexMatchCallback()
{
}


/****************************************************************************
 *
 * RegexParser
 *
 */

const WCHAR qs::RegexParser::wszSingleEscapeChar__[] = L"nrt\\|.-^?*+{}()[]";

qs::RegexParser::RegexParser(const WCHAR* pwszPattern) :
	pwszPattern_(pwszPattern),
	p_(pwszPattern),
	nGroup_(0)
{
}

qs::RegexParser::~RegexParser()
{
}

std::auto_ptr<RegexRegexNode> qs::RegexParser::parse()
{
	p_ = pwszPattern_;
	
	std::auto_ptr<RegexRegexNode> pNode(parseRegex());
	if (*p_ != L'\0')
		return 0;
	return pNode;
}

std::auto_ptr<RegexRegexNode> qs::RegexParser::parseRegex()
{
	std::auto_ptr<RegexRegexNode> pRegexNode(new RegexRegexNode(nGroup_++));
	
	while (true) {
		std::auto_ptr<RegexNode> pNode(parseBranch());
		if (!pNode.get())
			return 0;
		pRegexNode->addNode(pNode);
		
		if (*p_ != L'|' || *p_ == L')')
			break;
		++p_;
	}
	
	return pRegexNode;
}

std::auto_ptr<RegexNode> qs::RegexParser::parseBranch()
{
	std::auto_ptr<RegexBrunchNode> pBrunchNode;
	std::auto_ptr<RegexPieceNode> pPieceNode;
	while (*p_ != L'|' && *p_ != L')' && *p_ != L'\0') {
		if (pPieceNode.get())
			pBrunchNode.reset(new RegexBrunchNode(pPieceNode));
		pPieceNode = parsePiece();
		if (!pPieceNode.get())
			return 0;
		if (pBrunchNode.get())
			pBrunchNode->addNode(pPieceNode);
	}
	
	if (pBrunchNode.get())
		return pBrunchNode;
	else if (pPieceNode.get())
		return pPieceNode;
	else
		return new RegexEmptyNode();
}

std::auto_ptr<RegexPieceNode> qs::RegexParser::parsePiece()
{
	std::auto_ptr<RegexAtom> pAtom;
	if (*p_ == L'(') {
		++p_;
		std::auto_ptr<RegexRegexNode> pNode(parseRegex());
		if (*p_ != L')')
			return 0;
		pAtom.reset(new RegexNodeAtom(pNode));
		++p_;
	}
	else if (*p_ == L'[') {
		pAtom = parseCharGroup();
		if (!pAtom.get())
			return 0;
	}
	else if (*p_ == L'.') {
		pAtom.reset(new RegexMultiEscapeAtom(RegexMultiEscapeAtom::TYPE_DOT, false));
		++p_;
	}
	else if (*p_ == L'\\') {
		++p_;
		if (wcschr(wszSingleEscapeChar__, *p_)) {
			pAtom.reset(new RegexCharAtom(*p_));
		}
		else if (*p_ == L's' || *p_ == L'S') {
			pAtom.reset(new RegexMultiEscapeAtom(
				RegexMultiEscapeAtom::TYPE_WHITESPACE, *p_ == L'S'));
		}
		else if (*p_ == L'w' || *p_ == L'W') {
			pAtom.reset(new RegexMultiEscapeAtom(
				RegexMultiEscapeAtom::TYPE_WORD, *p_ == L'W'));
		}
		else if (*p_ == L'd' || *p_ == L'D') {
			pAtom.reset(new RegexMultiEscapeAtom(
				RegexMultiEscapeAtom::TYPE_NUMBER, *p_ == L'D'));
		}
		else if (L'1' <= *p_ && *p_ <= L'9') {
			unsigned int n = *p_ - L'0';
			if (n >= nGroup_)
				return 0;
			while (L'0' <= *(p_ + 1) && *(p_ + 1) <= L'9') {
				unsigned int nNext = n*10 + (*(p_ + 1) - L'0');
				if (nNext >= nGroup_)
					break;
				n = nNext;
				++p_;
			}
			pAtom.reset(new RegexReferenceAtom(n));
		}
		else {
			return 0;
		}
		++p_;
	}
	else if (*p_ == L'?' || *p_ == L'*' || *p_ == L'+' ||
		*p_ == L']' || *p_ == L'{' || *p_ == L'}') {
		return 0;
	}
	else {
		pAtom.reset(new RegexCharAtom(*p_));
		++p_;
	}
	
	std::auto_ptr<RegexQuantifier> pQuantifier;
	if (*p_ == L'?') {
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_RANGE, 0, 1));
		++p_;
	}
	else if (*p_ == L'*') {
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_MIN, 0, 0));
		++p_;
	}
	else if (*p_ == L'+') {
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_MIN, 1, 0));
		++p_;
	}
	else if (*p_ == L'{') {
		pQuantifier = parseQuantity();
	}
	
	return new RegexPieceNode(pAtom, pQuantifier);
}

std::auto_ptr<RegexCharGroupAtom> qs::RegexParser::parseCharGroup()
{
	assert(*p_ == L'[');
	
	std::auto_ptr<RegexCharGroupAtom> pCharGroupAtom(new RegexCharGroupAtom());
	
	++p_;
	bool bNegative = *p_ == L'^';
	if (bNegative)
		++p_;
	pCharGroupAtom->setNegative(bNegative);
	
	if (*p_ == L'-') {
		pCharGroupAtom->addRangeCharGroup(L'-', L'-');
		++p_;
	}
	else if (*p_ == L'[' || *p_ == L']') {
		return 0;
	}
	
	while (*p_ != L']') {
		WCHAR cStart = L'\0';
		bool bDash = false;
		if (*p_ == L'[') {
			return 0;
		}
		else if (*p_ == L'\\') {
			++p_;
			if (wcschr(wszSingleEscapeChar__, *p_)) {
				cStart = *p_;
			}
			else if (*p_ == L's' || *p_ == L'S') {
				std::auto_ptr<RegexMultiEscapeAtom> pAtom(new RegexMultiEscapeAtom(
					RegexMultiEscapeAtom::TYPE_WHITESPACE, *p_ == L'S'));
				pCharGroupAtom->addAtomCharGroup(pAtom);
			}
			else {
				return 0;
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
					if (cStart != L'\0')
						pCharGroupAtom->addRangeCharGroup(cStart, cStart);
					pCharGroupAtom->addRangeCharGroup(L'-', L'-');
				}
				else if (*p_ == L'[') {
					if (cStart != L'\0')
						pCharGroupAtom->addRangeCharGroup(cStart, cStart);
					std::auto_ptr<RegexCharGroupAtom> pSubAtom(new RegexCharGroupAtom());
					pCharGroupAtom->setSubAtom(pSubAtom);
					if (*p_ != L']')
						return 0;
				}
				else if (*p_ == L'\\' && !bDash) {
					++p_;
					if (wcschr(wszSingleEscapeChar__, *p_))
						pCharGroupAtom->addRangeCharGroup(cStart, *p_);
					else
						return 0;
					++p_;
				}
				else if (!bDash) {
					pCharGroupAtom->addRangeCharGroup(cStart, *p_);
					++p_;
				}
				else {
					return 0;
				}
			}
			else {
				pCharGroupAtom->addRangeCharGroup(cStart, cStart);
			}
		}
	}
	++p_;
	
	return pCharGroupAtom;
}

std::auto_ptr<RegexQuantifier> qs::RegexParser::parseQuantity()
{
	assert(*p_ == L'{');
	
	std::auto_ptr<RegexQuantifier> pQuantifier;
	
	++p_;
	if (*p_ < L'0' || L'9' < *p_)
		return 0;
	
	unsigned int nMin = 0;
	while (L'0' <= *p_ && *p_ <= L'9') {
		nMin = nMin*10 + (*p_ - L'0');
		++p_;
	}
	if (*p_ == L'}') {
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_RANGE, nMin, nMin));
		++p_;
	}
	else if (*p_ == L',') {
		++p_;
		if (*p_ == L'}') {
			pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_MIN, nMin, 0));
			++p_;
		}
		else if (L'0' <= *p_ && *p_ <= L'9') {
			unsigned int nMax = 0;
			while (L'0' <= *p_ && *p_ <= L'9') {
				nMax = nMax*10 + (*p_ - L'0');
				++p_;
			}
			if (nMin > nMax)
				return 0;
			if (*p_ == L'}') {
				pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_RANGE, nMin, nMax));
				++p_;
			}
			else {
				return 0;
			}
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
	
	return pQuantifier;
}
