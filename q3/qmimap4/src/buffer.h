/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <qssocket.h>
#include <qsstring.h>


namespace qmimap4 {

class Buffer;
class TokenValue;

class Imap4Callback;


/****************************************************************************
 *
 * Buffer
 *
 */

class Buffer
{
public:
	Buffer(const CHAR* psz);
	Buffer(const CHAR* psz,
		   qs::SocketBase* pSocket);
	~Buffer();

public:
	CHAR get(size_t n);
	CHAR get(size_t n,
			 Imap4Callback* pCallback,
			 size_t nStart);
	void set(size_t n,
			 CHAR c);
	size_t find(CHAR c,
				size_t n);
	size_t find(const CHAR* psz,
				size_t n);
	qs::string_ptr substr(size_t nPos,
						  size_t nLen) const;
	const CHAR* str() const;
	
	unsigned int getError() const;
	
	size_t addToken(const CHAR* psz,
					size_t nLen);
	const std::pair<const CHAR*, size_t>& getToken(size_t nIndex) const;
	void clearTokens();
	
	size_t free(size_t n);

private:
	bool receive(size_t n,
				 Imap4Callback* pCallback,
				 size_t nStart);
	void adjustTokens(ssize_t nOffset);

private:
	Buffer(const Buffer&);
	Buffer& operator=(const Buffer&);

private:
	typedef std::vector<std::pair<const CHAR*, size_t> > TokenList;

private:
	qs::XStringBuffer<qs::XSTRING> buf_;
	qs::SocketBase* pSocket_;
	unsigned int nError_;
	TokenList listToken_;
};


/****************************************************************************
 *
 * TokenValue
 *
 */

class TokenValue
{
public:
	TokenValue();

public:
	std::pair<const CHAR*, size_t> get() const;

public:
	void set(Buffer* pBuffer,
			 size_t nIndex);

private:
	Buffer* pBuffer_;
	size_t nIndex_;
};


/****************************************************************************
 *
 * TokenUtil
 *
 */

struct TokenUtil
{
	static bool isEqual(const std::pair<const CHAR*, size_t>& s,
						const CHAR* psz);
	static bool isEqualIgnoreCase(const std::pair<const CHAR*, size_t>& s,
								  const CHAR* psz);
	static bool string2number(const std::pair<const CHAR*, size_t>& s,
							  unsigned long* pn);
};

}

#include "buffer.inl"

#endif // __BUFFER_H__
