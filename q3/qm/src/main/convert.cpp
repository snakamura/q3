/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfilenames.h>
#include <qmmessageholder.h>
#include <qmmessageindex.h>

#include <qsconv.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qsstream.h>
#include <qsstring.h>
#include <qswindow.h>

#include <tchar.h>

#include "convert.h"

using namespace qm;
using namespace qs;


bool qm::Convert::convert(const WCHAR* pwszMailFolder)
{
	if (!check(pwszMailFolder))
		return true;
	
	const WCHAR* pwszMessageE = L"Some accounts use message boxes in old format.\n"
		L"These accounts are upgraded to the new format automatically.\n"
		L"This process may take several minutes.\n"
		L"MAKE SURE YOU HAVE MADE BACKUP OF ALL DATA BEFORE CONTINUE.\n"
		L"If you have not made backup yet, press NO to exit.\n"
		L"Press YES to upgrade to the new format.";
	const WCHAR* pwszMessageJ = L"いくつかのアカウントが古いフォーマットで保存されています。\n"
		L"これらのデータは自動的に新しいフォーマットに変換されます。\n"
		L"変換には数分間要します。\n"
		L"変換を行う前に必ずバックアップを取ってください。\n"
		L"バックアップをとっていない場合、「いいえ」を押して終了してください。\n"
		L"変換を行って続行するには「はい」を押してください。";
	const WCHAR* pwszMessage = ::GetACP() == 932 ? pwszMessageJ : pwszMessageE;
	
	if (messageBox(pwszMessage, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES)
		return false;
	
	wstring_ptr wstrFilter(concat(pwszMailFolder, L"\\accounts\\*.*"));
	W2T(wstrFilter.get(), ptszFilter);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFilter, &fd));
	if (!hFind.get())
		return true;
	
	do {
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;
		else if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
			_tcscmp(fd.cFileName, _T("..")) == 0)
			continue;
		
		T2W(fd.cFileName, pwszFileName);
		wstring_ptr wstrPath(concat(pwszMailFolder, L"\\accounts\\", pwszFileName));
		if (!convertAccount(wstrPath.get()))
			return false;
	} while (::FindNextFile(hFind.get(), &fd));
	
	return true;
}

bool qm::Convert::check(const WCHAR* pwszMailFolder)
{
	wstring_ptr wstrFilter(concat(pwszMailFolder, L"\\accounts\\*.*"));
	W2T(wstrFilter.get(), ptszFilter);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFilter, &fd));
	if (!hFind.get())
		return false;
	
	do {
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;
		else if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
			_tcscmp(fd.cFileName, _T("..")) == 0)
			continue;
		
		T2W(fd.cFileName, pwszFileName);
		
		ConcatW c[] = {
			{ pwszMailFolder,		-1	},
			{ L"\\accounts\\",		-1	},
			{ pwszFileName,			-1	},
			{ L"\\cache.map",		-1	}
		};
		wstring_ptr wstrPath(concat(c, countof(c)));
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) != 0xffffffff)
			return true;
	} while (::FindNextFile(hFind.get(), &fd));
	
	return false;
}

bool qm::Convert::convertAccount(const WCHAR* pwszDir)
{
	wstring_ptr wstrCachePath(concat(pwszDir, L"\\cache.map"));
	W2T(wstrCachePath.get(), ptszCachePath);
	if (::GetFileAttributes(ptszCachePath) == 0xffffffff)
		return true;
	
	wstring_ptr wstrFilter(concat(pwszDir, L"\\*.msglist"));
	W2T(wstrFilter.get(), ptszFilter);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFilter, &fd));
	if (!hFind.get())
		return true;
	
	wstring_ptr wstrProfile(concat(pwszDir, L"\\account.xml"));
	XMLProfile profile(wstrProfile.get());
	if (!profile.load())
		return false;
	
	int nCacheBlockSize = profile.getInt(L"Global", L"CacheBlockSize", -1);
	if (nCacheBlockSize == 0)
		nCacheBlockSize = -1;
	else if (nCacheBlockSize != -1)
		nCacheBlockSize *= 1024*1024;
	
	ClusterStorage cacheStorage(pwszDir, L"cache", L".box", L".map", nCacheBlockSize);
	if (!cacheStorage)
		return false;
	
	profile.setInt(L"Global", L"IndexBlockSize", profile.getInt(L"Global", L"CacheBlockSize", -1));
	
	ClusterStorage indexStorage(pwszDir, FileNames::INDEX, FileNames::BOX_EXT, FileNames::MAP_EXT, nCacheBlockSize);
	if (!indexStorage)
		return false;
	
	do {
		T2W(fd.cFileName, pwszFileName);
		wstring_ptr wstrPath(concat(pwszDir, L"\\", pwszFileName));
		if (!convertMessages(wstrPath.get(), &cacheStorage, &indexStorage))
			return false;
	} while (::FindNextFile(hFind.get(), &fd));
	
	if (!profile.save())
		return false;
	
	if (!indexStorage.close())
		return false;
	
	cacheStorage.close();
	
	clearAccount(pwszDir);
	
	return true;
}

bool qm::Convert::convertMessages(const WCHAR* pwszPath,
								  ClusterStorage* pCacheStorage,
								  ClusterStorage* pIndexStorage)
{
	const WCHAR* pFileName = wcsrchr(pwszPath, L'\\');
	if (!pFileName)
		return false;
	++pFileName;
	
	const WCHAR* pExt = wcsrchr(pwszPath, L'.');
	if (!pExt)
		return false;
	
	wstring_ptr wstrId(allocWString(pFileName, pExt - pFileName));
	unsigned int nId = 0;
	swscanf(wstrId.get(), L"%03x", &nId);
	
	WCHAR wsz[32];
	swprintf(wsz, L"%03d%s", nId, FileNames::INDEX_EXT);
	wstring_ptr wstrNewPath(concat(pwszPath, pFileName - pwszPath, wsz, -1));
	
	struct OldInit
	{
		unsigned int nId_;
		unsigned int nFlags_;
		unsigned int nDate_;
		unsigned int nTime_;
		unsigned int nSize_;
		unsigned int nKey_;
		unsigned int nOffset_;
		unsigned int nLength_;
		unsigned int nHeaderLength_;
	};
	
	FileInputStream oldFileStream(pwszPath);
	if (!oldFileStream)
		return false;
	BufferedInputStream oldStream(&oldFileStream, false);
	
	FileOutputStream newFileStream(wstrNewPath.get());
	if (!newFileStream)
		return false;
	BufferedOutputStream newStream(&newFileStream, false);
	
	size_t nAllocSize = 1024;
	malloc_ptr<unsigned char> pData(static_cast<unsigned char*>(malloc(nAllocSize)));
	if (!pData.get())
		return false;
	
	OldInit oldInit;
	size_t nRead = 0;
	while (true) {
		size_t nRead = oldStream.read(reinterpret_cast<unsigned char*>(&oldInit), sizeof(oldInit));
		if (nRead == 0)
			break;
		else if (nRead != sizeof(oldInit))
			return false;
		
		unsigned int nLoad = pCacheStorage->load(pData.get(), oldInit.nKey_, nAllocSize);
		if (nLoad == -1 || nLoad < sizeof(size_t))
			return false;
		
		size_t nDataLen = *reinterpret_cast<size_t*>(pData.get());
		if (nDataLen + sizeof(nDataLen) > nAllocSize) {
			nAllocSize = nDataLen + sizeof(nDataLen);
			unsigned char* p = static_cast<unsigned char*>(
				realloc(pData.get(), nAllocSize));
			if (!p)
				return false;
			pData.release();
			pData.reset(p);
			
			if (pCacheStorage->load(pData.get(), oldInit.nKey_, nAllocSize) == -1)
				return false;
		}
		
		StringBuffer<WSTRING> buf;
		
		unsigned char* p = pData.get();
		p += sizeof(size_t);
		
		UTF8Converter converter;
		unsigned char* pOrg = p;
		for (int n = 0; n < NAME_MAX; ++n) {
			size_t nLen = 0;
			memcpy(&nLen, p, sizeof(nLen));
			for (int m = 0; m < sizeof(size_t); ++m)
				*p++ = 0;
			p += nLen;
		}
		size_t nDecode = p - pOrg;
		wxstring_size_ptr strDecoded(converter.decode(
			reinterpret_cast<CHAR*>(pOrg), &nDecode));
		const WCHAR* pw = strDecoded.get();
		for (int n = 0; n < NAME_MAX; ++n) {
			while (*pw != L'\0')
				++pw;
			for (int m = 0; m < sizeof(size_t); ++m)
				++pw;
			
			buf.append(pw);
			buf.append(L'\n');
		}
		
		const unsigned char* pBuf = reinterpret_cast<const unsigned char*>(buf.getCharArray());
		size_t nLen = buf.getLength()*sizeof(WCHAR);
		unsigned int nKey = pIndexStorage->save(&pBuf, &nLen, 1);
		if (nKey == -1)
			return false;
		
		MessageHolder::Init init = {
			oldInit.nId_,
			oldInit.nFlags_,
			oldInit.nDate_,
			oldInit.nTime_,
			oldInit.nSize_,
			nKey,
			nLen,
			oldInit.nOffset_,
			oldInit.nLength_,
			oldInit.nHeaderLength_
		};
		if (newStream.write(reinterpret_cast<const unsigned char*>(&init), sizeof(init)) == -1)
			return false;
	}
	
	return true;
}

void qm::Convert::clearAccount(const WCHAR* pwszDir)
{
	deleteFiles(pwszDir, L"\\cache.map");
	deleteFiles(pwszDir, L"\\cache*.box");
	deleteFiles(pwszDir, L"\\*.msglist");
}

void qm::Convert::deleteFiles(const WCHAR* pwszDir,
							  const WCHAR* pwszFilter)
{
	wstring_ptr wstrFilter(concat(pwszDir, pwszFilter));
	W2T(wstrFilter.get(), ptszFilter);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFilter, &fd));
	if (!hFind.get())
		return;
	
	do {
		T2W(fd.cFileName, pwszFileName);
		wstring_ptr wstrPath(concat(pwszDir, L"\\", pwszFileName));
		W2T(wstrPath.get(), ptszPath);
		::DeleteFile(ptszPath);
	} while (::FindNextFile(hFind.get(), &fd));
}
