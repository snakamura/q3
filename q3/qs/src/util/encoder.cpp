/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsencoder.h>
#include <qsstring.h>
#include <qsinit.h>
#include <qsstl.h>

#include <vector>
#include <utility>
#include <algorithm>
#ifndef _WIN32_WCE
#	include <cstdio>
#endif

using namespace qs;

namespace qs {
struct EncoderFactoryImpl;
struct Base64EncoderImpl;
struct UuencodeEncoderImpl;
}


/****************************************************************************
 *
 * Encoder
 *
 */

qs::Encoder::~Encoder()
{
}


/****************************************************************************
 *
 * EncoderFactoryImpl
 *
 */

struct qs::EncoderFactoryImpl
{
	typedef std::vector<EncoderFactory*> FactoryMap;
	static FactoryMap* pMap__;
	static class InitializerImpl : public Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual bool init();
		virtual void term();
	} init__;
};

EncoderFactoryImpl::FactoryMap* qs::EncoderFactoryImpl::pMap__;
EncoderFactoryImpl::InitializerImpl qs::EncoderFactoryImpl::init__;

qs::EncoderFactoryImpl::InitializerImpl::InitializerImpl()
{
}

qs::EncoderFactoryImpl::InitializerImpl::~InitializerImpl()
{
}

bool qs::EncoderFactoryImpl::InitializerImpl::init()
{
	EncoderFactoryImpl::pMap__ = new FactoryMap();
	return true;
}

void qs::EncoderFactoryImpl::InitializerImpl::term()
{
	delete EncoderFactoryImpl::pMap__;
	EncoderFactoryImpl::pMap__ = 0;
}


/****************************************************************************
 *
 * EncoderFactory
 *
 */

qs::EncoderFactory::EncoderFactory()
{
}

qs::EncoderFactory::~EncoderFactory()
{
}

std::auto_ptr<Encoder> qs::EncoderFactory::getInstance(const WCHAR* pwszName)
{
	assert(pwszName);
	
	typedef EncoderFactoryImpl::FactoryMap Map;
	Map* pMap = EncoderFactoryImpl::pMap__;
	
	Map::iterator it = pMap->begin();
	while (it != pMap->end()) {
		if (_wcsicmp((*it)->getName(), pwszName) == 0)
			break;
		++it;
	}
	if (it == pMap->end())
		return 0;
	
	return (*it)->createInstance();
}

void qs::EncoderFactory::registerFactory(EncoderFactory* pFactory)
{
	assert(pFactory);
	EncoderFactoryImpl::pMap__->push_back(pFactory);
}

void qs::EncoderFactory::unregisterFactory(EncoderFactory* pFactory)
{
	assert(pFactory);
	
	typedef EncoderFactoryImpl::FactoryMap Map;
	Map* pMap = EncoderFactoryImpl::pMap__;
	
	Map::iterator it = std::remove(pMap->begin(), pMap->end(), pFactory);
	assert(it != pMap->end());
	pMap->erase(it, pMap->end());
}


/****************************************************************************
 *
 * Base64EncoderImpl
 *
 */

struct qs::Base64EncoderImpl
{
	static unsigned char encodeByte(unsigned char c);
	static unsigned char decodeByte(unsigned char c);
	
	static const unsigned char szEncode__[];
};

const unsigned char qs::Base64EncoderImpl::szEncode__[] = {
	L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H',
	L'I', L'J', L'K', L'L', L'M', L'N', L'O', L'P',
	L'Q', L'R', L'S', L'T', L'U', L'V', L'W', L'X',
	L'Y', L'Z', L'a', L'b', L'c', L'd', L'e', L'f',
	L'g', L'h', L'i', L'j', L'k', L'l', L'm', L'n',
	L'o', L'p', L'q', L'r', L's', L't', L'u', L'v',
	L'w', L'x', L'y', L'z', L'0', L'1', L'2', L'3',
	L'4', L'5', L'6', L'7', L'8', L'9', L'+', L'/'
};

inline unsigned char qs::Base64EncoderImpl::encodeByte(unsigned char c)
{
	assert(c < 0x40);
	return szEncode__[c];
}

inline unsigned char qs::Base64EncoderImpl::decodeByte(unsigned char c)
{
	if ('A' <= c && c <= 'Z')
		return c - 'A';
	else if ('a' <= c && c <= 'z')
		return c - 'a' + 26;
	else if ('0' <= c && c <= '9')
		return c - '0' + 52;
	else if (c == '+')
		return 62;
	else if (c == '/')
		return 63;
	else if (c == '=')
		return static_cast<unsigned char>(-2);
	else
		return static_cast<unsigned char>(-1);
}


/****************************************************************************
 *
 * Base64Encoder
 *
 */

qs::Base64Encoder::Base64Encoder(bool bFold) :
	bFold_(bFold)
{
}

qs::Base64Encoder::~Base64Encoder()
{
}

malloc_size_ptr<unsigned char> qs::Base64Encoder::encode(const unsigned char* pSrc,
														 size_t nSrcLen)
{
	assert(pSrc);
	
	size_t nLen = (nSrcLen/3 + (nSrcLen % 3 ? 1 : 0))*4;
	if (bFold_)
		nLen += (nLen/FOLD_LENGTH)*2;
	malloc_ptr<unsigned char> pDst(static_cast<unsigned char*>(malloc(nLen)));
	if (!pDst.get())
		return malloc_size_ptr<unsigned char>();
	size_t nDstLen = 0;
	encode(pSrc, nSrcLen, bFold_, pDst.get(), &nDstLen);
	assert(nDstLen == nLen);
	
	return malloc_size_ptr<unsigned char>(pDst, nDstLen);
}

malloc_size_ptr<unsigned char> qs::Base64Encoder::decode(const unsigned char* pSrc,
														 size_t nSrcLen)
{
	assert(pSrc);
	
	size_t nLen = (nSrcLen/4 + (nSrcLen % 4 ? 1 : 0))*3;
	malloc_ptr<unsigned char> pDst(static_cast<unsigned char*>(malloc(nLen)));
	if (!pDst.get())
		return malloc_size_ptr<unsigned char>();
	
	unsigned long nDecode = 0;
	unsigned char* p = pDst.get();
	int nCounter = 0;
	int nDelete = 0;
	for (size_t n = 0; n < nSrcLen; ++n) {
		unsigned char nCode = Base64EncoderImpl::decodeByte(pSrc[n]);
		if (nCode == static_cast<unsigned char>(-2)) {
			nCode = 0;
			++nDelete;
		}
		if (nCode != static_cast<unsigned char>(-1)) {
			nDecode |= nCode << (3 - nCounter)*6;
			++nCounter;
		}
		
		if (nCounter == 4) {
			if (n != 0) {
				if (nDelete < 3)
					*p++ = static_cast<unsigned char>((nDecode >> 16) & 0xff);
				if (nDelete < 2)
					*p++ = static_cast<unsigned char>((nDecode >> 8) & 0xff);
				if (nDelete < 1)
					*p++ = static_cast<unsigned char>(nDecode & 0xff);
			}
			nDecode = 0;
			nCounter = 0;
		}
	}
	
	return malloc_size_ptr<unsigned char>(pDst, p - pDst.get());
}

void qs::Base64Encoder::encode(const unsigned char* pSrc,
							   size_t nSrcLen,
							   bool bFold,
							   unsigned char* pDst,
							   size_t* pnDstLen)
{
	assert(pSrc);
	assert(pDst);
	assert(pnDstLen);
	
	unsigned char* pDstOrg = pDst;
	
	unsigned long nEncode = 0;
	int nBlock = 0;
	for (size_t n = 0; n < nSrcLen + (3 - (nSrcLen%3 ? nSrcLen%3 : 3)); ++n) {
		nEncode |= (n < nSrcLen ? pSrc[n] : 0) << (2 - n%3)*8;
		if (n % 3 == 2) {
			*pDst++ = Base64EncoderImpl::encodeByte(
				static_cast<unsigned char>((nEncode >> 18) & 0x3f));
			*pDst++ = Base64EncoderImpl::encodeByte(
				static_cast<unsigned char>((nEncode >> 12) & 0x3f));
			*pDst++ = (n < nSrcLen + 1) ?
				Base64EncoderImpl::encodeByte(
					static_cast<unsigned char>((nEncode >> 6) & 0x3f)) :
				'=';
			*pDst++ = (n < nSrcLen) ?
				Base64EncoderImpl::encodeByte(
					static_cast<unsigned char>(nEncode & 0x3f)) :
				'=';
			nEncode = 0;
			
			if (bFold) {
				if (++nBlock == FOLD_LENGTH/4) {
					*pDst++ = '\r';
					*pDst++ = '\n';
					nBlock = 0;
				}
			}
		}
	}
	*pnDstLen = pDst - pDstOrg;
}


/****************************************************************************
 *
 * Base64EncoderFactory
 *
 */

qs::Base64EncoderFactory::Base64EncoderFactory()
{
	registerFactory(this);
}

qs::Base64EncoderFactory::~Base64EncoderFactory()
{
	unregisterFactory(this);
}

const WCHAR* qs::Base64EncoderFactory::getName() const
{
	return L"base64";
}

std::auto_ptr<Encoder> qs::Base64EncoderFactory::createInstance()
{
	return new Base64Encoder(true);
}


/****************************************************************************
 *
 * BEncoderFactory
 *
 */

qs::BEncoderFactory::BEncoderFactory()
{
	registerFactory(this);
}

qs::BEncoderFactory::~BEncoderFactory()
{
	unregisterFactory(this);
}

const WCHAR* qs::BEncoderFactory::getName() const
{
	return L"b";
}

std::auto_ptr<Encoder> qs::BEncoderFactory::createInstance()
{
	return new Base64Encoder(false);
}


/****************************************************************************
 *
 * QuotedPrintableEncoder
 *
 */

qs::QuotedPrintableEncoder::QuotedPrintableEncoder(bool bQ) :
	bQ_(bQ)
{
}

qs::QuotedPrintableEncoder::~QuotedPrintableEncoder()
{
}

malloc_size_ptr<unsigned char> qs::QuotedPrintableEncoder::encode(const unsigned char* pSrc,
																  size_t nSrcLen)
{
	assert(pSrc);
	
	malloc_ptr<unsigned char> pDst(static_cast<unsigned char*>(malloc(nSrcLen)));
	if (!pDst.get())
		return malloc_size_ptr<unsigned char>();
	unsigned char* pDstEnd = pDst.get() + nSrcLen;
	unsigned char* p = pDst.get();
	malloc_ptr<unsigned char> pSpace;
	size_t nSpaceBufSize = 0;
	size_t nSpaceLen = 0;
	int nLine = 0;
	for (size_t n = 0; n < nSrcLen; ++n) {
		if (static_cast<size_t>(pDstEnd - p) < (1 + nSpaceLen)*3 + 10) {
			size_t nNewSize = (pDstEnd - pDst.get())*2;
			malloc_ptr<unsigned char> pNew(static_cast<unsigned char*>(
				realloc(pDst.get(), nNewSize)));
			if (!pNew.get())
				return malloc_size_ptr<unsigned char>();
			pDstEnd = pNew.get() + nNewSize;
			p = pNew.get() + (p - pDst.get());
			pDst.release();
			pDst = pNew;
		}
		
		if (nLine + nSpaceLen*3 >= 72) {
			assert(pDstEnd - p >= 3);
			memcpy(p, "=\n", 3);
			p += 3;
			nLine = 0;
		}
		
		unsigned char c = pSrc[n];
		if ((33 <= c && c <= 60) || (62 <= c && c <= 126)) {
			if (nSpaceLen != 0) {
				assert(static_cast<size_t>(pDstEnd - p) >= nSpaceLen);
				memcpy(p, pSpace.get(), nSpaceLen);
				p += nSpaceLen;
				nLine += nSpaceLen;
				nSpaceLen = 0;
			}
			assert(pDstEnd - p >= 1);
			*p++ = c;
			++nLine;
		}
		else if ((!bQ_ && c == '\t') || c == ' ') {
			if (bQ_) {
				assert(c == ' ');
				assert(pDstEnd - p >= 1);
				*p++ = '_';
			}
			else {
				if (nSpaceLen == nSpaceBufSize) {
					nSpaceBufSize = nSpaceBufSize == 0 ? 10 : nSpaceBufSize*2;
					malloc_ptr<unsigned char> pNew(static_cast<unsigned char*>(
						realloc(pSpace.get(), nSpaceBufSize)));
					if (!pNew.get())
						return malloc_size_ptr<unsigned char>();
					pSpace.release();
					pSpace =  pNew;
				}
				*(pSpace.get() + nSpaceLen++) = c;
			}
		}
		else if (c == '\r') {
		}
		else if (c == '\n') {
			if (nSpaceLen != 0) {
				assert(static_cast<size_t>(pDstEnd - p) >= nSpaceLen*3);
				for (size_t nSpace = 0; nSpace < nSpaceLen; ++nSpace) {
					if (pSpace[nSpace] == '\t')
						memcpy(p, "=09", 3);
					else if (pSpace[nSpace] = ' ')
						memcpy(p, "=20", 3);
					p += 3;
					nLine += 3;
				}
				nSpaceLen = 0;
			}
			assert(pDstEnd - p >= 1);
			*p++ = c;
			nLine = 0;
		}
		else {
			if (nSpaceLen != 0) {
				assert(static_cast<size_t>(pDstEnd - p) >= nSpaceLen);
				memcpy(p, pSpace.get(), nSpaceLen);
				p += nSpaceLen;
				nLine += nSpaceLen;
				nSpaceLen = 0;
			}
			assert(pDstEnd - p >= 3);
			sprintf(reinterpret_cast<char*>(p), "=%02X", c);
			p += 3;
			nLine += 3;
		}
	}
	
	return malloc_size_ptr<unsigned char>(pDst, p - pDst.get());
}

malloc_size_ptr<unsigned char> qs::QuotedPrintableEncoder::decode(const unsigned char* pSrc,
																  size_t nSrcLen)
{
	assert(pSrc);
	
	malloc_ptr<unsigned char> pDst(static_cast<unsigned char*>(malloc(nSrcLen)));
	if (!pDst.get())
		return malloc_size_ptr<unsigned char>();
	unsigned char* p = pDst.get();
	
	for (size_t n = 0; n < nSrcLen; ++n) {
		unsigned char c = pSrc[n];
		if (c == '=') {
			if (n + 1 < nSrcLen && pSrc[n + 1] == '\n') {
				++n;
			}
			else if (n + 1 < nSrcLen && pSrc[n + 1] == '\r') {
				if (n + 2 < nSrcLen && pSrc[n + 2] == '\n')
					n += 2;
				else
					++n;
			}
			else if (n + 2 < nSrcLen) {
				unsigned char b = 0;
				for (int m = 0; m < 2; m++) {
					unsigned char cTemp = pSrc[n + m + 1];
					if ('0' <= cTemp && cTemp <= '9')
						cTemp = cTemp - '0';
					else if ('A' <= cTemp && cTemp <= 'F')
						cTemp = cTemp - 'A' + 10;
					else if ('a' <= cTemp && cTemp <= 'f')
						cTemp = cTemp - 'a' + 10;
					b += cTemp << (4*(1 - m));
				}
				*p++ = b;
				n += 2;
			}
			else {
				*p++ = c;
			}
		}
		else if (bQ_ && c == '_') {
			*p++ = ' ';
		}
		else {
			*p ++ = c;
		}
	}
	
	return malloc_size_ptr<unsigned char>(pDst, p - pDst.get());
}


/****************************************************************************
 *
 * QuotedPrintableEncoderFactory
 *
 */

qs::QuotedPrintableEncoderFactory::QuotedPrintableEncoderFactory()
{
	registerFactory(this);
}

qs::QuotedPrintableEncoderFactory::~QuotedPrintableEncoderFactory()
{
	unregisterFactory(this);
}

const WCHAR* qs::QuotedPrintableEncoderFactory::getName() const
{
	return L"quoted-printable";
}

std::auto_ptr<Encoder> qs::QuotedPrintableEncoderFactory::createInstance()
{
	return new QuotedPrintableEncoder(false);
}


/****************************************************************************
 *
 * QEncoderFactory
 *
 */

qs::QEncoderFactory::QEncoderFactory()
{
	registerFactory(this);
}

qs::QEncoderFactory::~QEncoderFactory()
{
	unregisterFactory(this);
}

const WCHAR* qs::QEncoderFactory::getName() const
{
	return L"q";
}

std::auto_ptr<Encoder> qs::QEncoderFactory::createInstance()
{
	return new QuotedPrintableEncoder(true);
}


/****************************************************************************
 *
 * UuencodeEncoderImpl
 *
 */

struct qs::UuencodeEncoderImpl
{
	static unsigned char decodeChar(unsigned char c);
	static bool checkChar(unsigned char c);
	static unsigned char getChar(const unsigned char* pBegin,
								 const unsigned char* pEnd,
								 const unsigned char* p);
	static const unsigned char* find(const unsigned char* p,
									 const char* pFind);
};

inline unsigned char qs::UuencodeEncoderImpl::decodeChar(unsigned char c)
{
	assert(checkChar(c));
	return c == '`' ? 0x00 : c - 0x20;
}

inline bool qs::UuencodeEncoderImpl::checkChar(unsigned char c)
{
	return 0x20 <= c && c <= 0x60;
}

inline unsigned char qs::UuencodeEncoderImpl::getChar(const unsigned char* pBegin,
													  const unsigned char* pEnd,
													  const unsigned char* p)
{
	assert(pBegin <= p && p < pBegin + 4);
	return p < pEnd ? *p : '`';
}

const unsigned char* qs::UuencodeEncoderImpl::find(const unsigned char* p,
												   const char* pFind)
{
	return reinterpret_cast<const unsigned char*>(strstr(
		reinterpret_cast<const char*>(p), pFind));
}


/****************************************************************************
 *
 * UuencodeEncoder
 *
 */

qs::UuencodeEncoder::UuencodeEncoder()
{
}

qs::UuencodeEncoder::~UuencodeEncoder()
{
}

malloc_size_ptr<unsigned char> qs::UuencodeEncoder::encode(const unsigned char* pSrc,
														   size_t nSrcLen)
{
	assert(false);
	return malloc_size_ptr<unsigned char>();
}

malloc_size_ptr<unsigned char> qs::UuencodeEncoder::decode(const unsigned char* pSrc,
														   size_t nSrcLen)
{
	assert(pSrc);
	
	const unsigned char* pBegin = pSrc;
	const unsigned char* pEnd = UuencodeEncoderImpl::find(pBegin, "\r\n");
	if (!pEnd)
		return malloc_size_ptr<unsigned char>();
	
	while (pEnd && strncmp(reinterpret_cast<const char*>(pBegin), "begin ", 6) != 0) {
		pBegin = pEnd + 2;
		pEnd = UuencodeEncoderImpl::find(pBegin, "\r\n");
	}
	if (!pEnd)
		return malloc_size_ptr<unsigned char>();
	
	pBegin = pEnd + 2;
	pEnd = UuencodeEncoderImpl::find(pBegin, "\r\n");
	if (!pEnd)
		return malloc_size_ptr<unsigned char>();
	
	size_t nLen = (nSrcLen/4 + (nSrcLen % 4 ? 1 : 0))*3;
	malloc_ptr<unsigned char> pDst(static_cast<unsigned char*>(malloc(nLen)));
	if (!pDst.get())
		return malloc_size_ptr<unsigned char>();
	
	unsigned char* p = pDst.get();
	bool bEnd = false;
	while (pEnd && !bEnd) {
		if (!UuencodeEncoderImpl::checkChar(*pBegin))
			return malloc_size_ptr<unsigned char>();
		
		int nLen = UuencodeEncoderImpl::decodeChar(*pBegin);
		bEnd = nLen == 0;
		if (nLen != 0) {
			int nLineLen = (nLen/3 + (nLen % 3 ? 1 : 0))*4;
			if (pEnd - pBegin - 1 <= nLineLen - 4 || nLineLen < pEnd - pBegin - 1)
				return malloc_size_ptr<unsigned char>();
			
			for (++pBegin; pBegin < pEnd; pBegin += 4) {
				char c[4] = {
					UuencodeEncoderImpl::getChar(pBegin, pEnd, pBegin),
					UuencodeEncoderImpl::getChar(pBegin, pEnd, pBegin + 1),
					UuencodeEncoderImpl::getChar(pBegin, pEnd, pBegin + 2),
					UuencodeEncoderImpl::getChar(pBegin, pEnd, pBegin + 3)
				};
				if (!UuencodeEncoderImpl::checkChar(c[0]) ||
					!UuencodeEncoderImpl::checkChar(c[1]) ||
					!UuencodeEncoderImpl::checkChar(c[2]) ||
					!UuencodeEncoderImpl::checkChar(c[3]))
					return malloc_size_ptr<unsigned char>();
				unsigned long nDecode =
					static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[0]) << 18) +
					static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[1]) << 12) +
					static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[2]) << 6) +
					static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[3]));
				if (nLen-- > 0)
					*p++ = static_cast<unsigned char>((nDecode >> 16) & 0xff);
				if (nLen-- > 0)
					*p++ = static_cast<unsigned char>((nDecode >> 8) & 0xff);
				if (nLen-- > 0)
					*p++ = static_cast<unsigned char>(nDecode & 0xff);
			}
		}
		pBegin = pEnd + 2;
		pEnd = UuencodeEncoderImpl::find(pBegin, "\r\n");
	}
	
	while (pEnd && strncmp(reinterpret_cast<const char*>(pBegin), "end", 3) != 0) {
		if (pBegin != pEnd)
			return malloc_size_ptr<unsigned char>();
		pBegin = pEnd + 2;
		pEnd = UuencodeEncoderImpl::find(pBegin, "\r\n");
	}
	if (!pEnd)
		return malloc_size_ptr<unsigned char>();
	
	return malloc_size_ptr<unsigned char>(pDst, p - pDst.get());
}


/****************************************************************************
 *
 * UuencodeEncoderFactory
 *
 */

qs::UuencodeEncoderFactory::UuencodeEncoderFactory()
{
	registerFactory(this);
}

qs::UuencodeEncoderFactory::~UuencodeEncoderFactory()
{
	unregisterFactory(this);
}

const WCHAR* qs::UuencodeEncoderFactory::getName() const
{
	return L"uuencode";
}

std::auto_ptr<Encoder> qs::UuencodeEncoderFactory::createInstance()
{
	return new UuencodeEncoder();
}


/****************************************************************************
 *
 * XUuencodeEncoderFactory
 *
 */

qs::XUuencodeEncoderFactory::XUuencodeEncoderFactory()
{
	registerFactory(this);
}

qs::XUuencodeEncoderFactory::~XUuencodeEncoderFactory()
{
	unregisterFactory(this);
}

const WCHAR* qs::XUuencodeEncoderFactory::getName() const
{
	return L"x-uuencode";
}

std::auto_ptr<Encoder> qs::XUuencodeEncoderFactory::createInstance()
{
	return new UuencodeEncoder();
}
