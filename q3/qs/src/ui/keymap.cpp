/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaccelerator.h>
#include <qsinit.h>
#include <qskeymap.h>
#include <qslog.h>
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
	
	bool load(InputStream* pInputStream);
	void clear();
	void loadAccelerator(const WCHAR* pwszName,
						 const KeyNameToId* pKeyNameToId,
						 int nMapSize,
						 AccelList* pListAccel) const;
	void getAccel(const WCHAR* pwszAccel,
				  unsigned int nCommand,
				  AccelList* pListAccel) const;
	
	typedef std::vector<std::pair<std::pair<WSTRING, WSTRING>, ACCEL> > AccelMap;
	AccelMap mapAccel_;
};

bool qs::KeyMapImpl::load(InputStream* pInputStream)
{
	XMLReader reader;
	KeyMapContentHandler handler(&mapAccel_);
	reader.setContentHandler(&handler);
	InputSource source(pInputStream);
	if (!reader.parse(&source)) {
		clear();
		return false;
	}
	return true;
}

void qs::KeyMapImpl::clear()
{
	std::for_each(mapAccel_.begin(), mapAccel_.end(),
		unary_compose_f_gx(
			unary_compose_fx_gx(
				string_free<WSTRING>(),
				string_free<WSTRING>()),
			std::select1st<AccelMap::value_type>()));
	mapAccel_.clear();
}

void qs::KeyMapImpl::loadAccelerator(const WCHAR* pwszName,
									 const KeyNameToId* pKeyNameToId,
									 int nMapSize,
									 AccelList* pListAccel) const
{
	assert(pwszName);
	assert(pKeyNameToId);
	assert(pListAccel);
	
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
			pListAccel->push_back(accel);
			bAdd = true;
			++it;
		}
		if (!bAdd && pKeyNameToId->pwszDefault_)
			getAccel(pKeyNameToId->pwszDefault_,
				pKeyNameToId->nId_, pListAccel);
	}
}

void qs::KeyMapImpl::getAccel(const WCHAR* pwszAccel,
							  unsigned int nCommand,
							  AccelList* pListAccel) const
{
	assert(pwszAccel);
	assert(pListAccel);
	
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
				if (accel.key)
					pListAccel->push_back(accel);
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
}


/****************************************************************************
 *
 * KeyMap
 *
 */

qs::KeyMap::KeyMap(const WCHAR* pwszPath) :
	pImpl_(0)
{
	assert(pwszPath);
	
	pImpl_ = new KeyMapImpl();
	
	FileInputStream fileStream(pwszPath);
	if (!fileStream)
		return;
	BufferedInputStream stream(&fileStream, false);
	
	if (!pImpl_->load(&stream)) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::KeyMap");
		log.error(L"Could not load keymap.");
	}
}

qs::KeyMap::KeyMap(InputStream* pInputStream) :
	pImpl_(0)
{
	assert(pInputStream);
	
	pImpl_ = new KeyMapImpl();
	
	if (!pImpl_->load(pInputStream)) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::KeyMap");
		log.error(L"Could not load keymap.");
	}
}

qs::KeyMap::~KeyMap()
{
	if (pImpl_) {
		pImpl_->clear();
		delete pImpl_;
	}
}

std::auto_ptr<Accelerator> qs::KeyMap::createAccelerator(AcceleratorFactory* pFactory,
														 const WCHAR* pwszName,
														 const KeyNameToId* pKeyNameToId,
														 int nMapSize) const
{
	assert(pFactory);
	assert(pwszName);
	assert(pKeyNameToId);
	
	KeyMapImpl::AccelList listAccel;
	pImpl_->loadAccelerator(pwszName, pKeyNameToId, nMapSize, &listAccel);
	return pFactory->createAccelerator(listAccel.begin(), listAccel.size());
}


/****************************************************************************
 *
 * KeyMapContentHandler
 *
 */

qs::KeyMapContentHandler::KeyMapContentHandler(AccelMap* pMapAccel) :
	pMapAccel_(pMapAccel),
	state_(STATE_ROOT)
{
}

qs::KeyMapContentHandler::~KeyMapContentHandler()
{
}

bool qs::KeyMapContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											const WCHAR* pwszLocalName,
											const WCHAR* pwszQName,
											const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"keymaps") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_KEYMAPS;
	}
	else if (wcscmp(pwszLocalName, L"keymap") == 0) {
		if (state_ != STATE_KEYMAPS)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			if (wcscmp(attributes.getLocalName(n), L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!wstrCurrentName_.get());
		wstrCurrentName_ = allocWString(pwszName);
		
		state_ = STATE_KEYMAP;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		if (state_ != STATE_KEYMAP)
			return false;
		
		const WCHAR* pwszAction = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			if (wcscmp(attributes.getLocalName(n), L"name") == 0)
				pwszAction = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszAction)
			return false;
		
		assert(!wstrCurrentAction_.get());
		wstrCurrentAction_ = allocWString(pwszAction);
		
		state_ = STATE_ACTION;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		if (state_ != STATE_ACTION)
			return false;
		
		ACCEL accel = { FVIRTKEY | FNOINVERT, 0, 0 };
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			const WCHAR* pwszValue = attributes.getValue(n);
			if (wcscmp(pwszAttrName, L"key") == 0) {
				if (accel.key != 0)
					return false;
				if (wcslen(pwszValue) != 1)
					return false;
				accel.key = *pwszValue;
			}
			else if (wcscmp(pwszAttrName, L"code") == 0) {
				if (accel.key != 0)
					return false;
				WCHAR* pEnd = 0;
				accel.key = static_cast<WORD>(wcstol(pwszValue, &pEnd, 16));
				if (*pEnd)
					return false;
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
				return false;
			}
		}
		if (accel.key == 0)
			return false;
		
		assert(wstrCurrentName_.get());
		wstring_ptr wstrName(allocWString(wstrCurrentName_.get()));
		assert(wstrCurrentAction_.get());
		wstring_ptr wstrAction(allocWString(wstrCurrentAction_.get()));
		
		AccelMap::value_type value(std::make_pair(
			std::make_pair(wstrName.get(), wstrAction.get()), accel));
		AccelMap::iterator it = std::lower_bound(
			pMapAccel_->begin(), pMapAccel_->end(), value,
			binary_compose_f_gx_hy(
				pair_less(string_less<WCHAR>(), string_less<WCHAR>()),
				std::select1st<AccelMap::value_type>(),
				std::select1st<AccelMap::value_type>()));
		pMapAccel_->insert(it, value);
		wstrName.release();
		wstrAction.release();
		
		state_ = STATE_KEY;
	}
	else {
		return false;
	}
	
	return true;
}

bool qs::KeyMapContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										  const WCHAR* pwszLocalName,
										  const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"keymaps") == 0) {
		assert(state_ == STATE_KEYMAPS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"keymap") == 0) {
		assert(state_ == STATE_KEYMAP);
		assert(wstrCurrentName_.get());
		wstrCurrentName_.reset(0);
		state_ = STATE_KEYMAPS;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		assert(state_ == STATE_ACTION);
		assert(wstrCurrentAction_.get());
		wstrCurrentAction_.reset(0);
		state_ = STATE_KEYMAP;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		assert(state_ == STATE_KEY);
		state_ = STATE_ACTION;
	}
	else {
		return false;
	}
	
	return true;
}

bool qs::KeyMapContentHandler::characters(const WCHAR* pwsz,
										  size_t nStart,
										  size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return false;
	}
	return true;
}
