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
struct QuotedPrintableEncoderImpl;
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

malloc_size_ptr<unsigned char> qs::Encoder::encode(const unsigned char* p,
												   size_t nLen)
												   QNOTHROW()
{
	ByteInputStream is(p, nLen, false);
	ByteOutputStream os;
	if (!os.reserve(getEstimatedEncodeLen(nLen)) ||
		!encodeImpl(&is, &os))
		return malloc_size_ptr<unsigned char>();
	return os.releaseSizeBuffer();
}

malloc_size_ptr<unsigned char> qs::Encoder::decode(const unsigned char* p,
												   size_t nLen)
												   QNOTHROW()
{
	ByteInputStream is(p, nLen, false);
	ByteOutputStream os;
	if (!os.reserve(getEstimatedDecodeLen(nLen)) ||
		!decodeImpl(&is, &os))
		return malloc_size_ptr<unsigned char>();
	return os.releaseSizeBuffer();
}

bool qs::Encoder::encode(InputStream* pInputStream,
						 OutputStream* pOutputStream)
{
	return encodeImpl(pInputStream, pOutputStream);
}

bool qs::Encoder::decode(InputStream* pInputStream,
						 OutputStream* pOutputStream)
{
	return decodeImpl(pInputStream, pOutputStream);
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
		return std::auto_ptr<Encoder>(0);
	
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

void qs::Base64Encoder::encodeBuffer(const unsigned char* pSrc,
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

bool qs::Base64Encoder::isEncodedChar(CHAR c)
{
	const unsigned char* p = Base64EncoderImpl::szEncode__;
	for (int n = 0; n < countof(Base64EncoderImpl::szEncode__); ++n, ++p) {
		if (*p == c)
			return true;
	}
	return false;
}

bool qs::Base64Encoder::encodeImpl(InputStream* pInputStream,
								   OutputStream* pOutputStream)
{
	unsigned char bufIn[FOLD_LENGTH/4*3];
	unsigned char bufOut[FOLD_LENGTH + 2];
	
	while (true) {
		size_t nLen = pInputStream->read(bufIn, sizeof(bufIn));
		if (nLen == -1)
			return false;
		else if (nLen == 0)
			break;
		
		size_t nDstLen = 0;
		encodeBuffer(bufIn, nLen, bFold_, bufOut, &nDstLen);
		if (pOutputStream->write(bufOut, nDstLen) != nDstLen)
			return false;
	}
	
	return true;
}

bool qs::Base64Encoder::decodeImpl(InputStream* pInputStream,
								   OutputStream* pOutputStream)
{
	unsigned long nDecode = 0;
	int nCounter = 0;
	int nDelete = 0;
	while (true) {
		unsigned char c = 0;
		size_t nRead = pInputStream->read(&c, 1);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			break;
		
		unsigned char nCode = Base64EncoderImpl::decodeByte(c);
		if (nCode == unsigned char(-2)) {
			nCode = 0;
			++nDelete;
		}
		if (nCode != unsigned char(-1)) {
			nDecode |= nCode << (3 - nCounter)*6;
			++nCounter;
		}
		
		if (nCounter == 4) {
			if (nDelete < 3) {
				unsigned char c = static_cast<unsigned char>((nDecode >> 16) & 0xff);
				if (pOutputStream->write(&c, 1) != 1)
					return false;
			}
			if (nDelete < 2) {
				unsigned char c = static_cast<unsigned char>((nDecode >> 8) & 0xff);
				if (pOutputStream->write(&c, 1) != 1)
					return false;
			}
			if (nDelete < 1) {
				unsigned char c = static_cast<unsigned char>(nDecode & 0xff);
				if (pOutputStream->write(&c, 1) != 1)
					return false;
			}
			
			nDecode = 0;
			nCounter = 0;
		}
	}
	
	return true;
}

size_t qs::Base64Encoder::getEstimatedEncodeLen(size_t nLen)
{
	return (nLen/3 + 1)*4 + (nLen/45)*2;
}

size_t qs::Base64Encoder::getEstimatedDecodeLen(size_t nLen)
{
	return (nLen/4 + 1)*3;
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
	return std::auto_ptr<Encoder>(new Base64Encoder(true));
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
	return std::auto_ptr<Encoder>(new Base64Encoder(false));
}


/****************************************************************************
 *
 * QuotedPrintableEncoderImpl
 *
 */

struct qs::QuotedPrintableEncoderImpl
{
	static bool isEncodedChar(unsigned char c);
	static unsigned char decode(unsigned char* p);
};


inline bool qs::QuotedPrintableEncoderImpl::isEncodedChar(unsigned char c)
{
	return ('0' <= c && c <= '9') ||
		('a' <= c && c <= 'f') ||
		('A' <= c && c <= 'F');
}

inline unsigned char qs::QuotedPrintableEncoderImpl::decode(unsigned char* p)
{
	unsigned char b = 0;
	for (int m = 0; m < 2; m++, ++p) {
		unsigned char c = *p;
		if ('0' <= c && c <= '9')
			c = c - '0';
		else if ('A' <= c && c <= 'F')
			c = c - 'A' + 10;
		else if ('a' <= c && c <= 'f')
			c = c - 'a' + 10;
		b += c << (4*(1 - m));
	}
	return b;
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

bool qs::QuotedPrintableEncoder::encodeImpl(InputStream* pInputStream,
											OutputStream* pOutputStream)
{
	malloc_ptr<unsigned char> pSpace;
	size_t nSpaceBufSize = 0;
	size_t nSpaceLen = 0;
	int nLine = 0;
	while (true) {
		if (nLine + nSpaceLen*3 >= 72) {
			if (pOutputStream->write(reinterpret_cast<unsigned char*>("=\n"), 3) != 3)
				return false;
			nLine = 0;
		}
		
		unsigned char c = 0;
		size_t nRead = pInputStream->read(&c, 1);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			break;
		
		if ((33 <= c && c <= 60) || (62 <= c && c <= 126)) {
			if (nSpaceLen != 0) {
				if (pOutputStream->write(pSpace.get(), nSpaceLen) != nSpaceLen)
					return false;
				nLine += nSpaceLen;
				nSpaceLen = 0;
			}
			if (pOutputStream->write(&c, 1) != 1)
				return false;
			++nLine;
		}
		else if ((!bQ_ && c == '\t') || c == ' ') {
			if (bQ_) {
				assert(c == ' ');
				unsigned char cEncode = '_';
				if (pOutputStream->write(&cEncode, 1) != 1)
					return false;
			}
			else {
				if (nSpaceLen == nSpaceBufSize) {
					nSpaceBufSize = nSpaceBufSize == 0 ? 10 : nSpaceBufSize*2;
					malloc_ptr<unsigned char> pNew(static_cast<unsigned char*>(
						realloc(pSpace.get(), nSpaceBufSize)));
					if (!pNew.get())
						return false;
					pSpace.release();
					pSpace = pNew;
				}
				*(pSpace.get() + nSpaceLen++) = c;
			}
		}
		else if (c == '\r') {
		}
		else if (c == '\n') {
			if (nSpaceLen != 0) {
				for (size_t nSpace = 0; nSpace < nSpaceLen; ++nSpace) {
					const unsigned char* p = 0;
					if (pSpace[nSpace] == '\t')
						p = reinterpret_cast<const unsigned char*>("=09");
					else
						p = reinterpret_cast<const unsigned char*>("=20");
					if (pOutputStream->write(p, 3) != 3)
						return false;
					nLine += 3;
				}
				nSpaceLen = 0;
			}
			if (pOutputStream->write(&c, 1) != 1)
				return false;
			nLine = 0;
		}
		else {
			if (nSpaceLen != 0) {
				if (pOutputStream->write(pSpace.get(), nSpaceLen) != nSpaceLen)
					return false;
				nLine += nSpaceLen;
				nSpaceLen = 0;
			}
			
			unsigned char buf[3];
			sprintf(reinterpret_cast<char*>(buf), "=%02X", c);
			if (pOutputStream->write(buf, 3) != 3)
				return false;
			nLine += 3;
		}
	}
	
	return true;
}

bool qs::QuotedPrintableEncoder::decodeImpl(InputStream* pInputStream,
											OutputStream* pOutputStream)
{
	malloc_ptr<unsigned char> pPeek;
	size_t nPeekBufSize = 0;
	size_t nPeekLen = 0;
	
	while (true) {
		unsigned char c = 0;
		if (nPeekLen == 0) {
			size_t nRead = pInputStream->read(&c, 1);
			if (nRead == -1)
				return false;
			else if (nRead == 0)
				break;
		}
		else {
			c = *pPeek.get();
			--nPeekLen;
			memmove(pPeek.get() + 1, pPeek.get(), nPeekLen);
		}
		
		if (c == '=') {
			unsigned char buf[2];
			size_t nRead = pInputStream->read(buf, 2);
			if (nRead == -1)
				return false;
			
			size_t nCopy = 2;
			if (nRead > 0 && buf[0] == '\n') {
				nCopy = 1;
			}
			else if (nRead > 0 && buf[0] == '\r') {
				nCopy = nRead > 1 && buf[1] == '\n' ? 0 : 1;
			}
			else if (nRead > 1 &&
				QuotedPrintableEncoderImpl::isEncodedChar(buf[0]) &&
				QuotedPrintableEncoderImpl::isEncodedChar(buf[1])) {
				unsigned char cDecode = QuotedPrintableEncoderImpl::decode(buf);
				if (pOutputStream->write(&cDecode, 1) != 1)
					return false;
				nCopy = 0;
			}
			else {
				if (pOutputStream->write(&c, 1) != 1)
					return false;
			}
			
			if (nCopy > 0) {
				if (nPeekLen + nCopy > nPeekBufSize) {
					nPeekBufSize = nPeekBufSize == 0 ? 10 : nPeekBufSize*2;
					malloc_ptr<unsigned char> pNew(static_cast<unsigned char*>(
						realloc(pPeek.get(), nPeekBufSize)));
					if (!pNew.get())
						return false;
					pPeek.release();
					pPeek = pNew;
				}
				memcpy(pPeek.get() + nPeekLen, buf, nCopy);
				nPeekLen += nCopy;
			}
		}
		else if (bQ_ && c == '_') {
			unsigned char cDecode = ' ';
			if (pOutputStream->write(&cDecode, 1) != 1)
				return false;
		}
		else {
			if (pOutputStream->write(&c, 1) != 1)
				return false;
		}
	}
	
	return true;
}

size_t qs::QuotedPrintableEncoder::getEstimatedEncodeLen(size_t nLen)
{
	return nLen;
}

size_t qs::QuotedPrintableEncoder::getEstimatedDecodeLen(size_t nLen)
{
	return nLen;
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
	return std::auto_ptr<Encoder>(new QuotedPrintableEncoder(false));
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
	return std::auto_ptr<Encoder>(new QuotedPrintableEncoder(true));
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
	static size_t readLine(InputStream* pInputStream,
						   malloc_ptr<unsigned char>* ppBuf,
						   size_t* pnBufSize);
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

inline const unsigned char* qs::UuencodeEncoderImpl::find(const unsigned char* p,
														  const char* pFind)
{
	return reinterpret_cast<const unsigned char*>(strstr(
		reinterpret_cast<const char*>(p), pFind));
}

inline size_t qs::UuencodeEncoderImpl::readLine(InputStream* pInputStream,
												malloc_ptr<unsigned char>* ppBuf,
												size_t* pnBufSize)
{
	unsigned char* p = ppBuf->get();
	while (true) {
		if (static_cast<size_t>(p - ppBuf->get()) >= *pnBufSize) {
			*pnBufSize = *pnBufSize == 0 ? 80 : *pnBufSize*2;
			malloc_ptr<unsigned char> pNew(static_cast<unsigned char*>(
				realloc(ppBuf->get(), *pnBufSize)));
			if (!pNew.get())
				return -1;
			p = pNew.get() + (p - ppBuf->get());
			ppBuf->release();
			*ppBuf = pNew;
		}
		
		size_t nRead = pInputStream->read(p, 1);
		if (nRead == -1)
			return -1;
		else if (nRead == 0)
			return p == ppBuf->get() ? -1 : 0;
		
		if (*p == '\r') {
			while (true) {
				nRead = pInputStream->read(p + 1, 1);
				if (nRead != 1)
					return -1;
				else if (*(p + 1) != '\r')
					break;
				++p;
			}
			if (*(p + 1) == '\n')
				break;
			else
				p += 2;
		}
		else {
			++p;
		}
	}
	return p - ppBuf->get();
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

bool qs::UuencodeEncoder::encodeImpl(InputStream* pInputStream,
									 OutputStream* pOutputStream)
{
	return false;
}

bool qs::UuencodeEncoder::decodeImpl(InputStream* pInputStream,
									 OutputStream* pOutputStream)
{
	malloc_ptr<unsigned char> pBuf;
	size_t nBufSize = 0;
	
	while (true) {
		if (UuencodeEncoderImpl::readLine(pInputStream, &pBuf, &nBufSize) == -1)
			return false;
		if (strncmp(reinterpret_cast<char*>(pBuf.get()), "begin ", 6) == 0)
			break;
	}
	
	while (true) {
		size_t nRead = UuencodeEncoderImpl::readLine(pInputStream, &pBuf, &nBufSize);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			continue;
		
		if (!UuencodeEncoderImpl::checkChar(*pBuf.get()))
			return false;
		
		int nLen = UuencodeEncoderImpl::decodeChar(*pBuf.get());
		if (nLen == 0)
			break;
		
		int nLineLen = (nLen/3 + (nLen % 3 ? 1 : 0))*4;
		if (static_cast<int>(nRead) - 1 <= nLineLen - 4 || nLineLen < static_cast<int>(nRead) - 1)
			return false;
		
		const unsigned char* pEnd = pBuf.get() + nRead;
		for (const unsigned char* p = pBuf.get() + 1; p < pEnd; p += 4) {
			char c[4] = {
				UuencodeEncoderImpl::getChar(p, pEnd, p),
				UuencodeEncoderImpl::getChar(p, pEnd, p + 1),
				UuencodeEncoderImpl::getChar(p, pEnd, p + 2),
				UuencodeEncoderImpl::getChar(p, pEnd, p + 3)
			};
			if (!UuencodeEncoderImpl::checkChar(c[0]) ||
				!UuencodeEncoderImpl::checkChar(c[1]) ||
				!UuencodeEncoderImpl::checkChar(c[2]) ||
				!UuencodeEncoderImpl::checkChar(c[3]))
				return false;
			unsigned long nDecode =
				static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[0]) << 18) +
				static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[1]) << 12) +
				static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[2]) << 6) +
				static_cast<unsigned long>(UuencodeEncoderImpl::decodeChar(c[3]));
			if (nLen-- > 0) {
				unsigned char c = static_cast<unsigned char>((nDecode >> 16) & 0xff);
				if (pOutputStream->write(&c, 1) != 1)
					return false;
			}
			if (nLen-- > 0) {
				unsigned char c = static_cast<unsigned char>((nDecode >> 8) & 0xff);
				if (pOutputStream->write(&c, 1) != 1)
					return false;
			}
			if (nLen-- > 0) {
				unsigned char c = static_cast<unsigned char>(nDecode & 0xff);
				if (pOutputStream->write(&c, 1) != 1)
					return false;
			}
		}
	}
	
	while (strncmp(reinterpret_cast<const char*>(pBuf.get()), "end", 3) != 0) {
		size_t nRead = UuencodeEncoderImpl::readLine(pInputStream, &pBuf, &nBufSize);
		if (nRead == -1)
			return false;
	}
	
	return true;
}

size_t qs::UuencodeEncoder::getEstimatedEncodeLen(size_t nLen)
{
	return 0;
}

size_t qs::UuencodeEncoder::getEstimatedDecodeLen(size_t nLen)
{
	return (nLen/4 + (nLen % 4 ? 1 : 0))*3;
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
	return std::auto_ptr<Encoder>(new UuencodeEncoder());
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
	return std::auto_ptr<Encoder>(new UuencodeEncoder());
}
