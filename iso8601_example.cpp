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

/** Examples that show how to use ISO8601::GetTimeInfo() to output DayDateTime/TimeInfo.

Compiled using g++ (GCC) 4.7.2. No warnings.
g++ -Wall -o iso8601_example iso8601_example.cpp iso8601.cpp timezone.cpp time.cpp

Compiled using VS2010 cl 16.00.40219.01. Warning for TimeInfo no assignment operator.
cl /W4 /EHsc iso8601_example.cpp iso8601.cpp timezone.cpp time.cpp

Preprocessor defines:
DEBUG_ST : Show SYSTEMTIME structs
*/

#include "iso8601.hpp"

#include <windows.h>
#include <time.h>
#include <assert.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>


using namespace std;
using namespace jay::time;



/* ISO8601 public data members are essentially sticky function parameters that you may modify
between calls. You can set most variables on instantiation and change all of them between calls.

eg: ISO8601 g_iso8601( true, TimeFormat( true ) ) // prefer local time and format USA style
*/
ISO8601 g_iso8601; // The default is to prefer local time

int main()
{
    //
    // DayDateTime receives either UTC or local time
    //
    DayDateTime ddt; // Clear()'d by default

    g_iso8601.GetTimeInfo( ddt ); // current time is written to ddt
    cout << endl <<  "DayDateTime formatted examples:" << endl;
    cout << endl << "Local time, ISO 8601 style:" << endl;
    ddt.Show();

    g_iso8601.prefer_local_time = false;
    g_iso8601.format.usa_style = true;
    g_iso8601.format.day_string_with_abbreviation = true;
    g_iso8601.format.time_string_with_milliseconds = true;
    g_iso8601.GetTimeInfo( ddt ); // current UTC time is written to ddt
    cout << endl << "UTC time with milliseconds, USA style, abbreviated day of the week:" << endl;
    ddt.Show();

    //
    // TimeInfo receives both UTC time and local time, with one of them preferred
    //
    TimeInfo ti; // Clear()'d by default

    g_iso8601.prefer_local_time = true;
    g_iso8601.format.usa_style = true;
    g_iso8601.format.day_string_with_abbreviation = false;
    g_iso8601.format.time_string_with_milliseconds = false;
    g_iso8601.GetTimeInfo( ti ); // current time is written to ti
    cout << endl << endl <<  "TimeInfo formatted examples:" << endl;

    /* since g_iso8601.prefer_local_time was true when GetTimeInfo() wrote to the object this will
    show local time
    */
    cout << endl << "ti.Show();" << endl << "Local time, USA style:" << endl;
    ti.Show();

    // you can also swap the times after the fact.. ie switch which stored time is preferred
    cout << endl << "ti.PreferLocalTime( false );" << endl << "ti.Show()" << endl;
    cout << "UTC time, USA style:" << endl;
    ti.PreferLocalTime( false );
    ti.Show();

    // you can also access the stored local and UTC time directly regardless of what's preferred
    cout << endl << "ti.local->Show();" << endl << "Local time, USA style:" << endl;
    ti.local->Show();
    cout << endl << "ti.utc->Show();" << endl << "UTC time, USA style:" << endl;
    ti.utc->Show();

    cout << endl;
    cout << "ti.timestamp is an ISO 8601 timestamp always UTC time with milliseconds:" << endl;
    cout << ti.timestamp << endl;

    /* For compatibility you can use the tm structs with strftime(), etc.
    Example: Some saved UTC point in time is used to create the TimeInfo object.
    Here a temporary ISO8601 object is also used and the prefer local time option is disabled.
    */
    FILETIME saved_utc_ft = {};
    GetSystemTimeAsFileTime( &saved_utc_ft );
    TimeInfo saved_ti( ISO8601( false ), saved_utc_ft );
    if( !saved_ti.valid ) // the stored FILETIME couldn't be converted to TimeInfo
    {
        assert( false );
    }
    cout << endl << endl << "strftime() formatted example:" << endl;
    char buf[ 256 ] = {};
    strftime( buf, sizeof( buf ), "%Y-%m-%d %H:%M:%SZ", &saved_ti.utc->tm );
    /* The stored UTC time was accessed from utc rather than use what's preferred since UTC timezone
    ("Z") is needed. The local time preference was disabled so in this case accessing the inherited
    from saved_ti.tm is the same thing. However since the local time preference can change and this
    example is written specifically for UTC time it's best to specify saved_ti.utc since that
    always holds the UTC time.
    */
    cout << endl << "Saved UTC time, ISO 8601 style: " << buf << endl;

    return 0;
}
