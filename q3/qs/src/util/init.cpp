/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qserror.h>
#include <qsinit.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsthread.h>
#include <qsutil.h>

#include <memory>
#include <vector>
#include <algorithm>
#include <new.h>

#include <commctrl.h>

using namespace qs;

class qs::Initializer;


/****************************************************************************
 *
 * Global functions
 *
 */

#ifdef _WIN32_WCE
//void stlNewHandler()
//{
//	::RaiseException(QSTATUS_OUTOFMEMORY, 0, 0, 0);
//}
#else
int newHandler(size_t)
{
	throw std::bad_alloc();
}
#endif


/****************************************************************************
 *
 * InitImpl
 *
 */

struct qs::InitImpl
{
	typedef std::vector<ConverterFactory*> ConverterFactoryList;
	typedef std::vector<EncoderFactory*> EncoderFactoryList;
	
	QSTATUS setSystemEncodingAndFonts(const WCHAR* pwszEncoding);
	
	ThreadLocal* pInitThread_;
	HINSTANCE hInst_;
	WSTRING wstrTitle_;
	WSTRING wstrSystemEncoding_;
	WSTRING wstrFixedWidthFont_;
	WSTRING wstrProportionalFont_;
	bool bLogEnabled_;
	WSTRING wstrLogDir_;
	Logger::Level logLevel_;
	ConverterFactoryList listConverterFactory_;
	EncoderFactoryList listEncoderFactory_;
	
	static Init* pInit__;
	static Initializer* pInitializer__;
};

Init* qs::InitImpl::pInit__ = 0;
Initializer* qs::InitImpl::pInitializer__ = 0;

QSTATUS qs::InitImpl::setSystemEncodingAndFonts(const WCHAR* pwszEncoding)
{
	freeWString(wstrSystemEncoding_);
	
	MIMECPINFO cpinfo;
	ComPtr<IMultiLanguage> pMultiLanguage;
	HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_ALL,
		IID_IMultiLanguage, reinterpret_cast<void**>(&pMultiLanguage));
	if (hr != S_OK)
		return QSTATUS_FAIL;
	hr = pMultiLanguage->GetCodePageInfo(::GetACP(), &cpinfo);
	if (hr != S_OK)
		return QSTATUS_FAIL;
	
	if (!pwszEncoding)
		pwszEncoding = cpinfo.wszWebCharset;
	
	wstrSystemEncoding_ = allocWString(pwszEncoding);
	if (!wstrSystemEncoding_)
		return QSTATUS_OUTOFMEMORY;
	
	wstrFixedWidthFont_ = allocWString(cpinfo.wszFixedWidthFont);
	if (!wstrFixedWidthFont_)
		return QSTATUS_OUTOFMEMORY;
	
	wstrProportionalFont_ = allocWString(cpinfo.wszProportionalFont);
	if (!wstrProportionalFont_)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Init
 *
 */

qs::Init::Init(HINSTANCE hInst, const WCHAR* pwszTitle,
	unsigned int nFlags, unsigned int nThreadFlags, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
#ifdef _WIN32_WCE
//	std::set_stl_new_handler(stlNewHandler);
#else
	::_set_new_handler(newHandler);
#endif
	
#if defined _WIN32_WCE && _WIN32_WCE < 300
	HRESULT hr = ::CoInitializeEx(0, COINIT_MULTITHREADED);
#elif defined _WIN32_WCE && _WIN32_WCE >= 300
	HRESULT hr = ::CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (hr == E_INVALIDARG)
		hr = ::CoInitializeEx(0, COINIT_MULTITHREADED);
#else
	HRESULT hr = ::OleInitialize(0);
#endif
	if (FAILED(hr)) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	
	INITCOMMONCONTROLSEX icc = {
		sizeof(icc),
		ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES |
		ICC_PROGRESS_CLASS | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES
	};
	::InitCommonControlsEx(&icc);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	assert(pImpl_);
	pImpl_->pInitThread_ = 0;
	pImpl_->hInst_ = hInst;
	pImpl_->wstrTitle_ = 0;
	pImpl_->wstrSystemEncoding_ = 0;
	pImpl_->wstrFixedWidthFont_ = 0;
	pImpl_->wstrProportionalFont_ = 0;
	pImpl_->bLogEnabled_ = false;
	pImpl_->wstrLogDir_ = 0;
	pImpl_->logLevel_ = Logger::LEVEL_DEBUG;
	
	if (pwszTitle) {
		pImpl_->wstrTitle_ = allocWString(pwszTitle);
		if (!pImpl_->wstrTitle_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
#ifdef QS_KCONVERT
	status = pImpl_->setSystemEncodingAndFonts(L"shift_jis");
#else
	status = pImpl_->setSystemEncodingAndFonts(0);
#endif
	CHECK_QSTATUS_SET(pstatus);
	
	Initializer* pInitializer = InitImpl::pInitializer__;
	while (pInitializer) {
		status = pInitializer->init();
		CHECK_QSTATUS_SET(pstatus);
		pInitializer = pInitializer->getNext();
	}
	
#define DECLARE_CONVERTERFACTORY(x) \
	x* p##x = 0; \
	status = newQsObject(&p##x); \
	CHECK_QSTATUS_SET(pstatus); \
	pImpl_->listConverterFactory_.push_back(p##x)
	
	STLWrapper<InitImpl::ConverterFactoryList>(pImpl_->listConverterFactory_).reserve(6);
	DECLARE_CONVERTERFACTORY(UTF8ConverterFactory);
	DECLARE_CONVERTERFACTORY(UTF7ConverterFactory);
#ifdef QS_KCONVERT
	DECLARE_CONVERTERFACTORY(ShiftJISConverterFactory);
	DECLARE_CONVERTERFACTORY(ISO2022JPConverterFactory);
	DECLARE_CONVERTERFACTORY(EUCJPConverterFactory);
#endif
	DECLARE_CONVERTERFACTORY(MLangConverterFactory);
	
#define DECLARE_ENCODERFACTORY(x) \
	x* p##x = 0; \
	status = newQsObject(&p##x); \
	CHECK_QSTATUS_SET(pstatus); \
	pImpl_->listEncoderFactory_.push_back(p##x)
	
	STLWrapper<InitImpl::EncoderFactoryList>(pImpl_->listEncoderFactory_).reserve(6);
	DECLARE_ENCODERFACTORY(Base64EncoderFactory);
	DECLARE_ENCODERFACTORY(BEncoderFactory);
	DECLARE_ENCODERFACTORY(QuotedPrintableEncoderFactory);
	DECLARE_ENCODERFACTORY(QEncoderFactory);
	DECLARE_ENCODERFACTORY(UuencodeEncoderFactory);
	DECLARE_ENCODERFACTORY(XUuencodeEncoderFactory);
#undef DECLARE_ENCODERFACTORY
	
	status = newQsObject(&pImpl_->pInitThread_);
	CHECK_QSTATUS_SET(pstatus);
	assert(pImpl_->pInitThread_);
	
	InitImpl::pInit__ = this;
	
	std::auto_ptr<InitThread> apInitThread;
	status = newQsObject(nThreadFlags, &apInitThread);
	CHECK_QSTATUS_SET(pstatus);
	status = pImpl_->pInitThread_->set(apInitThread.get());
	CHECK_QSTATUS_SET(pstatus);
	apInitThread.release();
}

qs::Init::~Init()
{
	if (pImpl_->pInitThread_) {
		delete getInitThread();
		delete pImpl_->pInitThread_;
		pImpl_->pInitThread_ = 0;
	}
	
	InitImpl::pInit__ = 0;
	
	freeWString(pImpl_->wstrTitle_);
	freeWString(pImpl_->wstrSystemEncoding_);
	freeWString(pImpl_->wstrFixedWidthFont_);
	freeWString(pImpl_->wstrProportionalFont_);
	freeWString(pImpl_->wstrLogDir_);
	
	std::for_each(pImpl_->listEncoderFactory_.begin(),
		pImpl_->listEncoderFactory_.end(), deleter<EncoderFactory>());
	
	std::for_each(pImpl_->listConverterFactory_.begin(),
		pImpl_->listConverterFactory_.end(), deleter<ConverterFactory>());
	
	Initializer* pInitializer = InitImpl::pInitializer__;
	while (pInitializer) {
		pInitializer->term();
		pInitializer = pInitializer->getNext();
	}
	
	delete pImpl_;
	pImpl_ = 0;
	
#ifdef _WIN32_WCE
	::CoUninitialize();
#else
	::OleUninitialize();
#endif
}

HINSTANCE qs::Init::getInstanceHandle() const
{
	return pImpl_->hInst_;
}

const WCHAR* qs::Init::getTitle() const
{
	return pImpl_->wstrTitle_;
}

const WCHAR* qs::Init::getSystemEncoding() const
{
	return pImpl_->wstrSystemEncoding_;
}

const WCHAR* qs::Init::getDefaultFixedWidthFont() const
{
	return pImpl_->wstrFixedWidthFont_;
}

const WCHAR* qs::Init::getDefaultProportionalFont() const
{
	return pImpl_->wstrProportionalFont_;
}

InitThread* qs::Init::getInitThread()
{
	void* pValue = 0;
	pImpl_->pInitThread_->get(&pValue);
	return static_cast<InitThread*>(pValue);
}

bool qs::Init::isLogEnabled() const
{
	return pImpl_->bLogEnabled_;
}

const WCHAR* qs::Init::getLogDirectory() const
{
	return pImpl_->wstrLogDir_;
}

Logger::Level qs::Init::getLogLevel() const
{
	return pImpl_->logLevel_;
}

QSTATUS qs::Init::setLogInfo(bool bEnabled,
	const WCHAR* pwszDir, Logger::Level level)
{
	assert(!pImpl_->wstrLogDir_);
	
	pImpl_->wstrLogDir_ = allocWString(pwszDir);
	if (!pImpl_->wstrLogDir_)
		return QSTATUS_OUTOFMEMORY;
	
	pImpl_->bLogEnabled_ = bEnabled;
	pImpl_->logLevel_ = level;
	
	return QSTATUS_SUCCESS;
}

void qs::Init::setInitThread(InitThread* pInitThread)
{
	pImpl_->pInitThread_->set(pInitThread);
}

Init& qs::Init::getInit()
{
	return *InitImpl::pInit__;
}


/****************************************************************************
 *
 * InitThreadImpl
 *
 */

struct qs::InitThreadImpl
{
	QSTATUS createLogger();
	
	Synchronizer* pSynchronizer_;
	Logger* pLogger_;
};


QSTATUS qs::InitThreadImpl::createLogger()
{
	assert(!pLogger_);
	
	DECLARE_QSTATUS();
	
	const Init& init = Init::getInit();
	if (init.isLogEnabled()) {
		const WCHAR* pwszLogDir = init.getLogDirectory();
		if (!pwszLogDir)
			return QSTATUS_FAIL;
		
		Time time(Time::getCurrentTime());
		WCHAR wszName[128];
		swprintf(wszName, L"\\log-%04d%02d%02d%02d%02d%02d%03d-%u.log",
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
			::GetCurrentThreadId());
		
		string_ptr<WSTRING> wstrPath(concat(pwszLogDir, wszName));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		
		std::auto_ptr<FileOutputStream> pStream;
		status = newQsObject(wstrPath.get(), &pStream);
		CHECK_QSTATUS();
		std::auto_ptr<BufferedOutputStream> pBufferedStream;
		status = newQsObject(pStream.get(), true, &pBufferedStream);
		CHECK_QSTATUS();
		pStream.release();
		
		std::auto_ptr<Logger> pLogger;
		status = newQsObject(pBufferedStream.get(),
			true, init.getLogLevel(), &pLogger);
		CHECK_QSTATUS();
		pBufferedStream.release();
		
		pLogger_ = pLogger.release();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * InitThread
 *
 */

qs::InitThread::InitThread(unsigned int nFlags, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pSynchronizer_ = 0;
	pImpl_->pLogger_ = 0;
	
	Initializer* pInitializer = InitImpl::pInitializer__;
	while (pInitializer) {
		status = pInitializer->initThread();
		CHECK_QSTATUS_SET(pstatus);
		pInitializer = pInitializer->getNext();
	}
	
	if (nFlags & FLAG_SYNCHRONIZER) {
		status = newQsObject(&pImpl_->pSynchronizer_);
		CHECK_QSTATUS_SET(pstatus);
	}
	
	Init::getInit().setInitThread(this);
}

qs::InitThread::~InitThread()
{
	delete pImpl_->pSynchronizer_;
	delete pImpl_->pLogger_;
	
	Initializer* pInitializer = InitImpl::pInitializer__;
	while (pInitializer) {
		pInitializer->termThread();
		pInitializer = pInitializer->getNext();
	}
	
	Init::getInit().setInitThread(0);
	
	delete pImpl_;
	pImpl_ = 0;
}

Synchronizer* qs::InitThread::getSynchronizer() const
{
	assert(pImpl_->pSynchronizer_);
	return pImpl_->pSynchronizer_;
}

Logger* qs::InitThread::getLogger() const
{
	if (!pImpl_->pLogger_)
		pImpl_->createLogger();
	return pImpl_->pLogger_;
}

InitThread& qs::InitThread::getInitThread()
{
	return *Init::getInit().getInitThread();
}


/****************************************************************************
 *
 * Initializer
 *
 */

qs::Initializer::Initializer() :
	pNext_(0)
{
	pNext_ = InitImpl::pInitializer__;
	InitImpl::pInitializer__ = this;
}

qs::Initializer::~Initializer()
{
}

Initializer* qs::Initializer::getNext() const
{
	return pNext_;
}

QSTATUS qs::Initializer::initThread()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Initializer::termThread()
{
	return QSTATUS_SUCCESS;
}
