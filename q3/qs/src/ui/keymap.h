/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __KEYMAP_H__
#define __KEYMAP_H__

#include <qskeymap.h>
#include <qssax.h>


namespace qs {

/****************************************************************************
 *
 * KeyMapItem
 *
 */

class KeyMapItem
{
public:
	typedef std::vector<ACCEL> AccelList;

public:
	KeyMapItem(const WCHAR* pwszName);
	~KeyMapItem();

public:
	const WCHAR* getName() const;
	const AccelList& getAccelList() const;

public:
	void add(ACCEL accel);

private:
	KeyMapItem(const KeyMapItem&);
	KeyMapItem& operator=(const KeyMapItem&);

private:
	wstring_ptr wstrName_;
	AccelList listAccel_;
};


/****************************************************************************
 *
 * KeyMapContentHandler
 *
 */

class KeyMapContentHandler : public DefaultHandler
{
public:
	typedef std::vector<KeyMapItem*> ItemList;

public:
	KeyMapContentHandler(ItemList* pItemList,
						 const ActionItem* pItem,
						 size_t nItemCount,
						 ActionParamMap* pActionParamMap);
	virtual ~KeyMapContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	const ActionItem* getActionItem(const WCHAR* pwszAction) const;

private:
	KeyMapContentHandler(const KeyMapContentHandler&);
	KeyMapContentHandler& operator=(const KeyMapContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_KEYMAPS,
		STATE_KEYMAP,
		STATE_ACTION,
		STATE_KEY
	};

private:
	ItemList* pItemList_;
	const ActionItem* pActionItem_;
	size_t nActionItemCount_;
	ActionParamMap* pActionParamMap_;
	State state_;
	KeyMapItem* pKeyMapItem_;
	unsigned int nActionId_;
};

}

#endif // __KEYMAP_H__
