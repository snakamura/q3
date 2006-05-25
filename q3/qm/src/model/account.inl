/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACCOUNT_INL__
#define __ACCOUNT_INL__


/****************************************************************************
 *
 * AccountNameEqual
 *
 */

inline qm::AccountNameEqual::AccountNameEqual(const WCHAR* pwszName) :
	pwszName_(pwszName)
{
}

inline bool qm::AccountNameEqual::operator()(const Account* pAccount) const
{
	return wcscmp(pwszName_, pAccount->getName()) == 0;
}


/****************************************************************************
 *
 * AccountEqual
 *
 */

inline bool qm::AccountEqual::operator()(const Account* pLhs,
										 const Account* pRhs) const
{
	return wcscmp(pLhs->getName(), pRhs->getName()) == 0;
}


/****************************************************************************
 *
 * AccountLess
 *
 */

inline bool qm::AccountLess::operator()(const Account* pLhs,
										const Account* pRhs) const
{
	return wcscmp(pLhs->getName(), pRhs->getName()) < 0;
}


/****************************************************************************
 *
 * RemoteFolderLess
 *
 */

inline bool qm::RemoteFolderLess::operator()(const std::pair<Folder*, bool>& lhs,
											 const std::pair<Folder*, bool>& rhs) const
{
	return lhs.first < rhs.first;
}

#endif // __ACCOUNT_INL__
