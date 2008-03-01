/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __DATAOBJECT_H__
#define __DATAOBJECT_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmmessageholder.h>

#include <qs.h>

#include <vector>

#include <objidl.h>


namespace qm {

class MessageDataObject;
class FolderDataObject;
#ifndef _WIN32_WCE
class URIDataObject;
#endif

class Account;
class AccountManager;
class MessageContext;
class MessageHolder;
class MessageHolderURI;
class MessageOperationCallback;
class TempFileCleaner;
class UndoManager;
class URI;
class URIResolver;


/****************************************************************************
 *
 * MessageDataObject
 *
 */

class MessageDataObject : public IDataObject
{
public:
	enum Flag {
		FLAG_NONE,
		FLAG_COPY,
		FLAG_MOVE
	};
	
	enum Format {
		FORMAT_FOLDER,
		FORMAT_MESSAGEHOLDERLIST,
		FORMAT_FLAG,
#ifndef _WIN32_WCE
		FORMAT_HDROP,
		FORMAT_FILEDESCRIPTOR,
		FORMAT_FILECONTENTS
#endif
	};

public:
	typedef std::vector<URI*> URIList;

public:
	MessageDataObject(AccountManager* pAccountManager,
					  const URIResolver* pURIResolver,
					  TempFileCleaner* pTempFileCleaner);
	MessageDataObject(AccountManager* pAccountManager,
					  const URIResolver* pURIResolver,
					  TempFileCleaner* pTempFileCleaner,
					  Folder* pFolder,
					  const MessageHolderList& l,
					  Flag flag);

#ifdef _WIN32_WCE
private:
	MessageDataObject(AccountManager* pAccountManager,
					  const URIResolver* pURIResolver);
#endif

public:
	~MessageDataObject();

public:
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);

public:
	STDMETHOD(GetData)(FORMATETC* pFormat,
					   STGMEDIUM* pMedium);
	STDMETHOD(GetDataHere)(FORMATETC* pFormat,
						   STGMEDIUM* pMedium);
	STDMETHOD(QueryGetData)(FORMATETC* pFormat);
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* pFormatIn,
									 FORMATETC* pFormatOut);
	STDMETHOD(SetData)(FORMATETC* pFormat,
					   STGMEDIUM* pMedium,
					   BOOL bRelease);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection,
							 IEnumFORMATETC** ppEnum);
	STDMETHOD(DAdvise)(FORMATETC* pFormat,
					   DWORD advf,
					   IAdviseSink* pSink,
					   DWORD* pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA** ppEnum);

public:
	static bool setClipboard(IDataObject* pDataObject);
	static qs::ComPtr<IDataObject> getClipboard(AccountManager* pAccountManager,
												const URIResolver* pURIResolver);
	static bool queryClipboard();
	static bool pasteMessages(IDataObject* pDataObject,
							  AccountManager* pAccountManager,
							  const URIResolver* pURIResolver,
							  NormalFolder* pFolderTo,
							  Flag flag,
							  MessageOperationCallback* pCallback,
							  UndoManager* pUndoManager);
	static bool canPasteMessage(IDataObject* pDataObject);
	static Flag getPasteFlag(IDataObject* pDataObject,
							 AccountManager* pAccountManager,
							 NormalFolder* pFolder);
	static Folder* getFolder(IDataObject* pDataObject,
							 AccountManager* pAccountManager);
	static bool getURIs(IDataObject* pDataObject,
						URIList* pList);

#ifndef _WIN32_WCE
private:
	bool createTempFiles();

private:
	static qs::wstring_ptr getName(MessageHolder* pmh,
								   int* pnUntitled);
	static qs::wstring_ptr getFileName(const WCHAR* pwszName);
#endif

private:
	MessageDataObject(const MessageDataObject&);
	MessageDataObject& operator=(const MessageDataObject&);

private:
	ULONG nRef_;
	AccountManager* pAccountManager_;
	const URIResolver* pURIResolver_;
	TempFileCleaner* pTempFileCleaner_;
	Folder* pFolder_;
	MessagePtrList listMessagePtr_;
	Flag flag_;
	qs::wstring_ptr wstrTempDir_;

public:
	static UINT nFormats__[];
	static FORMATETC formats__[];
};


/****************************************************************************
 *
 * FolderDataObject
 *
 */

class FolderDataObject : public IDataObject
{
public:
	enum Format {
		FORMAT_FOLDER
	};

public:
	explicit FolderDataObject(Account* pAccount);
	explicit FolderDataObject(Folder* pFolder);
	~FolderDataObject();

public:
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);

public:
	STDMETHOD(GetData)(FORMATETC* pFormat,
					   STGMEDIUM* pMedium);
	STDMETHOD(GetDataHere)(FORMATETC* pFormat,
						   STGMEDIUM* pMedium);
	STDMETHOD(QueryGetData)(FORMATETC* pFormat);
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* pFormatIn,
									 FORMATETC* pFormatOut);
	STDMETHOD(SetData)(FORMATETC* pFormat,
					   STGMEDIUM* pMedium,
					   BOOL bRelease);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection,
							 IEnumFORMATETC** ppEnum);
	STDMETHOD(DAdvise)(FORMATETC* pFormat,
					   DWORD advf,
					   IAdviseSink* pSink,
					   DWORD* pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA** ppEnum);

public:
	static bool canPasteFolder(IDataObject* pDataObject);
	static std::pair<Account*, Folder*> get(IDataObject* pDataObject,
											AccountManager* pAccountManager);

private:
	FolderDataObject(const FolderDataObject&);
	FolderDataObject& operator=(const FolderDataObject&);

private:
	ULONG nRef_;
	Account* pAccount_;
	Folder* pFolder_;

public:
	static UINT nFormats__[];
	static FORMATETC formats__[];
};


#ifndef _WIN32_WCE

/****************************************************************************
 *
 * URIDataObject
 *
 */

class URIDataObject : public IDataObject
{
public:
	enum Format {
		FORMAT_HDROP,
		FORMAT_FILEDESCRIPTOR,
		FORMAT_FILECONTENTS
	};

public:
	typedef std::vector<URI*> URIList;

public:
	URIDataObject(const URIResolver* pURIResolver,
				  TempFileCleaner* pTempFileCleaner,
				  unsigned int nSecurityMode,
				  URIList& listURI);
	~URIDataObject();

public:
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);

public:
	STDMETHOD(GetData)(FORMATETC* pFormat,
					   STGMEDIUM* pMedium);
	STDMETHOD(GetDataHere)(FORMATETC* pFormat,
						   STGMEDIUM* pMedium);
	STDMETHOD(QueryGetData)(FORMATETC* pFormat);
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* pFormatIn,
									 FORMATETC* pFormatOut);
	STDMETHOD(SetData)(FORMATETC* pFormat,
					   STGMEDIUM* pMedium,
					   BOOL bRelease);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection,
							 IEnumFORMATETC** ppEnum);
	STDMETHOD(DAdvise)(FORMATETC* pFormat,
					   DWORD advf,
					   IAdviseSink* pSink,
					   DWORD* pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA** ppEnum);

private:
	bool createTempFiles();
	const qs::Part* getPart(const URI* pURI,
							std::auto_ptr<MessageContext>* ppContext);

private:
	static qs::wstring_ptr getName(const URI* pURI,
								   int* pnUntitled);

private:
	URIDataObject(const URIDataObject&);
	URIDataObject& operator=(const URIDataObject&);

private:
	ULONG nRef_;
	const URIResolver* pURIResolver_;
	TempFileCleaner* pTempFileCleaner_;
	unsigned int nSecurityMode_;
	URIList listURI_;
	qs::wstring_ptr wstrTempDir_;

public:
	static UINT nFormats__[];
	static FORMATETC formats__[];
};

#endif


/****************************************************************************
 *
 * IEnumFORMATETCImpl
 *
 */

class IEnumFORMATETCImpl : public IEnumFORMATETC
{
public:
	IEnumFORMATETCImpl(FORMATETC* pFormatEtc,
					   size_t nCount);
	~IEnumFORMATETCImpl();

public:
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);

public:
	STDMETHOD(Next)(ULONG celt,
					FORMATETC* rgelt,
					ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumFORMATETC** ppEnum);

private:
	IEnumFORMATETCImpl(const IEnumFORMATETCImpl&);
	IEnumFORMATETCImpl& operator=(const IEnumFORMATETCImpl&);

private:
	ULONG nRef_;
	FORMATETC* pFormatEtc_;
	size_t nCount_;
	size_t nCurrent_;
};


}

#endif // __DATAOBJECT_H__
