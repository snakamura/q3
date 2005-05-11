/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSKEYMAP_H__
#define __QSKEYMAP_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

struct KeyNameToId;
class KeyMap;

class Accelerator;
class AcceleratorFactory;
class InputStream;


/****************************************************************************
 *
 * KeyNameToId
 *
 */

struct QSEXPORTCLASS KeyNameToId
{
	const WCHAR* pwszName_;
	unsigned int nId_;
	const WCHAR* pwszDefault_;
};


/****************************************************************************
 *
 * KeyMap
 *
 */

class QSEXPORTCLASS KeyMap
{
public:
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path to the file which keymap is loaded from.
	 * @exception std::bad_alloc Out of memory.
	 */
	KeyMap(const WCHAR* pwszPath);
	
	/**
	 * Create instance.
	 *
	 * @param pInputStream [in] Stream which keymap is loaded from.
	 * @exception std::bad_alloc Out of memory.
	 */
	KeyMap(InputStream* pInputStream);
	
	~KeyMap();

public:
	/**
	 * Create accelerator.
	 *
	 * @param pFactory [in] Accelerator factory.
	 * @param pwszName [in] Name of keymap.
	 * @param pKeyNameToId [in] Map from key name to accelerator id.
	 * @param nMapSize [in] Size of the map specified by pKeyNameToId.
	 * @return Created accelerator. Can not be null.
	 * @exception std::bad_alloc Out of memory.
	 */
	std::auto_ptr<Accelerator> createAccelerator(AcceleratorFactory* pFactory,
												 const WCHAR* pwszName,
												 const KeyNameToId* pKeyNameToId,
												 int nMapSize) const;

private:
	KeyMap(const KeyMap&);
	KeyMap& operator=(const KeyMap&);

private:
	struct KeyMapImpl* pImpl_;
};

}

#endif // __QSKEYMAP_H__
