/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	class RegexMultiEscapeAtom;
	class RegexCharGroupAtom;
class RegexQuantifier;
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
	typedef std::vector<RegexNode*> NodeList;

public:
	RegexRegexNode(unsigned int nGroup, QSTATUS* pstatus);
	virtual ~RegexRegexNode();

public:
	const NodeList& getNodeList() const;
	unsigned int getGroup() const;

public:
	QSTATUS addNode(RegexNode* pNode);

public:
	virtual Type getType() const;

private:
	RegexRegexNode(const RegexRegexNode&);
	RegexRegexNode& operator=(const RegexRegexNode&);

private:
	NodeList listNode_;
	unsigned int nGroup_;
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
	RegexBrunchNode(RegexPieceNode* pPieceNode, QSTATUS* pstatus);
	virtual ~RegexBrunchNode();

public:
	const NodeList& getNodeList() const;

public:
	QSTATUS addNode(RegexPieceNode* pPieceNode);

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
	RegexPieceNode(RegexAtom* pAtom,
		RegexQuantifier* pQuantifier, QSTATUS* pstatus);
	virtual ~RegexPieceNode();

public:
	const RegexAtom* getAtom() const;
	const RegexQuantifier* getQuantifier() const;

public:
	virtual Type getType() const;

private:
	RegexPieceNode(const RegexPieceNode&);
	RegexPieceNode& operator=(const RegexPieceNode&);

private:
	RegexAtom* pAtom_;
	RegexQuantifier* pQuantifier_;
};


/****************************************************************************
 *
 * RegexEmptyNode
 *
 */

class RegexEmptyNode : public RegexNode
{
public:
	RegexEmptyNode(QSTATUS* pstatus);
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
	virtual const RegexRegexNode* getNode() const;
	virtual bool match(WCHAR c) const = 0;
};


/****************************************************************************
 *
 * RegexCharAtom
 *
 */

class RegexCharAtom : public RegexAtom
{
public:
	RegexCharAtom(WCHAR c, QSTATUS* pstatus);
	virtual ~RegexCharAtom();

public:
	virtual bool match(WCHAR c) const;

private:
	RegexCharAtom(const RegexCharAtom&);
	RegexCharAtom& operator=(const RegexCharAtom&);

private:
	WCHAR c_;
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
		TYPE_DOT,
		TYPE_WHITESPACE,
		TYPE_WORD
	};

public:
	RegexMultiEscapeAtom(Type type, bool bNegative, QSTATUS* pstatus);
	virtual ~RegexMultiEscapeAtom();

public:
	virtual bool match(WCHAR c) const;

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
		RangeCharGroup(WCHAR cStart, WCHAR cEnd);
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
		AtomCharGroup(RegexAtom* pAtom);
		virtual ~AtomCharGroup();
	
	public:
		virtual bool match(WCHAR c) const;
	
	private:
		AtomCharGroup(const AtomCharGroup&);
		AtomCharGroup& operator=(const AtomCharGroup&);
	
	private:
		RegexAtom* pAtom_;
	};

public:
	typedef std::vector<CharGroup*> CharGroupList;

public:
	RegexCharGroupAtom(QSTATUS* pstatus);
	virtual ~RegexCharGroupAtom();

public:
	virtual bool match(WCHAR c) const;

public:
	void setNegative(bool bNegative);
	QSTATUS addRangeCharGroup(WCHAR cStart, WCHAR cEnd);
	QSTATUS addAtomCharGroup(RegexAtom* pAtom);
	void setSubAtom(RegexCharGroupAtom* pSubAtom);

private:
	RegexCharGroupAtom(const RegexCharGroupAtom&);
	RegexCharGroupAtom& operator=(const RegexCharGroupAtom&);

private:
	bool bNegative_;
	CharGroupList listCharGroup_;
	RegexCharGroupAtom* pSubAtom_;
};


/****************************************************************************
 *
 * RegexNodeAtom
 *
 */

class RegexNodeAtom : public RegexAtom
{
public:
	RegexNodeAtom(RegexRegexNode* pNode, QSTATUS* pstatus);
	virtual ~RegexNodeAtom();

public:
	virtual const RegexRegexNode* getNode() const;
	virtual bool match(WCHAR c) const;

private:
	RegexNodeAtom(const RegexNodeAtom&);
	RegexNodeAtom& operator=(const RegexNodeAtom&);

private:
	RegexRegexNode* pNode_;
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

public:
	RegexQuantifier(Type type, unsigned int nMin,
		unsigned int nMax, QSTATUS* pstatus);
	~RegexQuantifier();

public:
	Type getType() const;
	unsigned int getMin() const;
	unsigned int getMax() const;

private:
	RegexQuantifier(const RegexQuantifier&);
	RegexQuantifier& operator=(const RegexQuantifier&);

private:
	Type type_;
	unsigned int nMin_;
	unsigned int nMax_;
};


/****************************************************************************
 *
 * RegexParser
 *
 */

class RegexParser
{
public:
	RegexParser(const WCHAR* pwszPattern, QSTATUS* pstatus);
	~RegexParser();

public:
	QSTATUS parse(RegexRegexNode** ppNode);

private:
	QSTATUS parseRegex(std::auto_ptr<RegexRegexNode>* ppNode);
	QSTATUS parseBranch(std::auto_ptr<RegexNode>* ppNode);
	QSTATUS parsePiece(std::auto_ptr<RegexPieceNode>* ppPieceNode);
	QSTATUS parseCharGroup(std::auto_ptr<RegexCharGroupAtom>* ppAtom);
	QSTATUS parseQuantity(std::auto_ptr<RegexQuantifier>* ppQuantifier);

private:
	RegexParser(const RegexParser&);
	RegexParser& operator=(const RegexParser&);

private:
	const WCHAR* pwszPattern_;
	const WCHAR* p_;
	unsigned int nGroup_;

private:
	static const WCHAR wszSingleEscapeChar__[];
};

}


#endif // __REGEXPARSER_H__
