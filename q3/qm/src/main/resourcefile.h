/**
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RESOURCEFILE_H__
#define __RESOURCEFILE_H__

#include <qm.h>

#include <qssax.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class ResourceFileList;
class ResourceFile;
class ResourceFileContentHandler;
class ResourceFileWriter;


/****************************************************************************
 *
 * ResourceFileList
 *
 */

class ResourceFileList
{
public:
	typedef std::vector<ResourceFile*> List;

public:
	explicit ResourceFileList(const WCHAR* pwszPath);
	~ResourceFileList();

public:
	const List& getResourceFiles() const;
	const ResourceFile* getResourceFile(const WCHAR* pwszName) const;
	void setResourceFile(const WCHAR* pwszName,
						 unsigned int nRevision,
						 const FILETIME* pftModified);
	bool save();

public:
	void add(std::auto_ptr<ResourceFile> pResourceFile);
	void setModified();

private:
	bool load();

private:
	ResourceFileList(const ResourceFileList&);
	ResourceFileList& operator=(const ResourceFileList&);

private:
	qs::wstring_ptr wstrPath_;
	List list_;
	bool bModified_;
};


/****************************************************************************
 *
 * ResourceFile
 *
 */

class ResourceFile
{
public:
	ResourceFile(ResourceFileList* pList,
				 const WCHAR* pwszName,
				 unsigned int nRevision,
				 const FILETIME& ftModified);
	~ResourceFile();

public:
	const WCHAR* getName() const;
	unsigned int getRevision() const;
	void setRevision(unsigned int nRevision);
	const FILETIME& getModified() const;
	void setModified(const FILETIME& ft);

private:
	ResourceFile(const ResourceFile&);
	ResourceFile& operator=(const ResourceFile&);

private:
	ResourceFileList* pList_;
	qs::wstring_ptr wstrName_;
	unsigned int nRevision_;
	FILETIME ftModified_;
};


/****************************************************************************
 *
 * ResourceFileContentHandler
 *
 */

class ResourceFileContentHandler : public qs::DefaultHandler
{
public:
	explicit ResourceFileContentHandler(ResourceFileList* pList);
	virtual ~ResourceFileContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	ResourceFileContentHandler(const ResourceFileContentHandler&);
	ResourceFileContentHandler& operator=(const ResourceFileContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_RESOURCES,
		STATE_RESOURCE
	};

private:
	ResourceFileList* pList_;
	State state_;
};


/****************************************************************************
 *
 * ResourceFileWriter
 *
 */

class ResourceFileWriter
{
public:
	ResourceFileWriter(qs::Writer* pWriter,
					   const WCHAR* pwszEncoding);
	~ResourceFileWriter();

public:
	bool write(const ResourceFileList* pList);

private:
	ResourceFileWriter(const ResourceFileWriter&);
	ResourceFileWriter& operator=(const ResourceFileWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __RESOURCEFILE_H__
