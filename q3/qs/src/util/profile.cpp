/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
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
									 const WCHAR* pwszAppName)
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
	
	if (!pwszDefault)
		pwszDefault = L"";
	
	wstring_ptr wstrValue;
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (!reg)
		setString(pwszSection, pwszKey, pwszDefault);
	else
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
	
	int nValue = nDefault;
	
	wstring_ptr wstrRegKey(pImpl_->getKeyName(pwszSection));
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get());
	if (reg) {
		setInt(pwszSection, pwszKey, nDefault);
	}
	else {
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
	if (reg)
		reg.getValue(pwszKey, pValue, &nSize);
	
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
 * AbstractProfileImpl
 *
 */

struct qs::AbstractProfileImpl
{
	static wstring_ptr getEntry(const WCHAR* pwszSection,
								const WCHAR* pwszKey);
	
	wstring_ptr wstrPath_;
	AbstractProfile::Map map_;
	CriticalSection cs_;
};

wstring_ptr qs::AbstractProfileImpl::getEntry(const WCHAR* pwszSection,
											  const WCHAR* pwszKey)
{
	return concat(pwszSection, L"_", pwszKey);
}


/****************************************************************************
 *
 * AbstractProfile
 *
 */

qs::AbstractProfile::AbstractProfile(const WCHAR* pwszPath) :
	pImpl_(0)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	
	pImpl_ = new AbstractProfileImpl();
	pImpl_->wstrPath_ = wstrPath;
}

qs::AbstractProfile::~AbstractProfile()
{
	if (pImpl_) {
		std::for_each(pImpl_->map_.begin(), pImpl_->map_.end(),
			unary_compose_fx_gx(
				string_free<WSTRING>(),
				string_free<WSTRING>()));
		delete pImpl_;
		pImpl_ = 0;
	}
}

wstring_ptr qs::AbstractProfile::getString(const WCHAR* pwszSection,
										   const WCHAR* pwszKey,
										   const WCHAR* pwszDefault)
{
	assert(pwszSection);
	assert(pwszKey);
	
	if (!pwszDefault)
		pwszDefault = L"";
	
	wstring_ptr wstrEntry(AbstractProfileImpl::getEntry(pwszSection, pwszKey));
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	AbstractProfile::Map::iterator it = pImpl_->map_.find(wstrEntry.get());
	if (it == pImpl_->map_.end()) {
		wstring_ptr wstrValue(allocWString(pwszDefault));
		it = pImpl_->map_.insert(std::make_pair(wstrEntry.get(), wstrValue.get())).first;
		wstrEntry.release();
		wstrValue.release();
	}
	
	return allocWString((*it).second);
}

void qs::AbstractProfile::setString(const WCHAR* pwszSection,
									const WCHAR* pwszKey,
									const WCHAR* pwszValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pwszValue);
	
	wstring_ptr wstrEntry(AbstractProfileImpl::getEntry(pwszSection, pwszKey));
	wstring_ptr wstrValue(allocWString(pwszValue));
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	AbstractProfile::Map::iterator it = pImpl_->map_.find(wstrEntry.get());
	if (it == pImpl_->map_.end()) {
		pImpl_->map_.insert(std::make_pair(wstrEntry.get(), wstrValue.get()));
		wstrEntry.release();
		wstrValue.release();
	}
	else {
		freeWString((*it).second);
		(*it).second = wstrValue.release();
	}
}

void qs::AbstractProfile::getStringList(const WCHAR* pwszSection,
										const WCHAR* pwszKey,
										StringList* pListValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pListValue);
	
	wstring_ptr wstrValue(getString(pwszSection, pwszKey, L""));
	
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

void qs::AbstractProfile::setStringList(const WCHAR* pwszSection,
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

int qs::AbstractProfile::getInt(const WCHAR* pwszSection,
								const WCHAR* pwszKey,
								int nDefault)
{
	assert(pwszSection);
	assert(pwszKey);
	
	int nValue = nDefault;
	
	WCHAR wszDefault[32];
	swprintf(wszDefault, L"%d", nDefault);
	wstring_ptr wstrValue(getString(pwszSection, pwszKey, wszDefault));
	
	const WCHAR* p = wstrValue.get();
	if (*p == L'-')
		++p;
	if (*p) {
		while (*p && iswdigit(*p))
			++p;
		if (!*p)
			nValue = _wtoi(wstrValue.get());
	}
	
	return nValue;
}

void qs::AbstractProfile::setInt(const WCHAR* pwszSection,
								 const WCHAR* pwszKey,
								 int nValue)
{
	assert(pwszSection);
	assert(pwszKey);
	
	WCHAR wszValue[32];
	swprintf(wszValue, L"%d", nValue);
	setString(pwszSection, pwszKey, wszValue);
}

size_t qs::AbstractProfile::getBinary(const WCHAR* pwszSection,
									  const WCHAR* pwszKey,
									  unsigned char* pValue,
									  size_t nSize)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	wstring_ptr wstrEntry(AbstractProfileImpl::getEntry(pwszSection, pwszKey));
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	size_t nResultSize = 0;
	
	AbstractProfile::Map::iterator it = pImpl_->map_.find(wstrEntry.get());
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

void qs::AbstractProfile::setBinary(const WCHAR* pwszSection,
									const WCHAR* pwszKey,
									const unsigned char* pValue,
									int nSize)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	wstring_ptr wstrValue(allocWString(nSize*2 + 1));
	for (int n = 0; n < nSize; ++n)
		swprintf(wstrValue.get() + n*2, L"%02x", *(pValue + n));
	*(wstrValue.get() + nSize*2) = L'\0';
	
	setString(pwszSection, pwszKey, wstrValue.get());
}

bool qs::AbstractProfile::load()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return loadImpl(pImpl_->wstrPath_.get());
}

bool qs::AbstractProfile::save() const
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return saveImpl(pImpl_->wstrPath_.get());
}

bool qs::AbstractProfile::deletePermanent()
{
	W2T(pImpl_->wstrPath_.get(), ptszPath);
	return ::DeleteFile(ptszPath) != 0;
}

bool qs::AbstractProfile::rename(const WCHAR* pwszName)
{
	wstring_ptr wstrPath(allocWString(pwszName));
	W2T(pImpl_->wstrPath_.get(), ptszOldPath);
	W2T(wstrPath.get(), ptszNewPath);
	if (!::MoveFile(ptszOldPath, ptszNewPath))
		return false;
	
	pImpl_->wstrPath_ = wstrPath;
	
	return true;
}

AbstractProfile::Map& qs::AbstractProfile::getMap() const
{
	return pImpl_->map_;
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

qs::TextProfile::TextProfile(const WCHAR* pwszPath) :
	AbstractProfile(pwszPath)
{
}

qs::TextProfile::~TextProfile()
{
}

bool qs::TextProfile::loadImpl(const WCHAR* pwszPath)
{
	W2T(pwszPath, ptszPath);
	if (::GetFileAttributes(ptszPath) == 0xffffffff)
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
	
	AbstractProfile::Map& map = getMap();
	typedef std::vector<std::pair<WSTRING, std::pair<WSTRING, bool> > > EntryList;
	EntryList listEntry;
	listEntry.reserve(map.size());
	AbstractProfile::Map::const_iterator it = map.begin();
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
	
	bool bOrigFileExist = ::GetFileAttributes(ptszOrigPath) != 0xffffffff;
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
						binary_compose_f_gx_hy(string_less<WCHAR>(),
							std::select1st<EntryList::value_type>(),
							std::select1st<EntryList::value_type>()));
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

qs::XMLProfile::XMLProfile(const WCHAR* pwszPath) :
	AbstractProfile(pwszPath)
{
}

qs::XMLProfile::~XMLProfile()
{
}

bool qs::XMLProfile::loadImpl(const WCHAR* pwszPath)
{
	W2T(pwszPath, ptszPath);
	if (::GetFileAttributes(ptszPath) == 0xffffffff)
		return true;
	
	XMLReader reader;
	XMLProfileContentHandler handler(&getMap());
	reader.setContentHandler(&handler);
	return reader.parse(pwszPath);
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
	
	OutputHandler handler(&writer);
	
	if (!handler.startDocument())
		return false;
	
	if (!handler.startElement(0, 0, L"profile", DefaultAttributes()))
		return false;
	
	wstring_ptr wstrSection;
	size_t nSectionLen = 0;
	AbstractProfile::Map& map = getMap();
	AbstractProfile::Map::iterator it = map.begin();
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
		
		const WCHAR* p = wcschr(pwszEntry, L'_');
		assert(p);
		if (!handler.startElement(0, 0, L"key", SimpleAttributes(L"name", p + 1)))
			return false;
		if (!handler.characters((*it).second, 0, wcslen((*it).second)))
			return false;
		if (!handler.endElement(0, 0, L"key"))
			return false;
		
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

qs::XMLProfileContentHandler::XMLProfileContentHandler(AbstractProfile::Map* pMap) :
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
