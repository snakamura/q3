/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __COLORS_H__
#define __COLORS_H__

#include <qm.h>
#include <qmmacro.h>

#include <qs.h>
#include <qsregex.h>
#include <qssax.h>
#include <qsstring.h>

#include <vector>

#include "../util/confighelper.h"
#include "../util/util.h"

namespace qm {

class ColorManager;
class ColorManagerHandler;
class ColorManagerEvent;
class ColorSet;
class ColorEntry;
class ColorList;
class ColorContentHandler;
class ColorWriter;

class Folder;
class Macro;
class MacroContext;


/****************************************************************************
 *
 * ColorManager
 *
 */

class ColorManager
{
public:
	typedef std::vector<ColorSet*> ColorSetList;

public:
	explicit ColorManager(const WCHAR* pwszPath);
	~ColorManager();

public:
	const ColorSetList& getColorSets();
	void setColorSets(ColorSetList& listColorSet);
	std::auto_ptr<ColorList> getColorList(Folder* pFolder) const;
	bool save() const;
	void addColorManagerHandler(ColorManagerHandler* pHandler);
	void removeColorManagerHandler(ColorManagerHandler* pHandler);

public:
	void addColorSet(std::auto_ptr<ColorSet> pSet);
	void clear();

private:
	bool load();
	void fireColorSetsChanged();

private:
	ColorManager(const ColorManager&);
	ColorManager& operator=(const ColorManager&);

private:
	typedef std::vector<ColorManagerHandler*> HandlerList;

private:
	ColorSetList listColorSet_;
	ConfigHelper<ColorManager, ColorContentHandler, ColorWriter> helper_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * ColorManagerHandler
 *
 */

class ColorManagerHandler
{
public:
	virtual ~ColorManagerHandler();

public:
	virtual void colorSetsChanged(const ColorManagerEvent& event) = 0;
};


/****************************************************************************
 *
 * ColorManagerEvent
 *
 */

class ColorManagerEvent
{
public:
	explicit ColorManagerEvent(ColorManager* pColorManager);
	~ColorManagerEvent();

public:
	ColorManager* getColorManager() const;

private:
	ColorManagerEvent(const ColorManagerEvent&);
	ColorManagerEvent& operator=(const ColorManagerEvent&);

private:
	ColorManager* pColorManager_;
};


/****************************************************************************
 *
 * ColorSet
 *
 */

class ColorSet
{
public:
	typedef std::vector<ColorEntry*> ColorList;

public:
	ColorSet();
	ColorSet(RegexValue& account,
			 RegexValue& folder);
	ColorSet(const ColorSet& colorset);
	~ColorSet();

public:
	const WCHAR* getAccount() const;
	void setAccount(RegexValue& account);
	const WCHAR* getFolder() const;
	void setFolder(RegexValue& folder);
	const ColorList& getColors() const;
	void setColors(ColorList& listColor);
	bool match(Folder* pFolder) const;

public:
	void addEntry(std::auto_ptr<ColorEntry> pEntry);

private:
	void clear();

private:
	ColorSet& operator=(const ColorSet&);

private:
	RegexValue account_;
	RegexValue folder_;
	ColorList listColor_;
};


/****************************************************************************
 *
 * ColorEntry
 *
 */

class ColorEntry
{
public:
	ColorEntry();
	ColorEntry(std::auto_ptr<Macro> pCondition,
			   COLORREF cr,
			   const WCHAR* pwszDescription);
	ColorEntry(const ColorEntry& color);
	~ColorEntry();

public:
	const Macro* getCondition() const;
	void setCondition(std::auto_ptr<Macro> pCondition);
	bool match(MacroContext* pContext) const;
	COLORREF getColor() const;
	void setColor(COLORREF cr);
	const WCHAR* getDescription() const;
	void setDescription(const WCHAR* pwszDescription);

private:
	ColorEntry& operator=(const ColorEntry&);

private:
	std::auto_ptr<Macro> pCondition_;
	COLORREF cr_;
	qs::wstring_ptr wstrDescription_;
};


/****************************************************************************
 *
 * ColorList
 *
 */

class ColorList
{
public:
	typedef std::vector<const ColorEntry*> List;

public:
	ColorList(List& list);
	~ColorList();

public:
	COLORREF getColor(MacroContext* pContext) const;

private:
	ColorList(const ColorList&);
	ColorList& operator=(const ColorList&);

private:
	List list_;
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
	std::auto_ptr<Macro> pCondition_;
	qs::wstring_ptr wstrDescription_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * ColorWriter
 *
 */

class ColorWriter
{
public:
	explicit ColorWriter(qs::Writer* pWriter);
	~ColorWriter();

public:
	bool write(const ColorManager* pManager);

private:
	bool write(const ColorSet* pColorSet);
	bool write(const ColorEntry* pColor);

private:
	ColorWriter(const ColorWriter&);
	ColorWriter& operator=(const ColorWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __COLORS_H__
