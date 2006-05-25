/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MIME_H__
#define __MIME_H__

#include <qsmime.h>


namespace qs {

/****************************************************************************
 *
 * Tokenizer
 *
 */

class Tokenizer
{
public:
	enum TokenType {
		T_SPECIAL,
		T_ATOM,
		T_QSTRING,
		T_DOMAIN,
		T_COMMENT,
		T_END,
		T_ERROR
	};
	
	enum Flag {
		F_RECOGNIZECOMMENT	= 0x0001,
		F_RECOGNIZEDOMAIN	= 0x0002,
		F_SPECIAL			= 0x0100,
		F_TSPECIAL			= 0x0200,
		F_ESPECIAL			= 0x0400
	};
	
	struct Token
	{
		Token(TokenType type);
		Token(TokenType type,
			  string_ptr str);
		Token(Token& token);
		Token& operator=(Token& token);
		
		TokenType type_;
		string_ptr str_;
	};

public:
	Tokenizer(const CHAR* psz,
			  size_t nLen,
			  unsigned int nFlags);
	~Tokenizer();

public:
	Token getToken();

public:
	static bool isCtl(unsigned char c);
	static bool isSpace(unsigned char c);
	static bool isSpecial(unsigned char c,
						  unsigned int nFlags);

private:
	Tokenizer(const Tokenizer&);
	Tokenizer& operator=(const Tokenizer&);

private:
	string_ptr str_;
	const CHAR* p_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * AddrSpecParser
 *
 */

class AddrSpecParser
{
public:
	enum Type {
		TYPE_INBRACKET,
		TYPE_NORMAL,
		TYPE_INGROUP
	};
	
	enum State {
		S_BEGIN,
		S_LOCALPARTPERIOD,
		S_LOCALPARTWORD,
		S_ADDRSPECAT,
		S_SUBDOMAIN,
		S_SUBDOMAINPERIOD,
		S_END
	};

public:
	AddrSpecParser();
	~AddrSpecParser();

public:
	Part::Field parseAddrSpec(const Part& part,
							  Tokenizer& t,
							  State state,
							  Type type,
							  string_ptr* pstrMailbox,
							  string_ptr* pstrHost,
							  string_ptr* pstrComment,
							  bool* pbEnd) const;

private:
	AddrSpecParser(const AddrSpecParser&);
	AddrSpecParser& operator=(const AddrSpecParser&);
};


/****************************************************************************
 *
 * FieldComparator
 *
 */

class FieldComparator :
	public std::binary_function<std::pair<STRING, STRING>, std::pair<STRING, STRING>, bool>
{
public:
	bool operator()(const std::pair<STRING, STRING>& lhs,
					const std::pair<STRING, STRING>& rhs) const;
};

}

#endif __MIME_H__
