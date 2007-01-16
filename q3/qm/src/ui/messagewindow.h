/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEWINDOW_H__
#define __MESSAGEWINDOW_H__

#include <qssax.h>


namespace qm {

class MessageWindowItem;
struct MessageWindowCreateContext;
class MessageWindowFontManager;
class MessageWindowFontGroup;
class MessageWindowFontSet;
class MessageWindowFontContentHandler;

class Document;
class EncodingModel;
class Macro;
class MacroContext;
class MessageViewModeHolder;
class SecurityModel;
class UIManager;


/****************************************************************************
 *
 * MessageWindowItem
 *
 */

class MessageWindowItem
{
public:
	virtual ~MessageWindowItem();

public:
	virtual void copy() = 0;
	virtual bool canCopy() = 0;
	virtual void selectAll() = 0;
	virtual bool canSelectAll() = 0;
	virtual void setFocus() = 0;
};


/****************************************************************************
 *
 * MessageWindowCreateContext
 *
 */

struct MessageWindowCreateContext
{
	Document* pDocument_;
	UIManager* pUIManager_;
	MessageViewModeHolder* pMessageViewModeHolder_;
	EncodingModel* pEncodingModel_;
	SecurityModel* pSecurityModel_;
	MessageWindowFontManager* pFontManager_;
};


/****************************************************************************
 *
 * MessageWindowFontManager
 *
 */

class MessageWindowFontManager
{
public:
	explicit MessageWindowFontManager(const WCHAR* pwszPath);
	~MessageWindowFontManager();

public:
	const MessageWindowFontGroup* getGroup(const WCHAR* pwszName) const;

public:
	void addGroup(std::auto_ptr<MessageWindowFontGroup> pGroup);

private:
	bool load(const WCHAR* pwszPath);

private:
	MessageWindowFontManager(const MessageWindowFontManager&);
	MessageWindowFontManager& operator=(const MessageWindowFontManager&);

private:
	typedef std::vector<MessageWindowFontGroup*> GroupList;

private:
	GroupList listGroup_;
};


/****************************************************************************
 *
 * MessageWindowFontGroup
 *
 */

class MessageWindowFontGroup
{
public:
	explicit MessageWindowFontGroup(const WCHAR* pwszName);
	~MessageWindowFontGroup();

public:
	const WCHAR* getName() const;
	const MessageWindowFontSet* getFontSet(MacroContext* pContext) const;

public:
	void addFontSet(std::auto_ptr<MessageWindowFontSet> pFontSet);
	bool isSet() const;

private:
	MessageWindowFontGroup(const MessageWindowFontGroup&);
	MessageWindowFontGroup& operator=(const MessageWindowFontGroup&);

private:
	typedef std::vector<MessageWindowFontSet*> FontSetList;

private:
	qs::wstring_ptr wstrName_;
	FontSetList listFontSet_;
};


/****************************************************************************
 *
 * MessageWindowFontSet
 *
 */

class MessageWindowFontSet
{
public:
	class Font
	{
	public:
		Font(const WCHAR* pwszFace,
			 double dSize,
			 unsigned int nStyle,
			 unsigned int nCharset);
		~Font();
	
	public:
		HFONT createFont() const;
	
	private:
		qs::wstring_ptr wstrFace_;
		double dSize_;
		unsigned int nStyle_;
		unsigned int nCharset_;
	};

public:
	MessageWindowFontSet(std::auto_ptr<Macro> pCondition,
						 unsigned int nLineSpacing);
	~MessageWindowFontSet();

public:
	bool match(MacroContext* pContext) const;
	const Font* getFont() const;
	unsigned int getLineSpacing() const;

public:
	void setFont(std::auto_ptr<Font> pFont);
	bool isSet() const;

private:
	MessageWindowFontSet(const MessageWindowFontSet&);
	MessageWindowFontSet& operator=(const MessageWindowFontSet&);

private:
	std::auto_ptr<Macro> pCondition_;
	std::auto_ptr<Font> pFont_;
	unsigned int nLineSpacing_;
};


/****************************************************************************
 *
 * MessageWindowFontContentHandler
 *
 */

class MessageWindowFontContentHandler : public qs::DefaultHandler
{
public:
	explicit MessageWindowFontContentHandler(MessageWindowFontManager* pManager);
	virtual ~MessageWindowFontContentHandler();

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
	MessageWindowFontContentHandler(const MessageWindowFontContentHandler&);
	MessageWindowFontContentHandler& operator=(const MessageWindowFontContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_FONTS,
		STATE_GROUP,
		STATE_FONTSET,
		STATE_FONT
	};

private:
	MessageWindowFontManager* pManager_;
	State state_;
	std::auto_ptr<MessageWindowFontGroup> pGroup_;
	std::auto_ptr<MessageWindowFontSet> pFontSet_;
};


}

#endif // __MESSAGEWINDOW_H__
