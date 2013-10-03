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

/** Classes to hold Windows local or UTC times in ISO 8601 format or USA format.

Documentation is in iso8601.hpp. Example is in iso8601_example.cpp.
*/

#include "iso8601.hpp"
#include "time.hpp"
#include "timezone.hpp"

#include <windows.h>
#include <assert.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>


using namespace std;



namespace {
} // anonymous namespace



namespace jay {
namespace time {

string ISO8601::GetDayStringEnglish( const unsigned day_of_the_week ) const
{
    switch( day_of_the_week )
    {
    case 0: return ( format.day_string_with_abbreviation ? "Sun" : "Sunday" );
    case 1: return ( format.day_string_with_abbreviation ? "Mon" : "Monday" );
    case 2: return ( format.day_string_with_abbreviation ? "Tue" : "Tuesday" );
    case 3: return ( format.day_string_with_abbreviation ? "Wed" : "Wednesday" );
    case 4: return ( format.day_string_with_abbreviation ? "Thu" : "Thursday" );
    case 5: return ( format.day_string_with_abbreviation ? "Fri" : "Friday" );
    case 6: return ( format.day_string_with_abbreviation ? "Sat" : "Saturday" );
    }

    return "";
}


string ISO8601::GetDayString( const SYSTEMTIME &st ) const
{
    return GetDayStringEnglish( st.wDayOfWeek );
}


string ISO8601::GetDayStringUSA( const SYSTEMTIME &st ) const
{
    return GetDayStringEnglish( st.wDayOfWeek );
}



string ISO8601::GetDateString( const SYSTEMTIME &st ) const
{
    stringstream ss_date;

    ss_date.fill( '0' );

    if( st.wYear > 9999 )
        ss_date << "+";

    ss_date << setw( 4 ) << st.wYear;
    ss_date << "-" << setw( 2 ) << st.wMonth;
    ss_date << "-" << setw( 2 ) << st.wDay;

    return ss_date.str();
}


string ISO8601::GetDateStringUSA( const SYSTEMTIME &st ) const
{
    stringstream ss_date;

    ss_date << st.wMonth << "/" << st.wDay << "/" << st.wYear;

    return ss_date.str();
}



string ISO8601::GetTimeString( const SYSTEMTIME &st ) const
{
    stringstream ss_time;

    ss_time.fill( '0' );
    ss_time << setw( 2 ) << st.wHour;
    ss_time << ":" << setw( 2 ) << st.wMinute;
    ss_time << ":" << setw( 2 ) << st.wSecond;

    if( format.time_string_with_milliseconds )
        ss_time << "." << setw( 3 ) << st.wMilliseconds;

    return ss_time.str();
}


string ISO8601::GetTimeStringUSA( const SYSTEMTIME &st ) const
{
    stringstream ss_time;
    unsigned st_12hr = ( !st.wHour ? 12 : ( ( st.wHour > 12 ) ? ( st.wHour - 12 ) : st.wHour ) );

    ss_time.fill( '0' );
    ss_time << st_12hr << ":";
    ss_time << setw( 2 ) << st.wMinute;
    ss_time << ":" << setw( 2 ) << st.wSecond;

    if( format.time_string_with_milliseconds )
        ss_time << "." << setw( 3 ) << st.wMilliseconds;

    ss_time << " " << ( ( st.wHour < 12 ) ? "AM" : "PM" );

    return ss_time.str();
}



string ISO8601::GetUTCOffsetString( const long bias ) const
{
    if( !bias )
        return "Z";

    stringstream ss_utc;

    ss_utc.fill( '0' );
    ss_utc << ( ( bias > 0 ) ? "-" : "+" );
    ss_utc << setw( 2 ) << ( abs( bias ) / 60 );
    ss_utc << ":" << setw( 2 ) << ( abs( bias ) % 60 );

    return ss_utc.str();
}


string ISO8601::GetUTCOffsetStringUSA( const long bias ) const
{
    if( !bias )
        return "(UTC)";

    stringstream ss_utc;

    ss_utc.fill( '0' );
    ss_utc << "(UTC";
    ss_utc << ( ( bias > 0 ) ? "-" : "+" );
    ss_utc << setw( 2 ) << ( abs( bias ) / 60 );
    ss_utc << ":" << setw( 2 ) << ( abs( bias ) % 60 );
    ss_utc << ")";

    return ss_utc.str();
}



string ISO8601::GetUTCTimestampString( const SYSTEMTIME &utc_st ) const
{
    stringstream ss_timestamp;

    string date = GetDateString( utc_st );
    string time = GetTimeString( utc_st );

    if( !date.size() || !time.size() )
        return "";

    ss_timestamp << date << "T" << time;

    ss_timestamp.fill( '0' );

    // if time string wasn't already formatted with milliseconds
    if( !format.time_string_with_milliseconds )
        ss_timestamp << "." << setw( 3 ) << utc_st.wMilliseconds;

    ss_timestamp << "Z";

    return ss_timestamp.str();
}


string ISO8601::GetUTCTimestampStringUSA( const SYSTEMTIME &utc_st ) const
{
    return GetUTCTimestampString( utc_st );
}



bool ISO8601::GetTimeInfo( DayDateTime &ddt, const FILETIME &utc_ft ) const
{
    return GetTimeInfoLocalOrUTC( ddt, utc_ft, prefer_local_time );
}


bool ISO8601::GetTimeInfo( DayDateTime &ddt, const SYSTEMTIME &utc_st ) const
{
    FILETIME utc_ft = {};

    if( !SystemTimeToFileTime( &utc_st, &utc_ft ) )
    {
        ddt.Clear();
        return false;
    }

    return GetTimeInfo( ddt, utc_ft );
}


bool ISO8601::GetTimeInfo( DayDateTime &ddt ) const
{
    FILETIME utc_ft = {};

    GetSystemTimeAsFileTime( &utc_ft );

    return GetTimeInfo( ddt, utc_ft );
}


bool ISO8601::GetTimeInfo( TimeInfo &ti, const FILETIME &utc_ft ) const
{
    ti.Clear();

    ti.InitializeLocalTimePref( prefer_local_time );

    if( !GetTimeInfoLocalOrUTC( *ti._local, utc_ft, true )
        || !GetTimeInfoLocalOrUTC( *ti._utc, utc_ft, false )
    )
    {
        ti.Clear();
        return false;
    }

    ti.timestamp = GetUTCTimestampString( ti.utc->st );
    if( !ti.timestamp.size() )
    {
        ti.Clear();
        return false;
    }

    ti.valid = true;
    return ti.valid;
}


bool ISO8601::GetTimeInfo( TimeInfo &ti, const SYSTEMTIME &utc_st ) const
{
    FILETIME utc_ft = {};

    if( !SystemTimeToFileTime( &utc_st, &utc_ft ) )
    {
        ti.Clear();
        return false;
    }

    return GetTimeInfo( ti, utc_ft );
}


bool ISO8601::GetTimeInfo( TimeInfo &ti ) const
{
    FILETIME utc_ft = {};

    GetSystemTimeAsFileTime( &utc_ft );

    return GetTimeInfo( ti, utc_ft );
}



bool ISO8601::GetStrings( DayDateTime &ddt ) const
{
    ddt.day = GetDayString( ddt.st );
    ddt.date = GetDateString( ddt.st );
    ddt.time = GetTimeString( ddt.st );
    ddt.offset = GetUTCOffsetString( ddt.bias );

    return ddt.day.size() && ddt.date.size() && ddt.time.size() && ddt.offset.size();
}


bool ISO8601::GetStringsUSA( DayDateTime &ddt ) const
{
    ddt.day = GetDayStringUSA( ddt.st );
    ddt.date = GetDateStringUSA( ddt.st );
    ddt.time = GetTimeStringUSA( ddt.st );
    ddt.offset = GetUTCOffsetStringUSA( ddt.bias );

    return ddt.day.size() && ddt.date.size() && ddt.time.size() && ddt.offset.size();
}



bool ISO8601::GetTimeInfoLocalOrUTC(
    DayDateTime &ddt,
    const FILETIME &utc_ft,
    const bool convert_to_local_time
) const
{
    ddt.Clear();

    if( !IsFileTimeValid( utc_ft ) )
    {
        ddt.Clear();
        return false;
    }

    bool is_daylight_saving_time = false;

    if( convert_to_local_time )
    {
        SYSTEMTIME utc_st = {};
        DWORD tzi_id = 0;
        TIME_ZONE_INFORMATION tzi = {};

        if( !FileTimeToSystemTime( &utc_ft, &utc_st )
            || !UTCTimeToLocalTime( utc_st, ddt.st, tzi_id, tzi )
        )
        {
            ddt.Clear();
            return false;
        }

        if( tzi_id == TIME_ZONE_ID_DAYLIGHT )
        {
            is_daylight_saving_time = true;
            ddt.bias = tzi.Bias + tzi.DaylightBias;
        }
        else if( tzi_id == TIME_ZONE_ID_STANDARD )
        {
            ddt.bias = tzi.Bias + tzi.StandardBias;
        }
        else if( tzi_id == TIME_ZONE_ID_UNKNOWN )
        {
            ddt.bias = tzi.Bias;
        }
        else
        {
            ddt.Clear();
            return false;
        }

        ddt.ft = utc_ft;
        if( !FileTimeSubtractMinutes( ddt.ft, ddt.bias ) )
        {
            ddt.Clear();
            return false;
        }
    }
    else // Use UTC time, not local
    {
        ddt.ft = utc_ft;

        if( !FileTimeToSystemTime( &ddt.ft, &ddt.st ) )
        {
            ddt.Clear();
            return false;
        }

        ddt.bias = 0;
    }

    if( !SystemTimeToTm( ddt.st, is_daylight_saving_time, ddt.tm )
        || !( format.usa_style ? GetStringsUSA( ddt ) : GetStrings( ddt ) )
    )
    {
        ddt.Clear();
        return false;
    }

    ddt.format = format;

    ddt.valid = true;
    return ddt.valid;
}

} // namespace time
} // namespace jay
