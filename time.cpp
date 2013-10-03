/*
Copyright (C) 2013 Jay Satiro <raysatiro@yahoo.com>
All rights reserved.

This file is part of jay::time.

jay::time is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

jay::time is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with jay::time. If not, see <http://www.gnu.org/licenses/>.
*/

/** Functions for working with time in Windows.

Documentation is in time.hpp.
*/

#include "time.hpp"

#include <windows.h>
#include <time.h>
#include <limits.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>


using namespace std;



namespace {
} // anonymous namespace



namespace jay {
namespace time {

void ShowSystemTime( const SYSTEMTIME &st, std::ostream &output /* = std::cout */ )
{
#define SSTM(a) output << #a << ": " << (unsigned)a << endl;
    SSTM(st.wYear);
    SSTM(st.wMonth);
    SSTM(st.wDayOfWeek);
    SSTM(st.wDay);
    SSTM(st.wHour);
    SSTM(st.wMinute);
    SSTM(st.wSecond);
    SSTM(st.wMilliseconds);
}



bool IsLeapYear( const unsigned year )
{
    if( !( year % 400 ) )
        return true;
    else if( !( year % 100 ) )
        return false;
    else if( !( year % 4 ) )
        return true;
    else
        return false;
}



bool IsDateValid( const unsigned day, const unsigned month, const unsigned year )
{
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724950.aspx
    if( !IsYearValid( year )
        || ( ( month < 1 ) || ( month > 12 ) )
        || ( ( day < 1 ) || ( day > 31 ) )
    )
        return false;

    const unsigned days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    return ( day <= days_in_month[ month - 1 ] )
        || ( ( day == 29 ) && ( month == 2 ) && IsLeapYear( year ) );
}



// this should return [0=Sun, 6=Sat]
unsigned short GetDayOfWeek( const unsigned day, const unsigned month, const unsigned year )
{
#if 0
    SYSTEMTIME st = { year, month, 0, day, 0, 0, 0, 0 };
    FILETIME ft = {};

    if( !SystemTimeToFileTime( &st, &ft ) || !FileTimeToSystemTime( &ft, &st ) )
        return 0;

    return st.wDayOfWeek;
#else
    // Tomohiko Sakomoto method
    // http://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week

    if( ( month < 1 ) || ( month > 12 ) )
        return 0;

    const unsigned t[ 12 ] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    const unsigned y = year - ( month < 3 );
    return ( y + ( y / 4 ) - ( y / 100 ) + ( y / 400 ) + t[ month - 1 ] + day ) % 7;
#endif
}



bool IsSystemTimeValid_IgnoreDayOfWeek( const SYSTEMTIME &st )
{
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724950.aspx
    return IsDateValid( st.wDay, st.wMonth, st.wYear )
        && ( st.wHour <= 23 )
        && ( st.wMinute <= 59 )
        && ( st.wSecond <= 59 )
        && ( st.wMilliseconds <= 999 );
}


bool IsSystemTimeValid( const SYSTEMTIME &st )
{
    return IsSystemTimeValid_IgnoreDayOfWeek( st )
        && ( st.wDayOfWeek == GetDayOfWeek( st.wDay, st.wMonth, st.wYear ) );
}



bool IsFileTimeValid( const FILETIME &ft )
{
    // this is the same time as max SYSTEMTIME
    FILETIME max = { 0xF06C58F0, 0x7FFF35F4 };

    return ( ( ft.dwHighDateTime < max.dwHighDateTime )
        || ( ( ft.dwHighDateTime == max.dwHighDateTime )
            && ( ft.dwLowDateTime <= max.dwLowDateTime )
        )
    );
}



bool FileTimeSubtract100nsIntervals( FILETIME &ft, const long long intervals )
{
    if( !IsFileTimeValid( ft ) )
        return false;

    if( !intervals )
        return true;

    LARGE_INTEGER li = { { ft.dwLowDateTime, (LONG)ft.dwHighDateTime } };

    if( ( ( intervals > 0 ) && ( li.QuadPart < ( LLONG_MIN + intervals ) ) )
        || ( ( intervals < 0 ) && ( li.QuadPart > ( LLONG_MAX + intervals ) ) )
    ) // subtraction would cause overflow
        return false;

    li.QuadPart = li.QuadPart - intervals;
    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = (DWORD)li.HighPart;

    return IsFileTimeValid( ft );
}


bool FileTimeAdd100nsIntervals( FILETIME &ft, const long long intervals )
{
    if( ( ( intervals < 0 ) && ( ( LLONG_MAX + intervals ) < 0 ) )
        || ( ( intervals > 0 ) && ( ( LLONG_MIN + intervals ) > 0 ) )
    ) // sign change would cause overflow
        return false;

    return FileTimeSubtract100nsIntervals( ft, -intervals );
}



bool FileTimeSubtractMinutes( FILETIME &ft, const long long minutes )
{
    return FileTimeSubtract100nsIntervals( ft, minutes * 10000 * 1000 * 60 );
}


bool FileTimeAddMinutes( FILETIME &ft, const long long minutes )
{
    return FileTimeAdd100nsIntervals( ft, minutes * 10000 * 1000 * 60 );
}



bool SystemTimeSubtractMinutes( SYSTEMTIME &st, const long long minutes )
{
    FILETIME ft = {};

    return IsSystemTimeValid( st )
        && SystemTimeToFileTime( &st, &ft )
        && FileTimeSubtractMinutes( ft, minutes )
        && FileTimeToSystemTime( &ft, &st );
}


bool SystemTimeAddMinutes( SYSTEMTIME &st, const long long minutes )
{
    FILETIME ft = {};

    return IsSystemTimeValid( st )
        && SystemTimeToFileTime( &st, &ft )
        && FileTimeAddMinutes( ft, minutes )
        && FileTimeToSystemTime( &ft, &st );
}



bool SystemTimeToTm( const SYSTEMTIME &st, const bool st_isdst, tm &tm )
{
    if( !IsSystemTimeValid( st ) )
        return false;

    tm.tm_sec = st.wSecond;
    tm.tm_min = st.wMinute;
    tm.tm_hour = st.wHour;
    tm.tm_mday = st.wDay;
    tm.tm_mon = st.wMonth - 1;
    tm.tm_year = st.wYear - 1900;
    tm.tm_wday = st.wDayOfWeek;

    const int acc[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    tm.tm_yday = acc[ tm.tm_mon ] + tm.tm_mday - 1;
    if( IsLeapYear( st.wYear ) && ( tm.tm_mon > 1 ) )
        ++tm.tm_yday;

    tm.tm_isdst = st_isdst;

    return true;
}



#define RETURN_IF_NOT_EQUAL(a,b)   if( a < b ) return -1; else if( a > b ) return 1;

int CompareSystemTimes_IgnoreDayOfWeek( const SYSTEMTIME &a, const SYSTEMTIME &b )
{
    RETURN_IF_NOT_EQUAL( a.wYear, b.wYear );
    RETURN_IF_NOT_EQUAL( a.wMonth, b.wMonth );
    RETURN_IF_NOT_EQUAL( a.wDay, b.wDay );
    RETURN_IF_NOT_EQUAL( a.wHour, b.wHour );
    RETURN_IF_NOT_EQUAL( a.wMinute, b.wMinute );
    RETURN_IF_NOT_EQUAL( a.wSecond, b.wSecond );
    RETURN_IF_NOT_EQUAL( a.wMilliseconds, b.wMilliseconds );
    return 0;
}


int CompareSystemTimes( const SYSTEMTIME &a, const SYSTEMTIME &b )
{
    int x = CompareSystemTimes_IgnoreDayOfWeek( a, b );
    if( x )
        return x;

    RETURN_IF_NOT_EQUAL( a.wDayOfWeek, b.wDayOfWeek );
    return 0;
}

} // namespace time
} // namespace jay
