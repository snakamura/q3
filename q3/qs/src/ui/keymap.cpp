/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaccelerator.h>
#include <qserror.h>
#include <qskeymap.h>
#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>
#include <vector>

#include <windows.h>

#include "keymap.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * KeyMapImpl
 *
 */

struct qs::KeyMapImpl
{
	typedef std::vector<ACCEL> AccelList;
	
	QSTATUS load(InputStream* pInputStream);
	QSTATUS loadAccelerator(const WCHAR* pwszName,
		const KeyNameToId* pKeyNameToId, int nMapSize,
		AccelList* pListAccel) const;
	QSTATUS getAccel(const WCHAR* pwszAccel,
		unsigned int nCommand, AccelList* pListAccel) const;
	
	typedef std::vector<std::pair<std::pair<WSTRING, WSTRING>, ACCEL> > AccelMap;
	AccelMap mapAccel_;
};

QSTATUS qs::KeyMapImpl::load(InputStream* pInputStream)
{
	DECLARE_QSTATUS();
	
	XMLReader reader(&status);
	CHECK_QSTATUS();
	KeyMapContentHandler handler(&mapAccel_, &status);
	CHECK_QSTATUS();
	reader.setContentHandler(&handler);
	InputSource source(pInputStream, &status);
	CHECK_QSTATUS();
	status = reader.parse(&source);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::KeyMapImpl::loadAccelerator(const WCHAR* pwszName,
	const KeyNameToId* pKeyNameToId, int nMapSize, AccelList* pListAccel) const
{
	assert(pwszName);
	assert(pKeyNameToId);
	assert(pListAccel);
	
	DECLARE_QSTATUS();
	
	for (int n = 0; n < nMapSize; ++n, ++pKeyNameToId) {
		bool bAdd = false;
		ACCEL accel = { 0, 0, 0 };
		AccelMap::value_type value(std::make_pair(
			std::make_pair(const_cast<WSTRING>(pwszName),
				const_cast<WSTRING>(pKeyNameToId->pwszName_)),
			accel));
		AccelMap::const_iterator it = std::lower_bound(
			mapAccel_.begin(), mapAccel_.end(), value,
			binary_compose_f_gx_hy(
				pair_less(string_less<WCHAR>(), string_less<WCHAR>()),
				std::select1st<AccelMap::value_type>(),
				std::select1st<AccelMap::value_type>()));
		while (it != mapAccel_.end() &&
			wcscmp((*it).first.first, value.first.first) == 0 &&
			wcscmp((*it).first.second, value.first.second) == 0) {
			ACCEL accel = (*it).second;
			accel.cmd = pKeyNameToId->nId_;
			status = STLWrapper<AccelList>(*pListAccel).push_back(accel);
			CHECK_QSTATUS();
			bAdd = true;
			++it;
		}
		if (!bAdd && pKeyNameToId->pwszDefault_) {
			status = getAccel(pKeyNameToId->pwszDefault_,
				pKeyNameToId->nId_, pListAccel);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::KeyMapImpl::getAccel(const WCHAR* pwszAccel,
	unsigned int nCommand, AccelList* pListAccel) const
{
	assert(pwszAccel);
	assert(pListAccel);
	
	DECLARE_QSTATUS();
	
	STLWrapper<AccelList> wrapper(*pListAccel);
	bool bEscape = false;
	ACCEL accel = { FNOINVERT | FVIRTKEY, 0, nCommand };
	const WCHAR* p = pwszAccel;
	do {
		WCHAR c = *p;
		if (bEscape) {
			accel.key = c;
			bEscape = false;
		}
		else {
			if (c == L',' || c == L'\0') {
				if (accel.key) {
					status = wrapper.push_back(accel);
					CHECK_QSTATUS();
				}
				accel.fVirt = FNOINVERT | FVIRTKEY;
				accel.key = 0;
			}
			else if (c == L'\\') {
				bEscape = true;
			}
			else if (c == L'^') {
				accel.fVirt |= FCONTROL;
			}
			else if (c == L'+') {
				accel.fVirt |= FSHIFT;
			}
			else if (c == L'@') {
				accel.fVirt |= FALT;
			}
			else if (c == L'_') {
				accel.fVirt &= ~FVIRTKEY;
			}
			else if (c == L'%') {
				WCHAR* pEnd = 0;
				accel.key = static_cast<WORD>(wcstol(p + 1, &pEnd, 16));
				p = pEnd - 1;
			}
			else {
				accel.key = c;
			}
		}
	} while (*p++ != L'\0');
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * KeyMap
 *
 */

qs::KeyMap::KeyMap(const WCHAR* pwszPath, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	FileInputStream fileStream(pwszPath, &status);
	CHECK_QSTATUS_SET(pstatus);
	BufferedInputStream stream(&fileStream, false, &status);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pImpl_->load(&stream);
	CHECK_QSTATUS_SET(pstatus);
}

qs::KeyMap::KeyMap(InputStream* pInputStream, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pInputStream);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pImpl_->load(pInputStream);
	CHECK_QSTATUS_SET(pstatus);
}

qs::KeyMap::~KeyMap()
{
	if (pImpl_) {
		std::for_each(pImpl_->mapAccel_.begin(), pImpl_->mapAccel_.end(),
			unary_compose_f_gx(
				unary_compose_fx_gx(
					string_free<WSTRING>(),
					string_free<WSTRING>()),
				std::select1st<KeyMapImpl::AccelMap::value_type>()));
		
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::KeyMap::createAccelerator(AcceleratorFactory& factory,
	const WCHAR* pwszName, const KeyNameToId* pKeyNameToId, int nMapSize,
	Accelerator** ppAccelerator) const
{
	DECLARE_QSTATUS();
	
	KeyMapImpl::AccelList listAccel;
	status = pImpl_->loadAccelerator(pwszName,
		pKeyNameToId, nMapSize, &listAccel);
	CHECK_QSTATUS();
	
	return factory.createAccelerator(
		listAccel.begin(), listAccel.size(), ppAccelerator);
}


/****************************************************************************
 *
 * KeyMapContentHandler
 *
 */

qs::KeyMapContentHandler::KeyMapContentHandler(
	AccelMap* pMapAccel, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pMapAccel_(pMapAccel),
	state_(STATE_ROOT),
	wstrCurrentName_(0),
	wstrCurrentAction_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qs::KeyMapContentHandler::~KeyMapContentHandler()
{
	freeWString(wstrCurrentName_);
	freeWString(wstrCurrentAction_);
}

QSTATUS qs::KeyMapContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"keymaps") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		state_ = STATE_KEYMAPS;
	}
	else if (wcscmp(pwszLocalName, L"keymap") == 0) {
		if (state_ != STATE_KEYMAPS)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			if (wcscmp(attributes.getLocalName(n), L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		assert(!wstrCurrentName_);
		wstrCurrentName_ = allocWString(pwszName);
		if (!wstrCurrentName_)
			return QSTATUS_OUTOFMEMORY;
		
		state_ = STATE_KEYMAP;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		if (state_ != STATE_KEYMAP)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszAction = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			if (wcscmp(attributes.getLocalName(n), L"name") == 0)
				pwszAction = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszAction)
			return QSTATUS_FAIL;
		
		assert(!wstrCurrentAction_);
		wstrCurrentAction_ = allocWString(pwszAction);
		if (!wstrCurrentAction_)
			return QSTATUS_OUTOFMEMORY;
		
		state_ = STATE_ACTION;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		if (state_ != STATE_ACTION)
			return QSTATUS_FAIL;
		
		ACCEL accel = { FVIRTKEY | FNOINVERT, 0, 0 };
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			const WCHAR* pwszValue = attributes.getValue(n);
			if (wcscmp(pwszAttrName, L"key") == 0) {
				if (accel.key != 0)
					return QSTATUS_FAIL;
				if (wcslen(pwszValue) != 1)
					return QSTATUS_FAIL;
				accel.key = *pwszValue;
			}
			else if (wcscmp(pwszAttrName, L"code") == 0) {
				if (accel.key != 0)
					return QSTATUS_FAIL;
				WCHAR* pEnd = 0;
				accel.key = static_cast<WORD>(wcstol(pwszValue, &pEnd, 16));
				if (*pEnd)
					return QSTATUS_FAIL;
			}
			else if (wcscmp(pwszAttrName, L"shift") == 0) {
				if (wcscmp(pwszValue, L"true") == 0)
					accel.fVirt |= FSHIFT;
			}
			else if (wcscmp(pwszAttrName, L"ctrl") == 0) {
				if (wcscmp(pwszValue, L"true") == 0)
					accel.fVirt |= FCONTROL;
			}
			else if (wcscmp(pwszAttrName, L"alt") == 0) {
				if (wcscmp(pwszValue, L"true") == 0)
					accel.fVirt |= FALT;
			}
			else if (wcscmp(pwszAttrName, L"virtual") == 0) {
				if (wcscmp(pwszValue, L"false") == 0)
					accel.fVirt &= ~FVIRTKEY;
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		if (accel.key == 0)
			return QSTATUS_FAIL;
		
		assert(wstrCurrentName_);
		string_ptr<WSTRING> wstrName(allocWString(wstrCurrentName_));
		if (!wstrName.get())
			return QSTATUS_FAIL;
		assert(wstrCurrentAction_);
		string_ptr<WSTRING> wstrAction(allocWString(wstrCurrentAction_));
		if (!wstrAction.get())
			return QSTATUS_FAIL;
		
		AccelMap::value_type value(std::make_pair(
			std::make_pair(wstrName.get(), wstrAction.get()), accel));
		AccelMap::iterator it = std::lower_bound(
			pMapAccel_->begin(), pMapAccel_->end(), value,
			binary_compose_f_gx_hy(
				pair_less(string_less<WCHAR>(), string_less<WCHAR>()),
				std::select1st<AccelMap::value_type>(),
				std::select1st<AccelMap::value_type>()));
		AccelMap::iterator p;
		status = STLWrapper<AccelMap>(*pMapAccel_).insert(it, value, &p);
		CHECK_QSTATUS();
		wstrName.release();
		wstrAction.release();
		
		state_ = STATE_KEY;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::KeyMapContentHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"keymaps") == 0) {
		assert(state_ == STATE_KEYMAPS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"keymap") == 0) {
		assert(state_ == STATE_KEYMAP);
		assert(wstrCurrentName_);
		freeWString(wstrCurrentName_);
		wstrCurrentName_ = 0;
		state_ = STATE_KEYMAPS;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		assert(state_ == STATE_ACTION);
		assert(wstrCurrentAction_);
		freeWString(wstrCurrentAction_);
		wstrCurrentAction_ = 0;
		state_ = STATE_KEYMAP;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		assert(state_ == STATE_KEY);
		state_ = STATE_ACTION;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::KeyMapContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return QSTATUS_FAIL;
	}
	return QSTATUS_SUCCESS;
}
