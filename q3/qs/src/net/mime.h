/*
 * $Id: mime.h,v 1.3 2003/05/20 16:06:07 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	enum Token {
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

public:
	Tokenizer(const CHAR* psz, size_t nLen,
		unsigned int nFlags, QSTATUS* pstatus);
	~Tokenizer();

public:
	QSTATUS getToken(Token* pToken, STRING* pstrToken);

public:
	static bool isCtl(unsigned char c);
	static bool isSpace(unsigned char c);
	static bool isSpecial(unsigned char c, unsigned int nFlags);

private:
	Tokenizer(const Tokenizer&);
	Tokenizer& operator=(const Tokenizer&);

private:
	STRING str_;
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
		S_LOCALPARTPERIOD,
		S_LOCALPARTWORD,
		S_ADDRSPECAT,
		S_SUBDOMAIN,
		S_SUBDOMAINPERIOD,
		S_END
	};

public:
	AddrSpecParser(QSTATUS* pstatus);
	~AddrSpecParser();

public:
	QSTATUS parseAddrSpec(const Part& part, Tokenizer& t, State state,
		Type type, STRING* pstrMailbox, STRING* pstrHost,
		STRING* pstrComment, Part::Field* pField, bool* pbEnd) const;

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
