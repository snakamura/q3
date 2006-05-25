/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qsinit.h>
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
	
	bool setSystemEncodingAndFonts(const WCHAR* pwszEncoding);
	
	std::auto_ptr<ThreadLocal> pInitThread_;
	HINSTANCE hInst_;
	wstring_ptr wstrTitle_;
	wstring_ptr wstrSystemEncoding_;
	wstring_ptr wstrMailEncoding_;
	wstring_ptr wstrFixedWidthFont_;
	wstring_ptr wstrProportionalFont_;
	wstring_ptr wstrUIFont_;
	bool bLogEnabled_;
	wstring_ptr wstrLogDir_;
	Logger::Level logLevel_;
	wstring_ptr wstrLogFilter_;
	wstring_ptr wstrLogTimeFormat_;
	CriticalSection csLog_;
	ConverterFactoryList listConverterFactory_;
	EncoderFactoryList listEncoderFactory_;
	
	static Init* pInit__;
	static Initializer* pInitializer__;
};

Init* qs::InitImpl::pInit__ = 0;
Initializer* qs::InitImpl::pInitializer__ = 0;

bool qs::InitImpl::setSystemEncodingAndFonts(const WCHAR* pwszEncoding)
{
	MIMECPINFO cpinfo;
	ComPtr<IMultiLanguage> pMultiLanguage;
	HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_ALL,
		IID_IMultiLanguage, reinterpret_cast<void**>(&pMultiLanguage));
	if (hr != S_OK)
		return false;
	hr = pMultiLanguage->GetCodePageInfo(::GetACP(), &cpinfo);
	if (hr != S_OK)
		return false;
	
	if (!pwszEncoding)
		pwszEncoding = cpinfo.wszWebCharset;
	
	const WCHAR* pwszBodyEncoding = cpinfo.wszBodyCharset;
	if (!pwszBodyEncoding)
		pwszBodyEncoding = pwszEncoding;
	
	wstrSystemEncoding_ = allocWString(pwszEncoding);
	wstrMailEncoding_ = allocWString(pwszBodyEncoding);
	wstrFixedWidthFont_ = allocWString(cpinfo.wszFixedWidthFont);
	wstrProportionalFont_ = allocWString(cpinfo.wszProportionalFont);
	
#ifndef _WIN32_WCE
	HFONT hfont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
#else
	HFONT hfont = reinterpret_cast<HFONT>(::GetStockObject(SYSTEM_FONT));
#endif
	if (hfont) {
		LOGFONT lf;
		::GetObject(hfont, sizeof(lf), &lf);
		T2W(lf.lfFaceName, pwszFaceName);
		wstrUIFont_ = allocWString(pwszFaceName);
	}
	else {
		wstrUIFont_ = allocWString(wstrProportionalFont_.get());
	}
	
	return true;
}


/****************************************************************************
 *
 * Init
 *
 */

qs::Init::Init(HINSTANCE hInst,
			   const WCHAR* pwszTitle,
			   unsigned int nFlags,
			   unsigned int nThreadFlags) :
	pImpl_(0)
{
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
	if (FAILED(hr))
		return;
	
	INITCOMMONCONTROLSEX icc = {
		sizeof(icc),
		ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES |
		ICC_PROGRESS_CLASS | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES
	};
	::InitCommonControlsEx(&icc);
	
	pImpl_ = new InitImpl();
	pImpl_->hInst_ = hInst;
	pImpl_->bLogEnabled_ = false;
	pImpl_->logLevel_ = Logger::LEVEL_DEBUG;
	
	if (pwszTitle)
		pImpl_->wstrTitle_ = allocWString(pwszTitle);
	
#ifdef QS_KCONVERT
	pImpl_->setSystemEncodingAndFonts(L"shift_jis");
#else
	pImpl_->setSystemEncodingAndFonts(0);
#endif
	
	Initializer* pInitializer = InitImpl::pInitializer__;
	while (pInitializer) {
		if (!pInitializer->init())
			return;
		pInitializer = pInitializer->getNext();
	}
	
	pImpl_->listConverterFactory_.push_back(new UTF8ConverterFactory());
	pImpl_->listConverterFactory_.push_back(new UTF7ConverterFactory());
	pImpl_->listConverterFactory_.push_back(new MLangConverterFactory());
	
	pImpl_->listEncoderFactory_.push_back(new EightBitEncoderFactory());
	pImpl_->listEncoderFactory_.push_back(new Base64EncoderFactory());
	pImpl_->listEncoderFactory_.push_back(new BEncoderFactory());
	pImpl_->listEncoderFactory_.push_back(new QuotedPrintableEncoderFactory());
	pImpl_->listEncoderFactory_.push_back(new QEncoderFactory());
	pImpl_->listEncoderFactory_.push_back(new UuencodeEncoderFactory());
	pImpl_->listEncoderFactory_.push_back(new XUuencodeEncoderFactory());
	
	pImpl_->pInitThread_.reset(new ThreadLocal());
	InitImpl::pInit__ = this;
	pImpl_->pInitThread_->set(new InitThread(nThreadFlags));
}

qs::Init::~Init()
{
	if (pImpl_->pInitThread_.get())
		delete getInitThread();
	
	InitImpl::pInit__ = 0;
	
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
	return pImpl_->wstrTitle_.get();
}

const WCHAR* qs::Init::getSystemEncoding() const
{
	return pImpl_->wstrSystemEncoding_.get();
}

const WCHAR* qs::Init::getMailEncoding() const
{
	return pImpl_->wstrMailEncoding_.get();
}

const WCHAR* qs::Init::getDefaultFixedWidthFont() const
{
	return pImpl_->wstrFixedWidthFont_.get();
}

const WCHAR* qs::Init::getDefaultProportionalFont() const
{
	return pImpl_->wstrProportionalFont_.get();
}

const WCHAR* qs::Init::getDefaultUIFont() const
{
	return pImpl_->wstrUIFont_.get();
}

InitThread* qs::Init::getInitThread()
{
	return static_cast<InitThread*>(pImpl_->pInitThread_->get());
}

bool qs::Init::isLogEnabled() const
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	return pImpl_->bLogEnabled_;
}

void qs::Init::setLogEnabled(bool bEnabled)
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	pImpl_->bLogEnabled_ = bEnabled;
}

wstring_ptr qs::Init::getLogDirectory() const
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	return allocWString(pImpl_->wstrLogDir_.get());
}

void qs::Init::setLogDirectory(const WCHAR* pwszDir)
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	pImpl_->wstrLogDir_ = allocWString(pwszDir);
}

Logger::Level qs::Init::getLogLevel() const
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	return pImpl_->logLevel_;
}

void qs::Init::setLogLevel(Logger::Level level)
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	pImpl_->logLevel_ = level;
}

wstring_ptr qs::Init::getLogFilter() const
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	return pImpl_->wstrLogFilter_.get() ? allocWString(pImpl_->wstrLogFilter_.get()) : 0;
}

void qs::Init::setLogFilter(const WCHAR* pwszFilter)
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	if (pwszFilter)
		pImpl_->wstrLogFilter_ = allocWString(pwszFilter);
	else
		pImpl_->wstrLogFilter_.reset(0);
}

wstring_ptr qs::Init::getLogTimeFormat() const
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	return allocWString(pImpl_->wstrLogTimeFormat_.get());
}

void qs::Init::setLogTimeFormat(const WCHAR* pwszTimeFormat)
{
	Lock<CriticalSection> lock(pImpl_->csLog_);
	pImpl_->wstrLogTimeFormat_ = allocWString(pwszTimeFormat);
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
	bool createLogger();
	
	std::auto_ptr<Synchronizer> pSynchronizer_;
	ModalHandler* pModalHandler_;
	std::auto_ptr<Logger> pLogger_;
};


bool qs::InitThreadImpl::createLogger()
{
	assert(!pLogger_.get());
	
	const Init& init = Init::getInit();
	if (init.isLogEnabled()) {
		wstring_ptr wstrLogDir(init.getLogDirectory());
		if (!wstrLogDir.get())
			return false;
		
		Time time(Time::getCurrentTime());
		WCHAR wszName[128];
		_snwprintf(wszName, countof(wszName), L"\\log-%04d%02d%02d%02d%02d%02d%03d-%u.log",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
			time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
		
		wstring_ptr wstrPath(concat(wstrLogDir.get(), wszName));
		std::auto_ptr<FileLogHandler> pLogHandler(new FileLogHandler(
			wstrPath.get(), init.getLogTimeFormat().get()));
		std::auto_ptr<Logger> pLogger(new Logger(pLogHandler.get(),
			true, init.getLogLevel(), init.getLogFilter().get()));
		pLogHandler.release();
		
		pLogger_ = pLogger;
	}
	
	return true;
}


/****************************************************************************
 *
 * InitThread
 *
 */

qs::InitThread::InitThread(unsigned int nFlags) :
	pImpl_(0)
{
	pImpl_ = new InitThreadImpl();
	pImpl_->pModalHandler_ = 0;
	
	Initializer* pInitializer = InitImpl::pInitializer__;
	while (pInitializer) {
		pInitializer->initThread();
		pInitializer = pInitializer->getNext();
	}
	
	if (nFlags & FLAG_SYNCHRONIZER)
		pImpl_->pSynchronizer_.reset(new Synchronizer());
	
	Init::getInit().setInitThread(this);
}

qs::InitThread::~InitThread()
{
	pImpl_->pSynchronizer_.reset(0);
	
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
	assert(pImpl_->pSynchronizer_.get());
	return pImpl_->pSynchronizer_.get();
}

ModalHandler* qs::InitThread::getModalHandler() const
{
	return pImpl_->pModalHandler_;
}

void qs::InitThread::setModalHandler(ModalHandler* pModalHandler)
{
	pImpl_->pModalHandler_ = pModalHandler;
}

Logger* qs::InitThread::getLogger() const
{
	if (!pImpl_->pLogger_.get())
		pImpl_->createLogger();
	return pImpl_->pLogger_.get();
}

void qs::InitThread::resetLogger()
{
	pImpl_->pLogger_.reset(0);
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

bool qs::Initializer::initThread()
{
	return true;
}

void qs::Initializer::termThread()
{
}
