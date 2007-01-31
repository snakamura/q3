/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsregex.h>
#include <qsutil.h>

#include <cstdio>

using namespace qs;


/****************************************************************************
 *
 * Time
 *
 */

const WCHAR* qs::Time::pwszWeeks__[] = {
	L"Sun",
	L"Mon",
	L"Tue",
	L"Wed",
	L"Thu",
	L"Fri",
	L"Sat"
};
const WCHAR* qs::Time::pwszLongWeeks__[] = {
	L"Sunday",
	L"Monday",
	L"Tuesday",
	L"Wednesday",
	L"Thursday",
	L"Friday",
	L"Saturday"
};
const WCHAR* qs::Time::pwszMonths__[] = {
	L"Jan",
	L"Feb",
	L"Mar",
	L"Apr",
	L"May",
	L"Jun",
	L"Jul",
	L"Aug",
	L"Sep",
	L"Oct",
	L"Nov",
	L"Dec"
};
const WCHAR* qs::Time::pwszLongMonths__[] = {
	L"January",
	L"February",
	L"March",
	L"April",
	L"May",
	L"June",
	L"July",
	L"August",
	L"September",
	L"October",
	L"November",
	L"December"
};

wstring_ptr qs::Time::wstrDefaultFormat__(allocWString(L"%Y4/%M0/%D %h:%m:%s"));

qs::Time::Time()
{
	wYear = 0;
	wMonth = 0;
	wDayOfWeek = 0;
	wDay = 0;
	wHour = 0;
	wMinute = 0;
	wSecond = 0;
	wMilliseconds = 0;
	nTimeZone_ = 0;
}

qs::Time::Time(const SYSTEMTIME& st,
			   int nTimeZone)
{
	static_cast<SYSTEMTIME&>(*this) = st;
	nTimeZone_ = nTimeZone;
}

qs::Time::Time(int nYear,
			   int nMonth,
			   int nDayOfWeek,
			   int nDay,
			   int nHour,
			   int nMinute,
			   int nSecond,
			   int nMilliseconds,
			   int nTimeZone)
{
	wYear = nYear;
	wMonth = nMonth;
	wDayOfWeek = nDayOfWeek;
	wDay = nDay;
	wHour = nHour;
	wMinute = nMinute;
	wSecond = nSecond;
	wMilliseconds = nMilliseconds;
	nTimeZone_ = nTimeZone;
}

qs::Time::Time(const Time& time)
{
	static_cast<SYSTEMTIME&>(*this) = time;
	nTimeZone_ = time.nTimeZone_;
}

qs::Time::~Time()
{
}

Time& qs::Time::operator=(const Time& time)
{
	if (&time != this) {
		static_cast<SYSTEMTIME&>(*this) = time;
		nTimeZone_ = time.nTimeZone_;
	}
	return *this;
}

int qs::Time::getTimeZone() const
{
	return nTimeZone_;
}

void qs::Time::setTimeZone(int nTimeZone)
{
	nTimeZone_ = nTimeZone;
}

Time& qs::Time::addSecond(int nSecond)
{
	int nSecondNew = wSecond + nSecond;
	if (nSecondNew > 59) {
		int nMinute = nSecondNew/60;
		wSecond = nSecondNew%60;
		addMinute(nMinute);
	}
	else if (nSecondNew < 0) {
		int nMinute = nSecondNew/60 - 1;
		wSecond = 60 + nSecondNew%60;
		addMinute(nMinute);
	}
	else {
		wSecond = nSecondNew;
	}
	return *this;
}

Time& qs::Time::addMinute(int nMinute)
{
	int nMinuteNew = wMinute + nMinute;
	if (nMinuteNew > 59) {
		int nHour = nMinuteNew/60;
		wMinute = nMinuteNew%60;
		addHour(nHour);
	}
	else if (nMinuteNew < 0) {
		int nHour = nMinuteNew/60 - 1;
		wMinute = 60 + nMinuteNew%60;
		addHour(nHour);
	}
	else {
		wMinute = nMinuteNew;
	}
	return *this;
}

Time& qs::Time::addHour(int nHour)
{
	int nHourNew = wHour + nHour;
	if (nHourNew > 23) {
		int nDay = nHourNew/24;
		wHour = nHourNew%24;
		addDay(nDay);
	}
	else if (nHourNew < 0) {
		int nDay = nHourNew/24 - 1;
		wHour = 24 + nHourNew%24;
		addDay(nDay);
	}
	else {
		wHour = nHourNew;
	}
	return *this;
}

Time& qs::Time::addDay(int nDay)
{
	int nDayNew = wDay + nDay;
	if (nDayNew > getDayCount(wYear, wMonth)) {
		while (nDayNew > getDayCount(wYear, wMonth)) {
			nDayNew -= getDayCount(wYear, wMonth);
			if (wMonth < 12) {
				wMonth++;
			}
			else {
				wMonth = 1;
				wYear++;
			}
		}
		wDay = nDayNew;
	}
	else if (nDayNew <= 0) {
		while (nDayNew <= 0) {
			if (wMonth > 1) {
				wMonth--;
			}
			else {
				wMonth = 12;
				wYear--;
			}
			nDayNew += getDayCount(wYear, wMonth);
		}
		wDay = nDayNew;
	}
	else {
		wDay = nDayNew;
	}
	
	int nDayOfWeekNew = (wDayOfWeek + nDay)%7;
	wDayOfWeek = nDayOfWeekNew + (nDayOfWeekNew < 0 ? 7 : 0);
	
	return *this;
}

Time& qs::Time::convertToZone()
{
	addHour(nTimeZone_/100);
	addMinute(nTimeZone_%100);
	return *this;
}

wstring_ptr qs::Time::format(const WCHAR* pwszFormat,
							 Format format) const
{
	Time time(*this);
	
	int nTimeZone = nTimeZone_;
	switch (format) {
	case FORMAT_UTC:
		nTimeZone = 0;
		break;
	case FORMAT_LOCAL:
		nTimeZone = getSystemTimeZone();
		time.addHour(nTimeZone/100);
		time.addMinute(nTimeZone%100);
		break;
	case FORMAT_ORIGINAL:
		time.addHour(nTimeZone/100);
		time.addMinute(nTimeZone%100);
		break;
	default:
		assert(false);
		return 0;
	}
	
	StringBuffer<WSTRING> buf;
	WCHAR wsz[32];
	for (const WCHAR* p = pwszFormat; *p; ++p) {
		if (*p == L'%') {
			switch (*(p + 1)) {
			case L'Y':
				switch (*(p + 2)) {
				case L'2':
					_snwprintf(wsz, countof(wsz), L"%02d", time.wYear%100);
					break;
				case L'4':
					_snwprintf(wsz, countof(wsz), L"%04d", time.wYear);
					break;
				default:
					return 0;
				}
				buf.append(wsz);
				p += 2;
				break;
			case L'M':
				switch (*(p + 2)) {
				case L'0':
					_snwprintf(wsz, countof(wsz), L"%02d", time.wMonth);
					buf.append(wsz);
					break;
				case L'1':
					buf.append(pwszMonths__[time.wMonth - 1]);
					break;
				case L'2':
					buf.append(pwszLongMonths__[time.wMonth - 1]);
					break;
				case '3':
				case '4':
					{
						wstring_ptr wstr(getMonthName(time.wMonth, *(p + 2) == '4'));
						if (!wstr.get())
							return 0;
						buf.append(wstr.get());
					}
					break;
				default:
					return 0;
				}
				p += 2;
				break;
			case L'D':
				_snwprintf(wsz, countof(wsz), L"%02d", time.wDay);
				buf.append(wsz);
				++p;
				break;
			case L'W':
				switch (*(p + 2)) {
				case '0':
					buf.append(pwszWeeks__[time.wDayOfWeek]);
					++p;
					break;
				case '1':
					buf.append(pwszLongWeeks__[time.wDayOfWeek]);
					++p;
					break;
				case '2':
				case '3':
					{
						wstring_ptr wstr(getDayName(time.wDayOfWeek, *(p + 2) == '3'));
						if (!wstr.get())
							return 0;
						buf.append(wstr.get());
						++p;
					}
					break;
				default:
					buf.append(pwszWeeks__[time.wDayOfWeek]);
					break;
				}
				++p;
				break;
			case L'h':
				_snwprintf(wsz, countof(wsz), L"%02d", time.wHour);
				buf.append(wsz);
				++p;
				break;
			case L'm':
				_snwprintf(wsz, countof(wsz), L"%02d", time.wMinute);
				buf.append(wsz);
				++p;
				break;
			case L's':
				_snwprintf(wsz, countof(wsz), L"%02d", time.wSecond);
				buf.append(wsz);
				++p;
				break;
			case L'z':
				_snwprintf(wsz, countof(wsz), L"%c%04d",
					nTimeZone < 0 ? L'-' : L'+', abs(nTimeZone));
				buf.append(wsz);
				++p;
				break;
			case L'Z':
				_snwprintf(wsz, countof(wsz), L"%c%02d:%02d",
					nTimeZone < 0 ? L'-' : L'+',
					abs(nTimeZone)/100, abs(nTimeZone)%100);
				buf.append(wsz);
				++p;
				break;
			case L'%':
				buf.append(L'%');
				break;
			default:
				return 0;
			}
		}
		else {
			buf.append(*p);
		}
	}
	
	return buf.getString();
}

wstring_ptr qs::Time::format() const
{
	return format(L"%Y4-%M0-%DT%h:%m:%s%Z", Time::FORMAT_LOCAL);
}

bool qs::Time::parse(const WCHAR* pwszTime)
{
	const WCHAR* pwszRegex = L"^(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})(\\+|-)(\\d{2}):(\\d{2})$";
	RegexRangeList l;
	if (!Regex::match(pwszRegex, pwszTime, &l))
		return false;
	
	wYear = l.list_[1].getInt();
	wMonth = l.list_[2].getInt();
	wDay = l.list_[3].getInt();
	wDayOfWeek = getDayOfWeek(wYear, wMonth, wDay);
	wHour = l.list_[4].getInt();
	wMinute = l.list_[5].getInt();
	wSecond = l.list_[6].getInt();
	wMilliseconds = 0;
	nTimeZone_ = l.list_[8].getInt()*100 + l.list_[9].getInt();
	if (*l.list_[7].pStart_ == L'-')
		nTimeZone_ = -nTimeZone_;
	
	return true;
}

Time qs::Time::getCurrentTime()
{
	SYSTEMTIME st;
	::GetSystemTime(&st);
	return Time(st, getSystemTimeZone());
}

int qs::Time::getDayCount(int nYear,
						  int nMonth)
{
	assert(0 < nMonth && nMonth <= 12);
	assert(nYear >= 0);
	
	int nDays = 31;
	switch (nMonth) {
	case 4:
	case 6:
	case 9:
	case 11:
		nDays = 30;
		break;
	case 2:
		nDays = nYear%4 ? 28 : nYear%100 ? 29 : nYear%400 ? 28 : 29;
		break;
	}
	return nDays;
}

int qs::Time::getDayCount(int nYear)
{
	return nYear % 4 ? 365 : nYear%100 ? 366 : nYear%400 ? 365 : 366;
}

int qs::Time::getDayOfWeek(int nYear,
						   int nMonth,
						   int nDay)
{
	assert(0 < nMonth && nMonth <= 12);
	assert(0 < nDay && nDay <= getDayCount(nYear, nMonth));
	
	const int nOffset[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	if (nMonth < 3)
		--nYear;
	return (nYear + nYear/4 - nYear/100 + nYear/400 + nOffset[nMonth - 1] + nDay) % 7;
}

wstring_ptr qs::Time::getMonthName(int nMonth,
								   bool bLong)
{
	assert(0 < nMonth && nMonth <= 12);
	
	const CALTYPE caltypeShort[] = {
		LOCALE_SABBREVMONTHNAME1,
		LOCALE_SABBREVMONTHNAME2,
		LOCALE_SABBREVMONTHNAME3,
		LOCALE_SABBREVMONTHNAME4,
		LOCALE_SABBREVMONTHNAME5,
		LOCALE_SABBREVMONTHNAME6,
		LOCALE_SABBREVMONTHNAME7
	};
	const CALTYPE caltypeLong[] = {
		LOCALE_SMONTHNAME1,
		LOCALE_SMONTHNAME2,
		LOCALE_SMONTHNAME3,
		LOCALE_SMONTHNAME4,
		LOCALE_SMONTHNAME5,
		LOCALE_SMONTHNAME6,
		LOCALE_SMONTHNAME7
	};
	return getLocaleInfo(bLong ? caltypeLong[nMonth - 1] : caltypeShort[nMonth - 1]);
}

wstring_ptr qs::Time::getDayName(int nDayOfWeek,
								 bool bLong)
{
	assert(0 <= nDayOfWeek && nDayOfWeek < 7);
	
	const CALTYPE caltypeShort[] = {
		LOCALE_SABBREVDAYNAME1,
		LOCALE_SABBREVDAYNAME2,
		LOCALE_SABBREVDAYNAME3,
		LOCALE_SABBREVDAYNAME4,
		LOCALE_SABBREVDAYNAME5,
		LOCALE_SABBREVDAYNAME6,
		LOCALE_SABBREVDAYNAME7
	};
	const CALTYPE caltypeLong[] = {
		LOCALE_SDAYNAME1,
		LOCALE_SDAYNAME2,
		LOCALE_SDAYNAME3,
		LOCALE_SDAYNAME4,
		LOCALE_SDAYNAME5,
		LOCALE_SDAYNAME6,
		LOCALE_SDAYNAME7
	};
	return getLocaleInfo(bLong ? caltypeLong[nDayOfWeek] : caltypeShort[nDayOfWeek]);
}

int qs::Time::getSystemTimeZone()
{
	static int nTimezone = -1;
	if (nTimezone == -1) {
		TIME_ZONE_INFORMATION tzi;
		DWORD dw = ::GetTimeZoneInformation(&tzi);
		int nBias = tzi.Bias;
#ifdef _WIN32_WCE
		if (tzi.DaylightDate.wMonth != 0) {
			SYSTEMTIME st;
			::GetSystemTime(&st);
			st.wYear = 0;
			Time timeCurrent(st, 0);
			Time timeDaylight(getTransitionDate(tzi.DaylightDate));
			Time timeStandard(getTransitionDate(tzi.StandardDate));
			
			if (timeDaylight < timeStandard) {
				if (timeDaylight < timeCurrent && timeCurrent < timeStandard)
					nBias += tzi.DaylightBias;
				else
					nBias += tzi.StandardBias;
			}
			else {
				if (timeStandard < timeCurrent && timeCurrent < timeDaylight)
					nBias += tzi.StandardBias;
				else
					nBias += tzi.DaylightBias;
			}
		}
		else {
			nBias += tzi.StandardBias;
		}
#else
		switch (dw) {
		case TIME_ZONE_ID_STANDARD:
		case TIME_ZONE_ID_UNKNOWN:
			nBias += tzi.StandardBias;
			break;
		case TIME_ZONE_ID_DAYLIGHT:
			nBias += tzi.DaylightBias;
			break;
		default:
			break;
		}
#endif
		nTimezone = -(nBias/60*100 + (nBias%60));
	}
	return nTimezone;
}

Time qs::Time::getTransitionDate(const SYSTEMTIME& time)
{
	int nDay = 1;
	SYSTEMTIME st;
	::GetSystemTime(&st);
	int nDayOfWeek = getDayOfWeek(st.wYear, time.wMonth, 1);
	if (nDayOfWeek < time.wDayOfWeek)
		nDay += time.wDayOfWeek - nDayOfWeek;
	else if (nDayOfWeek > time.wDayOfWeek)
		nDay += (7 - nDayOfWeek) + time.wDayOfWeek;
	if (1 < time.wDay && time.wDay < 5) {
		nDay += 7*(time.wDay - 1);
	}
	else if (time.wDay == 5) {
		int nDayCount = getDayCount(st.wYear, time.wMonth);
		while (nDay + 7 <= nDayCount)
			nDay += 7;
	}
	return Time(0, time.wMonth, time.wDayOfWeek, nDay,
		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, 0);
}

const WCHAR* qs::Time::getDefaultFormat()
{
	return wstrDefaultFormat__.get();
}

void qs::Time::setDefaultFormat(const WCHAR* pwszDefaultFormat)
{
	wstrDefaultFormat__ = allocWString(pwszDefaultFormat);
}

wstring_ptr qs::Time::getLocaleInfo(LCTYPE type)
{
	int nLen = ::GetLocaleInfo(LOCALE_USER_DEFAULT, type, 0, 0);
	if (nLen == 0)
		return 0;
	
	tstring_ptr tstr(allocTString(nLen));
	if (::GetLocaleInfo(LOCALE_USER_DEFAULT, type, tstr.get(), nLen) == 0)
		return 0;

#ifdef UNICODE
	return tstr;
#else
	return tcs2wcs(tstr.get());
#endif
}

QSEXPORTPROC bool qs::operator==(const Time& lhs,
								 const Time& rhs)
{
	return lhs.wYear == rhs.wYear &&
		lhs.wMonth == rhs.wMonth &&
		lhs.wDay == rhs.wDay &&
		lhs.wHour == rhs.wHour &&
		lhs.wMinute == rhs.wMinute &&
		lhs.wSecond == rhs.wSecond &&
		lhs.wMilliseconds == rhs.wMilliseconds;
}

QSEXPORTPROC bool qs::operator!=(const Time& lhs,
								 const Time& rhs)
{
	return !(lhs == rhs);
}

QSEXPORTPROC bool qs::operator<(const Time& lhs,
								const Time& rhs)
{
	if (lhs.wYear != rhs.wYear)
		return lhs.wYear < rhs.wYear;
	if (lhs.wMonth != rhs.wMonth)
		return lhs.wMonth < rhs.wMonth;
	if (lhs.wDay != rhs.wDay)
		return lhs.wDay < rhs.wDay;
	if (lhs.wHour != rhs.wHour)
		return lhs.wHour < rhs.wHour;
	if (lhs.wMinute != rhs.wMinute)
		return lhs.wMinute < rhs.wMinute;
	if (lhs.wSecond != rhs.wSecond)
		return lhs.wSecond < rhs.wSecond;
	return lhs.wMilliseconds < rhs.wMilliseconds;
}

QSEXPORTPROC bool qs::operator<=(const Time& lhs,
								 const Time& rhs)
{
	return !(rhs < lhs);
}

QSEXPORTPROC bool qs::operator>(const Time& lhs,
								const Time& rhs)
{
	return rhs < lhs;
}

QSEXPORTPROC bool qs::operator>=(const Time& lhs,
								 const Time& rhs)
{
	return !(lhs < rhs);
}

QSEXPORTPROC int qs::compare(const Time& lhs,
							 const Time& rhs,
							 bool bIgnoreTimeZone)
{
	if (bIgnoreTimeZone) {
		return (lhs < rhs) ? -1 : (lhs > rhs) ? 1 : 0;
	}
	else {
		Time time1(lhs);
		time1.convertToZone();
		Time time2(rhs);
		time2.convertToZone();
		return (time1 < time2) ? -1 : (time1 > time2) ? 1 : 0;
	}
}
