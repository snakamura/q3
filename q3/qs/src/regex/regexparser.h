/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __REGEXPARSER_H__
#define __REGEXPARSER_H__

#include <qs.h>

#include <vector>

namespace qs {

class RegexNode;
	class RegexRegexNode;
	class RegexBrunchNode;
	class RegexPieceNode;
	class RegexEmptyNode;
class RegexAtom;
	class RegexCharAtom;
	class RegexCharsAtom;
	class RegexMultiEscapeAtom;
	class RegexCharGroupAtom;
	class RegexNodeAtom;
	class RegexAnchorAtom;
	class RegexReferenceAtom;
class RegexQuantifier;
class RegexMatchCallback;
class RegexParser;


/****************************************************************************
 *
 * RegexNode
 *
 */

class RegexNode
{
public:
	enum Type {
		TYPE_REGEX,
		TYPE_BRUNCH,
		TYPE_PIECE,
		TYPE_EMPTY
	};

public:
	virtual ~RegexNode();

public:
	virtual Type getType() const = 0;
};


/****************************************************************************
 *
 * RegexRegexNode
 *
 */

class RegexRegexNode : public RegexNode
{
public:
	enum GroupType {
		GROUPTYPE_NORMAL,
		GROUPTYPE_POSITIVELOOKAHEAD,
		GROUPTYPE_NEGATIVELOOKAHEAD,
		GROUPTYPE_POSITIVELOOKBEHIND,
		GROUPTYPE_NEGATIVELOOKBEHIND,
		GROUPTYPE_INDEPENDENT
	};

public:
	typedef std::vector<RegexNode*> NodeList;

public:
	RegexRegexNode(unsigned int nGroup,
				   GroupType groupType);
	virtual ~RegexRegexNode();

public:
	const NodeList& getNodeList() const;
	unsigned int getGroup() const;
	GroupType getGroupType() const;
	void resetGroupType();

public:
	void addNode(std::auto_ptr<RegexNode> pNode);
	void addNode(const RegexNode* pNode);

public:
	virtual Type getType() const;

private:
	RegexRegexNode(const RegexRegexNode&);
	RegexRegexNode& operator=(const RegexRegexNode&);

private:
	typedef std::vector<int> FreeList;

private:
	NodeList listNode_;
	FreeList listFree_;
	unsigned int nGroup_;
	GroupType groupType_;
};


/****************************************************************************
 *
 * RegexBrunchNode
 *
 */

class RegexBrunchNode : public RegexNode
{
public:
	typedef std::vector<RegexPieceNode*> NodeList;

public:
	explicit RegexBrunchNode(std::auto_ptr<RegexPieceNode> pPieceNode);
	virtual ~RegexBrunchNode();

public:
	const NodeList& getNodeList() const;

public:
	void addNode(std::auto_ptr<RegexPieceNode> pPieceNode);

public:
	virtual Type getType() const;

private:
	RegexBrunchNode(const RegexBrunchNode&);
	RegexBrunchNode& operator=(const RegexBrunchNode&);

private:
	NodeList listNode_;
};


/****************************************************************************
 *
 * RegexPieceNode
 *
 */

class RegexPieceNode : public RegexNode
{
public:
	RegexPieceNode(std::auto_ptr<RegexAtom> pAtom,
				   std::auto_ptr<RegexQuantifier> pQuantifier);
	virtual ~RegexPieceNode();

public:
	const RegexAtom* getAtom() const;
	RegexQuantifier* getQuantifier() const;

public:
	virtual Type getType() const;

private:
	RegexPieceNode(const RegexPieceNode&);
	RegexPieceNode& operator=(const RegexPieceNode&);

private:
	std::auto_ptr<RegexAtom> pAtom_;
	std::auto_ptr<RegexQuantifier> pQuantifier_;
};


/****************************************************************************
 *
 * RegexEmptyNode
 *
 */

class RegexEmptyNode : public RegexNode
{
public:
	RegexEmptyNode();
	virtual ~RegexEmptyNode();

public:
	virtual Type getType() const;

private:
	RegexEmptyNode(const RegexEmptyNode&);
	RegexEmptyNode& operator=(const RegexEmptyNode&);
};


/****************************************************************************
 *
 * RegexAtom
 *
 */

class RegexAtom
{
public:
	virtual ~RegexAtom();

public:
	virtual RegexRegexNode* getNode() const;
	virtual const WCHAR* match(const WCHAR* pStart,
							   const WCHAR* pEnd,
							   const WCHAR* p,
							   RegexMatchCallback* pCallback) const;

protected:
	virtual bool matchChar(WCHAR c) const;
};


/****************************************************************************
 *
 * RegexCharAtom
 *
 */

class RegexCharAtom : public RegexAtom
{
public:
	explicit RegexCharAtom(WCHAR c);
	virtual ~RegexCharAtom();

protected:
	virtual bool matchChar(WCHAR c) const;

private:
	RegexCharAtom(const RegexCharAtom&);
	RegexCharAtom& operator=(const RegexCharAtom&);

private:
	WCHAR c_;
};


/****************************************************************************
 *
 * RegexCharsAtom
 *
 */

class RegexCharsAtom : public RegexAtom
{
public:
	RegexCharsAtom(const WCHAR* pStart,
				   const WCHAR* pEnd);
	virtual ~RegexCharsAtom();

public:
	virtual const WCHAR* match(const WCHAR* pStart,
							   const WCHAR* pEnd,
							   const WCHAR* p,
							   RegexMatchCallback* pCallback) const;

private:
	RegexCharsAtom(const RegexCharsAtom&);
	RegexCharsAtom& operator=(const RegexCharsAtom&);

private:
	wstring_ptr wstr_;
	size_t nLen_;
};


/****************************************************************************
 *
 * RegexMultiEscapeAtom
 *
 */

class RegexMultiEscapeAtom : public RegexAtom
{
public:
	enum Type {
		TYPE_ALL,
		TYPE_NOLINETERMINATOR,
		TYPE_WHITESPACE,
		TYPE_WORD,
		TYPE_NUMBER
	};

public:
	RegexMultiEscapeAtom(Type type,
						 bool bNegative);
	virtual ~RegexMultiEscapeAtom();

protected:
	virtual bool matchChar(WCHAR c) const;

private:
	RegexMultiEscapeAtom(const RegexMultiEscapeAtom&);
	RegexMultiEscapeAtom& operator=(const RegexMultiEscapeAtom&);

private:
	Type type_;
	bool bNegative_;
};


/****************************************************************************
 *
 * RegexCharGroupAtom
 *
 */

class RegexCharGroupAtom : public RegexAtom
{
private:
	class CharGroup
	{
	public:
		virtual ~CharGroup();
	
	public:
		virtual bool match(WCHAR c) const = 0;
	};
	
	class RangeCharGroup : public CharGroup
	{
	public:
		RangeCharGroup(WCHAR cStart,
					   WCHAR cEnd);
		virtual ~RangeCharGroup();
	
	public:
		virtual bool match(WCHAR c) const;
	
	private:
		RangeCharGroup(const RangeCharGroup&);
		RangeCharGroup& operator=(const RangeCharGroup&);
	
	private:
		WCHAR cStart_;
		WCHAR cEnd_;
	};
	
	class AtomCharGroup : public CharGroup
	{
	public:
		AtomCharGroup(std::auto_ptr<RegexMultiEscapeAtom> pAtom);
		virtual ~AtomCharGroup();
	
	public:
		virtual bool match(WCHAR c) const;
	
	private:
		AtomCharGroup(const AtomCharGroup&);
		AtomCharGroup& operator=(const AtomCharGroup&);
	
	private:
		std::auto_ptr<RegexMultiEscapeAtom> pAtom_;
	};

public:
	typedef std::vector<CharGroup*> CharGroupList;

public:
	RegexCharGroupAtom();
	virtual ~RegexCharGroupAtom();

protected:
	virtual bool matchChar(WCHAR c) const;

public:
	void setNegative(bool bNegative);
	void addRangeCharGroup(WCHAR cStart,
						   WCHAR cEnd);
	void addAtomCharGroup(std::auto_ptr<RegexMultiEscapeAtom> pAtom);
	void setSubAtom(std::auto_ptr<RegexCharGroupAtom> pSubAtom);

private:
	RegexCharGroupAtom(const RegexCharGroupAtom&);
	RegexCharGroupAtom& operator=(const RegexCharGroupAtom&);

private:
	bool bNegative_;
	CharGroupList listCharGroup_;
	std::auto_ptr<RegexCharGroupAtom> pSubAtom_;
};


/****************************************************************************
 *
 * RegexNodeAtom
 *
 */

class RegexNodeAtom : public RegexAtom
{
public:
	explicit RegexNodeAtom(std::auto_ptr<RegexRegexNode> pNode);
	virtual ~RegexNodeAtom();

public:
	virtual RegexRegexNode* getNode() const;

private:
	RegexNodeAtom(const RegexNodeAtom&);
	RegexNodeAtom& operator=(const RegexNodeAtom&);

private:
	std::auto_ptr<RegexRegexNode> pNode_;
};


/****************************************************************************
 *
 * RegexAnchorAtom
 *
 */

class RegexAnchorAtom : public RegexAtom
{
public:
	enum Type {
		TYPE_LINESTART,
		TYPE_LINEEND,
		TYPE_START,
		TYPE_END,
		TYPE_ENDSTRICT,
		TYPE_WORDBOUNDARY,
		TYPE_NOWORDBOUNDARY
	};

public:
	explicit RegexAnchorAtom(Type type);
	virtual ~RegexAnchorAtom();

public:
	virtual const WCHAR* match(const WCHAR* pStart,
							   const WCHAR* pEnd,
							   const WCHAR* p,
							   RegexMatchCallback* pCallback) const;

private:
	RegexAnchorAtom(const RegexAnchorAtom&);
	RegexAnchorAtom& operator=(const RegexAnchorAtom&);

private:
	Type type_;
};


/****************************************************************************
 *
 * RegexReferenceAtom
 *
 */

class RegexReferenceAtom : public RegexAtom
{
public:
	explicit RegexReferenceAtom(unsigned int n);
	virtual ~RegexReferenceAtom();

public:
	virtual const WCHAR* match(const WCHAR* pStart,
							   const WCHAR* pEnd,
							   const WCHAR* p,
							   RegexMatchCallback* pCallback) const;

private:
	RegexReferenceAtom(const RegexReferenceAtom&);
	RegexReferenceAtom& operator=(const RegexReferenceAtom&);

private:
	unsigned int n_;
};


/****************************************************************************
 *
 * RegexQuantifier
 *
 */

class RegexQuantifier
{
public:
	enum Type {
		TYPE_RANGE,
		TYPE_MIN
	};
	enum Option {
		OPTION_GREEDY,
		OPTION_RELUCTANT,
		OPTION_POSSESSIVE
	};

public:
	RegexQuantifier(Type type,
					unsigned int nMin,
					unsigned int nMax,
					Option option);
	~RegexQuantifier();

public:
	Type getType() const;
	unsigned int getMin() const;
	unsigned int getMax() const;
	Option getOption() const;
	void resetOption();

private:
	RegexQuantifier(const RegexQuantifier&);
	RegexQuantifier& operator=(const RegexQuantifier&);

private:
	Type type_;
	unsigned int nMin_;
	unsigned int nMax_;
	Option option_;
};


/****************************************************************************
 *
 * RegexMatchCallback
 *
 */

class RegexMatchCallback
{
public:
	virtual ~RegexMatchCallback();

public:
	virtual std::pair<const WCHAR*, const WCHAR*> getReference(unsigned int n) = 0;
};


/****************************************************************************
 *
 * RegexParser
 *
 */

class RegexParser
{
public:
	RegexParser(const WCHAR* pwszPattern,
				unsigned int nMode);
	~RegexParser();

public:
	std::auto_ptr<RegexRegexNode> parse();

private:
	std::auto_ptr<RegexRegexNode> parseRegex(bool bCapture,
											 RegexRegexNode::GroupType groupType);
	std::auto_ptr<RegexNode> parseBranch();
	std::auto_ptr<RegexPieceNode> parsePiece();
	std::auto_ptr<RegexCharGroupAtom> parseCharGroup();
	std::auto_ptr<RegexQuantifier> parseQuantity();
	RegexQuantifier::Option parseQuantifierOption();
	WCHAR parseEscapedChar();
	WCHAR parseHexEscapedChar();
	WCHAR parseOctEscapedChar();
	bool checkReference(unsigned int nGroup) const;

private:
	static WCHAR getSingleEscapedChar(WCHAR c);
	static std::auto_ptr<RegexMultiEscapeAtom> getMultiEscapedAtom(WCHAR c);
	static bool isSingleEscapeChar(WCHAR c);
	static bool isMultiEscapeChar(WCHAR c);
	static bool isSpecialChar(WCHAR c);
	static bool isQuantifierChar(WCHAR c);
	static int getHex(WCHAR c);
	static int getOct(WCHAR c);

private:
	RegexParser(const RegexParser&);
	RegexParser& operator=(const RegexParser&);

private:
	typedef std::vector<unsigned int> GroupStack;

private:
	const WCHAR* pwszPattern_;
	unsigned int nMode_;
	const WCHAR* p_;
	unsigned int nGroup_;
	GroupStack stackGroup_;

private:
	static const WCHAR wszSingleEscapeChar__[];
	static const WCHAR wszMultiEscapeChar__[];
	static const WCHAR wszSpecialChar__[];
	static const WCHAR wszQuantifierChar__[];
};


/****************************************************************************
 *
 * RegexUtil
 *
 */

class RegexUtil
{
public:
	static bool isLineTerminator(WCHAR c);
	static bool isWhitespace(WCHAR c);
	static bool isWord(WCHAR c);
	static bool isNumber(WCHAR c);
};

}


#endif // __REGEXPARSER_H__
