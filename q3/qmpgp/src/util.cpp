/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>

#include <qsconv.h>
#include <qsstream.h>
#include <qsutil.h>

#include "util.h"

using namespace qmpgp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Util
 *
 */

wstring_ptr qmpgp::Util::writeTemporaryFile(const CHAR* psz)
{
	Time time(Time::getCurrentTime());
	WCHAR wszName[128];
	swprintf(wszName, L"pgp-%04d%02d%02d%02d%02d%02d%03d",
		time.wYear, time.wMonth, time.wDay, time.wHour,
		time.wMinute, time.wSecond, time.wMilliseconds);
	
	wstring_ptr wstrPath(concat(Application::getApplication().getTemporaryFolder(), wszName));
	
	FileOutputStream stream(wstrPath.get());
	if (!stream)
		return 0;
	BufferedOutputStream bufferedStream(&stream, false);
	if (bufferedStream.write(reinterpret_cast<const unsigned char*>(psz), strlen(psz)) == -1)
		return 0;
	if (!bufferedStream.close())
		return 0;
	
	return wstrPath;
}


/****************************************************************************
 *
 * FileDeleter
 *
 */

qmpgp::FileDeleter::FileDeleter(const WCHAR* pwszPath) :
	pwszPath_(pwszPath)
{
}

qmpgp::FileDeleter::~FileDeleter()
{
	W2T(pwszPath_, ptszPath);
	::DeleteFile(ptszPath);
}
