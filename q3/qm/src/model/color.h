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
	typedef std::vector<ColorSet*> ColorSetList;

public:
	ColorManager();
	~ColorManager();

public:
	const ColorSetList& getColorSets();
	void setColorSets(ColorSetList& listColorSet);
	const ColorSet* getColorSet(Folder* pFolder) const;
	bool save() const;

public:
	void addColorSet(std::auto_ptr<ColorSet> pSet);

private:
	bool load();
	void clear();

private:
	ColorManager(const ColorManager&);
	ColorManager& operator=(const ColorManager&);

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
	typedef std::vector<ColorEntry*> ColorList;

public:
	ColorSet();
	ColorSet(const WCHAR* pwszAccount,
			 std::auto_ptr<qs::RegexPattern> pAccount,
			 const WCHAR* pwszFolder,
			 std::auto_ptr<qs::RegexPattern> pFolder);
	ColorSet(const ColorSet& colorset);
	~ColorSet();

public:
	const WCHAR* getAccount() const;
	void setAccount(const WCHAR* pwszAccount,
					std::auto_ptr<qs::RegexPattern> pAccount);
	const WCHAR* getFolder() const;
	void setFolder(const WCHAR* pwszFolder,
				   std::auto_ptr<qs::RegexPattern> pFolder);
	const ColorList& getColors() const;
	void setColors(ColorList& listColor);
	bool match(Folder* pFolder) const;
	COLORREF getColor(MacroContext* pContext) const;

public:
	void addEntry(std::auto_ptr<ColorEntry> pEntry);

private:
	void clear();

private:
	ColorSet& operator=(const ColorSet&);

private:
	qs::wstring_ptr wstrAccount_;
	std::auto_ptr<qs::RegexPattern> pAccount_;
	qs::wstring_ptr wstrFolder_;
	std::auto_ptr<qs::RegexPattern> pFolder_;
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
			   COLORREF cr);
	ColorEntry(const ColorEntry& color);
	~ColorEntry();

public:
	const Macro* getCondition() const;
	void setCondition(std::auto_ptr<Macro> pCondition);
	bool match(MacroContext* pContext) const;
	COLORREF getColor() const;
	void setColor(COLORREF cr);

private:
	ColorEntry& operator=(const ColorEntry&);

private:
	std::auto_ptr<Macro> pCondition_;
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
	std::auto_ptr<Macro> pCondition_;
	qs::StringBuffer<qs::WSTRING> buffer_;
	MacroParser parser_;
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
