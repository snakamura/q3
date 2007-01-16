/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
	typedef __int64 Offset;

public:
	virtual ~File();

public:
	/**
	 * Close file.
	 *
	 * @return true if success, false otherwise.
	 */
	virtual bool close() = 0;
	
	/**
	 * Read.
	 *
	 * @param p [in] Buffer.
	 * @param nRead [in] Size to read.
	 * @return Read size. 0 if it reaches the end of file. -1 if fail.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual size_t read(unsigned char* p,
						size_t nRead) = 0;
	/**
	 * Write.
	 *
	 * @param p [in] Buffer.
	 * @param nWrite [in] Size to write.
	 * @return Written size. -1 if fail.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual size_t write(const unsigned char* p,
						 size_t nWrite) = 0;
	
	/**
	 * Flush.
	 *
	 * @return true if success, false otherwise.
	 */
	virtual bool flush() = 0;
	
	/**
	 * Get current seek position.
	 *
	 * @return Current seek position.
	 */
	virtual Offset getPosition() = 0;
	
	/**
	 * Set current seek position.
	 *
	 * @param nPosition [in] New position relative to seekOrigin.
	 * @param seekOrigin [in] Seek origin.
	 * @return New seek position. -1 if fail.
	 */
	virtual Offset setPosition(Offset nPosition,
							   SeekOrigin seekOrigin) = 0;
	
	/**
	 * Set end of file at the current seek position.
	 *
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool setEndOfFile() = 0;
	
	/**
	 * Get file size.
	 *
	 * @return File size. -1 if failed.
	 */
	virtual Offset getSize() = 0;

public:
	/**
	 * Check if the specified file exists.
	 *
	 * @param pwszPath path to the file.
	 * @return true if it exists and it's a file.
	 */
	static bool isFileExisting(const WCHAR* pwszPath);
	
	/**
	 * Check if the specified directory exists.
	 *
	 * @param pwszPath path to the directory.
	 * @return true if it exists and it's a directory.
	 */
	static bool isDirectoryExisting(const WCHAR* pwszPath);
	
	/**
	 * Get temporary file name.
	 *
	 * @param pwszDir [in] Directory where a temporary file is created.
	 * @return Path of a temporary file.
	 * @exception std::bad_alloc Out of memory.
	 */
	static wstring_ptr getTempFileName(const WCHAR* pwszDir);
	
	/**
	 * Create directory recursively.
	 *
	 * @param pwszDir [in] Directory which is created.
	 * @return true if success, false otherwise.
	 */
	static bool createDirectory(const WCHAR* pwszDir);
	
	/**
	 * Remove directory recursively.
	 *
	 * @param pwszDir [in] Directory which is removed.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	static bool removeDirectory(const WCHAR* pwszDir);
	
	/**
	 * Check if the specified directory is empty or not.
	 *
	 * @param pwszDir [in] Directory.
	 * @return true if the specified directory exists and is empty, false otherwise.
	 */
	static bool isDirectoryEmpty(const WCHAR* pwszDir);
	
	/**
	 * Check if the specified name is device name such as CON, PRN, etc... or not.
	 *
	 * @param pwszName [in] File name
	 * @param true if the specified name is device name, false otherwise.
	 */
	static bool isDeviceName(const WCHAR* pwszName);
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
	/**
	 * Create instance.
	 * Call operator! to check if file is opened or not.
	 *
	 * @param pwszPath [in] Path of the file.
	 * @param nMode [in] Mode.
	 * @param nBufferSize [in] Buffer size.
	 *                         If 0, the default buffer size is used.
	 * @exception std::bad_alloc Out of memory.
	 */
	BinaryFile(const WCHAR* pwszPath,
			   unsigned int nMode,
			   size_t nBufferSize);
	
	virtual ~BinaryFile();

public:
	/**
	 * Check if file is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	virtual bool close();
	virtual size_t read(unsigned char* p,
						size_t nRead);
	virtual size_t write(const unsigned char* p,
						 size_t nWrite);
	virtual bool flush();
	virtual Offset getPosition();
	virtual Offset setPosition(Offset nPosition,
							   SeekOrigin seekOrigin);
	virtual bool setEndOfFile();
	virtual Offset getSize();

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
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path.
	 * @param nBlockSize [in] Block size in bytes.
	 * @param nMode [in] Mode.
	 * @param nBufferSize [in] Buffer size.
	 *                         If 0, the default buffer size is used.
	 * @exception std::bad_alloc Out of memory.
	 */
	DividedFile(const WCHAR* pwszPath,
				unsigned int nBlockSize,
				unsigned int nMode,
				size_t nBufferSize);
	
	virtual ~DividedFile();

public:
	virtual bool close();
	virtual size_t read(unsigned char* p,
						size_t nRead);
	virtual size_t write(const unsigned char* p,
						 size_t nWrite);
	virtual bool flush();
	virtual Offset getPosition();
	virtual Offset setPosition(Offset nPosition,
							   SeekOrigin seekOrigin);
	virtual bool setEndOfFile();
	virtual Offset getSize();

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
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Original path.
	 * @exception std::bad_alloc Out of memory.
	 */
	TemporaryFileRenamer(const WCHAR* pwszPath);
	
	~TemporaryFileRenamer();

public:
	/**
	 * Get the path to the temporary file.
	 *
	 * @return Temporary path.
	 */
	const WCHAR* getPath() const;
	
	/**
	 * Rename temporary file to original file.
	 *
	 * @return true if success, false otherwise.
	 */
	bool rename();

private:
	TemporaryFileRenamer(const TemporaryFileRenamer&);
	TemporaryFileRenamer& operator=(const TemporaryFileRenamer&);

private:
	struct TemporaryFileRenamerImpl* pImpl_;
};

}

#endif // __QSFILE_H__
