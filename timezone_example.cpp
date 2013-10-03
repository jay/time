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

/** Example to show Timezone ID for the current timezone and time.

Compiled using g++ (GCC) 4.7.2. No warnings.
g++ -Wall -o tzid timezone_example.cpp timezone.cpp time.cpp

Compiled using VS2010 cl 16.00.40219.01. No warnings.
cl /W4 /EHsc /Fetzid timezone_example.cpp timezone.cpp time.cpp

Preprocessor defines:
DEBUG_ST : Show SYSTEMTIME structs
COMPARE_TO_WINAPI : Show the timezone ID returned by WinAPI as well as jay::time. If the comparison
is not equal set the program exit code to 1.
*/

#include "timezone.hpp"
#include "time.hpp"

#include <windows.h>
#include <time.h>
#include <assert.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>


#ifdef DEBUG_ST
#undef DEBUG_ST
#define DEBUG_ST(a) cout << endl << #a << ": " << endl; jay::time::ShowSystemTime( a );
#else
#define DEBUG_ST(a)
#endif


using namespace std;
using namespace jay::time;



int main()
{
    SYSTEMTIME local = {}, utc = {};
    DWORD tzi_id = 0;

    GetSystemTime( &utc );
    DEBUG_ST( utc );

    // Convert the UTC time to local time and a timezone ID
    if( !UTCTimeToLocalTime( utc, local, tzi_id ) )
    {
        DWORD gle = GetLastError();
        cout << "UTCTimeToLocalTime() failed. GetLastError(): " << gle << endl;
        return 1;
    }

    DEBUG_ST( local );

    cout << endl;
    ShowTimezoneId( tzi_id );


/* The code below outputs the timezone id retrieved from ::GetDynamicTimeZoneInformation().
I used this for testing purposes when I was writing timezone.cpp.
*/
#ifdef COMPARE_TO_WINAPI
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

    DWORD ( WINAPI *pfnGetDynamicTimeZoneInformation ) ( DYNAMIC_TIME_ZONE_INFORMATION * ) =
        ( DWORD ( WINAPI * ) ( DYNAMIC_TIME_ZONE_INFORMATION * ) )
            GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "GetDynamicTimeZoneInformation" );

    assert( pfnGetDynamicTimeZoneInformation );

    DYNAMIC_TIME_ZONE_INFORMATION dtzi = {};
    DWORD ret1 = pfnGetDynamicTimeZoneInformation( &dtzi );

    ShowTimezoneId( ret1 );

    return ( tzi_id != ret1 );
#endif // COMPARE_TO_WINAPI


    return 0;
}
