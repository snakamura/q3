/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qsregex.h>
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

qs::RegexRegexNode::RegexRegexNode(unsigned int nGroup,
								   GroupType groupType) :
	nGroup_(nGroup),
	groupType_(groupType)
{
}

qs::RegexRegexNode::~RegexRegexNode()
{
	for (NodeList::size_type n = 0; n < listNode_.size(); ++n) {
		if (listFree_[n])
			delete listNode_[n];
	}
}

const RegexRegexNode::NodeList& qs::RegexRegexNode::getNodeList() const
{
	return listNode_;
}

unsigned int qs::RegexRegexNode::getGroup() const
{
	return nGroup_;
}

RegexRegexNode::GroupType qs::RegexRegexNode::getGroupType() const
{
	return groupType_;
}

void qs::RegexRegexNode::resetGroupType()
{
	groupType_ = GROUPTYPE_NORMAL;
}

void qs::RegexRegexNode::addNode(std::auto_ptr<RegexNode> pNode)
{
	assert(pNode.get());
	listNode_.push_back(pNode.get());
	listFree_.push_back(1);
	pNode.release();
}

void qs::RegexRegexNode::addNode(const RegexNode* pNode)
{
	assert(pNode);
	listNode_.push_back(const_cast<RegexNode*>(pNode));
	listFree_.push_back(0);
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

RegexQuantifier* qs::RegexPieceNode::getQuantifier() const
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

RegexRegexNode* qs::RegexAtom::getNode() const
{
	return 0;
}

const WCHAR* qs::RegexAtom::match(const WCHAR* pStart,
								  const WCHAR* pEnd,
								  const WCHAR* p,
								  RegexMatchCallback* pCallback) const
{
	assert(pStart <= p && p <= pEnd);
	if (p != pEnd)
		return matchChar(*p) ? p + 1 : 0;
	else
		return 0;
}

bool qs::RegexAtom::matchChar(WCHAR c) const
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

bool qs::RegexCharAtom::matchChar(WCHAR c) const
{
	return c == c_;
}


/****************************************************************************
 *
 * RegexCharsAtom
 *
 */

qs::RegexCharsAtom::RegexCharsAtom(const WCHAR* pStart,
								   const WCHAR* pEnd) :
	nLen_(pEnd - pStart)
{
	wstr_ = allocWString(pStart, pEnd - pStart);
}

qs::RegexCharsAtom::~RegexCharsAtom()
{
}

const WCHAR* qs::RegexCharsAtom::match(const WCHAR* pStart,
									   const WCHAR* pEnd,
									   const WCHAR* p,
									   RegexMatchCallback* pCallback) const
{
	if (nLen_ <= static_cast<size_t>(pEnd - p))
		return wcsncmp(wstr_.get(), p, nLen_) == 0 ? p + nLen_ : 0;
	else
		return 0;
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

bool qs::RegexMultiEscapeAtom::matchChar(WCHAR c) const
{
	bool bMatch = false;
	switch (type_) {
	case TYPE_ALL:
		bMatch = true;
		break;
	case TYPE_NOLINETERMINATOR:
		bMatch = !RegexUtil::isLineTerminator(c);
		break;
	case TYPE_WHITESPACE:
		bMatch = RegexUtil::isWhitespace(c);
		break;
	case TYPE_WORD:
		bMatch = RegexUtil::isWord(c);
		break;
	case TYPE_NUMBER:
		bMatch = RegexUtil::isNumber(c);
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

qs::RegexCharGroupAtom::AtomCharGroup::AtomCharGroup(std::auto_ptr<RegexMultiEscapeAtom> pAtom) :
	pAtom_(pAtom)
{
}

qs::RegexCharGroupAtom::AtomCharGroup::~AtomCharGroup()
{
}

bool qs::RegexCharGroupAtom::AtomCharGroup::match(WCHAR c) const
{
	return pAtom_->match(&c, &c + 1, &c, 0) != 0;
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

bool qs::RegexCharGroupAtom::matchChar(WCHAR c) const
{
	bool bMatch = false;
	for (CharGroupList::const_iterator it = listCharGroup_.begin(); it != listCharGroup_.end() && !bMatch; ++it)
		bMatch = (*it)->match(c);
	if ((!bMatch && !bNegative_) || (bMatch && bNegative_))
		return false;
	else if (pSubAtom_.get())
		return !pSubAtom_->match(&c, &c + 1, &c, 0);
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

void qs::RegexCharGroupAtom::addAtomCharGroup(std::auto_ptr<RegexMultiEscapeAtom> pAtom)
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

RegexRegexNode* qs::RegexNodeAtom::getNode() const
{
	return pNode_.get();
}


/****************************************************************************
 *
 * RegexAnchorAtom
 *
 */

qs::RegexAnchorAtom::RegexAnchorAtom(Type type) :
	type_(type)
{
}

qs::RegexAnchorAtom::~RegexAnchorAtom()
{
}

const WCHAR* qs::RegexAnchorAtom::match(const WCHAR* pStart,
										const WCHAR* pEnd,
										const WCHAR* p,
										RegexMatchCallback* pCallback) const
{
	assert(pStart <= p && p <= pEnd);
	
	bool bMatch = false;
	switch (type_) {
	case TYPE_LINESTART:
		bMatch = p == pStart || RegexUtil::isLineTerminator(*(p - 1));
		break;
	case TYPE_LINEEND:
		bMatch = p == pEnd || RegexUtil::isLineTerminator(*p);
		break;
	case TYPE_START:
		bMatch = p == pStart;
		break;
	case TYPE_END:
		bMatch = p == pEnd || (p == pEnd - 1 && RegexUtil::isLineTerminator(*p));
		break;
	case TYPE_ENDSTRICT:
		bMatch = p == pEnd;
		break;
	case TYPE_WORDBOUNDARY:
		if (p != pEnd)
			bMatch = (p == pStart || RegexUtil::isWord(*(p - 1)) != RegexUtil::isWord(*p));
		else
			bMatch = RegexUtil::isWord(*(p - 1));
		break;
	case TYPE_NOWORDBOUNDARY:
		if (p != pEnd)
			bMatch = (p != pStart && RegexUtil::isWord(*(p - 1)) == RegexUtil::isWord(*p));
		else
			bMatch = !RegexUtil::isWord(*(p - 1));
		break;
	default:
		assert(false);
		break;
	}
	return bMatch ? p : 0;
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
										   const WCHAR* p,
										   RegexMatchCallback* pCallback) const
{
	std::pair<const WCHAR*, const WCHAR*> reference = pCallback->getReference(n_);
	assert(reference.first && reference.second);
	size_t nLen = reference.second - reference.first;
	if (static_cast<size_t>(pEnd - p) >= nLen &&
		wcsncmp(p, reference.first, nLen) == 0)
		return p + nLen;
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
									 unsigned int nMax,
									 Option option) :
	type_(type),
	nMin_(nMin),
	nMax_(nMax),
	option_(option)
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

RegexQuantifier::Option qs::RegexQuantifier::getOption() const
{
	return option_;
}

void qs::RegexQuantifier::resetOption()
{
	option_ = OPTION_GREEDY;
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

const WCHAR qs::RegexParser::wszSingleEscapeChar__[] = L"nrt\\|.-^?*+{}()[]$";
const WCHAR qs::RegexParser::wszMultiEscapeChar__[] = L"sSwWdD";
const WCHAR qs::RegexParser::wszSpecialChar__[] = L"\\|.-^?*+{}()[]$";
const WCHAR qs::RegexParser::wszQuantifierChar__[] = L"*+?{";

qs::RegexParser::RegexParser(const WCHAR* pwszPattern,
							 unsigned int nMode) :
	pwszPattern_(pwszPattern),
	nMode_(nMode),
	p_(pwszPattern),
	nGroup_(1)
{
}

qs::RegexParser::~RegexParser()
{
}

std::auto_ptr<RegexRegexNode> qs::RegexParser::parse()
{
	p_ = pwszPattern_;
	
	std::auto_ptr<RegexRegexNode> pNode(parseRegex(
		false, RegexRegexNode::GROUPTYPE_NORMAL));
	if (*p_ != L'\0')
		return std::auto_ptr<RegexRegexNode>(0);
	return pNode;
}

std::auto_ptr<RegexRegexNode> qs::RegexParser::parseRegex(bool bCapture,
														  RegexRegexNode::GroupType groupType)
{
	std::auto_ptr<RegexRegexNode> pRegexNode(new RegexRegexNode(
		bCapture ? nGroup_ : -1, groupType));
	if (bCapture) {
		stackGroup_.push_back(nGroup_);
		++nGroup_;
	}
	
	while (true) {
		std::auto_ptr<RegexNode> pNode(parseBranch());
		if (!pNode.get())
			return std::auto_ptr<RegexRegexNode>(0);
		pRegexNode->addNode(pNode);
		
		if (*p_ != L'|' || *p_ == L')')
			break;
		++p_;
	}
	
	if (bCapture)
		stackGroup_.pop_back();
	
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
			return std::auto_ptr<RegexNode>(0);
		if (pBrunchNode.get())
			pBrunchNode->addNode(pPieceNode);
	}
	
	if (pBrunchNode.get())
		return pBrunchNode;
	else if (pPieceNode.get())
		return pPieceNode;
	else
		return std::auto_ptr<RegexNode>(new RegexEmptyNode());
}

std::auto_ptr<RegexPieceNode> qs::RegexParser::parsePiece()
{
	std::auto_ptr<RegexAtom> pAtom;
	if (*p_ == L'(') {
		++p_;
		
		bool bCapture = true;
		RegexRegexNode::GroupType groupType = RegexRegexNode::GROUPTYPE_NORMAL;
		if (*p_ == L'?') {
			bCapture = false;
			
			++p_;
			if (*p_ == L':') {
			}
			else if (*p_ == L'=') {
				groupType = RegexRegexNode::GROUPTYPE_POSITIVELOOKAHEAD;
			}
			else if (*p_ == L'!') {
				groupType = RegexRegexNode::GROUPTYPE_NEGATIVELOOKAHEAD;
			}
			else if (*p_ == L'<') {
				++p_;
				if (*p_ == L'=')
					groupType = RegexRegexNode::GROUPTYPE_POSITIVELOOKBEHIND;
				else if (*p_ == L'!')
					groupType = RegexRegexNode::GROUPTYPE_NEGATIVELOOKBEHIND;
				else
					return std::auto_ptr<RegexPieceNode>(0);
			}
			else if (*p_ == L'>') {
				groupType = RegexRegexNode::GROUPTYPE_INDEPENDENT;
			}
			else {
				return std::auto_ptr<RegexPieceNode>(0);
			}
			++p_;
		}
		
		std::auto_ptr<RegexRegexNode> pNode(parseRegex(bCapture, groupType));
		if (*p_ != L')')
			return std::auto_ptr<RegexPieceNode>(0);
		pAtom.reset(new RegexNodeAtom(pNode));
		
		++p_;
	}
	else if (*p_ == L'[') {
		pAtom = parseCharGroup();
		if (!pAtom.get())
			return std::auto_ptr<RegexPieceNode>(0);
	}
	else if (*p_ == L'.') {
		RegexMultiEscapeAtom::Type type = nMode_ & RegexCompiler::MODE_DOTALL ?
			RegexMultiEscapeAtom::TYPE_ALL : RegexMultiEscapeAtom::TYPE_NOLINETERMINATOR;
		pAtom.reset(new RegexMultiEscapeAtom(type, false));
		++p_;
	}
	else if (*p_ == L'^') {
		RegexAnchorAtom::Type type = nMode_ & RegexCompiler::MODE_MULTILINE ?
			RegexAnchorAtom::TYPE_LINESTART : RegexAnchorAtom::TYPE_START;
		pAtom.reset(new RegexAnchorAtom(type));
		++p_;
	}
	else if (*p_ == L'$') {
		RegexAnchorAtom::Type type = nMode_ & RegexCompiler::MODE_MULTILINE ?
			RegexAnchorAtom::TYPE_LINEEND : RegexAnchorAtom::TYPE_END;
		pAtom.reset(new RegexAnchorAtom(type));
		++p_;
	}
	else if (*p_ == L'\\') {
		++p_;
		
		WCHAR c = parseEscapedChar();
		if (c != L'\0') {
			pAtom.reset(new RegexCharAtom(c));
		}
		else if (isMultiEscapeChar(*p_)) {
			pAtom = getMultiEscapedAtom(*p_);
		}
		else if (*p_ == L'b') {
			pAtom.reset(new RegexAnchorAtom(RegexAnchorAtom::TYPE_WORDBOUNDARY));
		}
		else if (*p_ == L'B') {
			pAtom.reset(new RegexAnchorAtom(RegexAnchorAtom::TYPE_NOWORDBOUNDARY));
		}
		else if (*p_ == L'A') {
			pAtom.reset(new RegexAnchorAtom(RegexAnchorAtom::TYPE_START));
		}
		else if (*p_ == L'Z') {
			pAtom.reset(new RegexAnchorAtom(RegexAnchorAtom::TYPE_END));
		}
		else if (*p_ == L'z') {
			pAtom.reset(new RegexAnchorAtom(RegexAnchorAtom::TYPE_ENDSTRICT));
		}
		else if (L'1' <= *p_ && *p_ <= L'9') {
			unsigned int n = *p_ - L'0';
			if (!checkReference(n))
				return std::auto_ptr<RegexPieceNode>(0);
			while (L'0' <= *(p_ + 1) && *(p_ + 1) <= L'9') {
				unsigned int nNext = n*10 + (*(p_ + 1) - L'0');
				if (!checkReference(nNext))
					break;
				n = nNext;
				++p_;
			}
			pAtom.reset(new RegexReferenceAtom(n));
		}
		else {
			return std::auto_ptr<RegexPieceNode>(0);
		}
		++p_;
	}
	else if (*p_ == L'?' || *p_ == L'*' || *p_ == L'+' ||
		*p_ == L']' || *p_ == L'{' || *p_ == L'}') {
		return std::auto_ptr<RegexPieceNode>(0);
	}
	else {
		const WCHAR* p = p_;
		do {
			++p_;
		} while (*p_ && !isSpecialChar(*p_) && (!*(p_ + 1) || !isQuantifierChar(*(p_ + 1))));
		if (p_ == p + 1)
			pAtom.reset(new RegexCharAtom(*p));
		else
			pAtom.reset(new RegexCharsAtom(p, p_));
	}
	
	std::auto_ptr<RegexQuantifier> pQuantifier;
	if (*p_ == L'?') {
		++p_;
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_RANGE,
			0, 1, parseQuantifierOption()));
	}
	else if (*p_ == L'*') {
		++p_;
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_MIN,
			0, 0, parseQuantifierOption()));
	}
	else if (*p_ == L'+') {
		++p_;
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_MIN,
			1, 0, parseQuantifierOption()));
	}
	else if (*p_ == L'{') {
		pQuantifier = parseQuantity();
	}
	
	return std::auto_ptr<RegexPieceNode>(new RegexPieceNode(pAtom, pQuantifier));
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
		return std::auto_ptr<RegexCharGroupAtom>(0);
	}
	
	while (*p_ != L']') {
		WCHAR cStart = L'\0';
		bool bDash = false;
		if (*p_ == L'[') {
			return std::auto_ptr<RegexCharGroupAtom>(0);
		}
		else if (*p_ == L'\\') {
			++p_;
			
			cStart = parseEscapedChar();
			if (cStart == L'\0') {
				if (isMultiEscapeChar(*p_))
					pCharGroupAtom->addAtomCharGroup(getMultiEscapedAtom(*p_));
				else
					return std::auto_ptr<RegexCharGroupAtom>(0);
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
						return std::auto_ptr<RegexCharGroupAtom>(0);
				}
				else if (*p_ == L'\\' && !bDash) {
					++p_;
					
					WCHAR cEnd = parseEscapedChar();
					if (cEnd == L'\0')
						return std::auto_ptr<RegexCharGroupAtom>(0);
					pCharGroupAtom->addRangeCharGroup(cStart, cEnd);
					
					++p_;
				}
				else if (!bDash) {
					pCharGroupAtom->addRangeCharGroup(cStart, *p_);
					++p_;
				}
				else {
					return std::auto_ptr<RegexCharGroupAtom>(0);
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
		return std::auto_ptr<RegexQuantifier>(0);
	
	unsigned int nMin = 0;
	while (L'0' <= *p_ && *p_ <= L'9') {
		nMin = nMin*10 + (*p_ - L'0');
		++p_;
	}
	if (*p_ == L'}') {
		++p_;
		pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_RANGE,
			nMin, nMin, parseQuantifierOption()));
	}
	else if (*p_ == L',') {
		++p_;
		if (*p_ == L'}') {
			++p_;
			pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_MIN,
				nMin, 0, parseQuantifierOption()));
		}
		else if (L'0' <= *p_ && *p_ <= L'9') {
			unsigned int nMax = 0;
			while (L'0' <= *p_ && *p_ <= L'9') {
				nMax = nMax*10 + (*p_ - L'0');
				++p_;
			}
			if (nMin > nMax)
				return std::auto_ptr<RegexQuantifier>(0);
			if (*p_ == L'}') {
				++p_;
				pQuantifier.reset(new RegexQuantifier(RegexQuantifier::TYPE_RANGE,
					nMin, nMax, parseQuantifierOption()));
			}
			else {
				return std::auto_ptr<RegexQuantifier>(0);
			}
		}
		else {
			return std::auto_ptr<RegexQuantifier>(0);
		}
	}
	else {
		return std::auto_ptr<RegexQuantifier>(0);
	}
	
	return pQuantifier;
}

RegexQuantifier::Option qs::RegexParser::parseQuantifierOption()
{
	if (*p_ == L'?') {
		++p_;
		return RegexQuantifier::OPTION_RELUCTANT;
	}
	else if (*p_ == L'+') {
		++p_;
		return RegexQuantifier::OPTION_POSSESSIVE;
	}
	else {
		return RegexQuantifier::OPTION_GREEDY;
	}
}

WCHAR qs::RegexParser::parseEscapedChar()
{
	if (isSingleEscapeChar(*p_))
		return getSingleEscapedChar(*p_);
	else if (*p_ == L'x' || *p_ == L'u')
		return parseHexEscapedChar();
	else if (*p_ == L'0')
		return parseOctEscapedChar();
	else
		return L'\0';
}

WCHAR qs::RegexParser::parseHexEscapedChar()
{
	assert(*p_ == L'x' || *p_ == L'u');
	
	int nValue = 0;
	for (int nDigit = *p_ == L'x' ? 2 : 4; nDigit > 0; --nDigit) {
		++p_;
		int n = getHex(*p_);
		if (n == -1)
			return L'\0';
		nValue = nValue*16 + n;
	}
	return static_cast<WCHAR>(nValue);
}

WCHAR qs::RegexParser::parseOctEscapedChar()
{
	assert(*p_ == L'0');
	
	int nValue = 0;
	for (int nDigit = 3; nDigit > 0; --nDigit) {
		++p_;
		int n = getOct(*p_);
		if (n == -1 || (nDigit == 3 && n > 3))
			return L'\0';
		nValue = nValue*8 + n;
	}
	return static_cast<WCHAR>(nValue);
}

bool qs::RegexParser::checkReference(unsigned int nGroup) const
{
	return std::find(stackGroup_.begin(), stackGroup_.end(), nGroup) == stackGroup_.end();
}

WCHAR qs::RegexParser::getSingleEscapedChar(WCHAR c)
{
	assert(isSingleEscapeChar(c));
	
	switch (c) {
	case L'n':
		return L'\n';
	case L'r':
		return L'\r';
	case L't':
		return L'\t';
	default:
		return c;
	}
}

std::auto_ptr<RegexMultiEscapeAtom> qs::RegexParser::getMultiEscapedAtom(WCHAR c)
{
	assert(isMultiEscapeChar(c));
	
	std::auto_ptr<RegexMultiEscapeAtom> pAtom;
	switch (c) {
	case L's':
	case L'S':
		pAtom.reset(new RegexMultiEscapeAtom(
			RegexMultiEscapeAtom::TYPE_WHITESPACE, c == L'S'));
		break;
	case L'w':
	case L'W':
		pAtom.reset(new RegexMultiEscapeAtom(
			RegexMultiEscapeAtom::TYPE_WORD, c == L'W'));
		break;
	case L'd':
	case L'D':
		pAtom.reset(new RegexMultiEscapeAtom(
			RegexMultiEscapeAtom::TYPE_NUMBER, c == L'D'));
		break;
	default:
		assert(false);
		break;
	}
	return pAtom;
}

bool qs::RegexParser::isSingleEscapeChar(WCHAR c)
{
	const WCHAR* pEnd = wszSingleEscapeChar__ + countof(wszSingleEscapeChar__);
	return std::find(wszSingleEscapeChar__, pEnd, c) != pEnd;
}

bool qs::RegexParser::isMultiEscapeChar(WCHAR c)
{
	const WCHAR* pEnd = wszMultiEscapeChar__ + countof(wszMultiEscapeChar__);
	return std::find(wszMultiEscapeChar__, pEnd, c) != pEnd;
}

bool qs::RegexParser::isSpecialChar(WCHAR c)
{
	const WCHAR* pEnd = wszSpecialChar__ + countof(wszSpecialChar__);
	return std::find(wszSpecialChar__, pEnd, c) != pEnd;
}

bool qs::RegexParser::isQuantifierChar(WCHAR c)
{
	const WCHAR* pEnd = wszQuantifierChar__ + countof(wszQuantifierChar__);
	return std::find(wszQuantifierChar__, pEnd, c) != pEnd;
}

int qs::RegexParser::getHex(WCHAR c)
{
	if (L'0' <= c && c <= L'9')
		return c - L'0';
	else if (L'a' <= c && c <= L'f')
		return c - L'a' + 10;
	else if (L'A' <= c && c <= L'F')
		return c - L'A' + 10;
	else
		return -1;
}

int qs::RegexParser::getOct(WCHAR c)
{
	if (L'0' <= c && c <= L'7')
		return c - L'0';
	else
		return -1;
}


/****************************************************************************
 *
 * RegexUtil
 *
 */

bool qs::RegexUtil::isLineTerminator(WCHAR c)
{
	return c == L'\n' ||
		c == L'\r' ||
		c == 0x0085 ||
		c == 0x2028 ||
		c == 0x2029;
}

bool qs::RegexUtil::isWhitespace(WCHAR c)
{
	return c == L' ' ||
		c == L'\t' ||
		c == L'\r' ||
		c == L'\n';
}

bool qs::RegexUtil::isWord(WCHAR c)
{
	return (L'a' <= c && c <= L'z') ||
		(L'A' <= c && c <= L'Z') ||
		(L'0' <= c && c <= L'9') ||
		c == L'_';
}

bool qs::RegexUtil::isNumber(WCHAR c)
{
	return L'0' <= c && c <= L'9';
}
