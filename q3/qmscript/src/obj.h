/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __OBJ_H__
#define __OBJ_H__

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmainwindow.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>

#include <qs.h>
#include <qsprofile.h>

#include "actioninvoker.h"

#ifndef DEPENDCHECK
#	include "qmobj.h"
#endif


namespace qmscript {

class ObjectBase;
template<class T> class Object;
	template<class T, class I, const IID* piid> class DispObject;
template<class T> class ClassFactoryImpl;
template<class I, const IID* piid, class T, class Traits> class EnumBase;
template<class T> struct EnumTraits;
class ApplicationImpl;
class DocumentImpl;
class AccountImpl;
class AccountListImpl;
template<class T> class FolderBase;
class NormalFolderImpl;
class QueryFolderImpl;
class FolderListImpl;
class MessageHolderImpl;
class MessageHolderListImpl;
class MessageImpl;
class MainWindowImpl;
class EditFrameWindowImpl;
class MessageFrameWindowImpl;
class MacroImpl;
class ArgumentListImpl;
class ResultImpl;
class Factory;


/****************************************************************************
 *
 * ObjectBase
 *
 */

class ObjectBase
{
protected:
	ObjectBase();
	~ObjectBase();

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv) = 0;

private:
	ObjectBase(const ObjectBase&);
	ObjectBase& operator=(const ObjectBase&);
};


/****************************************************************************
 *
 * Object
 *
 */

template<class T>
class Object : public T
{
public:
	Object();
	~Object();

public:
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

private:
	Object(const Object&);
	Object& operator=(const Object&);

private:
	ULONG nRef_;
};


/****************************************************************************
 *
 * DispObject
 *
 */

template<class T, class I, const IID* piid>
class DispObject : public Object<T>
{
public:
	DispObject();
	~DispObject();

public:
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);

public:
	STDMETHOD(GetIDsOfNames)(REFIID riid,
							 LPOLESTR* ppwszNames,
							 unsigned int nNames,
							 LCID lcid,
							 DISPID* pDispId);
	STDMETHOD(GetTypeInfo)(unsigned int nTypeInfo,
						   LCID lcid,
						   ITypeInfo** ppTypeInfo);
	STDMETHOD(GetTypeInfoCount)(unsigned int* pnCount);
	STDMETHOD(Invoke)(DISPID dispId,
					  REFIID riid,
					  LCID lcid,
					  WORD wFlags,
					  DISPPARAMS* pDispParams,
					  VARIANT* pvarResult,
					  EXCEPINFO* pExcepInfo,
					  unsigned int* pnArgErr);

private:
	DispObject(const DispObject&);
	DispObject& operator=(const DispObject&);

private:
	ITypeInfo* pTypeInfo_;
};


/****************************************************************************
 *
 * ClassFacatoryImpl
 *
 */

template<class T>
class ClassFactoryImpl : public ObjectBase, public IClassFactory
{
protected:
	ClassFactoryImpl();
	~ClassFactoryImpl();

protected:
	HRESULT internalQueryInterface(REFIID riid,
								   void** ppv);

public:
	STDMETHOD(CreateInstance)(IUnknown* pUnkOuter,
							  REFIID riid,
							  void** ppv);
	STDMETHOD(LockServer)(BOOL bLock);

private:
	ClassFactoryImpl(const ClassFactoryImpl&);
	ClassFactoryImpl& operator=(const ClassFactoryImpl&);
};


/****************************************************************************
 *
 * EnumBase
 *
 */

template<class I, const IID* piid, class T, class Traits = EnumTraits<T> >
class EnumBase :
	public ObjectBase,
	public I
{
protected:
	EnumBase();
	~EnumBase();

public:
	bool init(T* p,
			  size_t nCount);
	bool init(T* p,
			  size_t nCount,
			  const Traits& traits);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(Next)(ULONG nElem,
					T* p,
					ULONG* pnFetched);
	STDMETHOD(Skip)(ULONG nElem);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(I** ppEnum);

private:
	EnumBase(const EnumBase&);
	EnumBase& operator=(const EnumBase&);

private:
	typedef std::vector<T> List;

private:
	Traits traits_;
	List list_;
	List::size_type n_;
};


/****************************************************************************
 *
 * EnumTraits
 *
 */

template<class T>
struct EnumTraits
{
	bool init(T* p) const;
	void destroy(T* p) const;
	bool copy(T* pTo,
			  T* pFrom) const;
};


template<>
struct EnumTraits<VARIANT>
{
	bool init(VARIANT* p) const;
	void destroy(VARIANT* p) const;
	bool copy(VARIANT* pTo,
			  VARIANT* pFrom) const;
};


/****************************************************************************
 *
 * ApplicationImpl
 *
 */

class ApplicationImpl :
	public ObjectBase,
	public IApplication
{
protected:
	ApplicationImpl();
	~ApplicationImpl();

public:
	void init(qm::Application* pApplication);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_version)(BSTR* pbstrVersion);
	STDMETHOD(get_nothing)(IDispatch** ppNothing);

private:
	ApplicationImpl(const ApplicationImpl&);
	ApplicationImpl& operator=(const ApplicationImpl&);

private:
	qm::Application* pApplication_;
};

typedef DispObject<ApplicationImpl, IApplication, &IID_IApplication> ApplicationObj;


/****************************************************************************
 *
 * DocumentImpl
 *
 */

class DocumentImpl :
	public ObjectBase,
	public IDocument
{
protected:
	DocumentImpl();
	~DocumentImpl();

public:
	void init(qm::Document* pDocument);
	qm::Document* getDocument() const;

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_accounts)(IAccountList** ppAccountList);
	STDMETHOD(get_offline)(VARIANT_BOOL* pbOffline);
	STDMETHOD(put_offline)(VARIANT_BOOL bOffline);

private:
	DocumentImpl(const DocumentImpl&);
	DocumentImpl& operator=(const DocumentImpl&);

private:
	qm::Document* pDocument_;
};

typedef DispObject<DocumentImpl, IDocument, &IID_IDocument> DocumentObj;


/****************************************************************************
 *
 * AccountImpl
 *
 */

class AccountImpl :
	public ObjectBase,
	public IAccount
{
protected:
	AccountImpl();
	~AccountImpl();

public:
	void init(qm::Account* pAccount);
	qm::Account* getAccount() const;

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_name)(BSTR* pbstrName);
	STDMETHOD(get_folders)(IFolderList** ppFolderList);

private:
	AccountImpl(const AccountImpl&);
	AccountImpl& operator=(const AccountImpl&);

private:
	qm::Account* pAccount_;
};

typedef DispObject<AccountImpl, IAccount, &IID_IAccount> AccountObj;


/****************************************************************************
 *
 * AccountListImpl
 *
 */

class AccountListImpl : public ObjectBase, public IAccountList
{
protected:
	AccountListImpl();
	~AccountListImpl();

public:
	void init(qm::Document* pDocument);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_item)(VARIANT varIndexOrName,
						IAccount** ppAccount);
	STDMETHOD(get_length)(unsigned int* pnLength);
	STDMETHOD(_newEnum)(IUnknown** ppUnk);

private:
	AccountListImpl(const AccountListImpl&);
	AccountListImpl& operator=(const AccountListImpl&);

private:
	qm::Document* pDocument_;
};

typedef DispObject<AccountListImpl, IAccountList, &IID_IAccountList> AccountListObj;


/****************************************************************************
 *
 * FolderBase
 *
 */

template<class T>
class FolderBase : public T
{
public:
	FolderBase();
	~FolderBase();

public:
	void init(qm::Folder* pFolder);

public:
	STDMETHOD(get_id)(unsigned int* pnId);
	STDMETHOD(get_type)(BSTR* pbstrType);
	STDMETHOD(get_name)(BSTR* pbstrName);
	STDMETHOD(get_fullName)(BSTR* pbstrFullName);
	STDMETHOD(get_separator)(BSTR* pbstrSeparator);
	STDMETHOD(get_flags)(unsigned int* pnFlags);
	STDMETHOD(get_parent)(IFolder** ppParent);
	STDMETHOD(get_account)(IAccount** ppAccount);
	STDMETHOD(get_messages)(IMessageHolderList** ppMessageHolderList);

private:
	FolderBase(const FolderBase&);
	FolderBase& operator=(const FolderBase&);

private:
	qm::Folder* pFolder_;
};


/****************************************************************************
 *
 * NormalFolderImpl
 *
 */

class NormalFolderImpl :
	public ObjectBase,
	public INormalFolder
{
protected:
	NormalFolderImpl();
	~NormalFolderImpl();

public:
	void init(qm::Folder* pFolder);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

private:
	NormalFolderImpl(const NormalFolderImpl&);
	NormalFolderImpl& operator=(const NormalFolderImpl&);
};

typedef DispObject<FolderBase<NormalFolderImpl>, INormalFolder, &IID_INormalFolder> NormalFolderObj;


/****************************************************************************
 *
 * QueryFolderImpl
 *
 */

class QueryFolderImpl :
	public ObjectBase,
	public IQueryFolder
{
protected:
	QueryFolderImpl();
	~QueryFolderImpl();

public:
	void init(qm::Folder* pFolder);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

private:
	QueryFolderImpl(const QueryFolderImpl&);
	QueryFolderImpl& operator=(const QueryFolderImpl&);
};

typedef DispObject<FolderBase<QueryFolderImpl>, IQueryFolder, &IID_IQueryFolder> QueryFolderObj;


/****************************************************************************
 *
 * FolderListImpl
 *
 */

class FolderListImpl :
	public ObjectBase,
	public IFolderList
{
protected:
	FolderListImpl();
	~FolderListImpl();

public:
	void init(qm::Account* pAccount);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_item)(VARIANT varIndexOrName,
						IFolder** ppFolder);
	STDMETHOD(get_length)(unsigned int* pnLength);
	STDMETHOD(_newEnum)(IUnknown** ppUnk);

private:
	FolderListImpl(const FolderListImpl&);
	FolderListImpl& operator=(const FolderListImpl&);

private:
	qm::Account* pAccount_;
};

typedef DispObject<FolderListImpl, IFolderList, &IID_IFolderList> FolderListObj;


/****************************************************************************
 *
 * MessageHolderImpl
 *
 */

class MessageHolderImpl :
	public ObjectBase,
	public IMessageHolder
{
protected:
	MessageHolderImpl();
	~MessageHolderImpl();

public:
	void init(qm::MessageHolder* pmh);
	qm::MessageHolder* getMessageHolder() const;

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_id)(unsigned int* pnId);
	STDMETHOD(get_flags)(unsigned int* pnFlags);
	STDMETHOD(get_from)(BSTR* pbstrFrom);
	STDMETHOD(get_to)(BSTR* pbstrTo);
	STDMETHOD(get_fromTo)(BSTR* pbstrFromTo);
	STDMETHOD(get_subject)(BSTR* pbstrSubject);
	STDMETHOD(get_date)(DATE* pDate);
	STDMETHOD(get_size)(unsigned int* pnSize);
	STDMETHOD(get_folder)(IFolder** ppFolder);
	STDMETHOD(get_message)(IMessage** ppMessage);

private:
	HRESULT getString(qs::wstring_ptr (qm::MessageHolder::*pfn)() const,
					  BSTR* pbstr);

private:
	MessageHolderImpl(const MessageHolderImpl&);
	MessageHolderImpl& operator=(const MessageHolderImpl&);

private:
	qm::MessageHolder* pmh_;
};

typedef DispObject<MessageHolderImpl, IMessageHolder, &IID_IMessageHolder> MessageHolderObj;


/****************************************************************************
 *
 * MessageHolderListImpl
 *
 */

class MessageHolderListImpl :
	public ObjectBase,
	public IMessageHolderList
{
protected:
	MessageHolderListImpl();
	~MessageHolderListImpl();

public:
	void init(qm::Folder* pFolder);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_item)(unsigned int nIndex,
						IMessageHolder** ppMessageHolder);
	STDMETHOD(get_length)(unsigned int* pnLength);

private:
	MessageHolderListImpl(const MessageHolderListImpl&);
	MessageHolderListImpl& operator=(const MessageHolderListImpl&);

private:
	qm::Folder* pFolder_;
};

typedef DispObject<MessageHolderListImpl, IMessageHolderList, &IID_IMessageHolderList> MessageHolderListObj;


/****************************************************************************
 *
 * MessageImpl
 *
 */

class MessageImpl : public ObjectBase, public IMessage
{
protected:
	MessageImpl();
	~MessageImpl();

public:
	qm::Message* getMessage();

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_content)(VARIANT* pvarContent);
	STDMETHOD(get_bodyText)(BSTR bstrQuote,
							BSTR bstrCharset,
							BSTR* pbstrBody);

private:
	MessageImpl(const MessageImpl&);
	MessageImpl& operator=(const MessageImpl&);

private:
	qm::Message msg_;
};

typedef DispObject<MessageImpl, IMessage, &IID_IMessage> MessageObj;


/****************************************************************************
 *
 * MainWindowImpl
 *
 */

class MainWindowImpl :
	public ObjectBase,
	public IMainWindow
{
protected:
	MainWindowImpl();
	~MainWindowImpl();

public:
	void init(qm::MainWindow* pMainWindow);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:

protected:
	const qm::ActionInvoker* getActionInvoker() const;

private:
	MainWindowImpl(const MainWindowImpl&);
	MainWindowImpl& operator=(const MainWindowImpl&);

private:
	qm::MainWindow* pMainWindow_;
};

typedef DispObject<ActionInvokeHelper<MainWindowImpl>,
	IMainWindow, &IID_IMainWindow> MainWindowObj;


/****************************************************************************
 *
 * EditFrameWindowImpl
 *
 */

class EditFrameWindowImpl :
	public ObjectBase,
	public IEditFrameWindow
{
protected:
	EditFrameWindowImpl();
	~EditFrameWindowImpl();

public:
	void init(qm::EditFrameWindow* pEditFrameWindow);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:

protected:
	const qm::ActionInvoker* getActionInvoker() const;

private:
	EditFrameWindowImpl(const EditFrameWindowImpl&);
	EditFrameWindowImpl& operator=(const EditFrameWindowImpl&);

private:
	qm::EditFrameWindow* pEditFrameWindow_;
};

typedef DispObject<ActionInvokeHelper<EditFrameWindowImpl>,
	IEditFrameWindow, &IID_IEditFrameWindow> EditFrameWindowObj;


/****************************************************************************
 *
 * MessageFrameWindowImpl
 *
 */

class MessageFrameWindowImpl :
	public ObjectBase,
	public IMessageFrameWindow
{
protected:
	MessageFrameWindowImpl();
	~MessageFrameWindowImpl();

public:
	void init(qm::MessageFrameWindow* pMessageFrameWindow);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:

protected:
	const qm::ActionInvoker* getActionInvoker() const;

private:
	MessageFrameWindowImpl(const MessageFrameWindowImpl&);
	MessageFrameWindowImpl& operator=(const MessageFrameWindowImpl&);

private:
	qm::MessageFrameWindow* pMessageFrameWindow_;
};

typedef DispObject<ActionInvokeHelper<MessageFrameWindowImpl>,
	IMessageFrameWindow, &IID_IMessageFrameWindow> MessageFrameWindowObj;


/****************************************************************************
 *
 * MacroImpl
 *
 */

class MacroImpl : public ObjectBase, public IMacro
{
protected:
	MacroImpl();
	~MacroImpl();

public:
	void init(std::auto_ptr<qm::Macro> pMacro,
			  qm::Document* pDocument,
			  qs::Profile* pProfile,
			  HWND hwnd);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(evaluate)(IMessageHolder* pMessageHolder,
						IAccount* pAccount,
						VARIANT* pvarResult);
	STDMETHOD(setVariable)(BSTR bstrName,
						   VARIANT var);
	STDMETHOD(getVariable)(BSTR bstrName,
						   VARIANT* pVar);
	STDMETHOD(removeVariable)(BSTR bstrName);

private:
	MacroImpl(const MacroImpl&);
	MacroImpl& operator=(const MacroImpl&);

private:
	typedef std::vector<std::pair<qs::WSTRING, VARIANT> > VariableList;

private:
	std::auto_ptr<qm::Macro> pMacro_;
	qm::Document* pDocument_;
	qs::Profile* pProfile_;
	HWND hwnd_;
	VariableList listVariable_;
};

typedef DispObject<MacroImpl, IMacro, &IID_IMacro> MacroObj;


/****************************************************************************
 *
 * MacroParserImpl
 *
 */

class MacroParserImpl : public ObjectBase, public IMacroParser
{
protected:
	MacroParserImpl();
	~MacroParserImpl();

public:
	void init(qm::Document* pDocument,
			  qs::Profile* pProfile,
			  HWND hwnd);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(parse)(BSTR bstrMacro,
					 IMacro** ppMacro);

private:
	MacroParserImpl(const MacroParserImpl&);
	MacroParserImpl& operator=(const MacroParserImpl&);

private:
	qm::Document* pDocument_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};

typedef DispObject<MacroParserImpl, IMacroParser, &IID_IMacroParser> MacroParserObj;


/****************************************************************************
 *
 * ArgumentListImpl
 *
 */

class ArgumentListImpl :
	public ObjectBase,
	public IArgumentList
{
protected:
	ArgumentListImpl();
	~ArgumentListImpl();

public:
	bool init(VARIANT* pvar,
			  size_t nCount);

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(get_item)(unsigned int nIndex,
						VARIANT* pvarArg);
	STDMETHOD(get_length)(unsigned int* pnLength);
	STDMETHOD(_newEnum)(IUnknown** ppUnk);

private:
	ArgumentListImpl(const ArgumentListImpl&);
	ArgumentListImpl& operator=(const ArgumentListImpl&);

private:
	typedef std::vector<VARIANT> ArgumentList;

private:
	ArgumentList listArgument_;
};

typedef DispObject<ArgumentListImpl, IArgumentList, &IID_IArgumentList> ArgumentListObj;


/****************************************************************************
 *
 * ResultImpl
 *
 */

class ResultImpl : public ObjectBase, public IResult
{
protected:
	ResultImpl();
	~ResultImpl();

public:
	VARIANT* getValue();

protected:
	virtual HRESULT internalQueryInterface(REFIID riid,
										   void** ppv);

public:
	STDMETHOD(put_value)(VARIANT varValue);

private:
	ResultImpl(const ResultImpl&);
	ResultImpl& operator=(const ResultImpl&);

private:
	VARIANT varValue_;
};

typedef DispObject<ResultImpl, IResult, &IID_IResult> ResultObj;


/****************************************************************************
 *
 * Factory
 *
 */

class Factory
{
private:
	Factory();

public:
	~Factory();

public:
	IApplication* createApplication(qm::Application* pApplication);
	IDocument* createDocument(qm::Document* pDocument);
	IAccount* createAccount(qm::Account* pAccount);
	IFolder* createFolder(qm::Folder* pFolder);
	IMessageHolder* createMessageHolder(qm::MessageHolder* pmh);

public:
	static Factory& getFactory();

private:
	Factory(const Factory&);
	Factory& operator=(const Factory&);

private:
	static Factory factory__;
};

}

#include "obj.inl"

#endif // __OBJ_H__
