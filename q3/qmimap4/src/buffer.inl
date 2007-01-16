/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */


/****************************************************************************
 *
 * Buffer
 *
 */

inline CHAR qmimap4::Buffer::get(size_t n)
{
	return get(n, 0, 0);
}

inline void qmimap4::Buffer::set(size_t n,
								 CHAR c)
{
	assert(n < buf_.getLength());
	buf_.set(n, c);
}

inline const CHAR* qmimap4::Buffer::str() const
{
	return buf_.getCharArray();
}

inline unsigned int qmimap4::Buffer::getError() const
{
	return nError_;
}

inline size_t qmimap4::Buffer::addToken(const CHAR* psz,
										size_t nLen)
{
	listToken_.push_back(std::make_pair(psz, nLen));
	return listToken_.size() - 1;
}

inline const std::pair<const CHAR*, size_t>& qmimap4::Buffer::getToken(size_t nIndex) const
{
	assert(nIndex < listToken_.size());
	return listToken_[nIndex];
}

inline void qmimap4::Buffer::clearTokens()
{
	listToken_.clear();
}


/****************************************************************************
 *
 * TokenValue
 *
 */

inline qmimap4::TokenValue::TokenValue() :
	pBuffer_(0),
	nIndex_(-1)
{
}

inline std::pair<const CHAR*, size_t> qmimap4::TokenValue::get() const
{
	if (!pBuffer_ || nIndex_ == -1)
		return std::pair<const CHAR*, size_t>(0, -1);
	else
		return pBuffer_->getToken(nIndex_);
}

inline void qmimap4::TokenValue::set(Buffer* pBuffer,
									 size_t nIndex)
{
	pBuffer_ = pBuffer;
	nIndex_ = nIndex;
}
