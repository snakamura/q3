/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEVIEWWINDOW_H__
#define __MESSAGEVIEWWINDOW_H__

#undef TranslateAccelerator

#include <qm.h>

#include <qsmenu.h>
#include <qsmime.h>
#include <qstextwindow.h>
#include <qswindow.h>

#ifdef QMHTMLVIEW
#	undef T2W
#	undef W2T
#	undef T2A
#	undef A2T
#	include <atlbase.h>
#	include <atliface.h>
#	include <exdisp.h>
#	include <mshtml.h>
#	include <mshtmhst.h>
#endif

#include "messagewindow.h"


namespace qm {

class MessageViewWindow;
	class TextMessageViewWindow;
#ifdef QMHTMLVIEW
	class HtmlMessageViewWindow;
#endif
class MessageViewWindowFactory;

class Document;
class MessageHolder;
class Message;
class Template;


/****************************************************************************
 *
 * MessageViewWindow
 *
 */

class MessageViewWindow : public MessageWindowItem
{
public:
	virtual ~MessageViewWindow();

public:
	virtual qs::Window& getWindow() = 0;
	virtual bool isActive() = 0;
	virtual qs::QSTATUS setActive() = 0;
	virtual qs::QSTATUS setMessage(MessageHolder* pmh, Message* pMessage,
		const Template* pTemplate, const WCHAR* pwszEncoding,
		bool bRawMode, bool bIncludeHeader) = 0;
	virtual qs::QSTATUS scrollPage(bool bPrev, bool* pbScrolled) = 0;
	virtual qs::QSTATUS setSelectMode(bool bSelectMode) = 0;
	virtual qs::QSTATUS find(const WCHAR* pwszFind,
		bool bMatchCase, bool bPrev, bool* pbFound) = 0;
	virtual qs::QSTATUS openLink() = 0;
};


/****************************************************************************
 *
 * MessageViewWindowFactory
 *
 */

class MessageViewWindowFactory
{
public:
	MessageViewWindowFactory(Document* pDocument, qs::Profile* pProfile,
		const WCHAR* pwszSection, qs::MenuManager* pMenuManager,
		bool bTextOnly, qs::QSTATUS* pstatus);
	~MessageViewWindowFactory();

public:
	qs::QSTATUS create(HWND hwnd);
	qs::QSTATUS getMessageViewWindow(const qs::ContentTypeParser* pContentType,
		MessageViewWindow** ppMessageViewWindow);
	TextMessageViewWindow* getTextMessageViewWindow() const;
	bool isSupported(const qs::ContentTypeParser* pContentType) const;

private:
	MessageViewWindowFactory(const MessageViewWindowFactory&);
	MessageViewWindowFactory& operator=(const MessageViewWindowFactory&);

private:
	Document* pDocument_;
	qs::Profile* pProfile_;
	const WCHAR* pwszSection_;
	qs::MenuManager* pMenuManager_;
	bool bTextOnly_;
	TextMessageViewWindow* pText_;
#ifdef QMHTMLVIEW
	HtmlMessageViewWindow* pHtml_;
#endif
};


/****************************************************************************
 *
 * TextMessageViewWindow
 *
 */

class TextMessageViewWindow :
	public qs::TextWindow,
	public qs::TextWindowLinkHandler,
	public MessageViewWindow
{
public:
	TextMessageViewWindow(Document* pDocument, qs::Profile* pProfile,
		const WCHAR* pwszSection, qs::MenuManager* pMenuManager,
		qs::QSTATUS* pstatus);
	virtual ~TextMessageViewWindow();

public:
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd, const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags, const POINT& pt);

public:
	qs::QSTATUS openLink(const WCHAR* pwszURL);

public:
	virtual qs::Window& getWindow();
	virtual bool isActive();
	virtual qs::QSTATUS setActive();
	virtual qs::QSTATUS setMessage(MessageHolder* pmh, Message* pMessage,
		const Template* pTemplate, const WCHAR* pwszEncoding,
		bool bRawMode, bool bIncludeHeader);
	virtual qs::QSTATUS scrollPage(bool bPrev, bool* pbScrolled);
	virtual qs::QSTATUS setSelectMode(bool bSelectMode);
	virtual qs::QSTATUS find(const WCHAR* pwszFind,
		bool bMatchCase, bool bPrev, bool* pbFound);
	virtual qs::QSTATUS openLink();

public:
	virtual qs::QSTATUS copy();
	virtual qs::QSTATUS canCopy(bool* pbCan);
	virtual qs::QSTATUS selectAll();
	virtual qs::QSTATUS canSelectAll(bool* pbCan);

private:
	TextMessageViewWindow(const TextMessageViewWindow&);
	TextMessageViewWindow& operator=(const TextMessageViewWindow&);

private:
	qs::ReadOnlyTextModel* pTextModel_;
	Document* pDocument_;
	qs::Profile* pProfile_;
	qs::MenuManager* pMenuManager_;
};


#ifdef QMHTMLVIEW

/****************************************************************************
 *
 * HtmlMessageViewWindow
 *
 */

class HtmlMessageViewWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public MessageViewWindow
{
public:
	HtmlMessageViewWindow(qs::MenuManager* pMenuManager,
		qs::QSTATUS* pstatus);
	virtual ~HtmlMessageViewWindow();

public:
	virtual qs::QSTATUS getSuperClass(qs::WSTRING* pwstrSuperClass);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();

public:
	virtual qs::Window& getWindow();
	virtual bool isActive();
	virtual qs::QSTATUS setActive();
	virtual qs::QSTATUS setMessage(MessageHolder* pmh, Message* pMessage,
		const Template* pTemplate, const WCHAR* pwszEncoding,
		bool bRawMode, bool bIncludeHeader);
	virtual qs::QSTATUS scrollPage(bool bPrev, bool* pbScrolled);
	virtual qs::QSTATUS setSelectMode(bool bSelectMode);
	virtual qs::QSTATUS find(const WCHAR* pwszFind,
		bool bMatchCase, bool bPrev, bool* pbFound);
	virtual qs::QSTATUS openLink();

public:
	virtual qs::QSTATUS copy();
	virtual qs::QSTATUS canCopy(bool* pbCan);
	virtual qs::QSTATUS selectAll();
	virtual qs::QSTATUS canSelectAll(bool* pbCan);

private:
	qs::QSTATUS prepareRelatedContent(
		const Message& msg, const qs::Part& partHtml,
		const WCHAR* pwszEncoding, qs::WSTRING* pwstrId);
	void clearRelatedContent();
	qs::QSTATUS prepareRelatedContent(const qs::Part& part,
		const WCHAR* pwszId, const WCHAR* pwszEncoding);

private:
	HtmlMessageViewWindow(const HtmlMessageViewWindow&);
	HtmlMessageViewWindow& operator=(const HtmlMessageViewWindow&);

private:
	struct Content
	{
		void destroy();
		
		qs::WSTRING wstrContentId_;
		qs::WSTRING wstrMimeType_;
		unsigned char* pData_;
		size_t nDataLen_;
	};
	
	class IInternetSecurityManagerImpl : public IInternetSecurityManager
	{
	public:
		IInternetSecurityManagerImpl(bool bProhibitAll, qs::QSTATUS* pstatus);
		~IInternetSecurityManagerImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	
	public:
		STDMETHOD(SetSecuritySite)(IInternetSecurityMgrSite* pSite);
		STDMETHOD(GetSecuritySite)(IInternetSecurityMgrSite** ppSite);
		STDMETHOD(MapUrlToZone)(LPCWSTR pwszUrl, DWORD* pdwZone, DWORD dwFlags);
		STDMETHOD(GetSecurityId)(LPCWSTR pwszUrl, BYTE* pbSecurityId,
			DWORD* pcbSecurityId, DWORD_PTR dwReserved);
		STDMETHOD(ProcessUrlAction)(LPCWSTR pwszUrl, DWORD dwAction,
			BYTE* pPolicy, DWORD cbPolicy, BYTE* pContext, DWORD cbContext,
			DWORD dwFlags, DWORD dwReserved);
		STDMETHOD(QueryCustomPolicy)(LPCWSTR pwszUrl, REFGUID guidKey,
			BYTE** ppPolicy, DWORD* pcbPolicy, BYTE* pContent,
			DWORD cbContent, DWORD dwReserved);
		STDMETHOD(SetZoneMapping)(DWORD dwZone, LPCWSTR pwszPattern, DWORD dwFlags);
		STDMETHOD(GetZoneMappings)(DWORD dwZone,
			IEnumString** ppenumString, DWORD dwFlags);
		
	private:
		IInternetSecurityManagerImpl(const IInternetSecurityManagerImpl&);
		IInternetSecurityManagerImpl& operator=(const IInternetSecurityManagerImpl&);
	
	private:
		ULONG nRef_;
		bool bProhibitAll_;
	};
	
	class InternetProtocol :
		public IInternetProtocol,
		public IInternetProtocolInfo
	{
	public:
		InternetProtocol(HtmlMessageViewWindow* pHtmlMessageViewWindow,
			qs::QSTATUS* pstatus);
		~InternetProtocol();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	
	public:
		STDMETHOD(Abort)(HRESULT hrReason, DWORD dwOptions);
		STDMETHOD(Continue)(PROTOCOLDATA* pProtocolData);
		STDMETHOD(Resume)();
		STDMETHOD(Start)(LPCWSTR pwszUrl, IInternetProtocolSink* pSink,
			IInternetBindInfo* pBindInfo, DWORD dwFlags, HANDLE_PTR dwReserved);
		STDMETHOD(Suspend)();
		STDMETHOD(Terminate)(DWORD dwOptions);
	
	public:
		STDMETHOD(LockRequest)(DWORD dwOptions);
		STDMETHOD(Read)(void* pv, ULONG cb, ULONG* pcbRead);
		STDMETHOD(Seek)(LARGE_INTEGER move, DWORD dwOrigin, ULARGE_INTEGER* pNewPos);
		STDMETHOD(UnlockRequest)();
	
	public:
		STDMETHOD(CombineUrl)(LPCWSTR pwszBaseUrl, LPCWSTR pwszRelativeUrl,
			DWORD dwCombineFlags, LPWSTR pwszResult, DWORD cchResult,
			DWORD* pcchResult, DWORD dwReserved);
		STDMETHOD(CompareUrl)(LPCWSTR pwszUrl1, LPCWSTR pwszUrl2, DWORD dwCompareFlags);
		STDMETHOD(ParseUrl)(LPCWSTR pwszUrl, PARSEACTION action, DWORD dwParseFlags,
			LPWSTR pwszResult, DWORD cchResult, DWORD* pcchResult, DWORD dwReserved);
		STDMETHOD(QueryInfo)(LPCWSTR pwszUrl, QUERYOPTION option, DWORD dwQueryFlags,
			LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuffer, DWORD dwReserved);
	
	private:
		InternetProtocol(const InternetProtocol&);
		InternetProtocol& operator=(const InternetProtocol&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pHtmlMessageViewWindow_;
		Content* pContent_;
		const unsigned char* pCurrent_;
		IInternetProtocolSink* pSink_;
	};
	
	class IServiceProviderImpl : public IServiceProvider
	{
	public:
		IServiceProviderImpl(HtmlMessageViewWindow* pHtmlMessageViewWindow,
			qs::QSTATUS* pstatus);
		~IServiceProviderImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	
	public:
		STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void** ppv);
	
	private:
		IServiceProviderImpl(const IServiceProviderImpl&);
		IServiceProviderImpl& operator=(const IServiceProviderImpl&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pHtmlMessageViewWindow_;
		IInternetSecurityManagerImpl* pSecurityManager_;
	};
	
	class IDocHostUIHandlerDispatchImpl : public IDocHostUIHandlerDispatch
	{
	public:
		IDocHostUIHandlerDispatchImpl(HtmlMessageViewWindow* pWindow,
			qs::QSTATUS* pstatus);
		~IDocHostUIHandlerDispatchImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	
	public:
		STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames,
			unsigned int cNames, LCID lcid, DISPID* pDispId);
		STDMETHOD(GetTypeInfo)(unsigned int nTypeInfo,
			LCID lcid, ITypeInfo** ppTypeInfo);
		STDMETHOD(GetTypeInfoCount)(unsigned int* pcTypeInfo);
		STDMETHOD(Invoke)(DISPID dispId, REFIID riid, LCID lcid,
			WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
			EXCEPINFO* pExcepInfo, unsigned int* pnArgErr);
	
	public:
		STDMETHOD(ShowContextMenu)(DWORD dwId, DWORD x, DWORD y,
			IUnknown* pUnk, IDispatch* pDisp, HRESULT* phrResult);
		STDMETHOD(GetHostInfo)(DWORD* pdwFlags, DWORD* pdwDoubleClick);
		STDMETHOD(ShowUI)(DWORD dwId, IUnknown* pActiveObject,
			IUnknown* pCommandTarget, IUnknown* pFrame,
			IUnknown* pUIWindow, HRESULT* phrResult);
		STDMETHOD(HideUI)();
		STDMETHOD(UpdateUI)();
		STDMETHOD(EnableModeless)(VARIANT_BOOL bEnable);
		STDMETHOD(OnDocWindowActivate)(VARIANT_BOOL bActivate);
		STDMETHOD(OnFrameWindowActivate)(VARIANT_BOOL bActivate);
		STDMETHOD(ResizeBorder)(long left, long top, long right, long buttom,
			IUnknown* pUIWindow, VARIANT_BOOL bFrameWindow);
		STDMETHOD(TranslateAccelerator)(DWORD hwnd, DWORD nMessage,
			DWORD wParam, DWORD lParam, BSTR bstrGuidCmdGroup,
			DWORD nCmdId, HRESULT* phrResult);
		STDMETHOD(GetOptionKeyPath)(BSTR* pbstrKey, DWORD dw);
		STDMETHOD(GetDropTarget)(IUnknown* pDropTarget, IUnknown** ppDropTarget);
		STDMETHOD(GetExternal)(IDispatch** ppDispatch);
		STDMETHOD(TranslateUrl)(DWORD dwTranslate, BSTR bstrURLIn, BSTR* bstrURLOut);
		STDMETHOD(FilterDataObject)(IUnknown* pInObject, IUnknown** ppOutObject);
	
	private:
		IDocHostUIHandlerDispatchImpl(const IDocHostUIHandlerDispatchImpl&);
		IDocHostUIHandlerDispatchImpl& operator=(const IDocHostUIHandlerDispatchImpl&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pWindow_;
	};
	friend class IDocHostUIHandlerDispatchImpl;
	
	class DWebBrowserEvents2Impl : public DWebBrowserEvents2
	{
	public:
		DWebBrowserEvents2Impl(HtmlMessageViewWindow* pHtmlMessageViewWindow,
			IWebBrowser2* pWebBrowser, qs::QSTATUS* pstatus);
		~DWebBrowserEvents2Impl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** pv);
	
	public:
		STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames,
			unsigned int cNames, LCID lcid, DISPID* pDispId);
		STDMETHOD(GetTypeInfo)(unsigned int nTypeInfo,
			LCID lcid, ITypeInfo** ppTypeInfo);
		STDMETHOD(GetTypeInfoCount)(unsigned int* pcTypeInfo);
		STDMETHOD(Invoke)(DISPID dispId, REFIID riid, LCID lcid,
			WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
			EXCEPINFO* pExcepInfo, unsigned int* pnArgErr);
	
	private:
		DWebBrowserEvents2Impl(const DWebBrowserEvents2Impl&);
		DWebBrowserEvents2Impl& operator=(const DWebBrowserEvents2Impl&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pHtmlMessageViewWindow_;
		IWebBrowser2* pWebBrowser_;
	};
	friend class DWebBrowserEvents2Impl;

private:
	typedef std::vector<Content> ContentList;

private:
	qs::MenuManager* pMenuManager_;
	IWebBrowser2* pWebBrowser_;
	IServiceProviderImpl* pServiceProvider_;
	DWebBrowserEvents2Impl* pWebBrowserEvents_;
	DWORD dwConnectionPointCookie_;
	ContentList listContent_;
	bool bActivate_;

friend class InternetProtocol;
};

#endif // QMHTMLVIEW

}

#endif // __MESSAGEVIEWWINDOW_H__
