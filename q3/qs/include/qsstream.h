/*
 * $Id: qsstream.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	virtual QSTATUS close() = 0;
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead) = 0;
};


/****************************************************************************
 *
 * FileInputStream
 *
 */

class QSEXPORTCLASS FileInputStream : public InputStream
{
public:
	FileInputStream(const WCHAR* pwszPath, QSTATUS* pstatus);
	virtual ~FileInputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);

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
	ByteInputStream(const unsigned char* p, size_t nLen, QSTATUS* pstatus);
	virtual ~ByteInputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);

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
	BufferedInputStream(InputStream* pInputStream,
		bool bDelete, QSTATUS* pstatus);
	virtual ~BufferedInputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);

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
	virtual QSTATUS close() = 0;
	virtual QSTATUS write(const unsigned char* p, size_t nWrite) = 0;
	virtual QSTATUS flush() = 0;
};


/****************************************************************************
 *
 * FileOutputStream
 *
 */

class QSEXPORTCLASS FileOutputStream : public OutputStream
{
public:
	FileOutputStream(const WCHAR* pwszPath, QSTATUS* pstatus);
	virtual ~FileOutputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const unsigned char* p, size_t nWrite);
	virtual QSTATUS flush();

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
	explicit ByteOutputStream(QSTATUS* pstatus);
	virtual ~ByteOutputStream();

public:
	const unsigned char* getBuffer() const;
	unsigned char* releaseBuffer();
	size_t getLength() const;

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const unsigned char* p, size_t nWrite);
	virtual QSTATUS flush();

private:
	ByteOutputStream(const ByteOutputStream&);
	ByteOutputStream& operator=(const ByteOutputStream&);

private:
	struct ByteOutputStreamImpl* pImpl_;
};


/****************************************************************************
 *
 * BufferedOutputStream
 *
 */

class QSEXPORTCLASS BufferedOutputStream : public OutputStream
{
public:
	BufferedOutputStream(OutputStream* pOutputStream,
		bool bDelete, QSTATUS* pstatus);
	virtual ~BufferedOutputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const unsigned char* p, size_t nWrite);
	virtual QSTATUS flush();

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
	virtual QSTATUS close() = 0;
	virtual QSTATUS read(WCHAR* p, size_t nRead, size_t* pnRead) = 0;
};


/****************************************************************************
 *
 * InputStreamReader
 *
 */

class QSEXPORTCLASS InputStreamReader : public Reader
{
public:
	InputStreamReader(InputStream* pInputStream, bool bDelete,
		const WCHAR* pwszEncoding, QSTATUS* pstatus);
	virtual ~InputStreamReader();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(WCHAR* p, size_t nRead, size_t* pnRead);

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
	StringReader(const WCHAR* pwsz, QSTATUS* pstatus);
	StringReader(const WCHAR* pwsz, size_t nLen, QSTATUS* pstatus);
	virtual ~StringReader();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(WCHAR* p, size_t nRead, size_t* pnRead);

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
	BufferedReader(Reader* pReader, bool bDelete, QSTATUS* pstatus);
	virtual ~BufferedReader();

public:
	QSTATUS readLine(WSTRING* pwstr);

public:
	virtual QSTATUS close();
	virtual QSTATUS read(WCHAR* p, size_t nRead, size_t* pnRead);

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
	virtual QSTATUS close() = 0;
	virtual QSTATUS write(const WCHAR* p, size_t nWrite) = 0;
};


/****************************************************************************
 *
 * OutputStreamWriter
 *
 */

class QSEXPORTCLASS OutputStreamWriter : public Writer
{
public:
	OutputStreamWriter(OutputStream* pOutputStream, bool bDelete,
		const WCHAR* pwszEncoding, QSTATUS* pstatus);
	virtual ~OutputStreamWriter();

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const WCHAR* p, size_t nWrite);

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
	explicit StringWriter(QSTATUS* pstatus);
	virtual ~StringWriter();

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const WCHAR* p, size_t nWrite);

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
	BufferedWriter(Writer* pWriter, bool bDelete, QSTATUS* pstatus);
	virtual ~BufferedWriter();

public:
	QSTATUS newLine();

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const WCHAR* p, size_t nWrite);

private:
	BufferedWriter(const BufferedWriter&);
	BufferedWriter& operator=(const BufferedWriter&);

private:
	struct BufferedWriterImpl* pImpl_;
};

}

#endif // __QSSTREAM_H__
