/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsthread.h>

#include <algorithm>

#include <boost/bind.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "profile.h"

using namespace qs;


namespace qs {
struct TextProfileImpl;
}


/****************************************************************************
 *
 * Profile
 *
 */

qs::Profile::~Profile()
{
}

wstring_ptr qs::Profile::getString(const WCHAR* pwszSection,
								   const WCHAR* pwszKey)
{
	return getString(pwszSection, pwszKey, 0);
}

int qs::Profile::getInt(const WCHAR* pwszSection,
						const WCHAR* pwszKey)
{
	return getInt(pwszSection, pwszKey, 0);
}


/****************************************************************************
 *
 * DefaultLess
 *
 */

inline bool qs::DefaultLess::operator()(const Profile::Default& lhs,
										const Profile::Default& rhs) const
{
	return compare(lhs, rhs) < 0;
}

inline int qs::DefaultLess::compare(const Profile::Default& lhs,
									const Profile::Default& rhs)
{
	int nComp = wcscmp(lhs.pwszSection_, rhs.pwszSection_);
	if (nComp == 0)
		nComp = wcscmp(lhs.pwszKey_, rhs.pwszKey_);
	return nComp;
}


/****************************************************************************
 *
 * AbstractProfileImpl
 *
 */

struct qs::AbstractProfileImpl
{
	typedef std::vector<Profile::Default> DefaultMap;
	
	DefaultMap mapDefault_;
};


/****************************************************************************
 *
 * AbstractProfile
 *
 */

qs::AbstractProfile::AbstractProfile(const Default* pDefault,
									 size_t nCount) :
	pImpl_(0)
{
	pImpl_ = new AbstractProfileImpl();
	
	if (pDefault && nCount != 0)
		addDefault(pDefault, nCount);
}

qs::AbstractProfile::~AbstractProfile()
{
	delete pImpl_;
	pImpl_ = 0;
}

void qs::AbstractProfile::addDefault(const Default* pDefault,
									 size_t nCount)
{
	assert(pDefault);
	
#ifndef NDEBUG
	for (size_t n = 0; n < nCount; ++n) {
		const Default* p = pDefault + n;
		assert(p->pwszSection_);
		assert(p->pwszKey_);
		assert(p->pwszValue_);
	}
#endif
	
	AbstractProfileImpl::DefaultMap& m = pImpl_->mapDefault_;
	m.reserve(m.size() + nCount);
	m.insert(m.end(), pDefault, pDefault + nCount);
	std::sort(m.begin(), m.end(), DefaultLess());
}

const WCHAR* qs::AbstractProfile::getDefault(const WCHAR* pwszSection,
											 const WCHAR* pwszKey,
											 const WCHAR* pwszDefault) const
{
	AbstractProfileImpl::DefaultMap& m = pImpl_->mapDefault_;
	Default d = { pwszSection, pwszKey, 0 };
	AbstractProfileImpl::DefaultMap::const_iterator it =
		std::lower_bound(m.begin(), m.end(), d, DefaultLess());
	return it != m.end() && DefaultLess::compare(*it, d) == 0 ? (*it).pwszValue_ : pwszDefault;
}

int qs::AbstractProfile::getDefault(const WCHAR* pwszSection,
									const WCHAR* pwszKey,
									int nDefault) const
{
	AbstractProfileImpl::DefaultMap& m = pImpl_->mapDefault_;
	Default d = { pwszSection, pwszKey, 0 };
	AbstractProfileImpl::DefaultMap::const_iterator it =
		std::lower_bound(m.begin(), m.end(), d, DefaultLess());
	if (it != m.end() && DefaultLess::compare(*it, d) == 0) {
		WCHAR* pEnd = 0;
		int n = wcstol((*it).pwszValue_, &pEnd, 10);
		if (!*pEnd)
			nDefault = n;
	}
	return nDefault;
}


/****************************************************************************
 *
 * RegistryProfileImpl
 *
 */

struct qs::RegistryProfileImpl
{
	wstring_ptr getKeyName(const WCHAR* pwszSection);
	
	wstring_ptr wstrKey_;
};

wstring_ptr qs::RegistryProfileImpl::getKeyName(const WCHAR* pwszSection)
{
	assert(pwszSection);
	return concat(wstrKey_.get(), pwszSection);
}


/****************************************************************************
 *
 * RegistryProfile
 *
 */

qs::RegistryProfile::RegistryProfile(const WCHAR* pwszCompanyName,
									 const WCHAR* pwszAppName,
									 const Default* pDefault,
									 size_t nCount) :
	AbstractProfile(pDefault, nCount)
{
	assert(pwszCompanyName);
	assert(pwszAppName);
	
	const ConcatW c[] = {
		{ L"Software\\",	-1 },
		{ pwszCompanyName,	-1 },
		{ L"\\",			-1 },
		{ pwszAppName,		-1 },
		{ L"\\"				-1 }
	};
	wstring_ptr wstrKey = concat(c, countof(c));
	
	pImpl_ = new RegistryProfileImpl();
	pImpl_->wstrKey_ = wstrKey;
}

qs::RegistryProfile::~RegistryProfile()
{
	delete pImpl_;
	pImpl_ = 0;
}

wstring_ptr qs::RegistryProfile::getString(const WCHAR* pwszSection,
										   const WCHAR* pwszKey,
										   const WCHAR* pwszDefault)
{
	assert(pImpl_);
	assert(pwszSection);
	assert(pwszKey);
	
	pwszDefault = getDefault(pwszSection, pwszKey, pwszDefault);
	if (!pwszDefault)
		pwszDefault = L"";
	
	wstring_ptr wstrValue;
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (reg)
		reg.getValue(pwszKey, &wstrValue);
	
	if (!wstrValue.get())
		wstrValue = allocWString(pwszDefault);
	
	return wstrValue;
}

void qs::RegistryProfile::setString(const WCHAR* pwszSection,
									const WCHAR* pwszKey,
									const WCHAR* pwszValue)
{
	assert(pImpl_);
	assert(pwszSection);
	assert(pwszKey);
	assert(pwszValue);
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (reg)
		reg.setValue(pwszKey, pwszValue);
}

void qs::RegistryProfile::getStringList(const WCHAR* pwszSection,
										const WCHAR* pwszKey,
										StringList* pListValue)
{
	assert(false);
}

void qs::RegistryProfile::setStringList(const WCHAR* pwszSection,
										const WCHAR* pwszKey,
										const StringList& listValue)
{
	assert(false);
}

int qs::RegistryProfile::getInt(const WCHAR* pwszSection,
								const WCHAR* pwszKey,
								int nDefault)
{
	assert(pImpl_);
	assert(pwszSection);
	assert(pwszKey);
	
	int nValue = getDefault(pwszSection, pwszKey, nDefault);
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (reg) {
		DWORD dwValue = 0;
		if (reg.getValue(pwszKey, &dwValue))
			nValue = dwValue;
	}
	
	return nValue;
}

void qs::RegistryProfile::setInt(const WCHAR* pwszSection,
								 const WCHAR* pwszKey,
								 int nValue)
{
	assert(pImpl_);
	assert(pwszSection);
	assert(pwszKey);
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (reg)
		reg.setValue(pwszKey, nValue);
}

size_t qs::RegistryProfile::getBinary(const WCHAR* pwszSection,
									  const WCHAR* pwszKey,
									  unsigned char* pValue,
									  size_t nSize)
{
	assert(pImpl_);
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (reg) {
		DWORD dwSize = static_cast<DWORD>(nSize);
		reg.getValue(pwszKey, pValue, &dwSize);
		nSize = dwSize;
	}
	
	return nSize;
}

void qs::RegistryProfile::setBinary(const WCHAR* pwszSection,
									const WCHAR* pwszKey,
									const unsigned char* pValue,
									int nSize)
{
	assert(pImpl_);
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (reg)
		reg.setValue(pwszKey, pValue, nSize);
}

bool qs::RegistryProfile::load()
{
	return true;
}

bool qs::RegistryProfile::save() const
{
	return true;
}

bool qs::RegistryProfile::deletePermanent()
{
	assert(pImpl_);
	
	wstring_ptr strRegKey(allocWString(pImpl_->wstrKey_.get()));
	strRegKey.get()[wcslen(strRegKey.get()) - 1] = L'\0';
	return Registry::deleteKey(HKEY_CURRENT_USER, strRegKey.get());
}

bool qs::RegistryProfile::rename(const WCHAR* pwszName)
{
	assert(false);
	return false;
}


/****************************************************************************
 *
 * AbstractTextProfileImpl
 *
 */

struct qs::AbstractTextProfileImpl
{
	static wstring_ptr getEntry(const WCHAR* pwszSection,
								const WCHAR* pwszKey);
	
	wstring_ptr wstrPath_;
	AbstractTextProfile::Map map_;
	CriticalSection cs_;
};

wstring_ptr qs::AbstractTextProfileImpl::getEntry(const WCHAR* pwszSection,
												  const WCHAR* pwszKey)
{
	return concat(pwszSection, L"_", pwszKey);
}


/****************************************************************************
 *
 * AbstractTextProfile
 *
 */

qs::AbstractTextProfile::AbstractTextProfile(const WCHAR* pwszPath,
											 const Default* pDefault,
											 size_t nCount) :
	AbstractProfile(pDefault, nCount),
	pImpl_(0)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	
	pImpl_ = new AbstractTextProfileImpl();
	pImpl_->wstrPath_ = wstrPath;
}

qs::AbstractTextProfile::~AbstractTextProfile()
{
	if (pImpl_) {
		using namespace boost::lambda;
		using boost::lambda::_1;
		std::for_each(pImpl_->map_.begin(), pImpl_->map_.end(),
			(bind(&freeWString, bind(&Map::value_type::first, _1)),
			 bind(&freeWString, bind(&Map::value_type::second, _1))));
		delete pImpl_;
		pImpl_ = 0;
	}
}

wstring_ptr qs::AbstractTextProfile::getString(const WCHAR* pwszSection,
											   const WCHAR* pwszKey,
											   const WCHAR* pwszDefault)
{
	assert(pwszSection);
	assert(pwszKey);
	
	pwszDefault = getDefault(pwszSection, pwszKey, pwszDefault);
	if (!pwszDefault)
		pwszDefault = L"";
	
	const WCHAR* pwszValue = get(pwszSection, pwszKey);
	return allocWString(pwszValue ? pwszValue : pwszDefault);
}

void qs::AbstractTextProfile::setString(const WCHAR* pwszSection,
										const WCHAR* pwszKey,
										const WCHAR* pwszValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pwszValue);
	
	wstring_ptr wstrEntry(AbstractTextProfileImpl::getEntry(pwszSection, pwszKey));
	wstring_ptr wstrValue;
	const WCHAR* pwszDefault = getDefault(pwszSection,
		pwszKey, static_cast<const WCHAR*>(0));
	if (!pwszDefault || wcscmp(pwszDefault, pwszValue) != 0)
		wstrValue = allocWString(pwszValue);
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	AbstractTextProfile::Map::iterator it = pImpl_->map_.find(wstrEntry.get());
	if (it == pImpl_->map_.end()) {
		if (wstrValue.get()) {
			pImpl_->map_.insert(std::make_pair(wstrEntry.get(), wstrValue.get()));
			wstrEntry.release();
			wstrValue.release();
		}
	}
	else {
		freeWString((*it).second);
		if (wstrValue.get()) {
			(*it).second = wstrValue.release();
		}
		else {
			freeWString((*it).first);
			pImpl_->map_.erase(it);
		}
	}
}

void qs::AbstractTextProfile::getStringList(const WCHAR* pwszSection,
											const WCHAR* pwszKey,
											StringList* pListValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pListValue);
	
	const WCHAR* pwszValue = get(pwszSection, pwszKey);
	if (!pwszValue)
		return;
	
	wstring_ptr wstrValue(allocWString(pwszValue));
	WCHAR* p = wcstok(wstrValue.get(), L" ");
	while (p) {
		StringBuffer<WSTRING> buf;
		while (*p) {
			if (*p == L'_') {
				if (*(p + 1) == L'_') {
					buf.append(L'_');
					++p;
				}
				else {
					buf.append(L' ');
				}
			}
			else {
				buf.append(*p);
			}
			++p;
		}
		if (buf.getLength() != 0)
			pListValue->push_back(buf.getString().release());
		
		p = wcstok(0, L" ");
	}
}

void qs::AbstractTextProfile::setStringList(const WCHAR* pwszSection,
											const WCHAR* pwszKey,
											const StringList& listValue)
{
	assert(pwszSection);
	assert(pwszKey);
	
	StringBuffer<WSTRING> buf;
	
	for (StringList::const_iterator it = listValue.begin(); it != listValue.end(); ++it) {
		if (buf.getLength() != 0)
			buf.append(L' ');
		
		const WCHAR* p = *it;
		while (*p) {
			if (*p == L' ')
				buf.append(L'_');
			else if (*p == L'_')
				buf.append(L"__");
			else
				buf.append(*p);
			++p;
		}
	}
	
	setString(pwszSection, pwszKey, buf.getCharArray());
}

int qs::AbstractTextProfile::getInt(const WCHAR* pwszSection,
									const WCHAR* pwszKey,
									int nDefault)
{
	assert(pwszSection);
	assert(pwszKey);
	
	int nValue = getDefault(pwszSection, pwszKey, nDefault);
	
	const WCHAR* pwszValue = get(pwszSection, pwszKey);
	if (pwszValue) {
		WCHAR* pEnd = 0;
		int n = wcstol(pwszValue, &pEnd, 10);
		if (!*pEnd)
			nValue = n;
	}
	
	return nValue;
}

void qs::AbstractTextProfile::setInt(const WCHAR* pwszSection,
									 const WCHAR* pwszKey,
									 int nValue)
{
	assert(pwszSection);
	assert(pwszKey);
	
	WCHAR wszValue[32];
	_snwprintf(wszValue, countof(wszValue), L"%d", nValue);
	setString(pwszSection, pwszKey, wszValue);
}

size_t qs::AbstractTextProfile::getBinary(const WCHAR* pwszSection,
										  const WCHAR* pwszKey,
										  unsigned char* pValue,
										  size_t nSize)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	wstring_ptr wstrEntry(AbstractTextProfileImpl::getEntry(pwszSection, pwszKey));
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	size_t nResultSize = 0;
	
	AbstractTextProfile::Map::iterator it = pImpl_->map_.find(wstrEntry.get());
	if (it != pImpl_->map_.end()) {
		const WCHAR* pwszValue = (*it).second;
		size_t nLen = wcslen(pwszValue);
		if (nLen % 2 == 0 && nLen <= static_cast<size_t>(nSize*2)) {
			unsigned char* p = pValue;
			WCHAR wsz[3];
			WCHAR* pEnd = 0;
			while (*pwszValue) {
				wcsncpy(wsz, pwszValue, 2);
				*p++ = static_cast<unsigned char>(wcstol(wsz, &pEnd, 16));
				pwszValue += 2;
			}
			nResultSize = p - pValue;
		}
	}
	
	return nResultSize;
}

void qs::AbstractTextProfile::setBinary(const WCHAR* pwszSection,
										const WCHAR* pwszKey,
										const unsigned char* pValue,
										int nSize)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	wstring_ptr wstrValue(allocWString(nSize*2 + 1));
	for (int n = 0; n < nSize; ++n)
		_snwprintf(wstrValue.get() + n*2, 3, L"%02x", *(pValue + n));
	*(wstrValue.get() + nSize*2) = L'\0';
	
	setString(pwszSection, pwszKey, wstrValue.get());
}

bool qs::AbstractTextProfile::load()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return loadImpl(pImpl_->wstrPath_.get());
}

bool qs::AbstractTextProfile::save() const
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return saveImpl(pImpl_->wstrPath_.get());
}

bool qs::AbstractTextProfile::deletePermanent()
{
	W2T(pImpl_->wstrPath_.get(), ptszPath);
	return ::DeleteFile(ptszPath) != 0;
}

bool qs::AbstractTextProfile::rename(const WCHAR* pwszName)
{
	wstring_ptr wstrPath(allocWString(pwszName));
	W2T(pImpl_->wstrPath_.get(), ptszOldPath);
	W2T(wstrPath.get(), ptszNewPath);
	if (!::MoveFile(ptszOldPath, ptszNewPath))
		return false;
	
	pImpl_->wstrPath_ = wstrPath;
	
	return true;
}

AbstractTextProfile::Map& qs::AbstractTextProfile::getMap() const
{
	return pImpl_->map_;
}

const WCHAR* qs::AbstractTextProfile::get(const WCHAR* pwszSection,
										  const WCHAR* pwszKey) const
{
	wstring_ptr wstrEntry(AbstractTextProfileImpl::getEntry(pwszSection, pwszKey));
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	AbstractTextProfile::Map::const_iterator it = pImpl_->map_.find(wstrEntry.get());
	return it != pImpl_->map_.end() ? (*it).second : 0;
}


/****************************************************************************
 *
 * TextProfileImpl
 *
 */

struct qs::TextProfileImpl
{
	static void parseLine(const WCHAR* pwszLine,
						  wstring_ptr* pwstrKey,
						  wstring_ptr* pwstrValue);
};

void qs::TextProfileImpl::parseLine(const WCHAR* pwszLine,
									wstring_ptr* pwstrKey,
									wstring_ptr* pwstrValue)
{
	assert(pwszLine);
	assert(pwstrKey);
	assert(pwstrValue);
	
	pwstrKey->reset(0);
	pwstrValue->reset(0);
	
	const WCHAR* p = wcschr(pwszLine, L':');
	if (p && p != pwszLine) {
		const WCHAR* pKeyEnd = p - 1;
		while (pKeyEnd != pwszLine && *pKeyEnd == L' ')
			--pKeyEnd;
		if (pKeyEnd != pwszLine) {
			++p;
			while (*p == L' ')
				++p;
			wstring_ptr wstrKey(allocWString(pwszLine, pKeyEnd - pwszLine + 1));
			wstring_ptr wstrValue(allocWString(p));
			*pwstrKey = wstrKey;
			*pwstrValue = wstrValue;
		}
	}
}


/****************************************************************************
 *
 * TextProfile
 *
 */

qs::TextProfile::TextProfile(const WCHAR* pwszPath,
							 const Default* pDefault,
							 size_t nCount) :
	AbstractTextProfile(pwszPath, pDefault, nCount)
{
}

qs::TextProfile::~TextProfile()
{
}

bool qs::TextProfile::loadImpl(const WCHAR* pwszPath)
{
	if (!File::isFileExisting(pwszPath))
		return true;
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	InputStreamReader reader(&stream, false, L"utf-8");
	if (!reader)
		return false;
	BufferedReader bufferedReader(&reader, false);
	
	while (true) {
		wxstring_ptr wstrLine;
		if (!bufferedReader.readLine(&wstrLine))
			return false;
		if (!wstrLine.get())
			break;
		
		if (*wstrLine.get() != L'#') {
			wstring_ptr wstrKey;
			wstring_ptr wstrValue;
			TextProfileImpl::parseLine(wstrLine.get(), &wstrKey, &wstrValue);
			
			if (wstrKey.get()) {
				if (getMap().insert(std::make_pair(wstrKey.get(), wstrValue.get())).second) {
					wstrKey.release();
					wstrValue.release();
				}
			}
		}
	}
	
	return true;
}

bool qs::TextProfile::saveImpl(const WCHAR* pwszPath) const
{
	class FileRemover
	{
	public:
		FileRemover(const TCHAR* ptszPath) :
			ptszPath_(ptszPath)
		{
		}
		
		~FileRemover()
		{
			if (ptszPath_)
				::DeleteFile(ptszPath_);
		}
	
	public:
		void release() { ptszPath_ = 0; }
	
	private:
		const TCHAR* ptszPath_;
	};
	
	W2T(pwszPath, ptszOrigPath);
	wstring_ptr wstrPath(concat(pwszPath, L".tmp"));
	W2T(wstrPath.get(), ptszPath);
	
	AbstractTextProfile::Map& map = getMap();
	typedef std::vector<std::pair<WSTRING, std::pair<WSTRING, bool> > > EntryList;
	EntryList listEntry;
	listEntry.reserve(map.size());
	AbstractTextProfile::Map::const_iterator it = map.begin();
	while (it != map.end()) {
		listEntry.push_back(std::make_pair(
			(*it).first, std::make_pair((*it).second, false)));
		++it;
	}
	
	FileOutputStream outputStream(wstrPath.get());
	if (!outputStream)
		return false;
	FileRemover fileRemover(ptszPath);
	BufferedOutputStream bufferedStream(&outputStream, false);
	OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	BufferedWriter bufferedWriter(&writer, false);
	
	bool bOrigFileExist = File::isFileExisting(pwszPath);
	bool bError = true;
	if (bOrigFileExist) {
		FileInputStream inputStream(pwszPath);
		if (!inputStream)
			return false;
		InputStreamReader reader(&inputStream, false, L"utf-8");
		if (!reader)
			return false;
		BufferedReader bufferedReader(&reader, false);
		
		while (true) {
			wxstring_ptr wstrLine;
			if (!bufferedReader.readLine(&wstrLine))
				return false;
			if (!wstrLine.get()) {
				bError = false;
				break;
			}
			
			bool bWritten = false;
			if (*wstrLine.get() != L'#') {
				wstring_ptr wstrKey;
				wstring_ptr wstrValue;
				TextProfileImpl::parseLine(wstrLine.get(), &wstrKey, &wstrValue);
				
				if (wstrKey.get()) {
					bWritten = true;
					if (bufferedWriter.write(wstrKey.get(), wcslen(wstrKey.get())) == -1 ||
						bufferedWriter.write(L": ", 2) == -1)
						break;
					const WCHAR* pwszValue = 0;
					EntryList::iterator it = std::lower_bound(
						listEntry.begin(), listEntry.end(),
						std::make_pair(wstrKey.get(), std::make_pair(static_cast<WCHAR*>(0), false)),
						boost::bind(string_less<WCHAR>(),
							boost::bind(&EntryList::value_type::first, _1),
							boost::bind(&EntryList::value_type::first, _2)));
					if (it != listEntry.end() && wcscmp((*it).first, wstrKey.get()) == 0) {
						pwszValue = (*it).second.first;
						(*it).second.second = true;
					}
					else {
						pwszValue = wstrValue.get();
					}
					if (bufferedWriter.write(pwszValue, wcslen(pwszValue)) == -1 ||
						!bufferedWriter.newLine())
						break;
				}
			}
			if (!bWritten) {
				if (bufferedWriter.write(wstrLine.get(), wcslen(wstrLine.get())) == -1 ||
					!bufferedWriter.newLine())
					break;
			}
		}
		if (!bError)
			bError = !bufferedReader.close();
	}
	
	if (!bError) {
		EntryList::iterator it = listEntry.begin();
		while (it != listEntry.end()) {
			if (!(*it).second.second) {
				if (bufferedWriter.write((*it).first, wcslen((*it).first)) == -1 ||
					bufferedWriter.write(L": ", 2) == -1 ||
					bufferedWriter.write((*it).second.first, wcslen((*it).second.first)) == -1 ||
					!bufferedWriter.newLine()) {
					bError = true;
					break;
				}
			}
			++it;
		}
	}
	if (!bError)
		bError = !bufferedWriter.close();
	
	if (!bError) {
		if ((bOrigFileExist && !::DeleteFile(ptszOrigPath)) || 
			!::MoveFile(ptszPath, ptszOrigPath))
			bError = true;
		fileRemover.release();
	}
	
	return !bError;
}


/****************************************************************************
 *
 * XMLProfile
 *
 */

qs::XMLProfile::XMLProfile(const WCHAR* pwszPath,
						   const Default* pDefault,
						   size_t nCount) :
	AbstractTextProfile(pwszPath, pDefault, nCount)
{
}

qs::XMLProfile::~XMLProfile()
{
}

bool qs::XMLProfile::loadImpl(const WCHAR* pwszPath)
{
	if (File::isFileExisting(pwszPath)) {
		XMLReader reader;
		XMLProfileContentHandler handler(&getMap());
		reader.setContentHandler(&handler);
		if (!reader.parse(pwszPath))
			return false;
	}
	return true;
}

bool qs::XMLProfile::saveImpl(const WCHAR* pwszPath) const
{
	TemporaryFileRenamer renamer(pwszPath);
	
	FileOutputStream outputStream(renamer.getPath());
	if (!outputStream)
		return false;
	BufferedOutputStream bufferedStream(&outputStream, false);
	OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	
	OutputHandler handler(&writer, L"utf-8");
	
	if (!handler.startDocument())
		return false;
	
	if (!handler.startElement(0, 0, L"profile", DefaultAttributes()))
		return false;
	
	wstring_ptr wstrSection;
	size_t nSectionLen = 0;
	AbstractTextProfile::Map& map = getMap();
	AbstractTextProfile::Map::iterator it = map.begin();
	while (it != map.end()) {
		const WCHAR* pwszEntry = (*it).first;
		if (!wstrSection.get() ||
			wcsncmp(pwszEntry, wstrSection.get(), nSectionLen) != 0 ||
			*(pwszEntry + nSectionLen) != L'_') {
			if (wstrSection.get()) {
				if (!handler.endElement(0, 0, L"section"))
					return false;
			}
			
			const WCHAR* p = wcschr(pwszEntry, L'_');
			assert(p);
			wstrSection = allocWString(pwszEntry, p - pwszEntry);
			nSectionLen = wcslen(wstrSection.get());
			
			if (!handler.startElement(0, 0, L"section",
				SimpleAttributes(L"name", wstrSection.get())))
				return false;
		}
		
		const WCHAR* pwszKey = wcschr(pwszEntry, L'_') + 1;
		const WCHAR* pwszValue = (*it).second;
		const WCHAR* pwszDefault = getDefault(wstrSection.get(),
			pwszKey, static_cast<const WCHAR*>(0));
		if (!pwszDefault || wcscmp(pwszValue, pwszDefault) != 0) {
			if (!handler.startElement(0, 0, L"key", SimpleAttributes(L"name", pwszKey)) ||
				!handler.characters(pwszValue, 0, wcslen(pwszValue)) ||
				!handler.endElement(0, 0, L"key"))
				return false;
		}
		
		++it;
	}
	if (wstrSection.get()) {
		if (!handler.endElement(0, 0, L"section"))
			return false;
	}
	if (!handler.endElement(0, 0, L"profile"))
		return false;
	
	if (!handler.endDocument())
		return false;
	
	if (!writer.close())
		return false;
	
	return renamer.rename();
}


/****************************************************************************
 *
 * XMLProfileContentHandler
 *
 */

qs::XMLProfileContentHandler::XMLProfileContentHandler(AbstractTextProfile::Map* pMap) :
	pMap_(pMap),
	state_(STATE_ROOT)
{
}

qs::XMLProfileContentHandler::~XMLProfileContentHandler()
{
}

bool qs::XMLProfileContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												const WCHAR* pwszLocalName,
												const WCHAR* pwszQName,
												const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"profile") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_PROFILE;
	}
	else if (wcscmp(pwszLocalName, L"section") == 0) {
		if (state_ != STATE_PROFILE)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!wstrSection_.get());
		wstrSection_ = allocWString(pwszName);
		
		state_ = STATE_SECTION;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		if (state_ != STATE_SECTION)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!wstrEntry_.get());
		wstrEntry_ = concat(wstrSection_.get(), L"_", pwszName);
		
		state_ = STATE_KEY;
	}
	else {
		return false;
	}
	
	return true;
}

bool qs::XMLProfileContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											  const WCHAR* pwszLocalName,
											  const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"profile") == 0) {
		assert(state_ == STATE_PROFILE);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"section") == 0) {
		assert(state_ == STATE_SECTION);
		assert(wstrSection_.get());
		wstrSection_.reset(0);
		state_ = STATE_PROFILE;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		assert(state_ == STATE_KEY);
		assert(wstrEntry_.get());
		
		wstring_ptr wstrValue(buffer_.getString());
		pMap_->insert(std::make_pair(wstrEntry_.get(), wstrValue.get()));
		wstrValue.release();
		wstrEntry_.release();
		wstrEntry_.reset(0);
		
		state_ = STATE_SECTION;
	}
	else {
		return false;
	}
	
	return true;
}

bool qs::XMLProfileContentHandler::characters(const WCHAR* pwsz,
											  size_t nStart,
											  size_t nLength)
{
	if (state_ == STATE_KEY) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return false;
		}
	}
	
	return true;
}
