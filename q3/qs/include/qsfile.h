/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSFILE_H__
#define __QSFILE_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

class File;
	class BinaryFile;


/****************************************************************************
 *
 * File
 *
 */

class QSEXPORTCLASS File
{
public:
	enum SeekOrigin {
		SEEKORIGIN_BEGIN,
		SEEKORIGIN_END,
		SEEKORIGIN_CURRENT
	};

public:
	virtual ~File();

public:
	virtual QSTATUS close() = 0;
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead) = 0;
	virtual QSTATUS write(const unsigned char* p, size_t nWrite) = 0;
	virtual QSTATUS flush() = 0;
	virtual QSTATUS getPosition(int* pnPosition) = 0;
	virtual QSTATUS setPosition(int nPosition, SeekOrigin seekOrigin) = 0;
	virtual QSTATUS setEndOfFile() = 0;
	virtual QSTATUS getSize(size_t* pnSize) = 0;

public:
	static QSTATUS getTempFileName(const WCHAR* pwszDir, WSTRING* pwstrPath);
	static QSTATUS removeDirectory(const WCHAR* pwszDir);
};


/****************************************************************************
 *
 * BinaryFile
 *
 */

class QSEXPORTCLASS BinaryFile : public File
{
public:
	enum Mode {
		MODE_READ		= 0x0001,
		MODE_WRITE		= 0x0002,
		MODE_CREATE		= 0x0100
	};

public:
	BinaryFile(const WCHAR* pwszPath, unsigned int nMode,
		size_t nBufferSize, QSTATUS* pstatus);
	virtual ~BinaryFile();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);
	virtual QSTATUS write(const unsigned char* p, size_t nWrite);
	virtual QSTATUS flush();
	virtual QSTATUS getPosition(int* pnPosition);
	virtual QSTATUS setPosition(int nPosition, SeekOrigin seekOrigin);
	virtual QSTATUS setEndOfFile();
	virtual QSTATUS getSize(size_t* pnSize);

private:
	BinaryFile(const BinaryFile&);
	BinaryFile& operator=(const BinaryFile&);

private:
	struct BinaryFileImpl* pImpl_;
};


/****************************************************************************
 *
 * DividedFile
 *
 */

class QSEXPORTCLASS DividedFile : public File
{
public:
	DividedFile(const WCHAR* pwszPath, size_t nBlockSize,
		unsigned int nMode, size_t nBufferSize, QSTATUS* pstatus);
	virtual ~DividedFile();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);
	virtual QSTATUS write(const unsigned char* p, size_t nWrite);
	virtual QSTATUS flush();
	virtual QSTATUS getPosition(int* pnPosition);
	virtual QSTATUS setPosition(int nPosition, SeekOrigin seekOrigin);
	virtual QSTATUS setEndOfFile();
	virtual QSTATUS getSize(size_t* pnSize);

private:
	DividedFile(const DividedFile&);
	DividedFile& operator=(const DividedFile&);

private:
	struct DividedFileImpl* pImpl_;
};


/****************************************************************************
 *
 * TemporaryFileRenamer
 *
 */

class QSEXPORTCLASS TemporaryFileRenamer
{
public:
	TemporaryFileRenamer(const WCHAR* pwszPath, QSTATUS* pstatus);
	~TemporaryFileRenamer();

public:
	const WCHAR* getPath() const;
	QSTATUS rename();

private:
	TemporaryFileRenamer(const TemporaryFileRenamer&);
	TemporaryFileRenamer& operator=(const TemporaryFileRenamer&);

private:
	WSTRING wstrOriginalPath_;
	WSTRING wstrTemporaryPath_;
#ifndef UNICODE
	TSTRING tstrOriginalPath_;
	TSTRING tstrTemporaryPath_;
#endif
	bool bRenamed_;
};

}

#endif // __QSFILE_H__
