/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __COLORS_H__
#define __COLORS_H__

#include <qm.h>

#include <qs.h>
#include <qsregex.h>
#include <qssax.h>
#include <qsstring.h>

#include <vector>

namespace qm {

class ColorManager;
class ColorSet;
class ColorEntry;
class ColorContentHandler;

class Folder;
class Macro;
class MacroContext;
class MacroParser;


/****************************************************************************
 *
 * ColorManager
 *
 */

class ColorManager
{
public:
	explicit ColorManager(qs::QSTATUS* pstatus);
	~ColorManager();

public:
	qs::QSTATUS getColorSet(Folder* pFolder, const ColorSet** ppSet) const;

public:
	qs::QSTATUS addColorSet(ColorSet* pSet);

private:
	qs::QSTATUS load();

private:
	ColorManager(const ColorManager&);
	ColorManager& operator=(const ColorManager&);

private:
	typedef std::vector<ColorSet*> ColorSetList;

private:
	ColorSetList listColorSet_;
};


/****************************************************************************
 *
 * ColorSet
 *
 */

class ColorSet
{
public:
	ColorSet(qs::RegexPattern* pAccountName,
		qs::RegexPattern* pFolderName, qs::QSTATUS* pstatus);
	~ColorSet();

public:
	qs::QSTATUS match(Folder* pFolder, bool* pbMatch) const;
	qs::QSTATUS getColor(MacroContext* pContext, COLORREF* pcr) const;

public:
	qs::QSTATUS addEntry(ColorEntry* pEntry);

private:
	ColorSet(const ColorSet&);
	ColorSet& operator=(const ColorSet&);

private:
	typedef std::vector<ColorEntry*> EntryList;

private:
	qs::RegexPattern* pAccountName_;
	qs::RegexPattern* pFolderName_;
	EntryList listEntry_;
};


/****************************************************************************
 *
 * ColorEntry
 *
 */

class ColorEntry
{
public:
	ColorEntry(Macro* pMacro, const WCHAR* pwszColor, qs::QSTATUS* pstatus);
	~ColorEntry();

public:
	qs::QSTATUS match(MacroContext* pContext, bool* pbMatch) const;
	COLORREF getColor() const;

private:
	ColorEntry(const ColorEntry&);
	ColorEntry& operator=(const ColorEntry&);

private:
	Macro* pMacro_;
	COLORREF cr_;
};


/****************************************************************************
 *
 * ColorContentHandler
 *
 */

class ColorContentHandler : public qs::DefaultHandler
{
public:
	ColorContentHandler(ColorManager* pManager, qs::QSTATUS* pstatus);
	virtual ~ColorContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	ColorContentHandler(const ColorContentHandler&);
	ColorContentHandler& operator=(const ColorContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_COLORS,
		STATE_COLORSET,
		STATE_COLOR
	};

private:
	ColorManager* pManager_;
	State state_;
	ColorSet* pColorSet_;
	Macro* pMacro_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
	MacroParser* pParser_;
};

}

#endif // __COLORS_H__
