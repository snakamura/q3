/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <qmscript.h>

#include <qmdocument.h>

#include <qs.h>

#include <activscp.h>

#include "obj.h"


namespace qmscript {

class ScriptImpl;
class ScriptFactoryImpl;
class ActiveScriptSite;
class ReaderIStream;


/****************************************************************************
 *
 * ScriptImpl
 *
 */

class ScriptImpl : public Script
{
public:
	ScriptImpl(const ScriptFactory::Init& init, qs::QSTATUS* pstatus);
	virtual ~ScriptImpl();

public:
	virtual qs::QSTATUS run(VARIANT* pvarArgs,
		size_t nArgCount, VARIANT* pvarResult);

private:
	qs::QSTATUS load(const ScriptFactory::Init& init);

private:
	ScriptImpl(const ScriptImpl&);
	ScriptImpl& operator=(const ScriptImpl&);

private:
	IActiveScript* pActiveScript_;
};


/****************************************************************************
 *
 * ScriptFactoryImpl
 *
 */

class ScriptFactoryImpl : public ScriptFactory
{
public:
	ScriptFactoryImpl(qs::QSTATUS* pstatus);
	virtual ~ScriptFactoryImpl();

public:
	virtual qs::QSTATUS newScript(const Init& init, Script** ppScript);
	virtual void deleteScript(Script* pScript);

private:
	ScriptFactoryImpl(const ScriptFactoryImpl&);
	ScriptFactoryImpl& operator=(const ScriptFactoryImpl&);
};


/****************************************************************************
 *
 * ActiveScriptSite
 *
 */

class ActiveScriptSite :
	public IActiveScriptSite,
	public IActiveScriptSiteWindow
{
public:
	ActiveScriptSite(const ScriptFactory::Init& init, qs::QSTATUS* pstatus);
	~ActiveScriptSite();

public:
	qs::QSTATUS setArguments(VARIANT* pvarArgs, size_t nCount);
	VARIANT* getResult();

public:
	STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

public:
	STDMETHOD(GetLCID)(LCID* plcid);
	STDMETHOD(GetItemInfo)(LPCOLESTR pwszName, DWORD dwReturnMask,
		IUnknown** ppUnkItem, ITypeInfo** ppTypeInfo);
	STDMETHOD(GetDocVersionString)(BSTR* pbstrVersionString);
	STDMETHOD(OnScriptTerminate)(const VARIANT* pvarResult,
		const EXCEPINFO* pExecpInfo);
	STDMETHOD(OnStateChange)(SCRIPTSTATE state);
	STDMETHOD(OnScriptError)(IActiveScriptError* pError);
	STDMETHOD(OnEnterScript)();
	STDMETHOD(OnLeaveScript)();

public:
	STDMETHOD(GetWindow)(HWND* phwnd);
	STDMETHOD(EnableModeless)(BOOL bEnable);

private:
	ActiveScriptSite(const ActiveScriptSite&);
	ActiveScriptSite& operator=(const ActiveScriptSite&);

private:
	ULONG nRef_;
	HWND hwnd_;
	qs::ModalHandler* pModalHandler_;
	IApplication* pApplication_;
	IDocument* pDocument_;
	IMacroParser* pMacroParser_;
	IArgumentList* pArgumentList_;
	IResult* pResult_;
	IMainWindow* pMainWindow_;
	IEditFrameWindow* pEditFrameWindow_;
	IMessageFrameWindow* pMessageFrameWindow_;
};


/****************************************************************************
 *
 * ReaderIStream
 *
 */

class ReaderIStream : public IStream
{
public:
	ReaderIStream(qs::Reader* pReader, qs::QSTATUS* pstatus);
	~ReaderIStream();

public:
	STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

public:
	STDMETHOD(Read)(void* pv, ULONG cb, ULONG* pcbRead);
	STDMETHOD(Write)(const void* pv, ULONG cb, ULONG* pcbWritten);

public:
	STDMETHOD(Seek)(LARGE_INTEGER nMove,
		DWORD dwOrigin, ULARGE_INTEGER* pnPosition);
	STDMETHOD(SetSize)(ULARGE_INTEGER nSize);
	STDMETHOD(CopyTo)(IStream* pStream, ULARGE_INTEGER cb,
		ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten);
	STDMETHOD(Commit)(DWORD dwFlags);
	STDMETHOD(Revert)();
	STDMETHOD(LockRegion)(ULARGE_INTEGER nOffset,
		ULARGE_INTEGER cb, DWORD dwType);
	STDMETHOD(UnlockRegion)(ULARGE_INTEGER nOffset,
		ULARGE_INTEGER cb, DWORD dwType);
	STDMETHOD(Stat)(STATSTG* pStatStg, DWORD dwFlags);
	STDMETHOD(Clone)(IStream** ppStream);

private:
	ReaderIStream(const ReaderIStream&);
	ReaderIStream& operator=(const ReaderIStream&);

private:
	qs::Reader* pReader_;
};

}

#endif // __SCRIPT_H__
