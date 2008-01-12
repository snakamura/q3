/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QSKEYMAP_H__
#define __QSKEYMAP_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

class KeyMap;

class Accelerator;
class AcceleratorFactory;
struct ActionItem;
class ActionParamMap;
class InputStream;


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
	KeyMap(const WCHAR* pwszPath,
		   const ActionItem* pItem,
		   size_t nItemCount,
		   ActionParamMap* pActionParamMap);
	
	/**
	 * Create instance.
	 *
	 * @param pInputStream [in] Stream which keymap is loaded from.
	 * @exception std::bad_alloc Out of memory.
	 */
	KeyMap(InputStream* pInputStream,
		   const ActionItem* pItem,
		   size_t nItemCount,
		   ActionParamMap* pActionParamMap);
	
	~KeyMap();

public:
	/**
	 * Create accelerator.
	 *
	 * @param pFactory [in] Accelerator factory.
	 * @param pwszName [in] Name of keymap.
	 * @return Created accelerator. Can not be null.
	 * @exception std::bad_alloc Out of memory.
	 */
	std::auto_ptr<Accelerator> createAccelerator(AcceleratorFactory* pFactory,
												 const WCHAR* pwszName) const;

private:
	KeyMap(const KeyMap&);
	KeyMap& operator=(const KeyMap&);

private:
	struct KeyMapImpl* pImpl_;
};

}

#endif // __QSKEYMAP_H__
