/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	KeyMap(const WCHAR* pwszPath, QSTATUS* pstatus);
	KeyMap(InputStream* pInputStream, QSTATUS* pstatus);
	~KeyMap();

public:
	QSTATUS createAccelerator(AcceleratorFactory& factory,
		const WCHAR* pwszName, const KeyNameToId* pKeyNameToId,
		int nMapSize, Accelerator** ppAccelerator) const;

private:
	KeyMap(const KeyMap&);
	KeyMap& operator=(const KeyMap&);

private:
	struct KeyMapImpl* pImpl_;
};

}

#endif // __QSKEYMAP_H__
