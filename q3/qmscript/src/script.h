/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <qmdocument.h>
#include <qmscript.h>

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

class ScriptImpl : public qm::Script
{
public:
	explicit ScriptImpl(const qm::ScriptFactory::Init& init);
	virtual ~ScriptImpl();

public:
	bool operator!() const;

public:
	virtual bool run(VARIANT* pvarArgs,
					 size_t nArgCount,
					 VARIANT* pvarResult);

private:
	bool load(const qm::ScriptFactory::Init& init);

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

class ScriptFactoryImpl : public qm::ScriptFactory
{
public:
	ScriptFactoryImpl();
	virtual ~ScriptFactoryImpl();

public:
	virtual std::auto_ptr<qm::Script> createScript(const Init& init);

private:
	ScriptFactoryImpl(const ScriptFactoryImpl&);
	ScriptFactoryImpl& operator=(const ScriptFactoryImpl&);

private:
	static ScriptFactoryImpl factory__;
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
	ActiveScriptSite(const qm::ScriptFactory::Init& init);
	~ActiveScriptSite();

public:
	bool setArguments(VARIANT* pvarArgs,
					  size_t nCount);
	VARIANT* getResult();

public:
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

public:
	STDMETHOD(GetLCID)(LCID* plcid);
	STDMETHOD(GetItemInfo)(LPCOLESTR pwszName,
						   DWORD dwReturnMask,
						   IUnknown** ppUnkItem,
						   ITypeInfo** ppTypeInfo);
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
	qs::ComPtr<IApplication> pApplication_;
	qs::ComPtr<IDocument> pDocument_;
	qs::ComPtr<IMacroParser> pMacroParser_;
	qs::ComPtr<IArgumentList> pArgumentList_;
	qs::ComPtr<IResult> pResult_;
	qs::ComPtr<IMainWindow> pMainWindow_;
	qs::ComPtr<IEditFrameWindow> pEditFrameWindow_;
	qs::ComPtr<IMessageFrameWindow> pMessageFrameWindow_;
	qs::ComPtr<IActionTarget> pActionTarget_;
};


/****************************************************************************
 *
 * ReaderIStream
 *
 */

class ReaderIStream : public IStream
{
public:
	explicit ReaderIStream(qs::Reader* pReader);
	~ReaderIStream();

public:
	STDMETHOD(QueryInterface)(REFIID riid,
							  void** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

public:
	STDMETHOD(Read)(void* pv,
					ULONG cb,
					ULONG* pcbRead);
	STDMETHOD(Write)(const void* pv,
					 ULONG cb,
					 ULONG* pcbWritten);

public:
	STDMETHOD(Seek)(LARGE_INTEGER nMove,
					DWORD dwOrigin,
					ULARGE_INTEGER* pnPosition);
	STDMETHOD(SetSize)(ULARGE_INTEGER nSize);
	STDMETHOD(CopyTo)(IStream* pStream,
					  ULARGE_INTEGER cb,
					  ULARGE_INTEGER* pcbRead,
					  ULARGE_INTEGER* pcbWritten);
	STDMETHOD(Commit)(DWORD dwFlags);
	STDMETHOD(Revert)();
	STDMETHOD(LockRegion)(ULARGE_INTEGER nOffset,
						  ULARGE_INTEGER cb,
						  DWORD dwType);
	STDMETHOD(UnlockRegion)(ULARGE_INTEGER nOffset,
							ULARGE_INTEGER cb,
							DWORD dwType);
	STDMETHOD(Stat)(STATSTG* pStatStg,
					DWORD dwFlags);
	STDMETHOD(Clone)(IStream** ppStream);

private:
	ReaderIStream(const ReaderIStream&);
	ReaderIStream& operator=(const ReaderIStream&);

private:
	qs::Reader* pReader_;
};

}

#endif // __SCRIPT_H__
