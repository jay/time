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

/** Usage and design:

Functions for working with time in Windows.

When a function returns [failure] all output parameters are invalid unless otherwise specified.
When a function returns [success] all output parameters are valid unless otherwise specified.
*/

#ifndef _JAY_TIME_TIME_HPP
#define _JAY_TIME_TIME_HPP

#include <windows.h>
#include <time.h>

#include <iostream>



namespace jay {
namespace time {

/* ShowSystemTime()
- Output all data members of a SYSTEMTIME
*/
void ShowSystemTime( const SYSTEMTIME &st, std::ostream &output = std::cout );


/* IsYearValid()
- Check if a year is within the range allowable by MS.

[ret][failure] (false) : 'year' is invalid
[ret][success] (true) : 'year' is valid
*/
inline bool IsYearValid( const unsigned year )
{
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724950.aspx
    return ( year >= 1601 ) && ( year <= 30827 );
}


/* IsLeapYear()
- Check if a year is a Gregorian calendar leap year and within the range allowable by MS.

[ret][failure] (false) : 'year' is not within range or not a leap year
[ret][success] (true) : 'year' is a valid Gregorian calendar leap year
*/
bool IsLeapYear( const unsigned year );


/* IsDateValid()
- Check if a date is valid.

[ret][failure] (false) : The date is invalid
[ret][success] (true) : The date is valid
*/
bool IsDateValid( const unsigned day, const unsigned month, const unsigned year );


/* GetDayOfWeek()
- Get the day of week from any date.

[ret][failure] (unsigned) : The date is invalid. Any number [0, 6] could be returned.
[ret][success] (unsigned) : The day of the week. [0=Sun, 6=Sat]
*/
unsigned short GetDayOfWeek( const unsigned day, const unsigned month, const unsigned year );


/* IsSystemTimeValid()
* IsSystemTimeValid_IgnoreDayOfWeek()
- Check if a SYSTEMTIME contains a valid point in time (UTC or local).

If the _IgnoreDayOfWeek version is called then data member wDayOfWeek is ignored. Some WinAPI
functions ignore the wDayOfWeek member when working with SYSTEMTIME structs.

[in] 'st' : Some point in time, UTC or local
[ret][failure] (false) : The time is invalid
[ret][success] (true) : The time is valid
*/
bool IsSystemTimeValid_IgnoreDayOfWeek( const SYSTEMTIME &st );
bool IsSystemTimeValid( const SYSTEMTIME &st );


/* IsFileTimeValid()
- Check if a FILETIME contains a valid point in time (UTC or local).

[in] 'ft' : Some point in time, UTC or local
[ret][failure] (false) : The time is invalid
[ret][success] (true) : The time is valid
*/
bool IsFileTimeValid( const FILETIME &ft );


/* FileTimeAdd100nsIntervals()
* FileTimeSubtract100nsIntervals()
- Add or Subtract 100ns intervals from a FILETIME.

The validity of the FILETIME is checked both before and after the addition/subtraction.

[in] 'ft' : Some point in time, UTC or local
[in] 'intervals' : The number of intervals to add or subtract from 'ft'
[ret][failure] (false) : 'ft' is invalid. 'intervals' may or may not have been added or subtracted.
[ret][success] (true) : 'intervals' has been added or subtracted from 'ft'
*/
bool FileTimeSubtract100nsIntervals( FILETIME &ft, const long long intervals );
bool FileTimeAdd100nsIntervals( FILETIME &ft, const long long intervals );


/* FileTimeAddMinutes()
* FileTimeSubtractMinutes()
- Add or Subtract minutes from a FILETIME.

The validity of the FILETIME is checked both before and after the addition/subtraction.

[in] 'ft' : Some point in time, UTC or local
[in] 'minutes' : The number of minutes to add or subtract from 'ft'
[ret][failure] (false) : 'ft' is invalid. 'minutes' may or may not have been added or subtracted.
[ret][success] (true) : 'minutes' has been added or subtracted from 'ft'
*/
bool FileTimeSubtractMinutes( FILETIME &ft, const long long minutes );
bool FileTimeAddMinutes( FILETIME &ft, const long long minutes );


/* SystemTimeAddMinutes()
* SystemTimeSubtractMinutes()
- Add or Subtract minutes from a SYSTEMTIME.

The validity of the SYSTEMTIME is checked both before and after the addition/subtraction.

[in] 'st' : Some point in time, UTC or local
[in] 'minutes' : The number of minutes to add or subtract from 'st'
[ret][failure] (false) : The time is invalid. The minutes may or may not have been added/subtracted.
[ret][success] (true) : The time is valid and the minutes have been added/subtracted.
*/
bool SystemTimeSubtractMinutes( SYSTEMTIME &st, const long long minutes );
bool SystemTimeAddMinutes( SYSTEMTIME &st, const long long minutes );


/* SystemTimeToTm()
- Convert a SYSTEMTIME to a tm struct.

[in] 'st' : Some point in time, UTC or local
[in] 'st_isdst' : Whether 'st' is adjusted for daylight saving time. Set false if 'st' is UTC time.
[out] 'tm' : Almost the same point in time as 'st' (struct tm doesn't hold milliseconds)
[ret][failure] (false) : Conversion failed
[ret][success] (true) : Conversion successful
*/
bool SystemTimeToTm( const SYSTEMTIME &st, const bool st_isdst, tm &tm );


/* CompareSystemTimes()
* CompareSystemTimes_IgnoreDayOfWeek()
- Compare two SYSTEMTIMEs.

If the _IgnoreDayOfWeek version is called then data member wDayOfWeek is ignored. Some WinAPI
functions ignore the wDayOfWeek member when working with SYSTEMTIME structs.

[ret] (-1) : a < b
[ret] (0) : a == b
[ret] (1) : a > b
*/
int CompareSystemTimes_IgnoreDayOfWeek( const SYSTEMTIME &a, const SYSTEMTIME &b );
int CompareSystemTimes( const SYSTEMTIME &a, const SYSTEMTIME &b );

} // namespace time
} // namespace jay
#endif // _JAY_TIME_TIME_HPP
