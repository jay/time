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

Functions for working with timezone time in Windows.

When a function returns [failure] all output parameters are invalid unless otherwise specified.
When a function returns [success] all output parameters are valid unless otherwise specified.


The code in timezone.cpp came out of a task to determine whether or not a given UTC time if
converted to local time is in Standard Time or Daylight Saving Time (DST) in the current timezone
for the year that local time is in.

Local time calculations may be incorrect if:

- Local time is in a past/future year and that year's timezone info is not available, not yet
determined, or the OS is < Vista SP1 which doesn't have the WinAPI to retrieve past/future years.

To convert a UTC time to local time use UTCTimeToLocalTime(). It is similar to
::SystemTimeToTzSpecificLocalTime() but it avoids most incorrect calculations of local time, except
those noted above.  For more on that refer to the comment blocks above the declarations for
GetTimezoneForYear() and UTCTimeToLocalTime().
*/

#ifndef _JAY_TIME_TIMEZONE_HPP
#define _JAY_TIME_TIMEZONE_HPP

#include <windows.h>

#include <iostream>



namespace jay {
namespace time {

/* ShowTimezoneId()
- Output the name of a timezone ID.

Recognized:
TIME_ZONE_ID_INVALID
TIME_ZONE_ID_UNKNOWN
TIME_ZONE_ID_STANDARD
TIME_ZONE_ID_DAYLIGHT

An unrecognized ID (eg 9) will output:
(TIME_ZONE_ID not recognized: 9)

[in] 'id' : Timezone ID received from a WinAPI function or one of my functions
[out][opt] 'output' : std::ostream to use for output
[ret][failure] : A message is output with the timezone ID indicating that it was not recognized
[ret][success] : The name of the timezone ID is output
*/
void ShowTimezoneId( const DWORD id, std::ostream &output = std::cout );


/* IsRelativeTimezoneTimeValid()
* IsAbsoluteTimezoneTimeValid()
* IsTimezoneTimeValid()
- Check if a timezone time is valid.

It is important to grasp the timezone time format before using this function.
http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)

Windows may allow and ignore invalid timezones. Refer to IsTimezoneTimeIgnored(), below.

[in] 'tzt' : Timezone time (StandardDate/DaylightDate)
[ret][failure] (false) : Timezone time is invalid
[ret][success] (true) : Timezone time is valid
*/
bool IsRelativeTimezoneTimeValid( const SYSTEMTIME &tzt );
bool IsAbsoluteTimezoneTimeValid( const SYSTEMTIME &tzt );
bool IsTimezoneTimeValid( const SYSTEMTIME &tzt );


/* IsTimezoneTimeIgnored()
- Check if a timezone time is ignored.

It is important to grasp the timezone time format before using this function.
http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)

Windows allows an invalid timezone time with wMonth == 0 to be used although it will ignore it:
"If the time zone does not support daylight saving time or if the caller needs to disable daylight
saving time, the wMonth member in the SYSTEMTIME structure must be zero."

[in] 'tzt' : Timezone time (StandardDate/DaylightDate)
[ret][failure] (false) : Timezone time is not ignored
[ret][success] (true) : Timezone time is ignored
*/
inline bool IsTimezoneTimeIgnored( const SYSTEMTIME &tzt )
{
    // I think it's possible there are other allowable cases that aren't documented.
    return !tzt.wMonth;
}


/* AreTimezoneBiasesValid()
- Check if Bias, StandardBias and DaylightBias are within range and calculable.

Bias and StandardBias/DaylightBias are often combined to get the local time bias from UTC time. This
function checks to make sure that when combined any possible bias combination is within [-24, 24]
hours and does not overflow.

[in] 'tzi' : Timezone information
[ret][failure] (false) : One or more biases are out of range or incalculable
[ret][success] (true) : The biases are within range and calculable
*/
bool AreTimezoneBiasesValid( const TIME_ZONE_INFORMATION &tzi );


/* IsTimezoneInfoValid()
- Check if a TIME_ZONE_INFORMATION is valid.

All members are checked: Biases, Dates (Timezone times) and Names.

If 'allow_ignored_dates' is set true then invalid timezone times which are permitted but ignored by
Windows are allowed. Refer to the comment block above the IsTimezoneTimeIgnored() declaration.

[in] 'tzi' : Timezone information
[in][opt] 'allow_ignored_dates' : Invalid StanardDate/DaylightDate are considered valid if ignored
[ret][failure] (false) : Timezone information is invalid
[ret][success] (true) : Timezone information is valid
*/
bool IsTimezoneInfoValid(
    const TIME_ZONE_INFORMATION &tzi,
    const bool allow_ignored_dates = false
);


/* LocalTimeToRelativeTimezoneTime()
- Convert a local time to a relative timezone time (StandardDate/DaylightDate).

It is important to grasp the timezone time format before using this function.
http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)

[in] 'local' : Local time. You may bias this to DST depending on the output. wDayOfWeek is ignored.
[in] 'make_final_occurrence_if_last_occurrence' : If the occurrence of the day of the week is the
last week of the month then set this true for it to always be the last week of the month.
[out] 'tzt' : Timezone time (StandardDate/DaylightDate)
[ret][failure] (false) : Conversion failed
[ret][success] (true) : Conversion successful
*/
bool LocalTimeToRelativeTimezoneTime(
    const SYSTEMTIME &local,
    const bool make_final_occurrence_if_last_occurrence,
    SYSTEMTIME &tzt
);


/* TimezoneTimeToLocalTime()
- Convert a timezone time (StandardDate/DaylightDate) to a local time.

It is important to grasp the timezone time format before using this function.
http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)

In the case that 'tzt' is valid and absolute:
'tzt' is copied to 'local',
'local.wDayOfWeek' is calculated ('tzt.wDayOfWeek' is ignored in this case),
and the return is true (success).

Do not set the 'tzt.wYear' data member to anything other than 0 for a relative timezone time because
that would cause it to be identified incorrectly as an absolute timezone time. To convert a relative
timezone time to local time set the 'tzt_year' parameter instead.

[in] 'tzt' : Timezone time (StandardDate/DaylightDate)
[in] 'tzt_year' : The year for the timezone time. Ignored if 'tzt' is absolute (tzt.wYear != 0).
[out] 'local' : Local time. It may be biased to DST depending on the input.
[ret][failure] (false) : Conversion failed
[ret][success] (true) : Conversion successful
*/
bool TimezoneTimeToLocalTime(
    const SYSTEMTIME &tzt,
    const unsigned tzt_year,
    SYSTEMTIME &local
);


/* CompareLocalTimeToTimezoneTime()
- Compare a local time to a timezone time (StandardDate/DaylightDate).

It is important to grasp the timezone time format before using this function.
http://msdn.microsoft.com/en-us/library/ms724253.aspx (refer to StandardDate & DaylightDate)

Timezone times are essentially local times but are expressed in a special format. You may need to
adjust 'local' for DST depending on which timezone time you are comparing to and what the comparison
is for.

[in] 'local' : Local time. It should be biased on standard or daylight time.
[in] 'tzt' : Timezone time (StandardDate/DaylightDate)
[ret] (-1) : local < tzt
[ret] (0) : local == tzt
[ret] (1) : local > tzt
*/
int CompareLocalTimeToTimezoneTime( const SYSTEMTIME &local, const SYSTEMTIME &tzt );


/* GetTimezoneForYear()
- Get the timezone information for the closest available and appropriate year.

This is a wrapper for the WinAPI get TimeZone calls. 'tzi' will receive timezone information from
::GetTimeZoneInformationForYear(), ::GetDynamicTimeZoneInformation() or ::GetTimeZoneInformation().

DST transition times in a timezone can vary from year to year. Microsoft calls this "dynamic"
timezone information. Windows versions < Vista SP1 may receive dynamic timezone information updates
but likely will not have the supporting ::GetTimeZoneInformationForYear() to access that
information.

Which WinAPI function's timezone information is received by 'tzi' depends on which is OS supported.
This is the order:
::GetTimeZoneInformationForYear()
::GetDynamicTimeZoneInformation()
::GetTimeZoneInformation()

If the conditions for ::GetTimeZoneInformationForYear() are not met or it's unsupported by the OS:

- The current year's timezone info is output regardless of the year requested.

If the conditions for ::GetTimeZoneInformationForYear() are met it's still possible the year
requested might not be the year returned. It makes its own determinations:

- If timezone info for a previous or future year is requested and is unavailable then the info for
closest appropriate year --which is determined by ::GetTimeZoneInformationForYear()-- is output.

Returning the closest appropriate year does not necessarily mean the information returned is
incorrect. For example in 2013 Windows only has two entries for United States Eastern: 2006 & 2007.
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time Zones\Eastern Standard Time
And the two ranges used by ::GetTimeZoneInformationForYear() would be:
[earliest year, 2006] [2007, current year]

So if you are in the year 2013 --and your Windows updates are up to date-- and you request 2013
you'll get the 2007 information, which also applies in 2013. The tricky thing is if you request a
future year because the information for that year could change sometime in the interim. For example
in 2013 you request 2016 but say in 2015 they decide to change the daylight saving time transition
dates starting in 2016. There's no way of knowing that until it actually happens.

You also may receive incorrect results if you request a previous year. Daylight Saving Time
information may not be accurate depending on the location and how far away the year. For example in
the United States, Michigan prior to 1972 (or a lot of places prior to 67). If your timezone is set
to Central Standard Time and you request 1972 you will receive the information for 2006 which may or
may not be correct depending on your region.
For more on this research your country and region. For the United States refer to:
http://en.wikipedia.org/wiki/Daylight_saving_time_in_the_United_States

---
If Windows' auto-DST setting ('Automatically adjust clock for Daylight Saving Time') is disabled:

- The timezone info 'tzi' does not contain the daylight saving bias information and possibly other
daylight saving information.

- In versions of Windows >= Vista SP1 the Bias for the requested year is still returned assuming
::GetTimeZoneInformationForYear() was successful. The other timezone information (StandardBias,
DaylightBias, StandardDate and DaylightDate) is zeroed out to emulate the way >= Vista zeroes out
that information.
---

Timezone times are local times so this function needs the year based on local time not UTC time. If
you have a UTC time consider calling UTCTimeToLocalTime() instead, which converts UTC time to local
time and can output the timezone information for that local time as well.

######
::GetLastError() codes set by this function:

ERROR_INVALID_TIME : 'year' is invalid.

If a failure occurs in a WinAPI function the error code will likely be different from those above.
######

[out] 'tzi' : Timezone information for the closest available and appropriate year, based on 'year'
[in] 'year' : The year requested, expressed as a local time value
[ret][failure] (false) : Failed to retrieve timezone information. An error code was set.
[ret][success] (true) : Timezone information was output
*/
bool GetTimezoneForYear( TIME_ZONE_INFORMATION &tzi, const unsigned year );


/* GetLocalTimeForTimezone()
- Get the local time based on a timezone.

######
It is not recommended to call this function to convert UTC time to local time because sometimes that
cannot be done accurately in a single call. This function has the same limitations as
::SystemTimeToTzSpecificLocalTime(). The local time calculation may be incorrect if:

- The time zone uses a different UTC offset for the old and new years.
- The UTC time to be converted and the calculated local time are in different years.

Practically speaking if the different years have any different timezone information the local time
could be incorrect. Call UTCTimeToLocalTime() instead because it handles these issues correctly.
######

Unlike ::GetTimeZoneInformation() this function will not return TIME_ZONE_ID_DAYLIGHT unless
'local_time' is in the range of Daylight Saving Time (DST) and:
- DST is observed ( ( standard transition time != daylight transition time ) || DaylightBias )
- Windows' auto-DST is not disabled
In other words it must actually be Daylight Saving Time.

In those cases where TIME_ZONE_ID_DAYLIGHT would be returned by ::GetTimeZoneInformation() but not
by this function it returns TIME_ZONE_ID_UNKNOWN. For more information on the difference in behavior
see the comment block at the end of this function's definition.

If TIME_ZONE_ID_DAYLIGHT is returned that does not necessarily mean the timezone DaylightBias != 0.

A valid TIME_ZONE_INFORMATION ('tzi') input is all zeroes and this could lead to a bug in your code
if you initialize a TIME_ZONE_INFORMATION struct to its default value (all zeroes) but neglect to
call a function that writes out your actual timezone information to that struct before passing it to
this function. Also the timezone information passed as input has to be correct for the converted
local time, which is not necessarily the same as the UTC time (see the warning at beginning of this
comment block). Consider calling UTCTimeToLocalTime() instead of this function. It outputs the
correct timezone information for the calculated local time and @tzi_id receives the TIME_ZONE_ID.

######
::GetLastError() codes set by this function:

ERROR_INVALID_TIME : A time conversion failed. 'tzi' and/or 'utc_st' is likely invalid.
ERROR_NOT_SUPPORTED : The calculated local time is in a different year and strict mode is enabled.

If a failure occurs in a WinAPI function the error code will likely be different from those above.
######

[out] 'local_time' : The calculated local time based on the UTC time and timezone information
[in] 'tzi' : Timezone information for 'local_time'
[in][opt] 'utc_st' : UTC time
[in][opt] 'tzi_year' : The year of 'tzi'
[in][opt] 'strict' : Set true to return TIME_ZONE_ID_INVALID w/ error code ERROR_NOT_SUPPORTED if
the calculated local time is in a year different from 'tzi_year'
[ret][failure] (TIME_ZONE_ID_INVALID) : Conversion failed. An error code was set.
[ret][success] (TIME_ZONE_ID_DAYLIGHT) : The local time is in Daylight Time
[ret][success] (TIME_ZONE_ID_STANDARD) : The local time is in Standard Time
[ret][success] (TIME_ZONE_ID_UNKNOWN) : Transition unknown, DST not observed or auto-DST disabled
*/
DWORD GetLocalTimeForTimezone(
    SYSTEMTIME &local_time,
    const TIME_ZONE_INFORMATION &tzi,
    const SYSTEMTIME &utc_st, // no param: current UTC system time
    unsigned tzi_year = 0, // no param or default: utc_st.wYear
    const bool strict = false
);
DWORD GetLocalTimeForTimezone( SYSTEMTIME &local_time, const TIME_ZONE_INFORMATION &tzi );


/* UTCTimeToLocalTime()
- Get the local time, timezone and timezone ID for a UTC time.

This function properly handles edge case times, specifically UTC dates that fall on XXXX-01-01 or
XXXX-12-31. When those dates occur it is possible that the calculated local time may be in the year
before or after, respectively, the year of the UTC time.

The timezone information output is what was used to calculate the local time. It is the timezone
information for the local time's closest available and appropriate year for which a valid
calculation can be made.

Local time calculations may be incorrect if:

- Local time is in a past/future year and that year's timezone info is not available, not yet
determined, or the OS is < Vista SP1 which doesn't have the WinAPI to retrieve past/future years.

It is possible for the local time to be calculated incorrectly if the supplied UTC date is too far
out of range of available years. How far is an issue defined by region and country. It is also
possible for the local time to be calculated incorrectly if the OS doesn't support
::GetTimeZoneInformationForYear(). For more information refer to the comment block above the
GetTimezoneForYear() declaration.

If Windows' auto-DST setting ('Automatically adjust clock for Daylight Saving Time') is disabled
then 'local_time' is not adjusted for daylight saving time and the timezone info 'tzi' does not
contain the daylight saving bias information and possibly other daylight saving information.

The error codes and TIME_ZONE_ID codes used are the same as in GetLocalTimeForTimezone().

[in] 'utc_st' : UTC time
[out] 'local_time' : Local time
[out][opt] 'tzi_id' : A valid TIME_ZONE_ID for 'local_time'
[out][opt] 'tzi' : Timezone information used to convert UTC time to local time
[ret][failure] (false) : Conversion failed. An error code was set.
[ret][success] (true) : Conversion successful
*/
bool UTCTimeToLocalTime(
    const SYSTEMTIME &utc_st,
    SYSTEMTIME &local_time,
    DWORD &tzi_id,
    TIME_ZONE_INFORMATION &tzi
);
bool UTCTimeToLocalTime( const SYSTEMTIME &utc_st, SYSTEMTIME &local_time, DWORD &tzi_id );
bool UTCTimeToLocalTime( const SYSTEMTIME &utc_st, SYSTEMTIME &local_time );

} // namespace time
} // namespace jay
#endif // _JAY_TIME_TIMEZONE_HPP
