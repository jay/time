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

Classes to hold Windows local or UTC times with strings in ISO 8601 format or USA format.

When a function returns [failure] all output parameters are invalid unless otherwise specified.
When a function returns [success] all output parameters are valid unless otherwise specified.


class TimeFormat
- Formatting options for the strings output by ISO8601 functions.

class ISO8601
- Class to hold functions that need "sticky" parameters.

ISO8601::GetTimeInfo()
- Convert input UTC time to a DayDateTime/TimeInfo.

class DayDateTime
- Output class for ISO8601::GetTimeInfo(). Receives input UTC or converted local time.

class TimeInfo : public DayDateTime
- Output class for ISO8601::GetTimeInfo(). Receives input UTC and converted local time.


You create an ISO8601 object that is persistent or temporary, set your sticky formatting and
conversion options and then call one of its GetTimeInfo() member functions to write to a TimeInfo or
a DayDateTime object. DayDateTime objects receive either a UTC or local time; TimeInfo objects
receive both. TimeInfo objects basically hold two DayDateTime objects with one preferred (the one
inherited from).

You can check the ISO8601 function's return or the DayDateTime/TimeInfo's 'valid' data member to
determine whether or not the object was written to successfully. If you are converting a stored UTC
time and not the current UTC time (which ISO8601 functions obtain from Windows), or if either class
is enhanced by a derived class, I advise you always check for success.

DayDateTime::Clear()/TimeInfo::Clear():
It's unlikely you will need to call this. By default the object is Clear()'d on instantiation. That
means its strings are empty and its other data members are zeroed out. The object is cleared before
it is written to so you do not need to call its Clear() before a call to ISO8601::GetTimeInfo().
Also the object is cleared if there was a failure in writing to it so you do not need to call
Clear() on failure either.


Here's the typical usage:

ISO8601 g_iso8601; // The default is to adjust UTC to local time
main()
{
    if( user wants time with milliseconds )
        g_iso8601.format.time_with_milliseconds = true;

    SomeFunction();
}
SomeFunction()
{
    DayDateTime ddt; // 'ddt' is cleared
    g_iso8601.GetTimeInfo( ddt ); // 'ddt' is updated with the current local time
    ddt.Show( std::cerr ); // 'ddt' is output to std::cerr
    std::cerr << errmsg << endl;
}

You can also pass a persistent or temporary ISO8601 object for the DayDateTime/TimeInfo object's
instantiation which will write to the object immediately. An ISO8601 object that is passed for
instantiation is used only for that. In any case ISO8601 objects do not remain associated with the
DayDateTime/TimeInfo they operated on.

    DayDateTime ddt( g_iso8601 ); // Same as the second line in SomeFunction()
    DayDateTime ddt( ISO8601() ); // ddt is created using the default ISO8601 options
    DayDateTime ddt( ISO8601(), utc_ft ); // converts the UTC time stored in FILETIME 'utc_ft'

For more examples refer to iso8601_example.cpp.
*/

#ifndef _JAY_TIME_ISO8601_HPP
#define _JAY_TIME_ISO8601_HPP

#include <windows.h>
#include <time.h>

#include <string>
#include <iostream>
#include <sstream>



namespace jay {
namespace time {

class TimeFormat;
class ISO8601;
class DayDateTime;
class TimeInfo;



/* class TimeFormat
- Formatting options for the strings output by ISO8601 functions
*/
class TimeFormat
{
public:
    // [false] : 'date, time, offset' strings are ISO 8601 style: 2013-08-11, 14:46:00, -04:00
    // [true] : 'date, time, offset' strings are USA style: 8/11/2013, 2:46:00 PM, (UTC-04:00)
    bool usa_style; // = false

    /* [false] : 'day' string unabbreviated: Sunday, Monday, Tuesday, Wednesday, Thursday, Friday
    or Saturday
    [true] : 'day' string with "long" abbreviation: Sun, Mon, Tue, Wed, Thu, Fri or Sat
    */
    bool day_string_with_abbreviation; // = false

    // [false] : 'time' string without milliseconds: 18:46:00
    // [true] : 'time' string with milliseconds: 18:46:00.085
    bool time_string_with_milliseconds; // = false

    void Clear()
    {
        usa_style = false;
        day_string_with_abbreviation = false;
        time_string_with_milliseconds = false;
    }

    explicit TimeFormat(
        bool usa_style = false,
        bool day_string_with_abbreviation = false,
        bool time_string_with_milliseconds = false
        ) :
        usa_style( usa_style ),
        day_string_with_abbreviation( day_string_with_abbreviation ),
        time_string_with_milliseconds( time_string_with_milliseconds )
    {}
};



/* class ISO8601
- Class to hold functions that need "sticky" parameters.

This is the class that holds the GetTimeInfo() functions, which write to TimeInfo or DayDateTime.
Public data members are essentially sticky function parameters that you may modify between calls.
*/
class ISO8601
{
public:
    /* prefer_local_time
    - Whether or not the (inherited from) DayDateTime object should receive the local time

    The GetTimeInfo() functions convert the input UTC time to either a DayDateTime or TimeInfo
    object. The former receives either the UTC time or converted local time depending on this
    preference. The latter receives both, and this preference is used to determine which of those is
    written to the inherited from DayDateTime object.

    [false] : write the UTC time to a DayDateTime object
    [true] : write the local time to a DayDateTime object
    */
    bool prefer_local_time; // = true

    /* This is the earliest year for which to apply daylight saving time (DST) if adjusting to
    local time. I chose 1967 because that was the first year of the Uniform Time Act in the USA.
    */
    unsigned dst_start_year; // = 1967

    /* ignore_dst
    - Whether or not to ignore daylight saving time (DST) during local time conversions.

    Any potential daylight saving time (DST) bias is applied to local time only if:
    Windows' auto-DST setting is enabled,
    && the local time is within Windows' DST range for your local timezone,
    && 'dst_start_year' <= the year of the converted local time,
    && 'ignore_dst' == false

    [false] : Don't ignore daylight saving time adjustments
    [true] : Ignore daylight saving time (DST) if adjusting to local time
    */
    bool ignore_dst; // = false

    // Formatting options for the output strings
    TimeFormat format;

    /* [in] 'day_of_the_week' : 0 (Sunday), 1 (Monday), 2 (Tuesday) , 3 (Wednesday), 4 (Thursday),
    5 (Friday), or 6 (Saturday)
    [ret][failure] (std::string) : Empty string
    [ret][success] (std::string) : The English day of the week
    */
    virtual std::string GetDayStringEnglish( const unsigned day_of_the_week ) const;

    // [ret][success][failure] (std::string) : GetDayStringEnglish()
    virtual std::string GetDayString( const SYSTEMTIME &st ) const;

    // [ret][success][failure] (std::string) : GetDayStringEnglish()
    virtual std::string GetDayStringUSA( const SYSTEMTIME &st ) const;

    // [ret][failure] (std::string) : Empty string
    // [ret][success] (std::string) : Date string: 2013-08-11
    virtual std::string GetDateString( const SYSTEMTIME &st ) const;

    // [ret][failure] (std::string) : Empty string
    // [ret][success] (std::string) : Date string, USA style: 8/11/2013
    virtual std::string GetDateStringUSA( const SYSTEMTIME &st ) const;

    // [ret][failure] (std::string) : Empty string
    // [ret][success] (std::string) : Time string: 14:46:00
    virtual std::string GetTimeString( const SYSTEMTIME &st ) const;

    // [ret][failure] (std::string) : Empty string
    // [ret][success] (std::string) : Time string, USA style: 2:46:00 PM
    virtual std::string GetTimeStringUSA( const SYSTEMTIME &st ) const;

    // [in] 'bias' : The offset from the UTC timezone in minutes
    // [ret][failure] (std::string) : Empty string
    // [ret][success] (std::string) : UTC offset string: -04:00
    virtual std::string GetUTCOffsetString( const long bias ) const;

    // [in] 'bias' : The offset from the UTC timezone in minutes
    // [ret][failure] (std::string) : Empty string
    // [ret][success] (std::string) : UTC offset string, USA style: (UTC-04:00)
    virtual std::string GetUTCOffsetStringUSA( const long bias ) const;

    // [in] 'utc_st' : Some point in time, UTC only
    // [ret][failure] (std::string) : Empty string
    // [ret][success] (std::string) : UTC Timestamp, always w/milliseconds: 2013-08-11T18:46:00.085Z
    virtual std::string GetUTCTimestampString( const SYSTEMTIME &utc_st ) const;

    // [in] 'utc_st' : Some point in time, UTC only
    // [ret][success][failure] (std::string) : GetUTCTimestampString()
    virtual std::string GetUTCTimestampStringUSA( const SYSTEMTIME &utc_st ) const;

    /* ISO8601::GetTimeInfo()
    - Convert input UTC time to a DayDateTime/TimeInfo.

    If a FILETIME/SYSTEMTIME is not passed then the current time is used.

    [out] 'ddt' / 'ti' : Local or UTC time
    [in][opt] 'utc_ft' / 'utc_st' : Some point in time, UTC only
    [ret][failure] (false) : Conversion failed. 'ddt' was cleared; see DayDateTime::Clear().
    [ret][success] (true) : Conversion successful
    */
    bool GetTimeInfo( DayDateTime &ddt, const FILETIME &utc_ft ) const;
    bool GetTimeInfo( DayDateTime &ddt, const SYSTEMTIME &utc_st ) const;
    bool GetTimeInfo( DayDateTime &ddt ) const;
    bool GetTimeInfo( TimeInfo &ti, const FILETIME &utc_ft ) const;
    bool GetTimeInfo( TimeInfo &ti, const SYSTEMTIME &utc_st ) const;
    bool GetTimeInfo( TimeInfo &ti ) const;

    explicit ISO8601(
        bool prefer_local_time = true,
        TimeFormat format = TimeFormat()
        ) :
        prefer_local_time( prefer_local_time ),
        dst_start_year( 1967 ),
        ignore_dst( false ),
        format( format )
    {}

    virtual ~ISO8601() {}

protected:
    /* ISO8601::GetStrings() const
    - Write all strings in ISO8601 format to DayDateTime object

    [in][part] 'ddt' : All time objects
    [out][part] 'ddt' : All strings
    [ret][failure] (false) : Some or all strings haven't been written
    [ret][success] (true) : All strings have been written
    */
    virtual bool GetStrings( DayDateTime &ddt ) const;

    /* ISO8601::GetStringsUSA() const
    - Write all strings in USA format to DayDateTime object

    [in][part] 'ddt' : All time objects
    [out][part] 'ddt' : All strings, USA style
    [ret][failure] (false) : Some or all strings haven't been written
    [ret][success] (true) : All strings have been written
    */
    virtual bool GetStringsUSA( DayDateTime &ddt ) const;

    /* ISO8601::GetTimeInfoLocalOrUTC() const
    - Convert input time to a DayDateTime object

    [out] 'ddt' : Local or UTC time
    [in] 'utc_ft' : Some point in time, UTC only
    [in] 'convert_to_local_time' : Convert UTC time to local time before converting to DayDateTime
    [ret][failure] (false) : Conversion failed. 'ddt' was cleared; see DayDateTime::Clear().
    [ret][success] (true) : Conversion successful
    */
    virtual bool GetTimeInfoLocalOrUTC(
        DayDateTime &ddt,
        const FILETIME &utc_ft,
        const bool convert_to_local_time
    ) const;
};



/* class DayDateTime
- Output class for ISO8601::GetTimeInfo(). Receives input UTC or converted local time.

On success all members are valid and 'valid' is true.

This object receives either a local or UTC time depending on ISO8601::prefer_local_time.
*/
class DayDateTime // [out]
{
    friend class ISO8601;

public:
    // [false] : object invalid
    // [true] : all members are valid; the object was updated successfully
    bool valid;

    // Some point in time, local or UTC
    FILETIME ft;

    // Almost the same point in time as 'ft' except 'st' has less resolution (no nanoseconds)
    SYSTEMTIME st;

    /* struct tm for compatibility with strftime() etc
    Almost the same point in time as 'st' except 'tm' has less resolution (no milliseconds)

    'tm.tm_isdst' is never set -1; it is always set 0 (false) or 1 (true)
    The requirements for applying Daylight Saving Time (DST) to a DayDateTime object --in which case
    'tm.tm_isdst' would be set true-- are much more stringent than OS API and are listed in the
    comment block for ISO8601::ignore_dst. For UTC time 'tm.tm_isdst' is always false.
    */
    struct tm tm;

    // Offset in minutes of 'ft, st, tm' from the UTC timezone
    // A nonzero value means there was a local time zone adjustment
    long bias;

    // English day of the week
    std::string day;

    // Date, time, and offset from UTC timezone in ISO 8601 or USA format (see TimeFormat)
    std::string date, time, offset;

    // Formatting options for the strings
    TimeFormat format;

    void Show( std::ostream &output = std::cout ) const
    {
        output << "--- " << day << " " << date << " " << time
            << ( format.usa_style ? " " : "" ) << offset << " ---" << std::endl;
    }

    void Clear()
    {
        valid = false;
        ZeroMemory( &ft, sizeof( ft ) );
        ZeroMemory( &st, sizeof( st ) );
        ZeroMemory( &tm, sizeof( tm ) );
        bias = 0;
        day = date = time = offset = "";
        format.Clear();
    }

    /* DayDateTime::DayDateTime()
    - Initialize DayDateTime by Clear() or converting input time.

    If no parameters are passed then *this is Clear()'d and valid == false.
    If an ISO8601 object is passed but not also a FILETIME/SYSTEMTIME then the current time is used.

    [in][opt] 'temp_iso8601_only_used_for_initialization'
    [in][opt] 'utc_ft' / 'utc_st' : Some point in time, UTC only
    [ret] (this->valid = false) : *this Clear()'d
    [ret][success] (this->valid = true) : Converted input UTC time to *this
    */
    DayDateTime() { Clear(); }
    //
    explicit DayDateTime( const ISO8601 &temp_iso8601_only_used_for_initialization )
    {
        temp_iso8601_only_used_for_initialization.GetTimeInfo( *this );
    }
    //
    DayDateTime( const ISO8601 &temp_iso8601_only_used_for_initialization, const FILETIME &utc_ft )
    {
        temp_iso8601_only_used_for_initialization.GetTimeInfo( *this, utc_ft );
    }
    //
    DayDateTime( const ISO8601 &temp_iso8601_only_used_for_initialization, const SYSTEMTIME &utc_st )
    {
        temp_iso8601_only_used_for_initialization.GetTimeInfo( *this, utc_st );
    }

    virtual ~DayDateTime() {}
};



/* class TimeInfo
- Output class for ISO8601::GetTimeInfo(). Receives input UTC and converted local time.

On success all members are valid and 'valid' is true.

This class is inherited from DayDateTime. That base subobject will receive either a local or UTC
time depending on ISO8601::prefer_local_time. After the object has been output you can call
PreferLocalTime() to change the preference and swap the other time into the base subobject. In any
case both local and UTC time are received by the object and accessible by using 'local' and 'utc'.
*/
class TimeInfo : // [out]
    public DayDateTime // 'prefer_local_time' ? local time : UTC time
{
    friend class ISO8601;

public:
    // [false] : object invalid
    // [true] : all members are valid; the object was updated successfully
    bool valid;

    // UTC time
    // This pointer points to an object all cases, even when 'valid' is false
    DayDateTime *const &utc; // = _utc

    // Local time
    // This pointer points to an object all cases, even when 'valid' is false
    DayDateTime *const &local; // = _local

    // [false] : UTC time is in the base subobject and local time is in '_alternate_time_info'
    // [true] : local time is in the base subobject and UTC time is in '_alternate_time_info'
    const bool &prefer_local_time; // = _prefer_local_time

    // ISO 8601 timestamp, always in UTC timezone with milliseconds: 2013-08-11T18:46:00.085Z
    std::string timestamp;

    /* TimeInfo::PreferLocalTime()
    - This function changes whether or UTC or local time is in the base DayDateTime subobject.

    This function can be called an unlimited number of times after a TimeInfo has been written to.

    [in] 'new_pref' (false) : prefer UTC time
    [in] 'new_pref' (true) : prefer local time
    [virtual] : A class that inherits may need to override to update/check members before/after
    [ret] : The local time preference has been updated
    */
    virtual void PreferLocalTime( const bool new_pref )
    {
        if( _prefer_local_time == new_pref )
            return;

        // swap pointers and set '_prefer_local_time'
        InitializeLocalTimePref( new_pref );

        // swap data
        using std::swap;
        swap( *_local, *_utc );
    }

    void Clear( bool also_clear_base_subobjects = true )
    {
        valid = false;
        timestamp = "";
        _utc = this;
        _local = &_alternate_time_info;
        _prefer_local_time = false;
        _alternate_time_info.Clear();

        if( also_clear_base_subobjects )
        {
            DayDateTime::Clear();
        }
    }

    /* TimeInfo::TimeInfo()
    - Initialize TimeInfo by Clear() or converting input time.

    If no parameters are passed then *this is Clear()'d and valid == false.
    If an ISO8601 object is passed but not also a FILETIME/SYSTEMTIME then the current time is used.

    [in][opt] 'temp_iso8601_only_used_for_initialization'
    [in][opt] 'utc_ft' / 'utc_st' : Some point in time, UTC only
    [ret] (this->valid = false) : *this Clear()'d
    [ret][success] (this->valid = true) : Converted input UTC time to *this
    */
    TimeInfo() :
        utc( _utc ), local( _local ), prefer_local_time( _prefer_local_time )
    {
        Clear( false ); // DayDateTime base subobject clears itself in its default ctor
    }
    //
    explicit TimeInfo( const ISO8601 &temp_iso8601_only_used_for_initialization ) :
        utc( _utc ), local( _local ), prefer_local_time( _prefer_local_time )
    {
        temp_iso8601_only_used_for_initialization.GetTimeInfo( *this );
    }
    //
    TimeInfo( const ISO8601 &temp_iso8601_only_used_for_initialization, const FILETIME &utc_ft ) :
        utc( _utc ), local( _local ), prefer_local_time( _prefer_local_time )
    {
        temp_iso8601_only_used_for_initialization.GetTimeInfo( *this, utc_ft );
    }
    //
    TimeInfo( const ISO8601 &temp_iso8601_only_used_for_initialization, const SYSTEMTIME &utc_st ) :
        utc( _utc ), local( _local ), prefer_local_time( _prefer_local_time )
    {
        temp_iso8601_only_used_for_initialization.GetTimeInfo( *this, utc_st );
    }

    ///////////////// add copy operators
    ~TimeInfo() {}

protected:
    /* TimeInfo::InitializeLocalTimePref()
    - Initializes the local time preference.

    Relatives that write to TimeInfo must call this during the initial object output.

    [in] 'v_prefer_local_time' : The local time preference
    [ret] : The local time preference has been initialized
    */
    void InitializeLocalTimePref( const bool v_prefer_local_time )
    {
        _utc = ( v_prefer_local_time ? &_alternate_time_info : this );
        _local = ( v_prefer_local_time ? this : &_alternate_time_info );
        _prefer_local_time = v_prefer_local_time;
    }

private:
    // UTC time
    // This pointer points to an object all cases, even when 'valid' is false
    DayDateTime *_utc;

    // Local time
    // This pointer points to an object all cases, even when 'valid' is false
    DayDateTime *_local;

    // [false] : UTC time is in the base subobject and local time is in '_alternate_time_info'
    // [true] : local time is in the base subobject and UTC time is in '_alternate_time_info'
    bool _prefer_local_time;

    // 'prefer_local_time' ? UTC time : local time
    DayDateTime _alternate_time_info;
};

} // namespace time
} // namespace jay
#endif // _JAY_TIME_ISO8601_HPP
