/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsaccelerator.h>
#include <qsaction.h>
#include <qsinit.h>
#include <qskeymap.h>
#include <qslog.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>
#include <vector>

#include <windows.h>

#include "keymap.h"

using namespace qs;


/****************************************************************************
 *
 * KeyMapImpl
 *
 */

struct qs::KeyMapImpl
{
	bool load(InputStream* pInputStream,
			  const ActionItem* pItem,
			  size_t nItemCount,
			  ActionParamMap* pActionParamMap);
	void clear();
	
	typedef std::vector<KeyMapItem*> ItemList;
	
	ItemList listItem_;
};

bool qs::KeyMapImpl::load(InputStream* pInputStream,
						  const ActionItem* pItem,
						  size_t nItemCount,
						  ActionParamMap* pActionParamMap)
{
	XMLReader reader;
	KeyMapContentHandler handler(&listItem_, pItem, nItemCount, pActionParamMap);
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
	std::for_each(listItem_.begin(), listItem_.end(), deleter<KeyMapItem>());
	listItem_.clear();
}


/****************************************************************************
 *
 * KeyMap
 *
 */

qs::KeyMap::KeyMap(const WCHAR* pwszPath,
				   const ActionItem* pItem,
				   size_t nItemCount,
				   ActionParamMap* pActionParamMap) :
	pImpl_(0)
{
	assert(pwszPath);
	
	pImpl_ = new KeyMapImpl();
	
	FileInputStream fileStream(pwszPath);
	if (!fileStream)
		return;
	BufferedInputStream stream(&fileStream, false);
	
	if (!pImpl_->load(&stream, pItem, nItemCount, pActionParamMap)) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::KeyMap");
		log.error(L"Could not load keymap.");
	}
}

qs::KeyMap::KeyMap(InputStream* pInputStream,
				   const ActionItem* pItem,
				   size_t nItemCount,
				   ActionParamMap* pActionParamMap) :
	pImpl_(0)
{
	assert(pInputStream);
	
	pImpl_ = new KeyMapImpl();
	
	if (!pImpl_->load(pInputStream, pItem, nItemCount, pActionParamMap)) {
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
														 const WCHAR* pwszName) const
{
	assert(pFactory);
	assert(pwszName);
	
	KeyMapImpl::ItemList::const_iterator it = std::find_if(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&KeyMapItem::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	
	KeyMapItem::AccelList listAccel;
	const KeyMapItem::AccelList& l = it != pImpl_->listItem_.end() ?
		(*it)->getAccelList() : listAccel;
	return pFactory->createAccelerator(&l[0], l.size());
}


/****************************************************************************
 *
 * KeyMapItem
 *
 */

qs::KeyMapItem::KeyMapItem(const WCHAR* pwszName)
{
	assert(pwszName);
	
	wstrName_ = allocWString(pwszName);
}

qs::KeyMapItem::~KeyMapItem()
{
}

const WCHAR* qs::KeyMapItem::getName() const
{
	return wstrName_.get();
}

const KeyMapItem::AccelList& qs::KeyMapItem::getAccelList() const
{
	return listAccel_;
}

void qs::KeyMapItem::add(ACCEL accel)
{
	listAccel_.push_back(accel);
}


/****************************************************************************
 *
 * KeyMapContentHandler
 *
 */

qs::KeyMapContentHandler::KeyMapContentHandler(ItemList* pItemList,
											   const ActionItem* pItem,
											   size_t nItemCount,
											   ActionParamMap* pActionParamMap) :
	pItemList_(pItemList),
	pActionItem_(pItem),
	nActionItemCount_(nItemCount),
	pActionParamMap_(pActionParamMap),
	state_(STATE_ROOT),
	pKeyMapItem_(0),
	nActionId_(-1)
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
		
		std::auto_ptr<KeyMapItem> pKeyMapItem(new KeyMapItem(pwszName));
		pItemList_->push_back(pKeyMapItem.get());
		pKeyMapItem_ = pKeyMapItem.release();
		
		state_ = STATE_KEYMAP;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		if (state_ != STATE_KEYMAP)
			return false;
		
		const WCHAR* pwszAction = 0;
		const WCHAR* pwszParam = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			if (wcscmp(attributes.getLocalName(n), L"name") == 0)
				pwszAction = attributes.getValue(n);
			else if (wcscmp(attributes.getLocalName(n), L"param") == 0)
				pwszParam = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszAction)
			return false;
		
		unsigned int nId = -1;
		const ActionItem* pItem = getActionItem(pwszAction);
		if (pItem) {
			nId = pItem->nId_;
			if (pwszParam) {
				std::auto_ptr<ActionParam> pParam(new ActionParam(nId, pwszParam));
				nId = pActionParamMap_->addActionParam(pItem->nMaxParamCount_, pParam);
			}
		}
		nActionId_ = nId;
		
		state_ = STATE_ACTION;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		if (state_ != STATE_ACTION)
			return false;
		
		if (nActionId_ != -1) {
			ACCEL accel = {
				FVIRTKEY | FNOINVERT,
				0,
				static_cast<WORD>(nActionId_)
			};
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
			
			assert(pKeyMapItem_);
			pKeyMapItem_->add(accel);
		}
		
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
		pKeyMapItem_ = 0;
		state_ = STATE_KEYMAPS;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		assert(state_ == STATE_ACTION);
		nActionId_ = -1;
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

const ActionItem* qs::KeyMapContentHandler::getActionItem(const WCHAR* pwszAction) const
{
	ActionItem item = {
		pwszAction,
		0
	};
	
	const ActionItem* pItem = std::lower_bound(
		pActionItem_, pActionItem_ + nActionItemCount_, item,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			mem_data_ref(&ActionItem::pwszAction_),
			mem_data_ref(&ActionItem::pwszAction_)));
	if (pItem == pActionItem_ + nActionItemCount_ ||
		wcscmp(pItem->pwszAction_, pwszAction) != 0 ||
		(pItem->nFlags_ != 0 && !(pItem->nFlags_ & ActionItem::FLAG_ACCELERATOR)))
		return 0;
	return pItem;
}
