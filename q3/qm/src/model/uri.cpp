/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsthread.h>

#include "uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * URIFragment
 *
 */

qm::URIFragment::URIFragment() :
	type_(TYPE_NONE)
{
}

qm::URIFragment::URIFragment(const Section& section,
							 Type type,
							 const WCHAR* pwszName) :
	section_(section),
	type_(type)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
}

qm::URIFragment::URIFragment(MessageHolder* pmh) :
	type_(TYPE_NONE)
{
	wstring_ptr wstrSubject(pmh->getSubject());
	wstrName_ = concat(wstrSubject.get(), L".msg");
}

qm::URIFragment::URIFragment(Message* pMessage,
							 const Part* pPart,
							 Type type) :
	type_(type)
{
	assert(pMessage);
	assert(pPart);
#ifndef NDEBUG
	switch (type) {
	case TYPE_NONE:
		break;
	case TYPE_MIME:
		assert(pPart->getParentPart());
		break;
	case TYPE_BODY:
		break;
	case TYPE_HEADER:
	case TYPE_TEXT:
		assert(pPart == pMessage || pPart->getEnclosedPart());
		break;
	default:
		assert(false);
		break;
	}
#endif
	
	const Part* pPartOrg = pPart;
	while (pPart != pMessage) {
		const Part* pParentPart = pPart->getParentPart();
		if (pParentPart) {
			const Part::PartList& l = pParentPart->getPartList();
			Part::PartList::const_iterator it = std::find(l.begin(), l.end(), pPart);
			assert(it != l.end());
			section_.push_back(it - l.begin() + 1);
			pPart = pParentPart;
		}
		else {
			pPart = PartUtil(*pPart).getEnclosingPart(pMessage);
			assert(pPart);
		}
	}
	std::reverse(section_.begin(), section_.end());
	
	if (type == TYPE_NONE) {
		UnstructuredParser subject;
		if (pPartOrg->getField(L"Subject", &subject) == Part::FIELD_EXIST)
			wstrName_ = concat(subject.getValue(), L".msg");
	}
	else if (type == TYPE_BODY) {
		ContentDispositionParser contentDisposition;
		if (pPartOrg->getField(L"Content-Disposition", &contentDisposition) == Part::FIELD_EXIST) {
			wstring_ptr wstrFileName(contentDisposition.getParameter(L"filename"));
			if (wstrFileName.get())
				wstrName_ = wstrFileName;
		}
	}
}

qm::URIFragment::URIFragment(const URIFragment& fragment) :
	section_(fragment.section_),
	type_(fragment.type_)
{
	if (fragment.wstrName_.get())
		wstrName_ = allocWString(fragment.wstrName_.get());
}

qm::URIFragment::~URIFragment()
{
}

const URIFragment::Section& qm::URIFragment::getSection() const
{
	return section_;
}

URIFragment::Type qm::URIFragment::getType() const
{
	return type_;
}

const WCHAR* qm::URIFragment::getName() const
{
	return wstrName_.get();
}

wstring_ptr qm::URIFragment::toString() const
{
	if (section_.empty() && type_ == TYPE_NONE && !wstrName_.get())
		return 0;
	
	StringBuffer<WSTRING> buf;
	
	for (Section::const_iterator it = section_.begin(); it != section_.end(); ++it) {
		if (buf.getLength() != 0)
			buf.append(L'.');
		WCHAR wsz[32];
		swprintf(wsz, L"%u", *it);
		buf.append(wsz);
	}
	
	if (type_ != TYPE_NONE && buf.getLength() != 0)
		buf.append(L'.');
	switch (type_) {
	case TYPE_NONE:
		break;
	case TYPE_MIME:
		buf.append(L"MIME");
		break;
	case TYPE_BODY:
		buf.append(L"BODY");
		break;
	case TYPE_HEADER:
		buf.append(L"HEADER");
		break;
	case TYPE_TEXT:
		buf.append(L"TEXT");
		break;
	default:
		assert(false);
		break;
	}
	
	if (wstrName_.get()) {
		buf.append(L'!');
		wstring_ptr wstrName(escape(wstrName_.get()));
		buf.append(wstrName.get());
	}
	
	return buf.getString();
}

const Part* qm::URIFragment::getPart(const Message* pMessage) const
{
	const Part* pPart = pMessage;
	
	for (Section::const_iterator it = section_.begin(); it != section_.end(); ++it) {
		assert(*it > 0);
		
		const Part* pEnclosedPart = pPart->getEnclosedPart();
		if (pEnclosedPart)
			pPart = pEnclosedPart;
		
		if (*it > pPart->getPartCount())
			return 0;
		pPart = pPart->getPart(*it - 1);
	}
	
	switch (type_) {
	case TYPE_NONE:
		break;
	case TYPE_MIME:
		if (!pPart->getParentPart())
			return 0;
		break;
	case TYPE_BODY:
		break;
	case TYPE_HEADER:
	case TYPE_TEXT:
		if (pPart != pMessage && !pPart->getEnclosedPart())
			return 0;
		break;
	default:
		assert(false);
		break;
	}
	
	return pPart;
}

wstring_ptr qm::URIFragment::escape(const WCHAR* pwsz)
{
	const WCHAR* pwszEscape = L"%# ";
	
	StringBuffer<WSTRING> buf;
	
	while (*pwsz) {
		if (wcschr(pwszEscape, *pwsz)) {
			WCHAR wsz[16];
			swprintf(wsz, L"%%%02X", *pwsz);
			buf.append(wsz);
		}
		else {
			buf.append(*pwsz);
		}
		++pwsz;
	}
	
	return buf.getString();
}

wstring_ptr qm::URIFragment::unescape(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	
	while (*pwsz) {
		if (*pwsz == L'%') {
			WCHAR wsz[3];
			wcsncpy(wsz, pwsz + 1, 2);
			wsz[2] = L'\0';
			WCHAR* pEnd = 0;
			long n = wcstol(wsz, &pEnd, 16);
			if (n != 0)
				buf.append(static_cast<WCHAR>(n));
			pwsz += 3;
		}
		else {
			buf.append(*pwsz);
			++pwsz;
		}
	}
	
	return buf.getString();
}


/****************************************************************************
 *
 * URI
 *
 */

qm::URI::URI(const WCHAR* pwszAccount,
			 const WCHAR* pwszFolder,
			 unsigned int nValidity,
			 unsigned int nId,
			 const URIFragment::Section& section,
			 URIFragment::Type type,
			 const WCHAR* pwszName) :
	nValidity_(nValidity),
	nId_(nId),
	fragment_(section, type, pwszName)
{
	assert(pwszAccount);
	assert(pwszFolder);
	
	wstrAccount_ = allocWString(pwszAccount);
	wstrFolder_ = allocWString(pwszFolder);
}

qm::URI::URI(MessageHolder* pmh) :
	nValidity_(-1),
	nId_(-1),
	fragment_(pmh)
{
	assert(pmh);
	
	NormalFolder* pFolder = pmh->getFolder();
	wstrAccount_ = allocWString(pFolder->getAccount()->getName());
	wstrFolder_ = pFolder->getFullName();
	nValidity_ = pFolder->getValidity();
	nId_ = pmh->getId();
}

qm::URI::URI(MessageHolder* pmh,
			 Message* pMessage,
			 const Part* pPart,
			 URIFragment::Type type) :
	nValidity_(-1),
	nId_(-1),
	fragment_(pMessage, pPart, type)
{
	assert(pmh);
	
	NormalFolder* pFolder = pmh->getFolder();
	wstrAccount_ = allocWString(pFolder->getAccount()->getName());
	wstrFolder_ = pFolder->getFullName();
	nValidity_ = pFolder->getValidity();
	nId_ = pmh->getId();
}

qm::URI::URI(const URI& uri) :
	nValidity_(uri.nValidity_),
	nId_(uri.nId_),
	fragment_(uri.fragment_)
{
	wstrAccount_ = allocWString(uri.wstrAccount_.get());
	wstrFolder_ = allocWString(uri.wstrFolder_.get());
}

qm::URI::~URI()
{
}

const WCHAR* qm::URI::getAccount() const
{
	return wstrAccount_.get();
}

const WCHAR* qm::URI::getFolder() const
{
	return wstrFolder_.get();
}

unsigned int qm::URI::getValidity() const
{
	return nValidity_;
}

unsigned int qm::URI::getId() const
{
	return nId_;
}

const URIFragment& qm::URI::getFragment() const
{
	return fragment_;
}

wstring_ptr qm::URI::toString() const
{
	WCHAR wszValidity[32];
	swprintf(wszValidity, L"%u", nValidity_);
	
	WCHAR wszId[32];
	swprintf(wszId, L"%u", nId_);
	
	wstring_ptr wstrFragment(fragment_.toString());
	
	StringBuffer<WSTRING> buf;
	buf.append(getScheme());
	buf.append(L"://");
	buf.append(wstrAccount_.get());
	buf.append(L'/');
	buf.append(wstrFolder_.get());
	buf.append(L'/');
	buf.append(wszValidity);
	buf.append(L'/');
	buf.append(wszId);
	if (wstrFragment.get()) {
		buf.append(L'#');
		buf.append(wstrFragment.get());
	}
	return buf.getString();
}

const WCHAR* qm::URI::getScheme()
{
	return L"urn:qmail";
}

std::auto_ptr<URI> qm::URI::parse(const WCHAR* pwszURI)
{
	assert(pwszURI);
	
	wstring_ptr wstrURI(allocWString(pwszURI));
	if (wcsncmp(wstrURI.get(), L"urn:qmail://", 12) != 0)
		return std::auto_ptr<URI>();
	
	WCHAR* pwszFragment = wcsrchr(wstrURI.get(), L'#');
	if (pwszFragment) {
		*pwszFragment = L'\0';
		++pwszFragment;
	}
	
	const WCHAR* pwszAccount = wstrURI.get() + 12;
	WCHAR* pwszFolder = wcschr(pwszAccount, L'/');
	if (!pwszFolder)
		return std::auto_ptr<URI>();
	*pwszFolder = L'\0';
	++pwszFolder;
	
	WCHAR* pwszId = wcsrchr(pwszFolder, L'/');
	if (!pwszId)
		return std::auto_ptr<URI>();
	*pwszId = L'\0';
	++pwszId;
	
	WCHAR* pEndId = 0;
	unsigned int nId = wcstol(pwszId, &pEndId, 10);
	if (*pEndId)
		return std::auto_ptr<URI>();
	
	WCHAR* pwszValidity = wcsrchr(pwszFolder, L'/');
	if (!pwszValidity)
		return std::auto_ptr<URI>();
	*pwszValidity = L'\0';
	++pwszValidity;
	
	WCHAR* pEndValidity = 0;
	unsigned int nValidity = wcstol(pwszValidity, &pEndValidity, 10);
	if (*pEndValidity)
		return std::auto_ptr<URI>();
	
	URIFragment::Section section;
	URIFragment::Type type = URIFragment::TYPE_NONE;
	wstring_ptr wstrName;
	if (pwszFragment) {
		while (true) {
			if (L'0' <= *pwszFragment && *pwszFragment <= L'9') {
				WCHAR* p = wcschr(pwszFragment, L'.');
				if (p)
					*p = L'\0';
				WCHAR* pEnd = 0;
				unsigned int n = wcstol(pwszFragment, &pEnd, 10);
				if (*pEnd || n == 0)
					return std::auto_ptr<URI>();
				section.push_back(n);
				if (!p)
					break;
				pwszFragment = p + 1;
			}
			else {
				WCHAR* pName = wcschr(pwszFragment, L'!');
				if (pName) {
					*pName = L'\0';
					++pName;
				}
				
				if (wcscmp(pwszFragment, L"") == 0)
					type = URIFragment::TYPE_NONE;
				else if (wcscmp(pwszFragment, L"MIME") == 0)
					type = URIFragment::TYPE_MIME;
				else if (wcscmp(pwszFragment, L"BODY") == 0)
					type = URIFragment::TYPE_BODY;
				else if (wcscmp(pwszFragment, L"HEADER") == 0)
					type = URIFragment::TYPE_HEADER;
				else if (wcscmp(pwszFragment, L"TEXT") == 0)
					type = URIFragment::TYPE_TEXT;
				else
					return std::auto_ptr<URI>();
				
				if (pName)
					wstrName = URIFragment::unescape(pName);
				
				break;
			}
		}
	}
	
	return std::auto_ptr<URI>(new URI(pwszAccount, pwszFolder,
		nValidity, nId, section, type, wstrName.get()));
}
