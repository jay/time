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

/** Functions for working with timezone time in Windows.

Documentation is in timezone.hpp. Example is in timezone_example.cpp.
*/

#include "timezone.hpp"
#include "time.hpp"

#include <windows.h>
#include <assert.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>


#ifdef DEBUG_ST
#undef DEBUG_ST
#define DEBUG_ST(a) cout << endl << #a << ": " << endl; ShowSystemTime( a );
#else
#define DEBUG_ST(a)
#endif


using namespace std;



namespace {

// http://msdn.microsoft.com/en-us/library/ms724253.aspx
typedef struct _TIME_DYNAMIC_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[ 128 ];
    BOOLEAN DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;

// >= Vista
DWORD ( WINAPI *pfnGetDynamicTimeZoneInformation ) ( ::DYNAMIC_TIME_ZONE_INFORMATION * ) =
    ( DWORD ( WINAPI * ) ( ::DYNAMIC_TIME_ZONE_INFORMATION * ) )
        GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "GetDynamicTimeZoneInformation" );

// >= Vista SP1
BOOL ( WINAPI *pfnGetTimeZoneInformationForYear ) ( USHORT, ::DYNAMIC_TIME_ZONE_INFORMATION *, TIME_ZONE_INFORMATION * ) =
    ( BOOL ( WINAPI * ) ( USHORT, ::DYNAMIC_TIME_ZONE_INFORMATION *, TIME_ZONE_INFORMATION * ) )
        GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "GetTimeZoneInformationForYear" );

} // anonymous namespace



namespace jay {
namespace time {

void ShowTimezoneId( const DWORD id, std::ostream &output /* = std::cout */ )
{
    switch( id )
    {
    case TIME_ZONE_ID_INVALID:
        output << "TIME_ZONE_ID_INVALID" << endl;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        output << "TIME_ZONE_ID_DAYLIGHT" << endl;
        break;
    case TIME_ZONE_ID_STANDARD:
        output << "TIME_ZONE_ID_STANDARD" << endl;
        break;
    case TIME_ZONE_ID_UNKNOWN:
        output << "TIME_ZONE_ID_UNKNOWN" << endl;
        break;
    default:
        output << "(TIME_ZONE_ID not recognized: " << (unsigned)id << ")" << endl;
        break;
    }
}



bool IsRelativeTimezoneTimeValid( const SYSTEMTIME &tzt )
{
    // http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724950.aspx
    return ( tzt.wYear == 0 )
        && ( ( tzt.wMonth >= 1 ) && ( tzt.wMonth <= 12 ) )
        && ( tzt.wDayOfWeek <= 6 )
        && ( ( tzt.wDay >= 1 ) && ( tzt.wDay <= 5 ) )
        && ( tzt.wHour <= 23 )
        && ( tzt.wMinute <= 59 )
        && ( tzt.wSecond <= 59 )
        && ( tzt.wMilliseconds <= 999 );
}


bool IsAbsoluteTimezoneTimeValid( const SYSTEMTIME &tzt )
{
    return IsSystemTimeValid_IgnoreDayOfWeek( tzt );
}


bool IsTimezoneTimeValid( const SYSTEMTIME &tzt )
{
    return IsRelativeTimezoneTimeValid( tzt ) || IsAbsoluteTimezoneTimeValid( tzt );
}



bool AreTimezoneBiasesValid( const TIME_ZONE_INFORMATION &tzi )
{
    /* The maximum difference in minutes between UTC time and local time.
    In practice I'm not seeing anything over 13 so maybe 24 is too much.
    I wrote some of the timezone code under the assumption that no difference is greater than a day;
    therefore this number should not be increased beyond 24 hours without a thorough review.
    */
    const long max_diff = 24 * 60;

    if(
        // Bias overflow
        ( ( tzi.Bias > 0 ) && ( tzi.Bias > max_diff ) )
        || ( ( tzi.Bias < 0 ) && ( tzi.Bias < -max_diff ) )
        // StandardBias overflow
        || ( ( tzi.StandardBias > 0 ) && ( tzi.Bias > ( max_diff - tzi.StandardBias ) ) )
        || ( ( tzi.StandardBias < 0 ) && ( tzi.Bias < ( -max_diff - tzi.StandardBias ) ) )
        // DaylightBias overflow
        || ( ( tzi.DaylightBias > 0 ) && ( tzi.Bias > ( max_diff - tzi.DaylightBias ) ) )
        || ( ( tzi.DaylightBias < 0 ) && ( tzi.Bias < ( -max_diff - tzi.DaylightBias ) ) )
    )
        return false;

    return true;
}



bool IsTimezoneInfoValid(
    const TIME_ZONE_INFORMATION &tzi,
    const bool allow_ignored_dates /* = false */
)
{
    return ( AreTimezoneBiasesValid( tzi )
        && ( IsTimezoneTimeValid( tzi.StandardDate )
            || ( allow_ignored_dates && IsTimezoneTimeIgnored( tzi.StandardDate ) )
        )
        && ( IsTimezoneTimeValid( tzi.DaylightDate )
            || ( allow_ignored_dates && IsTimezoneTimeIgnored( tzi.DaylightDate ) )
        )
        && wmemchr( tzi.StandardName, 0, ( sizeof( tzi.StandardName ) / sizeof( wchar_t ) ) )
        && wmemchr( tzi.DaylightName, 0, ( sizeof( tzi.DaylightName ) / sizeof( wchar_t ) ) )
    );
}



bool LocalTimeToRelativeTimezoneTime(
    const SYSTEMTIME &local,
    const bool make_final_occurrence_if_last_occurrence,
    SYSTEMTIME &tzt
)
{
    if( !IsSystemTimeValid_IgnoreDayOfWeek( local ) )
        return false;

    // 'local' is absolute and must be converted to relative
    // http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)

    //
    // local.wDay is currently set to the day of the month. [1, 31]
    // tz.wDay must be set to the "occurrence of the day of the week within the month." [1, 5]
    //

    // "the occurrence of the day of the week within the month". [1, 5]
    unsigned occurrence = ( ( local.wDay - 1 ) / 7 ) + 1;

    /* Any day of week has a maximum of [4, 5] occurrences in any month. MSDN says an occurrence
    number of 5 represents "the final occurrence during the month if that day of the week does not
    occur 5 times." Below checks to make sure 'occurrence' is set to 5 (final occurrence) if it's
    the last occurrence during the month.
    */
    if( ( occurrence == 4 )
        && !IsDateValid( local.wDay + 7, local.wMonth, local.wYear )
        && make_final_occurrence_if_last_occurrence
    )
        occurrence = 5;

    tzt = local;
    tzt.wDay = (WORD)occurrence; // [1, 5]
    tzt.wDayOfWeek = GetDayOfWeek( local.wDay, local.wMonth, local.wYear ); // [0, 6]
    tzt.wYear = 0;
    return true;
}



bool TimezoneTimeToLocalTime(
    const SYSTEMTIME &tzt,
    const unsigned tzt_year,
    SYSTEMTIME &local
)
{
    if( !IsRelativeTimezoneTimeValid( tzt ) )
    {
        // if tzt is already absolute no conversion is needed
        if( IsAbsoluteTimezoneTimeValid( tzt ) )
        {
            local = tzt;
            local.wDayOfWeek = GetDayOfWeek( local.wDay, local.wMonth, local.wYear ); // [0, 6]
            return true;
        }

        return false;
    }


    if( !IsYearValid( tzt_year ) )
        return false;

    // 'tzt' is relative and must be converted to absolute
    // http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)

    //
    // tzt.wDay is currently set to the "occurrence of the day of the week within the month." [1, 5]
    // local.wDay must be set to the day of the month. [1, 31]
    //

    // the first day of the week. [0=Sun, 6=Sat]
    const unsigned first_day_of_week = GetDayOfWeek( 1, tzt.wMonth, tzt_year );

    // days after the first "occurrence of the day of the week within the month." [0, 28]
    const unsigned days_after_first_occurrence = ( tzt.wDay - 1 ) * 7;

    // the day of the month. [1, 31]
    unsigned day = 0;

    // 'day' calculated based on the the occurrence of the day of week. [1, 35]
    if( first_day_of_week <= tzt.wDayOfWeek )
        day = ( tzt.wDayOfWeek - first_day_of_week ) + 1 + days_after_first_occurrence;
    else
        day = ( 6 - first_day_of_week ) + 1 + tzt.wDayOfWeek + 1 + days_after_first_occurrence;

    /* Any day of week has a maximum of [4, 5] occurrences in any month. MSDN says an occurrence
    number of 5 represents "the final occurrence during the month if that day of the week does not
    occur 5 times." Below checks to make sure 'day' is not out of range and corrects it if it is.
    */
    if( !IsDateValid( day, tzt.wMonth, tzt_year ) )
        day = day - 7;

    local = tzt;
    local.wDay = (WORD)day; // [1, 31]
    local.wYear = (WORD)tzt_year; // [1601, 30827]
    return true;
}



int CompareLocalTimeToTimezoneTime( const SYSTEMTIME &local, const SYSTEMTIME &tzt )
{
    SYSTEMTIME local2 = {};
    TimezoneTimeToLocalTime( tzt, local.wYear, local2 );
    return CompareSystemTimes( local, local2 );
}



bool GetTimezoneForYear( TIME_ZONE_INFORMATION &tzi, const unsigned year )
{
    if( !IsYearValid( year ) )
    {
        SetLastError( ERROR_INVALID_TIME );
        return false;
    }

    if( pfnGetDynamicTimeZoneInformation )
    {
        DWORD dtzi_id = TIME_ZONE_ID_INVALID;
        ::DYNAMIC_TIME_ZONE_INFORMATION dtzi = {};

        dtzi_id = pfnGetDynamicTimeZoneInformation( &dtzi );
        if( dtzi_id == TIME_ZONE_ID_INVALID )
            return false;

        if( pfnGetTimeZoneInformationForYear )
        {
            /* If dtzi.DynamicDaylightTimeDisabled is nonzero then ::GetTimeZoneInformationForYear()
            gets the current year and not the requested year. Even if NULL is passed instead of dtzi
            the function makes its own call and if auto-DST is disabled it will end up returning the
            current year so nothing changes. However if dtzi.DynamicDaylightTimeDisabled is set zero
            before the call then it will output the timezone information for the requested year, if
            successful.
            */

            // requested year's Bias is needed so temporarily zero out DynamicDaylightTimeDisabled
            BOOLEAN save = dtzi.DynamicDaylightTimeDisabled;
            dtzi.DynamicDaylightTimeDisabled = 0;

            if( !pfnGetTimeZoneInformationForYear( (WORD)year, &dtzi, &tzi ) )
                return false;

            // restore actual DST setting
            dtzi.DynamicDaylightTimeDisabled = save;

            /* If auto-DST is disabled then emulate Windows' behavior. Refer to the comment block at
            the end of GetLocalTimeForTimezone()'s definition for more information on that behavior.
            */
            if( dtzi.DynamicDaylightTimeDisabled )
            {
                tzi.StandardBias = 0;
                tzi.DaylightBias = 0;
                ZeroMemory( &tzi.StandardDate, sizeof( tzi.StandardDate ) );
                ZeroMemory( &tzi.DaylightDate, sizeof( tzi.DaylightDate ) );
                CopyMemory( tzi.DaylightName, tzi.StandardName, sizeof( tzi.DaylightName ) );
            }
        }
        else
        {
            CopyMemory( &tzi, &dtzi, sizeof( tzi ) );
        }
    }
    else
    {
        DWORD tzi_id = GetTimeZoneInformation( &tzi );

        if( tzi_id == TIME_ZONE_ID_INVALID )
            return false;
    }

    return true;
}



DWORD GetLocalTimeForTimezone(
    SYSTEMTIME &local_time,
    const TIME_ZONE_INFORMATION &tzi,
    const SYSTEMTIME &utc_st,
    unsigned tzi_year /* = 0 */,
    const bool strict /* = false */
)
{
// function-like macro to set and check 'local_time'
#define COPY_TO_local_time(newtime)   \
local_time = newtime; \
\
if( strict && ( local_time.wYear != tzi_year ) ) \
{ \
    SetLastError( ERROR_NOT_SUPPORTED ); \
    return TIME_ZONE_ID_INVALID; \
}

    if( !tzi_year )
        tzi_year = utc_st.wYear;

    if( !IsYearValid( tzi_year ) )
    {
        SetLastError( ERROR_INVALID_TIME );
        return false;
    }

    DEBUG_ST( utc_st );
    DEBUG_ST( tzi.StandardDate );
    DEBUG_ST( tzi.DaylightDate );

    SYSTEMTIME local_time_if_unknown = utc_st;
    SYSTEMTIME local_time_if_standard = utc_st;
    SYSTEMTIME local_time_if_daylight = utc_st;

    if( !IsTimezoneInfoValid( tzi, true )
        || !SystemTimeSubtractMinutes( local_time_if_unknown, tzi.Bias )
        || !SystemTimeSubtractMinutes( local_time_if_standard, tzi.Bias + tzi.StandardBias )
        || !SystemTimeSubtractMinutes( local_time_if_daylight, tzi.Bias + tzi.DaylightBias )
    )
    {
        SetLastError( ERROR_INVALID_TIME );
        return TIME_ZONE_ID_INVALID;
    }

    /* When strict mode is enabled the year in 'local_time' is compared after it's set to 'tzi_year'
    and the code will error out if they don't match. Therefore if strict mode is enabled and none of
    the candidates for 'local_time' have a year that matches 'tzi_year' then there's no point in
    continuing.
    */
    if( strict
        && ( local_time_if_unknown.wYear != tzi_year )
        && ( local_time_if_standard.wYear != tzi_year )
        && ( local_time_if_daylight.wYear != tzi_year )
    )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return TIME_ZONE_ID_INVALID;
    }

    SYSTEMTIME local_standard_start = {}, local_daylight_start = {};
    if( !TimezoneTimeToLocalTime( tzi.StandardDate, tzi_year, local_standard_start )
        || !TimezoneTimeToLocalTime( tzi.DaylightDate, tzi_year, local_daylight_start )
    )
    {
        COPY_TO_local_time( local_time_if_unknown );
        return TIME_ZONE_ID_UNKNOWN;
    }


#ifdef DEBUG_ST
    DEBUG_ST( local_standard_start );
    DEBUG_ST( local_daylight_start );

    SYSTEMTIME utc_standard_start = local_standard_start;
    SYSTEMTIME utc_daylight_start = local_daylight_start;
    if( !SystemTimeAddMinutes( utc_standard_start, tzi.Bias + tzi.DaylightBias )
        || !SystemTimeAddMinutes( utc_daylight_start, tzi.Bias + tzi.StandardBias )
    )
    {
        assert( false );
    }

    DEBUG_ST( utc_standard_start );
    DEBUG_ST( utc_daylight_start );

    DEBUG_ST( local_time_if_standard );
    DEBUG_ST( local_time_if_daylight );
#endif

    const bool is_standard_time_before_daylight_start =
        ( CompareSystemTimes( local_time_if_standard, local_daylight_start ) < 0 );

    const bool is_daylight_time_before_standard_start =
        ( CompareSystemTimes( local_time_if_daylight, local_standard_start ) < 0 );

    const int cmp_standard_start_to_daylight_start =
        CompareSystemTimes( local_standard_start, local_daylight_start );

    if( cmp_standard_start_to_daylight_start < 0 )
    {
        if( !is_daylight_time_before_standard_start && is_standard_time_before_daylight_start )
        {
            COPY_TO_local_time( local_time_if_standard );
            return TIME_ZONE_ID_STANDARD;
        }
        else
        {
            COPY_TO_local_time( local_time_if_daylight );
            return TIME_ZONE_ID_DAYLIGHT;
        }
    }
    else if( cmp_standard_start_to_daylight_start > 0 )
    {
        if( !is_standard_time_before_daylight_start && is_daylight_time_before_standard_start )
        {
            COPY_TO_local_time( local_time_if_daylight );
            return TIME_ZONE_ID_DAYLIGHT;
        }
        else
        {
            COPY_TO_local_time( local_time_if_standard );
            return TIME_ZONE_ID_STANDARD;
        }
    }

    /* Unlike ::GetTimeZoneInformation() this function will not return TIME_ZONE_ID_DAYLIGHT unless
    'local_time' is in the range of Daylight Saving Time (DST) and:
    - DST is observed ( ( standard transition time != daylight transition time ) || DaylightBias )
    - Windows' auto-DST is not disabled
    In other words it must actually be Daylight Saving Time.

    ######
    Here is background on ::GetTimeZoneInformation() behavior for the Windows operating system:

    If a timezone's transition to StandardDate is the same as the transition to DaylightDate
    (ie daylight_start == standard_start) and it's a valid timezone time then "DST is not observed
    by the time zone" according to MS. However in such a case ::GetTimeZoneInformation() will
    always return TIME_ZONE_ID_DAYLIGHT and the OS always calculates ActiveTimeBias as
    DaylightBias + Bias.

    ---
    And the following applies if Windows' auto-DST is disabled:

    In the case of Windows XP and maybe other early versions, the OS copies StandardDate to
    DaylightDate, copies StandardName to DaylightName and zeroes out StandardBias and DaylightBias.
    According to the rules above, and in practice, this means that ::GetTimeZoneInformation() always
    returns TIME_ZONE_ID_DAYLIGHT and ActiveTimeBias == ( DaylightBias + Bias ).

    In the case of Windows Vista and all later versions (tested to Windows 8) the OS copies
    StandardName to DaylightName and zeroes out StandardDate, DaylightDate, StandardBias and
    DaylightBias. Even though StandardDate == DaylightDate it's not a valid timezone time when it's
    zeroed out (it looks like the check is just for one member, a valid wMonth). The OS ignores it
    and returns TIME_ZONE_ID_UNKNOWN always and StandardBias/DaylightBias is ignored.
    ---
    ######

    That OS behavior is contrary to what seems logical to me, which would be for it to leave
    StandardBias alone and return TIME_ZONE_ID_STANDARD so that
    ActiveTimeBias == ( StandardBias + Bias ) which could be subtracted from UTC to get local
    standard time. Of the ~100 timezones listed in Windows 8 not a single one uses StandardBias
    though, so maybe this is moot.

    This function is very similar in return to ::GetTimeZoneInformation() but will not return
    TIME_ZONE_ID_DAYLIGHT unless the requirements at the beginning of this comment block have been
    met. This means that below instead of returning TIME_ZONE_ID_DAYLIGHT always, check for the
    existence of DaylightBias first. This change squares up the difference between operating systems
    as well.
    */

    //
    // local_standard_start == local_daylight_start
    //

    if( tzi.DaylightBias ) // DST is year round
    {
        COPY_TO_local_time( local_time_if_daylight );
        return TIME_ZONE_ID_DAYLIGHT;
    }

    // DST isn't observed or auto-DST is disabled
    COPY_TO_local_time( local_time_if_unknown );
    return TIME_ZONE_ID_UNKNOWN;
}


DWORD GetLocalTimeForTimezone( SYSTEMTIME &local_time, const TIME_ZONE_INFORMATION &tzi )
{
    SYSTEMTIME now = {};
    GetSystemTime( &now );
    return GetLocalTimeForTimezone( local_time, tzi, now );
}



bool UTCTimeToLocalTime(
    const SYSTEMTIME &utc_st,
    SYSTEMTIME &local_time,
    DWORD &tzi_id,
    TIME_ZONE_INFORMATION &tzi
)
{
    if( !IsSystemTimeValid( utc_st ) )
    {
        SetLastError( ERROR_INVALID_TIME );
        return false;
    }

    const unsigned current_year = utc_st.wYear;
    const unsigned previous_year = current_year - 1;
    const unsigned next_year = current_year + 1;

    SetLastError( 0 );

    if( ( utc_st.wMonth == 1 ) && ( utc_st.wDay == 1 ) )
    { // edge case
        if( GetTimezoneForYear( tzi, previous_year ) )
        {
            tzi_id = GetLocalTimeForTimezone( local_time, tzi, utc_st, previous_year, true );
            if( tzi_id != TIME_ZONE_ID_INVALID )
                return true;
        }

        if( GetTimezoneForYear( tzi, current_year ) )
        {
            tzi_id = GetLocalTimeForTimezone( local_time, tzi, utc_st, current_year, false );
            if( tzi_id != TIME_ZONE_ID_INVALID )
                return true;
        }
    }
    else
    {
        if( GetTimezoneForYear( tzi, current_year ) )
        {
            tzi_id = GetLocalTimeForTimezone( local_time, tzi, utc_st, current_year, true );
            if( tzi_id != TIME_ZONE_ID_INVALID )
                return true;
        }

        if( ( utc_st.wMonth == 12 ) && ( utc_st.wDay == 31 ) )
        { // edge case
            if( GetTimezoneForYear( tzi, next_year ) )
            {
                tzi_id = GetLocalTimeForTimezone( local_time, tzi, utc_st, next_year, false );
                if( tzi_id != TIME_ZONE_ID_INVALID )
                    return true;
            }
        }
    }

    if( !GetLastError() )
        SetLastError( ERROR_INVALID_TIME );

    return false;
}


bool UTCTimeToLocalTime( const SYSTEMTIME &utc_st, SYSTEMTIME &local_time, DWORD &tzi_id )
{
    TIME_ZONE_INFORMATION tzi = {};
    return UTCTimeToLocalTime( utc_st, local_time, tzi_id, tzi );
}


bool UTCTimeToLocalTime( const SYSTEMTIME &utc_st, SYSTEMTIME &local_time )
{
    DWORD tzi_id = 0;
    TIME_ZONE_INFORMATION tzi = {};
    return UTCTimeToLocalTime( utc_st, local_time, tzi_id, tzi );
}

} // namespace time
} // namespace jay
