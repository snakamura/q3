/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSTREAM_H__
#define __QSSTREAM_H__

#include <qs.h>
#include <qsstring.h>

namespace qs {

class InputStream;
	class FileInputStream;
	class ByteInputStream;
	class BufferedInputStream;
class OutputStream;
	class FileOutputStream;
	class ByteOutputStream;
	class XStringOutputStream;
	class BufferedOutputStream;
class Reader;
	class InputStreamReader;
	class StringReader;
	class BufferedReader;
class Writer;
	class OutputStreamWriter;
	class StringWriter;
	class BufferedWriter;


/****************************************************************************
 *
 * InputStream
 *
 */

class QSEXPORTCLASS InputStream
{
public:
	virtual ~InputStream();

public:
	/**
	 * Close stream.
	 *
	 * @return true if success, false otherwise.
	 */
	virtual bool close() = 0;
	
	/**
	 * Read from stream.
	 *
	 * @param p [in] Buffer.
	 * @param nRead [in] Size to read.
	 * @return Read size.
	 */
	virtual size_t read(unsigned char* p,
						size_t nRead) = 0;
};


/****************************************************************************
 *
 * FileInputStream
 *
 */

class QSEXPORTCLASS FileInputStream : public InputStream
{
public:
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param pwszPath [in] Path.
	 * @exception std::bad_alloc Out of memory.
	 */
	explicit FileInputStream(const WCHAR* pwszPath);
	
	virtual ~FileInputStream();

public:
	/**
	 * Check if stream is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	virtual bool close();
	virtual size_t read(unsigned char* p,
						size_t nRead);

private:
	FileInputStream(const FileInputStream&);
	FileInputStream& operator=(const FileInputStream&);

private:
	struct FileInputStreamImpl* pImpl_;
};


/****************************************************************************
 *
 * ByteInputStream
 *
 */

class QSEXPORTCLASS ByteInputStream : public InputStream
{
public:
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param p [in] Buffer.
	 * @param nLen [in] Buffer size.
	 * @param bCopy [in] Copy buffer or not.
	 * @exception std::bad_alloc Out of memory.
	 */
	ByteInputStream(const unsigned char* p,
					size_t nLen,
					bool bCopy);
	
	/**
	 * Create instance.
	 *
	 * @param p [in] Buffer.
	 * @param nLen [in] Buffer size.
	 * @exception std::bad_alloc Out of memory.
	 */
	ByteInputStream(malloc_ptr<unsigned char> p,
					size_t nLen);
	
	virtual ~ByteInputStream();

public:
	/**
	 * Check if stream is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	virtual bool close();
	virtual size_t read(unsigned char* p,
						size_t nRead);

private:
	ByteInputStream(const ByteInputStream&);
	ByteInputStream& operator=(const ByteInputStream&);

private:
	struct ByteInputStreamImpl* pImpl_;
};


/****************************************************************************
 *
 * BufferedInputStream
 *
 */

class QSEXPORTCLASS BufferedInputStream : public InputStream
{
public:
	/**
	 * Create instance.
	 *
	 * @param pInputStream [in] Wrapped stream.
	 * @param bDelete [in] true if delete wrapped stream on destroy, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	BufferedInputStream(InputStream* pInputStream,
						bool bDelete);
	
	virtual ~BufferedInputStream();

public:
	virtual bool close();
	virtual size_t read(unsigned char* p,
						size_t nRead);

private:
	BufferedInputStream(const BufferedInputStream&);
	BufferedInputStream& operator=(const BufferedInputStream&);

private:
	struct BufferedInputStreamImpl* pImpl_;
};


/****************************************************************************
 *
 * OutputStream
 *
 */

class QSEXPORTCLASS OutputStream
{
public:
	virtual ~OutputStream();

public:
	/**
	 * Close stream.
	 *
	 * @return true if success, false otherwise.
	 */
	virtual bool close() = 0;
	
	/**
	 * Write to stream.
	 *
	 * @param p [in] Buffer.
	 * @param nWrite [in] Buffer size.
	 * @return Size written.
	 */
	virtual size_t write(const unsigned char* p,
						 size_t nWrite) = 0;
	
	/**
	 * Flush stream.
	 *
	 * @return true if success, false otherwise.
	 */
	virtual bool flush() = 0;
};


/****************************************************************************
 *
 * FileOutputStream
 *
 */

class QSEXPORTCLASS FileOutputStream : public OutputStream
{
public:
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param pwszPath [in] Path.
	 * @exception std::bad_alloc Out of memory.
	 */
	explicit FileOutputStream(const WCHAR* pwszPath);
	
	virtual ~FileOutputStream();

public:
	/**
	 * Check if stream is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	virtual bool close();
	virtual size_t write(const unsigned char* p,
						 size_t nWrite);
	virtual bool flush();

private:
	FileOutputStream(const FileOutputStream&);
	FileOutputStream& operator=(const FileOutputStream&);

private:
	struct FileOutputStreamImpl* pImpl_;
};


/****************************************************************************
 *
 * ByteOutputStream
 *
 */

class QSEXPORTCLASS ByteOutputStream : public OutputStream
{
public:
	/**
	 * Create instance.
	 *
	 * @exception std::bad_alloc Out of memory.
	 */
	ByteOutputStream();
	
	virtual ~ByteOutputStream();

public:
	/**
	 * Get buffer.
	 *
	 * @return Buffer.
	 */
	const unsigned char* getBuffer() const;
	
	/**
	 * Release buffer.
	 *
	 * @return Buffer.
	 */
	malloc_ptr<unsigned char> releaseBuffer();
	
	/**
	 * Release buffer.
	 *
	 * @return Buffer
	 */
	malloc_size_ptr<unsigned char> releaseSizeBuffer();
	
	/**
	 * Get buffer size.
	 *
	 * @return Buffer size.
	 */
	size_t getLength() const;
	
	bool reserve(size_t nLength);

public:
	virtual bool close();
	virtual size_t write(const unsigned char* p,
						 size_t nWrite);
	virtual bool flush();

private:
	ByteOutputStream(const ByteOutputStream&);
	ByteOutputStream& operator=(const ByteOutputStream&);

private:
	struct ByteOutputStreamImpl* pImpl_;
};


/****************************************************************************
 *
 * XStringOutputStream
 *
 */

class QSEXPORTCLASS XStringOutputStream : public OutputStream
{
public:
	XStringOutputStream();
	virtual ~XStringOutputStream();

public:
	xstring_ptr getXString();
	bool reserve(size_t nLength);

public:
	virtual bool close();
	virtual size_t write(const unsigned char* p,
						 size_t nWrite);
	virtual bool flush();

private:
	XStringOutputStream(const XStringOutputStream&);
	XStringOutputStream& operator=(const XStringOutputStream&);

private:
	ByteOutputStream stream_;
};


/****************************************************************************
 *
 * BufferedOutputStream
 *
 */

class QSEXPORTCLASS BufferedOutputStream : public OutputStream
{
public:
	/**
	 * Create instance.
	 *
	 * @param pOutputStream [in] Wrapped stream.
	 * @param bDelete [in] true if delete wrapped stream on destroy, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	BufferedOutputStream(OutputStream* pOutputStream,
						 bool bDelete);
	
	virtual ~BufferedOutputStream();

public:
	virtual bool close();
	virtual size_t write(const unsigned char* p,
						 size_t nWrite);
	virtual bool flush();

private:
	BufferedOutputStream(const BufferedOutputStream&);
	BufferedOutputStream& operator=(const BufferedOutputStream&);

private:
	struct BufferedOutputStreamImpl* pImpl_;
};


/****************************************************************************
 *
 * Reader
 *
 */

class QSEXPORTCLASS Reader
{
public:
	virtual ~Reader();

public:
	/**
	 * Close reader.
	 *
	 * @return true if success, false otherwise.
	 */
	virtual bool close() = 0;
	
	/**
	 * Read from reader.
	 *
	 * @param p Buffer.
	 * @param nRead Size to read.
	 * @return Read size.
	 */
	virtual size_t read(WCHAR* p,
						size_t nRead) = 0;
};


/****************************************************************************
 *
 * InputStreamReader
 *
 */

class QSEXPORTCLASS InputStreamReader : public Reader
{
public:
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param pInputStream [in] Wrapped stream.
	 * @param bDelete [in] true if delete wrapped stream on destroy, false otherwise.
	 * @param pwszEncoding [in] Encoding. If null, use system encoding.
	 * @exception std::bad_alloc Out of memory.
	 */
	InputStreamReader(InputStream* pInputStream,
					  bool bDelete,
					  const WCHAR* pwszEncoding);
	
	virtual ~InputStreamReader();

public:
	/**
	 * Check if reader is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	virtual bool close();
	virtual size_t read(WCHAR* p, size_t nRead);

private:
	InputStreamReader(const InputStreamReader&);
	InputStreamReader& operator=(const InputStreamReader&);

private:
	struct InputStreamReaderImpl* pImpl_;
};


/****************************************************************************
 *
 * StringReader
 *
 */

class QSEXPORTCLASS StringReader : public Reader
{
public:
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param pwsz [in] String.
	 * @param bCopy [in] Copy string or not.
	 * @exception std::bad_alloc Out of memory.
	 */
	StringReader(const WCHAR* pwsz,
				 bool bCopy);
	
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param pwsz [in] String.
	 * @param nLen [in] String length.
	 * @param bCopy [in] Copy string or not.
	 * @exception std::bad_alloc Out of memory.
	 */
	StringReader(const WCHAR* pwsz,
				 size_t nLen,
				 bool bCopy);
	
	/**
	 * Create instance.
	 *
	 * @param wstr [in] String.
	 * @exception std::bad_alloc Out of memory.
	 */
	explicit StringReader(wxstring_ptr wstr);
	
	/**
	 * Create instance.
	 *
	 * @param wstr [in] String.
	 * @param nLen [in] String length.
	 * @exception std::bad_alloc Out of memory.
	 */
	StringReader(wxstring_ptr wstr,
				 size_t nLen);
	
	virtual ~StringReader();

public:
	/**
	 * Check if reader is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	virtual bool close();
	virtual size_t read(WCHAR* p,
						size_t nRead);

private:
	StringReader(const StringReader&);
	StringReader& operator=(const StringReader&);

private:
	struct StringReaderImpl* pImpl_;
};


/****************************************************************************
 *
 * BufferedReader
 *
 */

class QSEXPORTCLASS BufferedReader : public Reader
{
public:
	/**
	 * Create instance.
	 *
	 * @param pReader [in] Wrapped reader.
	 * @param bDelete [in] true if delete wrapped reader on destroy, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	BufferedReader(Reader* pReader,
				   bool bDelete);
	
	virtual ~BufferedReader();

public:
	/**
	 * Read line.
	 *
	 * @param pwstr [out] Line.
	 * @return true if success, false otherwise.
	 */
	bool readLine(wxstring_ptr* pwstr);

public:
	virtual bool close();
	virtual size_t read(WCHAR* p, size_t nRead);

private:
	BufferedReader(const BufferedReader&);
	BufferedReader& operator=(const BufferedReader&);

private:
	struct BufferedReaderImpl* pImpl_;
};


/****************************************************************************
 *
 * Writer
 *
 */

class QSEXPORTCLASS Writer
{
public:
	virtual ~Writer();

public:
	/**
	 * Close writer.
	 *
	 * @return true if success, false otherwise.
	 */
	virtual bool close() = 0;
	
	/**
	 * Write to writer.
	 *
	 * @param p [in] Buffer.
	 * @param nWrite [in] Size to write.
	 * @return Written size.
	 */
	virtual size_t write(const WCHAR* p,
						 size_t nWrite) = 0;
};


/****************************************************************************
 *
 * OutputStreamWriter
 *
 */

class QSEXPORTCLASS OutputStreamWriter : public Writer
{
public:
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param pOutputStream [in] Wrapped stream.
	 * @param bDelete [in] true if delete wrapped stream on destroy, false otherwise.
	 * @param pwszEncoding [in] Encoding. If null, use system encoding.
	 * @exception std::bad_alloc Out of memory.
	 */
	OutputStreamWriter(OutputStream* pOutputStream,
					   bool bDelete,
					   const WCHAR* pwszEncoding);
	
	virtual ~OutputStreamWriter();

public:
	/**
	 * Check if writer is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	virtual bool close();
	virtual size_t write(const WCHAR* p,
						 size_t nWrite);

private:
	OutputStreamWriter(const OutputStreamWriter&);
	OutputStreamWriter& operator=(const OutputStreamWriter&);

private:
	struct OutputStreamWriterImpl* pImpl_;
};


/****************************************************************************
 *
 * StringWriter
 *
 */

class QSEXPORTCLASS StringWriter : public Writer
{
public:
	/**
	 * Create instance.
	 *
	 * @exception std::bad_alloc Out of memory.
	 */
	StringWriter();
	
	virtual ~StringWriter();

public:
	virtual bool close();
	virtual size_t write(const WCHAR* p,
						 size_t nWrite);

private:
	StringWriter(const StringWriter&);
	StringWriter& operator=(const StringWriter&);

private:
	struct StringWriterImpl* pImpl_;
};


/****************************************************************************
 *
 * BufferedWriter
 *
 */

class QSEXPORTCLASS BufferedWriter : public Writer
{
public:
	/**
	 * Create instance.
	 *
	 * @param pWriter [in] Wrapped writer.
	 * @param bDelete [in] true if delete wrapped writer on destroy, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	BufferedWriter(Writer* pWriter,
				   bool bDelete);
	
	virtual ~BufferedWriter();

public:
	/**
	 * Write new line.
	 *
	 * @return true if success, false otherwise.
	 */
	bool newLine();

public:
	virtual bool close();
	virtual size_t write(const WCHAR* p,
						 size_t nWrite);

private:
	BufferedWriter(const BufferedWriter&);
	BufferedWriter& operator=(const BufferedWriter&);

private:
	struct BufferedWriterImpl* pImpl_;
};

}

#endif // __QSSTREAM_H__
