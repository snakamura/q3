/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	ColorManager();
	~ColorManager();

public:
	const ColorSet* getColorSet(Folder* pFolder) const;

public:
	void addColorSet(std::auto_ptr<ColorSet> pSet);

private:
	bool load();

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
	ColorSet(std::auto_ptr<qs::RegexPattern> pAccountName,
			 std::auto_ptr<qs::RegexPattern> pFolderName);
	~ColorSet();

public:
	bool match(Folder* pFolder) const;
	COLORREF getColor(MacroContext* pContext) const;

public:
	void addEntry(std::auto_ptr<ColorEntry> pEntry);

private:
	ColorSet(const ColorSet&);
	ColorSet& operator=(const ColorSet&);

private:
	typedef std::vector<ColorEntry*> EntryList;

private:
	std::auto_ptr<qs::RegexPattern> pAccountName_;
	std::auto_ptr<qs::RegexPattern> pFolderName_;
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
	ColorEntry(std::auto_ptr<Macro> pMacro,
			   COLORREF cr);
	~ColorEntry();

public:
	bool match(MacroContext* pContext) const;
	COLORREF getColor() const;

private:
	ColorEntry(const ColorEntry&);
	ColorEntry& operator=(const ColorEntry&);

private:
	std::auto_ptr<Macro> pMacro_;
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
	explicit ColorContentHandler(ColorManager* pManager);
	virtual ~ColorContentHandler();

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
	std::auto_ptr<Macro> pMacro_;
	qs::StringBuffer<qs::WSTRING> buffer_;
	MacroParser parser_;
};

}

#endif // __COLORS_H__
