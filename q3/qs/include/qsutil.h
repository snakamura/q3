/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
class Time;
class Color;
class MessageFormat;
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
	virtual void onUpdate(Observable* pObservable,
						  void* pParam) = 0;
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
	virtual void onUpdate(ObservableImpl* pObservable,
						  Param* pParam) = 0;

public:
	virtual void onUpdate(Observable* pObservable,
						  void* pParam);
};


/****************************************************************************
 *
 * Observable
 *
 */

class QSEXPORTCLASS Observable
{
public:
	Observable();
	~Observable();

public:
	void addObserver(Observer* pObserver);
	void removeObserver(Observer* pObserver);
	void removeObservers();
	void notifyObservers(void* pParam);
	void notifyObservers(void* pParam,
						 const Observer* pObserver);

private:
	Observable(const Observable&);
	Observable& operator=(const Observable&);

private:
	struct ObservableImpl* pImpl_;
};


#pragma warning(push)
#pragma warning(disable:4251)

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
	Time(const SYSTEMTIME& st,
		 int nTimeZone);
	Time(int nYear,
		 int nMonth,
		 int nDayOfWeek,
		 int nDay,
		 int nHour,
		 int nMinute,
		 int nSecond,
		 int nMilliseconds,
		 int nTimeZone);
	Time(const Time& time);
	~Time();

public:
	Time& operator=(const Time& time);

public:
	int getTimeZone() const;
	void setTimeZone(int nTimeZone);
	
	Time& addSecond(int nSecond);
	Time& addMinute(int nMinute);
	Time& addHour(int nHour);
	Time& addDay(int nDay);
	Time& convertToZone();
	
	wstring_ptr format(const WCHAR* pwszFormat,
					   Format format) const;

public:
	static Time getCurrentTime();
	static int getDayCount(int nYear,
						   int nMonth);
	static int getDayCount(int nYear);
	static int getDayOfWeek(int nYear,
							int nMonth,
							int nDay);

public:
	static int getSystemTimeZone();
	static Time getTransitionDate(const SYSTEMTIME& time);
	static const WCHAR* getDefaultFormat();
	static void setDefaultFormat(const WCHAR* pwszDefaultFormat);

private:
	int nTimeZone_;

private:
	static const WCHAR* pwszWeeks__[];
	static const WCHAR* pwszMonths__[];
	static wstring_ptr wstrDefaultFormat__;
};

#pragma warning(pop)

QSEXPORTPROC bool operator==(const Time& lhs,
							 const Time& rhs);
QSEXPORTPROC bool operator!=(const Time& lhs,
							 const Time& rhs);
QSEXPORTPROC bool operator<(const Time& lhs,
							const Time& rhs);
QSEXPORTPROC bool operator<=(const Time& lhs,
							 const Time& rhs);
QSEXPORTPROC bool operator>(const Time& lhs,
							const Time& rhs);
QSEXPORTPROC bool operator>=(const Time& lhs,
							 const Time& rhs);
QSEXPORTPROC int compare(const Time& lhs,
						 const Time& rhs,
						 bool bIgnoreTimeZone);


/****************************************************************************
 *
 * Color
 *
 */

class QSEXPORTCLASS Color
{
public:
	explicit Color(COLORREF cr);
	explicit Color(const WCHAR* pwszColor);

public:
	COLORREF getColor() const;
	void setColor(COLORREF cr);
	wstring_ptr getString() const;
	bool setString(const WCHAR* pwszColor);

private:
	COLORREF cr_;
};


/****************************************************************************
 *
 * MessageFormat
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS MessageFormat
{
public:
	typedef std::vector<const WCHAR*> ArgumentList;

public:
	MessageFormat(const WCHAR* pwszTemplate);
	~MessageFormat();

public:
	wstring_ptr format(const ArgumentList& listArgument) const;

public:
	static wstring_ptr format(const WCHAR* pwszTemplate,
							  const WCHAR* pwszArg0);
	static wstring_ptr format(const WCHAR* pwszTemplate,
							  const WCHAR* pwszArg0,
							  const WCHAR* pwszArg1);
	static wstring_ptr format(const WCHAR* pwszTemplate,
							  const ArgumentList& listArgument);

private:
	MessageFormat(const MessageFormat&);
	MessageFormat& operator=(const MessageFormat&);

private:
	wstring_ptr wstrTemplate_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * CommandLine
 *
 */

class QSEXPORTCLASS CommandLine
{
public:
	CommandLine(CommandLineHandler* pHandler);
	~CommandLine();

public:
	bool parse(const WCHAR* pwszCommandLine);
	bool parse(Reader* pReader);

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
	virtual bool process(const WCHAR* pwszOption) = 0;
};

}

#include <qsutil.inl>

#endif // __QSUTIL_H__
