/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DATAOBJECT_H__
#define __DATAOBJECT_H__

#include <qm.h>
#include <qmfolder.h>

#include <qs.h>

#include <vector>

#include <objidl.h>


namespace qm {

class MessageDataObject;

class Account;
class MessageOperationCallback;
class Document;
class MessageHolder;


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
		FORMAT_ACCOUNT,
		FORMAT_MESSAGEHOLDERLIST,
		FORMAT_FLAG,
#ifndef _WIN32_WCE
		FORMAT_FILEDESCRIPTOR,
		FORMAT_FILECONTENTS
#endif
	};

public:
	MessageDataObject(Account* pAccount, const MessageHolderList& l,
		Flag flag, qs::QSTATUS* pstatus);
	~MessageDataObject();

public:
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

public:
	STDMETHOD(GetData)(FORMATETC* pFormat, STGMEDIUM* pMedium);
	STDMETHOD(GetDataHere)(FORMATETC* pFormat, STGMEDIUM* pMedium);
	STDMETHOD(QueryGetData)(FORMATETC* pFormat);
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* pFormatIn, FORMATETC* pFormatOut);
	STDMETHOD(SetData)(FORMATETC* pFormat, STGMEDIUM* pMedium, BOOL bRelease);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC** ppEnum);
	STDMETHOD(DAdvise)(FORMATETC* pFormat, DWORD advf,
		IAdviseSink* pSink, DWORD* pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA** ppEnum);

public:
	static qs::QSTATUS pasteMessages(IDataObject* pDataObject,
		Document* pDocument, NormalFolder* pFolderTo,
		Flag flag, MessageOperationCallback* pCallback);
	static bool canPasteMessage(IDataObject* pDataObject);
	static Flag getPasteFlag(IDataObject* pDataObject,
		Document* pDocument, NormalFolder* pFolder);

private:
	static qs::QSTATUS getFileName(
		const WCHAR* pwszName, qs::WSTRING* pwstrName);

private:
	MessageDataObject(const MessageDataObject&);
	MessageDataObject& operator=(const MessageDataObject&);

private:
	class IEnumFORMATETCImpl : public IEnumFORMATETC
	{
	public:
		IEnumFORMATETCImpl(qs::QSTATUS* pstatus);
		~IEnumFORMATETCImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	
	public:
		STDMETHOD(Next)(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched);
		STDMETHOD(Skip)(ULONG celt);
		STDMETHOD(Reset)();
		STDMETHOD(Clone)(IEnumFORMATETC** ppEnum);
	
	private:
		IEnumFORMATETCImpl(const IEnumFORMATETCImpl&);
		IEnumFORMATETCImpl& operator=(const IEnumFORMATETCImpl&);
	
	private:
		ULONG nRef_;
		int nCurrent_;
	};

private:
	ULONG nRef_;
	Account* pAccount_;
	MessagePtrList listMessagePtr_;
	Flag flag_;

public:
	static UINT nFormats__[];
	static FORMATETC formats__[];
};

}

#endif // __DATAOBJECT_H__
