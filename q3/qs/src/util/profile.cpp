/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qserror.h>
#include <qsfile.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsthread.h>

#include <algorithm>

#include "profile.h"

#pragma warning(disable:4786)

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
	WSTRING wstrKey_;
	
	WSTRING getKeyName(const WCHAR* pwszSection);
};

WSTRING qs::RegistryProfileImpl::getKeyName(const WCHAR* pwszSection)
{
	assert(pwszSection);
	return concat(wstrKey_, pwszSection);
}


/****************************************************************************
 *
 * RegistryProfile
 *
 */

qs::RegistryProfile::RegistryProfile(const WCHAR* pwszCompanyName,
	const WCHAR* pwszAppName, QSTATUS* pstatus)
{
	assert(pwszCompanyName);
	assert(pwszAppName);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrKey_ = 0;
	
	const ConcatW c[] = {
		{ L"Software\\",	-1 },
		{ pwszCompanyName,	-1 },
		{ L"\\",			-1 },
		{ pwszAppName,		-1 },
		{ L"\\"				-1 }
	};
	WSTRING wstrKey = concat(c, countof(c));
	if (!wstrKey) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	pImpl_->wstrKey_ = wstrKey;
}

qs::RegistryProfile::~RegistryProfile()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrKey_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::RegistryProfile::getString(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const WCHAR* pwszDefault, WSTRING* pwstrValue)
{
	assert(pImpl_);
	
	assert(pwszSection);
	assert(pwszKey);
	assert(pwstrValue);
	
	if (!pwszDefault)
		pwszDefault = L"";
	*pwstrValue = 0;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrRegKey(pImpl_->getKeyName(pwszSection));
	if (!wstrRegKey.get())
		return QSTATUS_OUTOFMEMORY;
	
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get(), &status);
	CHECK_QSTATUS();
	if (!reg) {
		status = setString(pwszSection, pwszKey, pwszDefault);
		CHECK_QSTATUS();
		*pwstrValue = allocWString(pwszDefault);
		if (!*pwstrValue)
			return QSTATUS_OUTOFMEMORY;
	}
	else {
		LONG nRet = 0;
		status = reg.getValue(pwszKey, pwstrValue, &nRet);
		CHECK_QSTATUS();
		if (nRet != ERROR_SUCCESS) {
			assert(!*pwstrValue);
			*pwstrValue = allocWString(pwszDefault);
		}
		assert(*pwstrValue);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegistryProfile::setString(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const WCHAR* pwszValue)
{
	assert(pImpl_);
	
	assert(pwszSection);
	assert(pwszKey);
	assert(pwszValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrRegKey(pImpl_->getKeyName(pwszSection));
	if (!wstrRegKey.get())
		return QSTATUS_OUTOFMEMORY;
	
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get(), &status);
	CHECK_QSTATUS();
	if (!reg)
		return QSTATUS_FAIL;
	return reg.setValue(pwszKey, pwszValue);
}

QSTATUS qs::RegistryProfile::getInt(const WCHAR* pwszSection,
	const WCHAR* pwszKey, int nDefault, int* pnValue)
{
	assert(pImpl_);
	
	assert(pwszSection);
	assert(pwszKey);
	assert(pnValue);
	
	*pnValue = nDefault;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrRegKey(pImpl_->getKeyName(pwszSection));
	if (!wstrRegKey.get())
		return QSTATUS_OUTOFMEMORY;
	
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get(), &status);
	CHECK_QSTATUS();
	if (!reg) {
		status = setInt(pwszSection, pwszKey, nDefault);
		CHECK_QSTATUS();
	}
	else {
		LONG nRet = 0;
		DWORD dwValue = 0;
		status = reg.getValue(pwszKey, &dwValue, &nRet);
		CHECK_QSTATUS();
		if (nRet == ERROR_SUCCESS)
			*pnValue = dwValue;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegistryProfile::setInt(const WCHAR* pwszSection,
	const WCHAR* pwszKey, int nValue)
{
	assert(pImpl_);
	
	assert(pwszSection);
	assert(pwszKey);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrRegKey(pImpl_->getKeyName(pwszSection));
	if (!wstrRegKey.get())
		return QSTATUS_OUTOFMEMORY;
	
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get(), &status);
	CHECK_QSTATUS();
	if (!reg)
		return QSTATUS_FAIL;
	return reg.setValue(pwszKey, nValue);
}

QSTATUS qs::RegistryProfile::getBinary(const WCHAR* pwszSection,
	const WCHAR* pwszKey, unsigned char* pValue, int* pnSize)
{
	assert(pImpl_);
	
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	assert(pnSize);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrRegKey(pImpl_->getKeyName(pwszSection));
	if (!wstrRegKey.get())
		return QSTATUS_OUTOFMEMORY;
	
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get(), &status);
	CHECK_QSTATUS();
	if (reg) {
		LONG nRet = 0;
		status = reg.getValue(pwszKey, pValue, pnSize, &nRet);
		CHECK_QSTATUS();
		if (nRet != QSTATUS_SUCCESS)
			*pnSize = 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegistryProfile::setBinary(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const unsigned char* pValue, int nSize)
{
	assert(pImpl_);
	
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrRegKey(pImpl_->getKeyName(pwszSection));
	if (!wstrRegKey.get())
		return QSTATUS_OUTOFMEMORY;
	
	Registry reg(HKEY_CURRENT_USER, wstrRegKey.get(), &status);
	CHECK_QSTATUS();
	if (!reg)
		return QSTATUS_FAIL;
	return reg.setValue(pwszKey, pValue, nSize);
}

QSTATUS qs::RegistryProfile::load()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegistryProfile::save() const
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegistryProfile::deletePermanent()
{
	assert(pImpl_);
	
	DECLARE_QSTATUS();
	
	WSTRING strRegKey = allocWString(pImpl_->wstrKey_);
	strRegKey[wcslen(strRegKey) - 1] = L'\0';
	
	LONG nRet = 0;
	status = Registry::deleteKey(HKEY_CURRENT_USER, strRegKey, &nRet);
	CHECK_QSTATUS();
	return nRet == ERROR_SUCCESS ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::RegistryProfile::rename(const WCHAR* pwszName)
{
	assert(false);
	return QSTATUS_FAIL;
}


/****************************************************************************
 *
 * AbstractProfileImpl
 *
 */

struct qs::AbstractProfileImpl
{
	static QSTATUS getEntry(const WCHAR* pwszSection,
		const WCHAR* pwszKey, WSTRING* pwstrEntry);
	
	WSTRING wstrPath_;
	AbstractProfile::Map* pMap_;
	CriticalSection cs_;
};

QSTATUS qs::AbstractProfileImpl::getEntry(const WCHAR* pwszSection,
	const WCHAR* pwszKey, WSTRING* pwstrEntry)
{
	*pwstrEntry = concat(pwszSection, L"_", pwszKey);
	return *pwstrEntry ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}


/****************************************************************************
 *
 * AbstractProfile
 *
 */

qs::AbstractProfile::AbstractProfile(const WCHAR* pwszPath, QSTATUS* pstatus) :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(allocWString(pwszPath));
	if (!wstrPath.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	std::auto_ptr<AbstractProfile::Map> pMap;
	status = newObject(&pMap);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrPath_ = wstrPath.release();
	pImpl_->pMap_ = pMap.release();
}

qs::AbstractProfile::~AbstractProfile()
{
	if (pImpl_) {
		std::for_each(pImpl_->pMap_->begin(), pImpl_->pMap_->end(),
			unary_compose_fx_gx(
				string_free<WSTRING>(),
				string_free<WSTRING>()));
		freeWString(pImpl_->wstrPath_);
		delete pImpl_->pMap_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::AbstractProfile::getString(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const WCHAR* pwszDefault, WSTRING* pwstrValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrEntry;
	status = AbstractProfileImpl::getEntry(pwszSection, pwszKey, &wstrEntry);
	CHECK_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	AbstractProfile::Map::iterator it = pImpl_->pMap_->find(wstrEntry.get());
	if (it == pImpl_->pMap_->end()) {
		if (!pwszDefault)
			pwszDefault = L"";
		string_ptr<WSTRING> wstrValue(allocWString(pwszDefault));
		if (!wstrValue.get())
			return QSTATUS_OUTOFMEMORY;
		std::pair<AbstractProfile::Map::iterator, bool> ret;
		status = STLWrapper<AbstractProfile::Map>(*pImpl_->pMap_).insert(
			std::make_pair(wstrEntry.get(), wstrValue.get()), &ret);
		CHECK_QSTATUS();
		assert(ret.second);
		wstrEntry.release();
		wstrValue.release();
		it = ret.first;
	}
	
	*pwstrValue = allocWString((*it).second);
	if (!*pwstrValue)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AbstractProfile::setString(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const WCHAR* pwszValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pwszValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrEntry;
	status = AbstractProfileImpl::getEntry(pwszSection, pwszKey, &wstrEntry);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrValue(allocWString(pwszValue));
	if (!wstrValue.get())
		return QSTATUS_OUTOFMEMORY;
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	AbstractProfile::Map::iterator it = pImpl_->pMap_->find(wstrEntry.get());
	if (it == pImpl_->pMap_->end()) {
		std::pair<AbstractProfile::Map::iterator, bool> ret;
		status = STLWrapper<AbstractProfile::Map>(*pImpl_->pMap_).insert(
			std::make_pair(wstrEntry.get(), wstrValue.get()), &ret);
		CHECK_QSTATUS();
		assert(ret.second);
		wstrEntry.release();
	}
	else {
		freeWString((*it).second);
		(*it).second = wstrValue.get();
	}
	wstrValue.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AbstractProfile::getInt(const WCHAR* pwszSection,
	const WCHAR* pwszKey, int nDefault, int* pnValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pnValue);
	
	*pnValue = nDefault;
	
	DECLARE_QSTATUS();
	
	WCHAR wszDefault[32];
	swprintf(wszDefault, L"%d", nDefault);
	string_ptr<WSTRING> wstrValue;
	status = getString(pwszSection, pwszKey, wszDefault, &wstrValue);
	CHECK_QSTATUS();
	
	const WCHAR* p = wstrValue.get();
	if (*p == L'-')
		++p;
	if (*p) {
		while (*p && iswdigit(*p))
			++p;
		if (!*p)
			*pnValue = _wtoi(wstrValue.get());
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AbstractProfile::setInt(
	const WCHAR* pwszSection, const WCHAR* pwszKey, int nValue)
{
	assert(pwszSection);
	assert(pwszKey);
	
	WCHAR wszValue[32];
	swprintf(wszValue, L"%d", nValue);
	return setString(pwszSection, pwszKey, wszValue);
}

QSTATUS qs::AbstractProfile::getBinary(const WCHAR* pwszSection,
	const WCHAR* pwszKey, unsigned char* pValue, int* pnSize)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	assert(pnSize);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrEntry;
	status = AbstractProfileImpl::getEntry(pwszSection, pwszKey, &wstrEntry);
	CHECK_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	AbstractProfile::Map::iterator it = pImpl_->pMap_->find(wstrEntry.get());
	if (it != pImpl_->pMap_->end()) {
		const WCHAR* pwszValue = (*it).second;
		size_t nLen = wcslen(pwszValue);
		if (nLen % 2 == 0 && nLen <= static_cast<size_t>(*pnSize*2)) {
			unsigned char* p = pValue;
			WCHAR wsz[3];
			WCHAR* pEnd = 0;
			while (*pwszValue) {
				wcsncpy(wsz, pwszValue, 2);
				*p++ = static_cast<unsigned char>(wcstol(wsz, &pEnd, 16));
				pwszValue += 2;
			}
			*pnSize = p - pValue;
		}
		else {
			*pnSize = 0;
		}
	}
	else {
		*pnSize = 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AbstractProfile::setBinary(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const unsigned char* pValue, int nSize)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pValue);
	
	string_ptr<WSTRING> wstrValue(allocWString(nSize*2 + 1));
	if (!wstrValue.get())
		return QSTATUS_OUTOFMEMORY;
	for (int n = 0; n < nSize; ++n)
		swprintf(wstrValue.get() + n*2, L"%02x", *(pValue + n));
	*(wstrValue.get() + nSize*2) = L'\0';
	
	return setString(pwszSection, pwszKey, wstrValue.get());
}

QSTATUS qs::AbstractProfile::load()
{
	return loadImpl(pImpl_->wstrPath_);
}

QSTATUS qs::AbstractProfile::save() const
{
	return saveImpl(pImpl_->wstrPath_);
}

QSTATUS qs::AbstractProfile::deletePermanent()
{
	W2T(pImpl_->wstrPath_, ptszPath);
	return ::DeleteFile(ptszPath) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::AbstractProfile::rename(const WCHAR* pwszName)
{
	string_ptr<WSTRING> wstrPath(allocWString(pwszName));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(pImpl_->wstrPath_, ptszOldPath);
	W2T(wstrPath.get(), ptszNewPath);
	if (!::MoveFile(ptszOldPath, ptszNewPath))
		return QSTATUS_FAIL;
	
	freeWString(pImpl_->wstrPath_);
	pImpl_->wstrPath_ = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}

AbstractProfile::Map* qs::AbstractProfile::getMap() const
{
	return pImpl_->pMap_;
}

void qs::AbstractProfile::lock() const
{
	pImpl_->cs_.lock();
}

void qs::AbstractProfile::unlock() const
{
	pImpl_->cs_.unlock();
}


/****************************************************************************
 *
 * TextProfileImpl
 *
 */

struct qs::TextProfileImpl
{
	static QSTATUS parseLine(const WCHAR* pwszLine,
		WSTRING* pwstrKey, WSTRING* pwstrValue);
};

QSTATUS qs::TextProfileImpl::parseLine(const WCHAR* pwszLine,
	WSTRING* pwstrKey, WSTRING* pwstrValue)
{
	assert(pwszLine);
	assert(pwstrKey);
	assert(pwstrValue);
	
	*pwstrKey = 0;
	*pwstrValue = 0;
	
	const WCHAR* p = wcschr(pwszLine, L':');
	if (p && p != pwszLine) {
		const WCHAR* pKeyEnd = p - 1;
		while (pKeyEnd != pwszLine && *pKeyEnd == L' ')
			--pKeyEnd;
		if (pKeyEnd != pwszLine) {
			++p;
			while (*p == L' ')
				++p;
			string_ptr<WSTRING> wstrKey(
				allocWString(pwszLine, pKeyEnd - pwszLine + 1));
			if (!wstrKey.get())
				return QSTATUS_OUTOFMEMORY;
			string_ptr<WSTRING> wstrValue(allocWString(p));
			if (!wstrValue.get())
				return QSTATUS_OUTOFMEMORY;
			*pwstrKey = wstrKey.release();
			*pwstrValue = wstrValue.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TextProfile
 *
 */

qs::TextProfile::TextProfile(const WCHAR* pwszPath, QSTATUS* pstatus) :
	AbstractProfile(pwszPath, pstatus)
{
}

qs::TextProfile::~TextProfile()
{
}

QSTATUS qs::TextProfile::loadImpl(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	W2T(pwszPath, ptszPath);
	if (::GetFileAttributes(ptszPath) == 0xffffffff)
		return QSTATUS_SUCCESS;
	
	FileInputStream stream(pwszPath, &status);
	CHECK_QSTATUS();
	InputStreamReader reader(&stream, false, L"utf-8", &status);
	CHECK_QSTATUS();
	BufferedReader bufferedReader(&reader, false, &status);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrLine;
	while (true) {
		status = bufferedReader.readLine(&wstrLine);
		CHECK_QSTATUS();
		if (!wstrLine.get())
			break;
		
		if (*wstrLine.get() != L'#') {
			string_ptr<WSTRING> wstrKey;
			string_ptr<WSTRING> wstrValue;
			status = TextProfileImpl::parseLine(
				wstrLine.get(), &wstrKey, &wstrValue);
			CHECK_QSTATUS();
			
			if (wstrKey.get()) {
				std::pair<AbstractProfile::Map::iterator, bool> ret;
				status = STLWrapper<AbstractProfile::Map>(*getMap()).insert(
					std::make_pair(wstrKey.get(), wstrValue.get()), &ret);
				CHECK_QSTATUS();
				if (ret.second) {
					wstrKey.release();
					wstrValue.release();
				}
			}
		}
		
		wstrLine.reset(0);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextProfile::saveImpl(const WCHAR* pwszPath) const
{
	Lock<AbstractProfile> lock(*this);
	
	class FileRemover
	{
	public:
		FileRemover(const TCHAR* ptszPath) :
			ptszPath_(ptszPath) {}
		~FileRemover() { if (ptszPath_) ::DeleteFile(ptszPath_); }
	
	public:
		void release() { ptszPath_ = 0; }
	
	private:
		const TCHAR* ptszPath_;
	};
	
	DECLARE_QSTATUS();
	
	W2T(pwszPath, ptszOrigPath);
	
	string_ptr<WSTRING> wstrPath(concat(pwszPath, L".tmp"));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrPath.get(), ptszPath);
	
	AbstractProfile::Map* pMap = getMap();
	typedef std::vector<std::pair<WSTRING, std::pair<WSTRING, bool> > > EntryList;
	EntryList listEntry;
	STLWrapper<EntryList>(listEntry).reserve(pMap->size());
	AbstractProfile::Map::const_iterator it = pMap->begin();
	while (it != pMap->end()) {
		listEntry.push_back(std::make_pair(
			(*it).first, std::make_pair((*it).second, false)));
		++it;
	}
	
	FileOutputStream outputStream(wstrPath.get(), &status);
	CHECK_QSTATUS();
	FileRemover fileRemover(ptszPath);
	OutputStreamWriter writer(&outputStream, false, L"utf-8", &status);
	CHECK_QSTATUS();
	BufferedWriter bufferedWriter(&writer, false, &status);
	CHECK_QSTATUS();
	
	bool bOrigFileExist = ::GetFileAttributes(ptszOrigPath) != 0xffffffff;
	if (bOrigFileExist) {
		FileInputStream inputStream(pwszPath, &status);
		CHECK_QSTATUS();
		InputStreamReader reader(&inputStream, false, L"utf-8", &status);
		CHECK_QSTATUS();
		BufferedReader bufferedReader(&reader, false, &status);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrLine;
		while (true) {
			status = bufferedReader.readLine(&wstrLine);
			CHECK_QSTATUS();
			if (!wstrLine.get())
				break;
			
			bool bWritten = false;
			if (*wstrLine.get() != L'#') {
				string_ptr<WSTRING> wstrKey;
				string_ptr<WSTRING> wstrValue;
				status = TextProfileImpl::parseLine(wstrLine.get(), &wstrKey, &wstrValue);
				CHECK_QSTATUS();
				
				if (wstrKey.get()) {
					bWritten = true;
					if ((status = bufferedWriter.write(wstrKey.get(),
						wcslen(wstrKey.get()))) != QSTATUS_SUCCESS ||
						(status = bufferedWriter.write(L": ", 2)) != QSTATUS_SUCCESS)
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
					if ((status = bufferedWriter.write(pwszValue,
							wcslen(pwszValue))) != QSTATUS_SUCCESS ||
						(status = bufferedWriter.newLine()) != QSTATUS_SUCCESS)
						break;
				}
			}
			if (!bWritten) {
				if ((status = bufferedWriter.write(wstrLine.get(),
						wcslen(wstrLine.get()))) != QSTATUS_SUCCESS ||
					(status = bufferedWriter.newLine()) != QSTATUS_SUCCESS)
					break;
			}
			
			wstrLine.reset(0);
		}
		if (status == QSTATUS_SUCCESS)
			status = bufferedReader.close();
	}
	
	if (status == QSTATUS_SUCCESS) {
		EntryList::iterator it = listEntry.begin();
		while (it != listEntry.end()) {
			if (!(*it).second.second) {
				if ((status = bufferedWriter.write((*it).first,
						wcslen((*it).first))) != QSTATUS_SUCCESS ||
					(status = bufferedWriter.write(L": ", 2)) != QSTATUS_SUCCESS ||
					(status = bufferedWriter.write((*it).second.first,
						wcslen((*it).second.first))) != QSTATUS_SUCCESS ||
					(status = bufferedWriter.newLine()) != QSTATUS_SUCCESS)
					break;
			}
			++it;
		}
	}
	if (status == QSTATUS_SUCCESS)
		status = bufferedWriter.close();
	
	if (status == QSTATUS_SUCCESS) {
		if ((bOrigFileExist && !::DeleteFile(ptszOrigPath)) || 
			!::MoveFile(ptszPath, ptszOrigPath))
			status = QSTATUS_FAIL;
		fileRemover.release();
	}
	
	return status;
}


/****************************************************************************
 *
 * XMLProfile
 *
 */

qs::XMLProfile::XMLProfile(const WCHAR* pwszPath, QSTATUS* pstatus) :
	AbstractProfile(pwszPath, pstatus)
{
}

qs::XMLProfile::~XMLProfile()
{
}

QSTATUS qs::XMLProfile::loadImpl(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	W2T(pwszPath, ptszPath);
	if (::GetFileAttributes(ptszPath) == 0xffffffff)
		return QSTATUS_SUCCESS;
	
	XMLReader reader(&status);
	CHECK_QSTATUS();
	XMLProfileContentHandler handler(getMap(), &status);
	CHECK_QSTATUS();
	reader.setContentHandler(&handler);
	status = reader.parse(pwszPath);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLProfile::saveImpl(const WCHAR* pwszPath) const
{
	DECLARE_QSTATUS();
	
	TemporaryFileRenamer renamer(pwszPath, &status);
	CHECK_QSTATUS();
	
	FileOutputStream outputStream(renamer.getPath(), &status);
	CHECK_QSTATUS();
	OutputStreamWriter writer(&outputStream, false, L"utf-8", &status);
	CHECK_QSTATUS();
	BufferedWriter bufferedWriter(&writer, false, &status);
	CHECK_QSTATUS();
	
	OutputHandler handler(&bufferedWriter, &status);
	CHECK_QSTATUS();
	
	status = handler.startDocument();
	CHECK_QSTATUS();
	
	status = handler.startElement(0, 0, L"profile", DefaultAttributes());
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrSection;
	size_t nSectionLen = 0;
	AbstractProfile::Map* pMap = getMap();
	AbstractProfile::Map::iterator it = pMap->begin();
	while (it != pMap->end()) {
		const WCHAR* pwszEntry = (*it).first;
		if (!wstrSection.get() ||
			wcsncmp(pwszEntry, wstrSection.get(), nSectionLen) != 0 ||
			*(pwszEntry + nSectionLen) != L'_') {
			if (wstrSection.get()) {
				status = handler.endElement(0, 0, L"section");
				CHECK_QSTATUS();
			}
			
			const WCHAR* p = wcschr(pwszEntry, L'_');
			assert(p);
			wstrSection.reset(allocWString(pwszEntry, p - pwszEntry));
			if (!wstrSection.get())
				return QSTATUS_OUTOFMEMORY;
			nSectionLen = wcslen(wstrSection.get());
			
			status = handler.startElement(0, 0, L"section",
				XMLProfileAttributes(wstrSection.get()));
			CHECK_QSTATUS();
		}
		
		const WCHAR* p = wcschr(pwszEntry, L'_');
		assert(p);
		status = handler.startElement(0, 0,
			L"key", XMLProfileAttributes(p + 1));
		CHECK_QSTATUS();
		status = handler.characters((*it).second, 0, wcslen((*it).second));
		CHECK_QSTATUS();
		status = handler.endElement(0, 0, L"key");
		CHECK_QSTATUS();
		
		++it;
	}
	if (wstrSection.get()) {
		status = handler.endElement(0, 0, L"section");
		CHECK_QSTATUS();
	}
	status = handler.endElement(0, 0, L"profile");
	CHECK_QSTATUS();
	
	status = handler.endDocument();
	CHECK_QSTATUS();
	
	status = bufferedWriter.close();
	CHECK_QSTATUS();
	
	status = renamer.rename();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * XMLProfileContentHandler
 *
 */

qs::XMLProfileContentHandler::XMLProfileContentHandler(
	AbstractProfile::Map* pMap, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pMap_(pMap),
	state_(STATE_ROOT),
	wstrSection_(0),
	wstrEntry_(0),
	pBuffer_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
}

qs::XMLProfileContentHandler::~XMLProfileContentHandler()
{
	freeWString(wstrSection_);
	freeWString(wstrEntry_);
	delete pBuffer_;
}

QSTATUS qs::XMLProfileContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"profile") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_PROFILE;
	}
	else if (wcscmp(pwszLocalName, L"section") == 0) {
		if (state_ != STATE_PROFILE)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		assert(!wstrSection_);
		wstrSection_ = allocWString(pwszName);
		if (!wstrSection_)
			return QSTATUS_OUTOFMEMORY;
		
		state_ = STATE_SECTION;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		if (state_ != STATE_SECTION)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		assert(!wstrEntry_);
		wstrEntry_ = concat(wstrSection_, L"_", pwszName);
		if (!wstrEntry_)
			return QSTATUS_OUTOFMEMORY;
		
		state_ = STATE_KEY;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLProfileContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"profile") == 0) {
		assert(state_ == STATE_PROFILE);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"section") == 0) {
		assert(state_ == STATE_SECTION);
		assert(wstrSection_);
		freeWString(wstrSection_);
		wstrSection_ = 0;
		state_ = STATE_PROFILE;
	}
	else if (wcscmp(pwszLocalName, L"key") == 0) {
		assert(state_ == STATE_KEY);
		assert(wstrEntry_);
		
		string_ptr<WSTRING> wstrValue(pBuffer_->getString());
		std::pair<AbstractProfile::Map::iterator, bool> ret;
		status = STLWrapper<AbstractProfile::Map>(*pMap_).insert(
			std::make_pair(wstrEntry_, wstrValue.get()), &ret);
		CHECK_QSTATUS();
		wstrEntry_ = 0;
		wstrValue.release();
		
		state_ = STATE_SECTION;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLProfileContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_KEY) {
		status = pBuffer_->append(pwsz + nStart, nLength);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return QSTATUS_FAIL;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * XMLProfileAttributes
 *
 */

qs::XMLProfileAttributes::XMLProfileAttributes(const WCHAR* pwszName) :
	pwszName_(pwszName)
{
}

qs::XMLProfileAttributes::~XMLProfileAttributes()
{
}

int qs::XMLProfileAttributes::getLength() const
{
	return 1;
}

const WCHAR* qs::XMLProfileAttributes::getQName(int nIndex) const
{
	assert(nIndex == 0);
	return L"name";
}

const WCHAR* qs::XMLProfileAttributes::getValue(int nIndex) const
{
	assert(nIndex == 0);
	return pwszName_;
}
