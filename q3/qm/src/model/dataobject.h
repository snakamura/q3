/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
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
class MessageHolder;
class MessageOperationCallback;
class UndoManager;
class URI;


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
		FORMAT_FILEDESCRIPTOR,
		FORMAT_FILECONTENTS
#endif
	};

public:
	typedef std::vector<URI*> URIList;

public:
	MessageDataObject(AccountManager* pAccountManager);
	MessageDataObject(AccountManager* pAccountManager,
					  Folder* pFolder,
					  const MessageHolderList& l,
					  Flag flag);
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
	static qs::ComPtr<IDataObject> getClipboard(AccountManager* pAccountManager);
	static bool queryClipboard();
	static bool pasteMessages(IDataObject* pDataObject,
							  AccountManager* pAccountManager,
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

private:
	static qs::wstring_ptr getFileName(const WCHAR* pwszName);

private:
	MessageDataObject(const MessageDataObject&);
	MessageDataObject& operator=(const MessageDataObject&);

private:
	ULONG nRef_;
	AccountManager* pAccountManager_;
	Folder* pFolder_;
	MessagePtrList listMessagePtr_;
	Flag flag_;

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
		FORMAT_FILEDESCRIPTOR,
		FORMAT_FILECONTENTS
	};

public:
	typedef std::vector<URI*> URIList;

public:
	URIDataObject(AccountManager* pAccountManager,
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
	const qs::Part* getPart(URIList::size_type n,
							bool bBody,
							Message* pMessage);

private:
	URIDataObject(const URIDataObject&);
	URIDataObject& operator=(const URIDataObject&);

private:
	ULONG nRef_;
	AccountManager* pAccountManager_;
	unsigned int nSecurityMode_;
	URIList listURI_;

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
