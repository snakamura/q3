/*
 * $Id: qsutil.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSUTIL_H__
#define __QSUTIL_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

class Observer;
class Observable;
class CommandLine;
class CommandLineHandler;

class Reader;


/****************************************************************************
 *
 * Observer
 *
 */

class QSEXPORTCLASS Observer
{
public:
	virtual ~Observer();

public:
	virtual QSTATUS onUpdate(Observable* pObservable, void* pParam) = 0;
};


/****************************************************************************
 *
 * TemplateObserver
 *
 */

template<class ObservableImpl, class Param>
class TemplateObserver : public Observer
{
public:
	virtual ~TemplateObserver();

public:
	virtual QSTATUS onUpdate(ObservableImpl* pObservable, Param* pParam) = 0;

public:
	virtual QSTATUS onUpdate(Observable* pObservable, void* pParam);
};


/****************************************************************************
 *
 * Observable
 *
 */

class QSEXPORTCLASS Observable
{
public:
	explicit Observable(QSTATUS* pstatus);
	~Observable();

public:
	QSTATUS addObserver(Observer* pObserver);
	QSTATUS removeObserver(Observer* pObserver);
	QSTATUS removeObservers();
	QSTATUS notifyObservers(void* pParam);
	QSTATUS notifyObservers(void* pParam, const Observer* pObserver);

private:
	Observable(const Observable&);
	Observable& operator=(const Observable&);

private:
	struct ObservableImpl* pImpl_;
};


/****************************************************************************
 *
 * Time
 *
 */

class QSEXPORTCLASS Time : public SYSTEMTIME
{
public:
	enum Format {
		FORMAT_UTC,
		FORMAT_LOCAL,
		FORMAT_ORIGINAL
	};

public:
	Time();
	Time(const SYSTEMTIME& st, int nTimeZone);
	Time(int nYear, int nMonth, int nDayOfWeek, int nDay, int nHour,
		int nMinute, int nSecond, int nMilliseconds, int nTimeZone);
	Time(const Time& time);
	~Time();

public:
	Time& operator=(const Time& time);

public:
	int getTimeZone() const;
	void setTimeZone(int nTimeZone);
	
	Time& addMinute(int nMinute);
	Time& addHour(int nHour);
	Time& addDay(int nDay);
	Time& convertToZone();
	
	QSTATUS format(const WCHAR* pwszFormat, Format format,
		WSTRING* pwstrText) const;

public:
	static Time getCurrentTime();
	static int getDayCount(int nYear, int nMonth);
	static int getDayCount(int nYear);
	static int getDayOfWeek(int nYear, int nMonth, int nDay);

private:
	static int getSystemTimeZone();
	static Time getTransitionDate(const SYSTEMTIME& time);

private:
	int nTimeZone_;

private:
	static const WCHAR* pwszWeeks__[];
	static const WCHAR* pwszMonths__[];
};

QSEXPORTPROC bool operator==(const Time& lhs, const Time& rhs);
QSEXPORTPROC bool operator!=(const Time& lhs, const Time& rhs);
QSEXPORTPROC bool operator<(const Time& lhs, const Time& rhs);
QSEXPORTPROC bool operator<=(const Time& lhs, const Time& rhs);
QSEXPORTPROC bool operator>(const Time& lhs, const Time& rhs);
QSEXPORTPROC bool operator>=(const Time& lhs, const Time& rhs);
QSEXPORTPROC int compare(const Time& lhs, const Time& rhs, bool bIgnoreTimeZone);


/****************************************************************************
 *
 * Color
 *
 */

class QSEXPORTCLASS Color
{
public:
	explicit Color(COLORREF cr);
	Color(const WCHAR* pwszColor, QSTATUS* pstatus);

public:
	COLORREF getColor() const;
	void setColor(COLORREF cr);
	QSTATUS getString(WSTRING* pwstrColor) const;
	QSTATUS setString(const WCHAR* pwszColor);

private:
	COLORREF cr_;
};


/****************************************************************************
 *
 * CommandLine
 *
 */

class QSEXPORTCLASS CommandLine
{
public:
	CommandLine(CommandLineHandler* pHandler, QSTATUS* pstatus);
	~CommandLine();

public:
	QSTATUS parse(const WCHAR* pwszCommandLine);
	QSTATUS parse(Reader& reader);

private:
	CommandLine(const CommandLine&);
	CommandLine& operator=(const CommandLine&);

private:
	struct CommandLineImpl* pImpl_;
};


/****************************************************************************
 *
 * CommandLineHandler
 *
 */

class QSEXPORTCLASS CommandLineHandler
{
public:
	virtual ~CommandLineHandler();

public:
	virtual QSTATUS process(const WCHAR* pwszOption) = 0;
};

}

#include <qsutil.inl>

#endif // __QSUTIL_H__
