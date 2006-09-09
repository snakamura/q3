/**
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsfile.h>
#include <qsstream.h>

#include "resourcefile.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ResourceFileList
 *
 */

qm::ResourceFileList::ResourceFileList(const WCHAR* pwszPath) :
	bModified_(false)
{
	wstrPath_ = allocWString(pwszPath);
	
	load();
}

qm::ResourceFileList::~ResourceFileList()
{
	std::for_each(list_.begin(), list_.end(), qs::deleter<ResourceFile>());
}

const ResourceFileList::List& qm::ResourceFileList::getResourceFiles() const
{
	return list_;
}

const ResourceFile* qm::ResourceFileList::getResourceFile(const WCHAR* pwszName) const
{
	List::const_iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&ResourceFile::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != list_.end() ? *it : 0;
}

void qm::ResourceFileList::setResourceFile(const WCHAR* pwszName,
										   unsigned int nRevision,
										   const FILETIME* pftModified)
{
	List::iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&ResourceFile::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != list_.end()) {
		(*it)->setRevision(nRevision);
		if (pftModified)
			(*it)->setModified(*pftModified);
	}
	else {
		FILETIME ft;
		if (!pftModified) {
			SYSTEMTIME st;
			::GetSystemTime(&st);
			::SystemTimeToFileTime(&st, &ft);
			pftModified = &ft;
		}
		
		std::auto_ptr<ResourceFile> p(new ResourceFile(
			this, pwszName, nRevision, *pftModified));
		add(p);
		setModified();
	}
}

bool qm::ResourceFileList::save()
{
	if (!bModified_)
		return true;
	
	TemporaryFileRenamer renamer(wstrPath_.get());
	
	FileOutputStream stream(renamer.getPath());
	if (!stream)
		return false;
	BufferedOutputStream bufferedStream(&stream, false);
	OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	
	ResourceFileWriter resourceFileWriter(&writer, L"utf-8");
	if (!resourceFileWriter.write(this))
		return false;
	
	if (!writer.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	return true;
}

void qm::ResourceFileList::add(std::auto_ptr<ResourceFile> pResourceFile)
{
	list_.push_back(pResourceFile.get());
	pResourceFile.release();
}

void qm::ResourceFileList::setModified()
{
	bModified_ = true;
}

bool qm::ResourceFileList::load()
{
	W2T(wstrPath_.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader;
		ResourceFileContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(wstrPath_.get()))
			return false;
	}
	
	return true;
}


/****************************************************************************
*
* ResourceFile
*
*/

qm::ResourceFile::ResourceFile(ResourceFileList* pList,
							   const WCHAR* pwszName,
							   unsigned int nRevision,
							   const FILETIME& ftModified) :
	pList_(pList),
	nRevision_(nRevision),
	ftModified_(ftModified)
{
	wstrName_ = allocWString(pwszName);
}

qm::ResourceFile::~ResourceFile()
{
}

const WCHAR* qm::ResourceFile::getName() const
{
	return wstrName_.get();
}

unsigned int qm::ResourceFile::getRevision() const
{
	return nRevision_;
}

void qm::ResourceFile::setRevision(unsigned int nRevision)
{
	if (nRevision != nRevision_) {
		nRevision_ = nRevision;
		pList_->setModified();
	}
}

const FILETIME& qm::ResourceFile::getModified() const
{
	return ftModified_;
}

void qm::ResourceFile::setModified(const FILETIME& ft)
{
	if (::CompareFileTime(&ft, &ftModified_) != 0) {
		ftModified_ = ft;
		pList_->setModified();
	}
}


/****************************************************************************
 *
 * ResourceFileContentHandler
 *
 */

qm::ResourceFileContentHandler::ResourceFileContentHandler(ResourceFileList* pList) :
	pList_(pList),
	state_(STATE_ROOT)
{
}

qm::ResourceFileContentHandler::~ResourceFileContentHandler()
{
}

bool qm::ResourceFileContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												  const WCHAR* pwszLocalName,
												  const WCHAR* pwszQName,
												  const qs::Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"resources") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_RESOURCES;
	}
	else if (wcscmp(pwszLocalName, L"resource") == 0) {
		if (state_ != STATE_RESOURCES)
			return false;
		
		const WCHAR* pwszName = 0;
		const WCHAR* pwszRevision = 0;
		const WCHAR* pwszModified = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"revision") == 0)
				pwszRevision = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"modified") == 0)
				pwszModified = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName || !*pwszName || !pwszRevision || !pwszModified)
			return false;
		
		WCHAR* pEnd = 0;
		unsigned int nRevision = wcstol(pwszRevision, &pEnd, 10);
		if (*pEnd)
			return false;
		
		DWORD dwLow = wcstoul(pwszModified, &pEnd, 10);
		if (*pEnd != L':')
			return false;
		DWORD dwHigh = wcstoul(pEnd + 1, &pEnd, 10);
		if (*pEnd)
			return false;
		FILETIME ft = { dwLow, dwHigh };
		
		std::auto_ptr<ResourceFile> p(new ResourceFile(pList_, pwszName, nRevision, ft));
		pList_->add(p);
		
		state_ = STATE_RESOURCE;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::ResourceFileContentHandler::endElement(const WCHAR* pwszNamespaceURI,
												const WCHAR* pwszLocalName,
												const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"resources") == 0) {
		assert(state_ == STATE_RESOURCES);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"resource") == 0) {
		assert(state_ == STATE_RESOURCE);
		state_ = STATE_RESOURCES;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::ResourceFileContentHandler::characters(const WCHAR* pwsz,
												size_t nStart,
												size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return false;
	}
	return true;
}


/****************************************************************************
 *
 * ResourceFileWriter
 *
 */

qm::ResourceFileWriter::ResourceFileWriter(Writer* pWriter,
										   const WCHAR* pwszEncoding) :
	handler_(pWriter, pwszEncoding)
{
}

qm::ResourceFileWriter::~ResourceFileWriter()
{
}

bool qm::ResourceFileWriter::write(const ResourceFileList* pList)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"resources", DefaultAttributes()))
		return false;
	
	const ResourceFileList::List& l = pList->getResourceFiles();
	for (ResourceFileList::List::const_iterator it = l.begin(); it != l.end(); ++it) {
		const ResourceFile* p = *it;
		
		WCHAR wszRevision[32];
		_snwprintf(wszRevision, countof(wszRevision), L"%lu", p->getRevision());
		
		const FILETIME& ft = p->getModified();
		WCHAR wszModified[64];
		_snwprintf(wszModified, countof(wszModified),
			L"%lu:%lu", ft.dwLowDateTime, ft.dwHighDateTime);
		
		const SimpleAttributes::Item items[] = {
			{ L"name",		p->getName()	},
			{ L"revision",	wszRevision		},
			{ L"modified",	wszModified		}
		};
		SimpleAttributes attrs(items, countof(items));
		if (!handler_.startElement(0, 0, L"resource", attrs) ||
			!handler_.endElement(0, 0, L"resource"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"resources"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}
