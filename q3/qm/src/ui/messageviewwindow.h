/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
#include <qstheme.h>
#include <qswindow.h>

#ifdef QMHTMLVIEW
#	ifdef QMHTMLVIEWWEBBROWSER
#		undef T2W
#		undef W2T
#		undef T2A
#		undef A2T
#		include <atlbase.h>
#		include <atliface.h>
#		include <exdisp.h>
#		include <mshtml.h>
#		include <mshtmhst.h>
#	elif defined QMHTMLVIEWHTMLCTRL
#		include <webvw.h>
#		define HANDLE_PTR DWORD
#	else
#		error "Unknown HTMLVIEW"
#	endif
#	include <urlmon.h>
#endif

#include "messagewindow.h"


namespace qm {

class MessageViewWindow;
	class TextMessageViewWindow;
#ifdef QMHTMLVIEW
	class HtmlMessageViewWindow;
#endif
class MessageViewWindowCallback;
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
	enum Flag {
		FLAG_RAWMODE		= 0x01,
		FLAG_INCLUDEHEADER	= 0x02,
		FLAG_ONLINEMODE		= 0x04,
		FLAG_INTERNETZONE	= 0x08
	};

public:
	virtual ~MessageViewWindow();

public:
	virtual qs::Window& getWindow() = 0;
	virtual bool isActive() = 0;
	virtual void setActive() = 0;
	virtual bool setMessage(MessageHolder* pmh,
							Message* pMessage,
							const Template* pTemplate,
							const WCHAR* pwszEncoding,
							unsigned int nFlags,
							unsigned int nSecurityMode) = 0;
	virtual bool scrollPage(bool bPrev) = 0;
	virtual void setSelectMode(bool bSelectMode) = 0;
	virtual void setQuoteMode(bool bQuoteMode) = 0;
	virtual bool find(const WCHAR* pwszFind,
					  unsigned int nFlags) = 0;
	virtual unsigned int getSupportedFindFlags() const = 0;
	virtual std::auto_ptr<MessageWindow::Mark> mark() const = 0;
	virtual void reset(const MessageWindow::Mark& mark) = 0;
	virtual bool openLink() = 0;
};


/****************************************************************************
 *
 * MessageViewWindowCallback
 *
 */

class MessageViewWindowCallback
{
public:
	virtual ~MessageViewWindowCallback();

public:
	virtual void statusTextChanged(const WCHAR* pwszText) = 0;
};


/****************************************************************************
 *
 * MessageViewWindowFactory
 *
 */

class MessageViewWindowFactory
{
public:
	MessageViewWindowFactory(MessageWindow* pMessageWindow,
							 Document* pDocument,
							 qs::Profile* pProfile,
							 const WCHAR* pwszSection,
							 MessageModel* pMessageModel,
							 qs::MenuManager* pMenuManager,
							 MessageViewWindowCallback* pCallback,
							 bool bTextOnly);
	~MessageViewWindowFactory();

public:
	bool create();
	MessageViewWindow* getMessageViewWindow(const qs::ContentTypeParser* pContentType);
	TextMessageViewWindow* getTextMessageViewWindow();
	MessageViewWindow* getLinkMessageViewWindow();
	bool isSupported(const qs::ContentTypeParser* pContentType) const;
	void reloadProfiles();

private:
#ifdef QMHTMLVIEW
	bool isHtmlViewSupported() const;
	bool createHtmlView();
#endif

private:
	MessageViewWindowFactory(const MessageViewWindowFactory&);
	MessageViewWindowFactory& operator=(const MessageViewWindowFactory&);

private:
	enum {
		ID_TEXTMESSAGEVIEWWINDOW	= 1002,
		ID_HTMLMESSAGEVIEWWINDOW	= 1003
	};

private:
	MessageWindow* pMessageWindow_;
	Document* pDocument_;
	qs::Profile* pProfile_;
	const WCHAR* pwszSection_;
	MessageModel* pMessageModel_;
	qs::MenuManager* pMenuManager_;
	MessageViewWindowCallback* pCallback_;
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
	TextMessageViewWindow(Document* pDocument,
						  qs::Profile* pProfile,
						  const WCHAR* pwszSection,
						  MessageModel* pMessageModel,
						  qs::MenuManager* pMenuManager);
	virtual ~TextMessageViewWindow();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd, const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags, const POINT& pt);

public:
	bool openLink(const WCHAR* pwszURL);

public:
	virtual qs::Window& getWindow();
	virtual bool isActive();
	virtual void setActive();
	virtual bool setMessage(MessageHolder* pmh,
							Message* pMessage,
							const Template* pTemplate,
							const WCHAR* pwszEncoding,
							unsigned int nFlags,
							unsigned int nSecurityMode);
	virtual bool scrollPage(bool bPrev);
	virtual void setSelectMode(bool bSelectMode);
	virtual void setQuoteMode(bool bQuoteMode);
	virtual bool find(const WCHAR* pwszFind,
					  unsigned int nFlags);
	virtual unsigned int getSupportedFindFlags() const;
	virtual std::auto_ptr<MessageWindow::Mark> mark() const;
	virtual void reset(const MessageWindow::Mark& mark);
	virtual bool openLink();

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void selectAll();
	virtual bool canSelectAll();

private:
	TextMessageViewWindow(const TextMessageViewWindow&);
	TextMessageViewWindow& operator=(const TextMessageViewWindow&);

private:
	class MarkImpl : public MessageWindow::Mark
	{
	public:
		MarkImpl(size_t nLine,
				 size_t nChar);
		virtual ~MarkImpl();
	
	public:
		size_t nLine_;
		size_t nChar_;
	};

private:
	std::auto_ptr<qs::ReadOnlyTextModel> pTextModel_;
	Document* pDocument_;
	qs::Profile* pProfile_;
	MessageModel* pMessageModel_;
	qs::MenuManager* pMenuManager_;
};


#ifdef QMHTMLVIEW

/****************************************************************************
 *
 * HtmlContent
 *
 */

class HtmlContent
{
public:
	HtmlContent(const WCHAR* pwszContentId,
				const WCHAR* pwszMimeType,
				qs::malloc_size_ptr<unsigned char> pData,
				const void* pCookie);
	~HtmlContent();

public:
	const WCHAR* getContentId() const;
	const WCHAR* getMimeType() const;
	const unsigned char* getData() const;
	size_t getDataSize() const;
	const void* getCookie() const;

private:
	HtmlContent(const HtmlContent&);
	HtmlContent& operator=(const HtmlContent&);
	
private:
	qs::wstring_ptr wstrContentId_;
	qs::wstring_ptr wstrMimeType_;
	qs::malloc_size_ptr<unsigned char> pData_;
	const void* pCookie_;
};


/****************************************************************************
 *
 * HtmlContentManager
 *
 */

class HtmlContentManager
{
public:
	HtmlContentManager();
	~HtmlContentManager();

public:
	const HtmlContent* get(const WCHAR* pwszContentId) const;
	qs::wstring_ptr prepare(const Message& msg,
							const qs::Part& partHtml,
							const WCHAR* pwszEncoding,
							const void* pCookie);
	void clear(const void* pCookie);

private:
	void prepare(const qs::Part& part,
				 const WCHAR* pwszId,
				 const WCHAR* pwszEncoding,
				 const void* pCookie);

private:
	HtmlContentManager(const HtmlContentManager&);
	HtmlContentManager& operator=(const HtmlContentManager&);

private:
	typedef std::vector<HtmlContent*> ContentList;

private:
	ContentList listContent_;
};


/****************************************************************************
 *
 * InternetProtocol
 *
 */

class InternetProtocol :
	public IInternetProtocol,
	public IInternetProtocolInfo
{
public:
	explicit InternetProtocol(const HtmlContentManager& contentManager);
	~InternetProtocol();

public:
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);

public:
	STDMETHOD(Abort)(HRESULT hrReason,
					 DWORD dwOptions);
	STDMETHOD(Continue)(PROTOCOLDATA* pProtocolData);
	STDMETHOD(Resume)();
	STDMETHOD(Start)(LPCWSTR pwszUrl,
					 IInternetProtocolSink* pSink,
					 IInternetBindInfo* pBindInfo,
					 DWORD dwFlags,
					 HANDLE_PTR dwReserved);
	STDMETHOD(Suspend)();
	STDMETHOD(Terminate)(DWORD dwOptions);

public:
	STDMETHOD(LockRequest)(DWORD dwOptions);
	STDMETHOD(Read)(void* pv,
					ULONG cb,
					ULONG* pcbRead);
	STDMETHOD(Seek)(LARGE_INTEGER move,
					DWORD dwOrigin,
					ULARGE_INTEGER* pNewPos);
	STDMETHOD(UnlockRequest)();

public:
	STDMETHOD(CombineUrl)(LPCWSTR pwszBaseUrl,
						  LPCWSTR pwszRelativeUrl,
						  DWORD dwCombineFlags,
						  LPWSTR pwszResult,
						  DWORD cchResult,
						  DWORD* pcchResult,
						  DWORD dwReserved);
	STDMETHOD(CompareUrl)(LPCWSTR pwszUrl1,
						  LPCWSTR pwszUrl2,
						  DWORD dwCompareFlags);
	STDMETHOD(ParseUrl)(LPCWSTR pwszUrl,
						PARSEACTION action,
						DWORD dwParseFlags,
						LPWSTR pwszResult,
						DWORD cchResult,
						DWORD* pcchResult,
						DWORD dwReserved);
	STDMETHOD(QueryInfo)(LPCWSTR pwszUrl,
						 QUERYOPTION option,
						 DWORD dwQueryFlags,
						 LPVOID pBuffer,
						 DWORD cbBuffer,
						 DWORD* pcbBuffer,
						 DWORD dwReserved);

private:
	InternetProtocol(const InternetProtocol&);
	InternetProtocol& operator=(const InternetProtocol&);

private:
	ULONG nRef_;
	const HtmlContentManager& contentManager_;
	const HtmlContent* pContent_;
	const unsigned char* pCurrent_;
	IInternetProtocolSink* pSink_;
};


/****************************************************************************
 *
 * InternetProtocolFactory
 *
 */

class InternetProtocolFactory : public IClassFactory
{
public:
	explicit InternetProtocolFactory(const HtmlContentManager& contentManager);
	~InternetProtocolFactory();

public:
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);

public:
	STDMETHOD(CreateInstance)(IUnknown* pUnkOuter,
							  REFIID riid,
							  void** ppvObj);
	STDMETHOD(LockServer)(BOOL bLock);

private:
	InternetProtocolFactory(const InternetProtocolFactory&);
	InternetProtocolFactory& operator=(const InternetProtocolFactory&);

private:
	ULONG nRef_;
	const HtmlContentManager& contentManager_;
};


/****************************************************************************
 *
 * InternetSessionInit
 *
 */

class InternetSessionInit
{
private:
	InternetSessionInit();

public:
	~InternetSessionInit();

public:
	static HtmlContentManager& getContentManager();

private:
	InternetSessionInit(const InternetSessionInit&);
	InternetSessionInit& operator=(const InternetSessionInit&);

private:
	HtmlContentManager contentManager_;
	qs::ComPtr<IInternetSession> pInternetSession_;
	qs::ComPtr<IClassFactory> pClassFactory_;

private:
	static InternetSessionInit instance__;
};


/****************************************************************************
 *
 * HtmlMessageViewWindow
 *
 */

#ifdef QMHTMLVIEWWEBBROWSER

class HtmlMessageViewWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public MessageViewWindow
{
public:
	HtmlMessageViewWindow(qs::Profile* pProfile,
						  const WCHAR* pwszSection,
						  MessageWindow* pMessageWindow,
						  MessageModel* pMessageModel,
						  qs::MenuManager* pMenuManager,
						  MessageViewWindowCallback* pCallback);
	virtual ~HtmlMessageViewWindow();

public:
	virtual qs::wstring_ptr getSuperClass();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
#ifndef _WIN32_WCE
	LRESULT onNcPaint(HRGN hrgn);
	LRESULT onThemeChanged();
#endif

public:
	virtual qs::Window& getWindow();
	virtual bool isActive();
	virtual void setActive();
	virtual bool setMessage(MessageHolder* pmh,
							Message* pMessage,
							const Template* pTemplate,
							const WCHAR* pwszEncoding,
							unsigned int nFlags,
							unsigned int nSecurityMode);
	virtual bool scrollPage(bool bPrev);
	virtual void setSelectMode(bool bSelectMode);
	virtual void setQuoteMode(bool bQuoteMode);
	virtual bool find(const WCHAR* pwszFind,
					  unsigned int nFlags);
	virtual unsigned int getSupportedFindFlags() const;
	virtual std::auto_ptr<MessageWindow::Mark> mark() const;
	virtual void reset(const MessageWindow::Mark& mark);
	virtual bool openLink();

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void selectAll();
	virtual bool canSelectAll();

private:
	HtmlMessageViewWindow(const HtmlMessageViewWindow&);
	HtmlMessageViewWindow& operator=(const HtmlMessageViewWindow&);

private:
	class IInternetSecurityManagerImpl : public IInternetSecurityManager
	{
	public:
		IInternetSecurityManagerImpl();
		~IInternetSecurityManagerImpl();
	
	public:
		bool isInternetZone() const;
		void setInternetZone(bool bInternetZone);
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** ppv);
	
	public:
		STDMETHOD(SetSecuritySite)(IInternetSecurityMgrSite* pSite);
		STDMETHOD(GetSecuritySite)(IInternetSecurityMgrSite** ppSite);
		STDMETHOD(MapUrlToZone)(LPCWSTR pwszUrl,
								DWORD* pdwZone,
								DWORD dwFlags);
		STDMETHOD(GetSecurityId)(LPCWSTR pwszUrl,
								 BYTE* pbSecurityId,
								 DWORD* pcbSecurityId,
								 DWORD_PTR dwReserved);
		STDMETHOD(ProcessUrlAction)(LPCWSTR pwszUrl,
									DWORD dwAction,
									BYTE* pPolicy,
									DWORD cbPolicy,
									BYTE* pContext,
									DWORD cbContext,
									DWORD dwFlags,
									DWORD dwReserved);
		STDMETHOD(QueryCustomPolicy)(LPCWSTR pwszUrl,
									 REFGUID guidKey,
									 BYTE** ppPolicy,
									 DWORD* pcbPolicy,
									 BYTE* pContent,
									 DWORD cbContent,
									 DWORD dwReserved);
		STDMETHOD(SetZoneMapping)(DWORD dwZone,
								  LPCWSTR pwszPattern,
								  DWORD dwFlags);
		STDMETHOD(GetZoneMappings)(DWORD dwZone,
								   IEnumString** ppenumString,
								   DWORD dwFlags);
		
	private:
		IInternetSecurityManagerImpl(const IInternetSecurityManagerImpl&);
		IInternetSecurityManagerImpl& operator=(const IInternetSecurityManagerImpl&);
	
	private:
		ULONG nRef_;
		bool bInternetZone_;
	};
	
	class IServiceProviderImpl : public IServiceProvider
	{
	public:
		explicit IServiceProviderImpl(HtmlMessageViewWindow* pHtmlMessageViewWindow);
		~IServiceProviderImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** ppv);
	
	public:
		STDMETHOD(QueryService)(REFGUID guidService,
								REFIID riid,
								void** ppv);
	
	private:
		IServiceProviderImpl(const IServiceProviderImpl&);
		IServiceProviderImpl& operator=(const IServiceProviderImpl&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pHtmlMessageViewWindow_;
	};
	friend class IServiceProviderImpl;
	
	class IDocHostUIHandlerDispatchImpl : public IDocHostUIHandlerDispatch
	{
	public:
		explicit IDocHostUIHandlerDispatchImpl(HtmlMessageViewWindow* pWindow);
		~IDocHostUIHandlerDispatchImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** ppv);
	
	public:
		STDMETHOD(GetIDsOfNames)(REFIID riid,
								 OLECHAR** rgszNames,
								 unsigned int cNames,
								 LCID lcid,
								 DISPID* pDispId);
		STDMETHOD(GetTypeInfo)(unsigned int nTypeInfo,
							   LCID lcid,
							   ITypeInfo** ppTypeInfo);
		STDMETHOD(GetTypeInfoCount)(unsigned int* pcTypeInfo);
		STDMETHOD(Invoke)(DISPID dispId,
						  REFIID riid,
						  LCID lcid,
						  WORD wFlags,
						  DISPPARAMS* pDispParams,
						  VARIANT* pVarResult,
						  EXCEPINFO* pExcepInfo,
						  unsigned int* pnArgErr);
	
	public:
		STDMETHOD(ShowContextMenu)(DWORD dwId,
								   DWORD x,
								   DWORD y,
								   IUnknown* pUnk,
								   IDispatch* pDisp,
								   HRESULT* phrResult);
		STDMETHOD(GetHostInfo)(DWORD* pdwFlags,
							   DWORD* pdwDoubleClick);
		STDMETHOD(ShowUI)(DWORD dwId,
						  IUnknown* pActiveObject,
						  IUnknown* pCommandTarget,
						  IUnknown* pFrame,
						  IUnknown* pUIWindow,
						  HRESULT* phrResult);
		STDMETHOD(HideUI)();
		STDMETHOD(UpdateUI)();
		STDMETHOD(EnableModeless)(VARIANT_BOOL bEnable);
		STDMETHOD(OnDocWindowActivate)(VARIANT_BOOL bActivate);
		STDMETHOD(OnFrameWindowActivate)(VARIANT_BOOL bActivate);
		STDMETHOD(ResizeBorder)(long left,
								long top,
								long right,
								long buttom,
								IUnknown* pUIWindow,
								VARIANT_BOOL bFrameWindow);
		STDMETHOD(TranslateAccelerator)(DWORD hwnd,
										DWORD nMessage,
										DWORD wParam,
										DWORD lParam,
										BSTR bstrGuidCmdGroup,
										DWORD nCmdId,
										HRESULT* phrResult);
		STDMETHOD(GetOptionKeyPath)(BSTR* pbstrKey,
									DWORD dw);
		STDMETHOD(GetDropTarget)(IUnknown* pDropTarget,
								 IUnknown** ppDropTarget);
		STDMETHOD(GetExternal)(IDispatch** ppDispatch);
		STDMETHOD(TranslateUrl)(DWORD dwTranslate,
								BSTR bstrURLIn,
								BSTR* bstrURLOut);
		STDMETHOD(FilterDataObject)(IUnknown* pInObject,
									IUnknown** ppOutObject);
	
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
							   IWebBrowser2* pWebBrowser);
		~DWebBrowserEvents2Impl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** pv);
	
	public:
		STDMETHOD(GetIDsOfNames)(REFIID riid,
								 OLECHAR** rgszNames,
								 unsigned int cNames,
								 LCID lcid,
								 DISPID* pDispId);
		STDMETHOD(GetTypeInfo)(unsigned int nTypeInfo,
							   LCID lcid,
							   ITypeInfo** ppTypeInfo);
		STDMETHOD(GetTypeInfoCount)(unsigned int* pcTypeInfo);
		STDMETHOD(Invoke)(DISPID dispId,
						  REFIID riid,
						  LCID lcid,
						  WORD wFlags,
						  DISPPARAMS* pDispParams,
						  VARIANT* pVarResult,
						  EXCEPINFO* pExcepInfo,
						  unsigned int* pnArgErr);
	
	private:
		DWebBrowserEvents2Impl(const DWebBrowserEvents2Impl&);
		DWebBrowserEvents2Impl& operator=(const DWebBrowserEvents2Impl&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pHtmlMessageViewWindow_;
		IWebBrowser2* pWebBrowser_;
	};
	friend class DWebBrowserEvents2Impl;
	
	class AmbientDispatchHook : public IAxWinAmbientDispatch
	{
	public:
		struct IDispatchType;
		struct IDispatchVtbl
		{
			HRESULT (STDMETHODCALLTYPE* QueryInterface)(IDispatchType*,
														REFIID,
														void**);
			ULONG (STDMETHODCALLTYPE* AddRef)(IDispatchType*);
			ULONG (STDMETHODCALLTYPE* Release)(IDispatchType*);
			HRESULT (STDMETHODCALLTYPE* GetTypeInfoCount)(IDispatchType*,
														  UINT*);
			HRESULT (STDMETHODCALLTYPE* GetTypeInfo)(IDispatchType*,
													 UINT,
													 LCID,
													 ITypeInfo**);
			HRESULT (STDMETHODCALLTYPE* GetIDsOfNames)(IDispatchType*,
													   REFIID,
													   LPOLESTR*,
													   UINT,
													   LCID,
													   DISPID*);
			HRESULT (STDMETHODCALLTYPE* Invoke)(IDispatchType*,
												DISPID,
												REFIID,
												LCID,
												WORD,
												DISPPARAMS*,
												VARIANT*,
												EXCEPINFO*,
												UINT*);
			HRESULT (STDMETHODCALLTYPE* put_AllowWindowlessActivation)(IDispatchType*,
																	   VARIANT_BOOL);
			HRESULT (STDMETHODCALLTYPE* get_AllowWindowlessActivation)(IDispatchType*,
																	   VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* put_BackColor)(IDispatchType*,
													   OLE_COLOR);
			HRESULT (STDMETHODCALLTYPE* get_BackColor)(IDispatchType*,
													   OLE_COLOR*);
			HRESULT (STDMETHODCALLTYPE* put_ForeColor)(IDispatchType*,
													   OLE_COLOR);
			HRESULT (STDMETHODCALLTYPE* get_ForeColor)(IDispatchType*,
													   OLE_COLOR*);
			HRESULT (STDMETHODCALLTYPE* put_LocaleID)(IDispatchType*,
													  LCID);
			HRESULT (STDMETHODCALLTYPE* get_LocaleID)(IDispatchType*,
													  LCID*);
			HRESULT (STDMETHODCALLTYPE* put_UserMode)(IDispatchType*,
													  VARIANT_BOOL);
			HRESULT (STDMETHODCALLTYPE* get_UserMode)(IDispatchType*,
													  VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* put_DisplayAsDefault)(IDispatchType*,
															  VARIANT_BOOL);
			HRESULT (STDMETHODCALLTYPE* get_DisplayAsDefault)(IDispatchType*,
															  VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* put_Font)(IDispatchType*,
												  IFontDisp*);
			HRESULT (STDMETHODCALLTYPE* get_Font)(IDispatchType*,
												  IFontDisp**);
			HRESULT (STDMETHODCALLTYPE* put_MessageReflect)(IDispatchType*,
															VARIANT_BOOL);
			HRESULT (STDMETHODCALLTYPE* get_MessageReflect)(IDispatchType*,
															VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* get_ShowGrabHandles)(IDispatchType*,
															 VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* get_ShowHatching)(IDispatchType*,
														  VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* put_DocHostFlags)(IDispatchType*,
														  DWORD);
			HRESULT (STDMETHODCALLTYPE* get_DocHostFlags)(IDispatchType*,
														  DWORD*);
			HRESULT (STDMETHODCALLTYPE* put_DocHostDoubleClickFlags)(IDispatchType*,
																	 DWORD);
			HRESULT (STDMETHODCALLTYPE* get_DocHostDoubleClickFlags)(IDispatchType*,
																	 DWORD*);
			HRESULT (STDMETHODCALLTYPE* put_AllowContextMenu)(IDispatchType*,
															  VARIANT_BOOL);
			HRESULT (STDMETHODCALLTYPE* get_AllowContextMenu)(IDispatchType*,
															  VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* put_AllowShowUI)(IDispatchType*,
														 VARIANT_BOOL);
			HRESULT (STDMETHODCALLTYPE* get_AllowShowUI)(IDispatchType*,
														 VARIANT_BOOL*);
			HRESULT (STDMETHODCALLTYPE* put_OptionKeyPath)(IDispatchType*,
														   BSTR);
			HRESULT (STDMETHODCALLTYPE* get_OptionKeyPath)(IDispatchType*,
														   BSTR*);
		};
		
		struct IDispatchType
		{
			IDispatchVtbl* pVtbl;
		};
	
	public:
		typedef std::vector<std::pair<IDispatchType*, AmbientDispatchHook*> > Map;
	
	public:
		AmbientDispatchHook(HtmlMessageViewWindow* pHtmlMessageViewWindow,
							IDispatch* pDispatch);
		~AmbientDispatchHook();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** pv);
	
	public:
		STDMETHOD(GetIDsOfNames)(REFIID riid,
								 OLECHAR** rgszNames,
								 unsigned int cNames,
								 LCID lcid,
								 DISPID* pDispId);
		STDMETHOD(GetTypeInfo)(unsigned int nTypeInfo,
							   LCID lcid,
							   ITypeInfo** ppTypeInfo);
		STDMETHOD(GetTypeInfoCount)(unsigned int* pcTypeInfo);
		STDMETHOD(Invoke)(DISPID dispId,
						  REFIID riid,
						  LCID lcid,
						  WORD wFlags,
						  DISPPARAMS* pDispParams,
						  VARIANT* pVarResult,
						  EXCEPINFO* pExcepInfo,
						  unsigned int* pnArgErr);
	
	public:
		STDMETHOD(put_AllowWindowlessActivation)(VARIANT_BOOL b);
		STDMETHOD(get_AllowWindowlessActivation)(VARIANT_BOOL* pb);
		STDMETHOD(put_BackColor)(OLE_COLOR cr);
		STDMETHOD(get_BackColor)(OLE_COLOR* pcr);
		STDMETHOD(put_ForeColor)(OLE_COLOR cr);
		STDMETHOD(get_ForeColor)(OLE_COLOR* pcr);
		STDMETHOD(put_LocaleID)(LCID lcid);
		STDMETHOD(get_LocaleID)(LCID* plcid);
		STDMETHOD(put_UserMode)(VARIANT_BOOL b);
		STDMETHOD(get_UserMode)(VARIANT_BOOL* pb);
		STDMETHOD(put_DisplayAsDefault)(VARIANT_BOOL b);
		STDMETHOD(get_DisplayAsDefault)(VARIANT_BOOL* pb);
		STDMETHOD(put_Font)(IFontDisp* pFont);
		STDMETHOD(get_Font)(IFontDisp** ppFont);
		STDMETHOD(put_MessageReflect)(VARIANT_BOOL b);
		STDMETHOD(get_MessageReflect)(VARIANT_BOOL* pb);
		STDMETHOD(get_ShowGrabHandles)(VARIANT_BOOL* pb);
		STDMETHOD(get_ShowHatching)(VARIANT_BOOL* pb);
		STDMETHOD(put_DocHostFlags)(DWORD dw);
		STDMETHOD(get_DocHostFlags)(DWORD* pdw);
		STDMETHOD(put_DocHostDoubleClickFlags)(DWORD dw);
		STDMETHOD(get_DocHostDoubleClickFlags)(DWORD* pdw);
		STDMETHOD(put_AllowContextMenu)(VARIANT_BOOL b);
		STDMETHOD(get_AllowContextMenu)(VARIANT_BOOL* pb);
		STDMETHOD(put_AllowShowUI)(VARIANT_BOOL b);
		STDMETHOD(get_AllowShowUI)(VARIANT_BOOL* pb);
		STDMETHOD(put_OptionKeyPath)(BSTR bstr);
		STDMETHOD(get_OptionKeyPath)(BSTR* pbstr);
	
	private:
		AmbientDispatchHook* getThis();
		IDispatchType* getDispatchType();
		IDispatchVtbl* getVtbl();
	
	private:
		static AmbientDispatchHook* get(IDispatchType* pDispatchType);
	
	private:
		AmbientDispatchHook(const AmbientDispatchHook&);
		AmbientDispatchHook& operator=(const AmbientDispatchHook&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pHtmlMessageViewWindow_;
		IDispatch* pDispatch_;
		IDispatchVtbl* pDispatchVtbl_;
	
	private:
		static Map map__;
	};
	friend class AmbientDispatchHook;

private:
	qs::Profile* pProfile_;
	const WCHAR* pwszSection_;
	MessageModel* pMessageModel_;
	qs::MenuManager* pMenuManager_;
	MessageViewWindowCallback* pCallback_;
	IWebBrowser2* pWebBrowser_;
	IServiceProviderImpl* pServiceProvider_;
	IInternetSecurityManagerImpl* pSecurityManager_;
	DWebBrowserEvents2Impl* pWebBrowserEvents_;
	DWORD dwConnectionPointCookie_;
	bool bAllowExternal_;
	bool bActivate_;
	bool bOnlineMode_;
#ifndef _WIN32_WCE
	std::auto_ptr<qs::Theme> pTheme_;
#endif
};

#elif defined QMHTMLVIEWHTMLCTRL

class HtmlMessageViewWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public qs::NotifyHandler,
	public MessageViewWindow
{
public:
	HtmlMessageViewWindow(qs::Profile* pProfile,
						  const WCHAR* pwszSection,
						  MessageWindow* pMessageWindow,
						  MessageModel* pMessageModel,
						  qs::MenuManager* pMenuManager,
						  MessageViewWindowCallback* pCallback);
	virtual ~HtmlMessageViewWindow();

public:
	virtual qs::wstring_ptr getSuperClass();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual qs::Window& getWindow();
	virtual bool isActive();
	virtual void setActive();
	virtual bool setMessage(MessageHolder* pmh,
							Message* pMessage,
							const Template* pTemplate,
							const WCHAR* pwszEncoding,
							unsigned int nFlags,
							unsigned int nSecurityMode);
	virtual bool scrollPage(bool bPrev);
	virtual void setSelectMode(bool bSelectMode);
	virtual void setQuoteMode(bool bQuoteMode);
	virtual bool find(const WCHAR* pwszFind,
					  unsigned int nFlags);
	virtual unsigned int getSupportedFindFlags() const;
	virtual std::auto_ptr<MessageWindow::Mark> mark() const;
	virtual void reset(const MessageWindow::Mark& mark);
	virtual bool openLink();

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void selectAll();
	virtual bool canSelectAll();

private:
	LRESULT onContextMenu(NMHDR* pnmhdr,
						  bool* pbHandled);
	LRESULT onHotSpot(NMHDR* pnmhdr,
					  bool* pbHandled);
	LRESULT onInlineImage(NMHDR* pnmhdr,
						  bool* pbHandled);
	LRESULT onInline(NMHDR* pnmhdr,
					 bool* pbHandled);
	LRESULT onMeta(NMHDR* pnmhdr,
				   bool* pbHandled);

private:
	qs::wstring_ptr getTarget(NMHDR* pnmhdr) const;

private:
#if 0
	class DWebBrowserEvents2Impl : public _DPIEWebBrowserEvents2
	{
	public:
		DWebBrowserEvents2Impl(HtmlMessageViewWindow* pHtmlMessageViewWindow,
							   IBrowser2* pWebBrowser);
		~DWebBrowserEvents2Impl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** pv);
	
	public:
		STDMETHOD(GetIDsOfNames)(REFIID riid,
								 OLECHAR** rgszNames,
								 unsigned int cNames,
								 LCID lcid,
								 DISPID* pDispId);
		STDMETHOD(GetTypeInfo)(unsigned int nTypeInfo,
							   LCID lcid,
							   ITypeInfo** ppTypeInfo);
		STDMETHOD(GetTypeInfoCount)(unsigned int* pcTypeInfo);
		STDMETHOD(Invoke)(DISPID dispId,
						  REFIID riid,
						  LCID lcid,
						  WORD wFlags,
						  DISPPARAMS* pDispParams,
						  VARIANT* pVarResult,
						  EXCEPINFO* pExcepInfo,
						  unsigned int* pnArgErr);
	
	private:
		DWebBrowserEvents2Impl(const DWebBrowserEvents2Impl&);
		DWebBrowserEvents2Impl& operator=(const DWebBrowserEvents2Impl&);
	
	private:
		ULONG nRef_;
		HtmlMessageViewWindow* pHtmlMessageViewWindow_;
		IBrowser2* pWebBrowser_;
	};
	friend class DWebBrowserEvents2Impl;
#endif

private:
	HtmlMessageViewWindow(const HtmlMessageViewWindow&);
	HtmlMessageViewWindow& operator=(const HtmlMessageViewWindow&);

private:
	qs::Profile* pProfile_;
	const WCHAR* pwszSection_;
	MessageWindow* pMessageWindow_;
	MessageModel* pMessageModel_;
	qs::MenuManager* pMenuManager_;
	MessageViewWindowCallback* pCallback_;
	UINT nId_;
	IBrowser2* pWebBrowser_;
#if 0
	DWebBrowserEvents2Impl* pWebBrowserEvents_;
	DWORD dwConnectionPointCookie_;
#endif
	bool bAllowExternal_;
	bool bOnlineMode_;
};

#endif // QMHTMLVIEWHTMLCTRL

#endif // QMHTMLVIEW

}

#endif // __MESSAGEVIEWWINDOW_H__
