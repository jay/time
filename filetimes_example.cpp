/*
Copyright (C) 2017 Jay Satiro <raysatiro@yahoo.com>
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

/** Example to show a file's times: creation, last accessed, last modified.

Compiled using g++ (GCC) 4.7.2. No warnings.
g++ -Wall -o GetFileTime filetimes_example.cpp iso8601.cpp time.cpp timezone.cpp

Compiled using VS2010 cl 16.00.40219.01.
One expected warning: C4512 assignment operator could not be generated.
cl /W4 /EHsc /FeGetFileTime filetimes_example.cpp iso8601.cpp time.cpp timezone.cpp
*/

/* Sample of output when iso8601.format.usa_style = true;

Filename: C:\Windows\System32\kernel32.dll

Created (UTC):         Tuesday     3/14/2017   6:56:33 PM    131339913934428327
Created (UTC-04:00):   Tuesday     3/14/2017   2:56:33 PM    131339769934428327

Modified (UTC):        Thursday    2/09/2017   4:14:50 PM    131311304901300000
Modified (UTC-05:00):  Thursday    2/09/2017  11:14:50 AM    131311124901300000

Accessed (UTC):        Tuesday     3/14/2017   6:56:33 PM    131339913934428327
Accessed (UTC-04:00):  Tuesday     3/14/2017   2:56:33 PM    131339769934428327
*/

/* Sample of output when iso8601.format.usa_style = false;

Filename: C:\Windows\System32\kernel32.dll

Created Z:             Tuesday    2017-03-14     18:56:33    131339913934428327
Created -04:00:        Tuesday    2017-03-14     14:56:33    131339769934428327

Modified Z:            Thursday   2017-02-09     16:14:50    131311304901300000
Modified -05:00:       Thursday   2017-02-09     11:14:50    131311124901300000

Accessed Z:            Tuesday    2017-03-14     18:56:33    131339913934428327
Accessed -04:00:       Tuesday    2017-03-14     14:56:33    131339769934428327
*/

#include "iso8601.hpp"
#include "time.hpp"
#include "timezone.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>


using namespace std;
using namespace jay::time;


// Simply FileTimes("filename").Show() or:
// Set filename and then Refresh() to get filetimes; Show() to show filetimes.
// If valid is true, object is valid, filetimes were retrieved for filename.
class FileTimes
{
public:
  bool valid;
  string filename;
  TimeInfo creation_time, last_access_time, last_modified_time;
  void Clear()
  {
    filename.clear();
    creation_time.Clear();
    last_access_time.Clear();
    last_modified_time.Clear();
    valid = false;
  }
  bool Refresh()
  {
    valid = false;

    HANDLE hfile = CreateFileA(filename.c_str(), GENERIC_READ,
                               (FILE_SHARE_READ | FILE_SHARE_WRITE |
                                FILE_SHARE_DELETE),
                               NULL, OPEN_EXISTING, 0, NULL);

    if(hfile == INVALID_HANDLE_VALUE) {
      DWORD gle = GetLastError();
      cerr << "Error: Failed to open file \"" << filename << "\", "
           << "GetLastError: " << gle
           << (gle == ERROR_FILE_NOT_FOUND ? " (File not found)" :
               gle == ERROR_PATH_NOT_FOUND ? " (Path not found)" :
               gle == ERROR_ACCESS_DENIED ? " (Access denied)" : "")
           << "." << endl;
      return false;
    }

    FILETIME ft_creation, ft_last_access, ft_last_modified;
    if(!GetFileTime(hfile, &ft_creation, &ft_last_access, &ft_last_modified)) {
      DWORD gle = GetLastError();
      cerr << "Error: Failed to get times of file \"" << filename << "\", "
           << "GetLastError: " << gle
           << "." << endl;
      CloseHandle(hfile);
      return false;
    }

    CloseHandle(hfile);

    ISO8601 iso8601;
    iso8601.format.usa_style = true;
    iso8601.GetTimeInfo(creation_time, ft_creation);
    iso8601.GetTimeInfo(last_access_time, ft_last_access);
    iso8601.GetTimeInfo(last_modified_time, ft_last_modified);
    valid = creation_time.valid &&
            last_access_time.valid &&
            last_modified_time.valid;
    if(!valid) {
      cerr << "Error: Failed to convert times of file \"" << filename << "\""
           << "." << endl;
    }
    return valid;
  }
  bool Show()
  {
    cout << "Filename: " << filename << endl
         << endl;

    if(!valid) {
      cout << "Filetimes not available, object invalid." << endl
           << endl;
      return false;
    }

    ShowTimeInfo(creation_time);
    cout << endl;

    ShowTimeInfo(last_modified_time);
    cout << endl;

    ShowTimeInfo(last_access_time);
    cout << endl;

    return true;
  }
  FileTimes() {}
  FileTimes(string filename)
  {
    FileTimes::filename = filename;
    Refresh();
  }

private:
  string GetDateStr(DayDateTime &ddt)
  {
    stringstream ss;
    string spcdate;

    // DayDateTime USA date is format 1/1/1970 but for proper column alignment
    // we need day to be zero filled, eg 1/01/1970
    if(ddt.format.usa_style) {
      stringstream ss_usa_date;
      ss_usa_date.fill('0');
      ss_usa_date << ddt.st.wMonth << "/"
                  << setw(2) << ddt.st.wDay << "/"
                  << setw(4) << ddt.st.wYear;
      spcdate = ss_usa_date.str();
    }
    else
      spcdate = ddt.date;

    ss << left
       << setw(9) << ddt.day << "  "
       << right
       << setw(10) << spcdate << "  "
       << setw(11) << ddt.time << "  "
       << setw(20) << (((ULONGLONG)ddt.ft.dwHighDateTime << 32) +
                       ddt.ft.dwLowDateTime);
    return ss.str();
  }
  void ShowDayDateTime(string name, DayDateTime &ddt)
  {
    stringstream ss;
    ss << left
       // this assumes the friendly name of the ddt object is <= 8
       << setw(21) << string(name + " " + ddt.offset + ":") << "  "
       << right
       << GetDateStr(ddt);
    cout << ss.str() << endl;
  }
  void ShowTimeInfo(TimeInfo &ti)
  {
    string name = (&ti == &creation_time ? "Created" :
                   &ti == &last_modified_time ? "Modified" :
                   &ti == &last_access_time ? "Accessed" : "Unknown");

    ShowDayDateTime(name, *ti.utc);

    // if local time is different from UTC then show that as well
    if(ti.local->bias)
      ShowDayDateTime(name, *ti.local);
  }
};

int main(int argc, char *argv[])
{
  if(argc < 2) {
    cerr << "Usage: GetFileTime <filename> ..." << endl;
    exit(1);
  }

  bool allgood = true;

  for(int i = 1; i < argc; ++i) {
    cout << endl;
    if(!FileTimes(argv[i]).Show())
      allgood = false;
  }

  return allgood ? 0 : 1;
}
