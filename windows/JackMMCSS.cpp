/*
 Copyright (C) 2004-2008 Grame

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 */

#include "JackMMCSS.h"
#include "JackError.h"
#include <assert.h>
#include <stdio.h>

namespace Jack
{

avSetMmThreadCharacteristics JackMMCSS::ffMMCSSFun1 = NULL;
avSetMmThreadPriority JackMMCSS::ffMMCSSFun2 = NULL;
avRevertMmThreadCharacteristics JackMMCSS::ffMMCSSFun3 = NULL;
JACK_HANDLE JackMMCSS::fAvrtDll;

std::map<jack_native_thread_t, HANDLE> JackMMCSS::fHandleTable;

JackMMCSS::JackMMCSS()
{
    fAvrtDll = LoadJackModule("avrt.dll");

    if (fAvrtDll != NULL) {
        ffMMCSSFun1 = (avSetMmThreadCharacteristics)GetJackProc(fAvrtDll, "AvSetMmThreadCharacteristicsA");
        ffMMCSSFun2 = (avSetMmThreadPriority)GetJackProc(fAvrtDll, "AvSetMmThreadPriority");
        ffMMCSSFun3 = (avRevertMmThreadCharacteristics)GetJackProc(fAvrtDll, "AvRevertMmThreadCharacteristics");
    }
}

JackMMCSS::~JackMMCSS()
{}

int JackMMCSS::MMCSSAcquireRealTime(jack_native_thread_t thread, int priority)
{
    if (fHandleTable.find(thread) != fHandleTable.end()) {
        return 0;
    }

    if (ffMMCSSFun1) {
        DWORD dummy = 0;
        HANDLE task = ffMMCSSFun1("Pro Audio", &dummy);
        if (task == NULL) {
            jack_error("AvSetMmThreadCharacteristics error : %d", GetLastError());
        } else if (ffMMCSSFun2(task, Jack::AVRT_PRIORITY(priority - BASE_REALTIME_PRIORITY))) {
            fHandleTable[thread] = task;
            jack_log("AvSetMmThreadPriority success");
            return 0;
        } else {
            jack_error("AvSetMmThreadPriority error : %d", GetLastError());
        }
     }

     return -1;
}

int JackMMCSS::MMCSSDropRealTime(jack_native_thread_t thread)
{
    if (fHandleTable.find(thread) != fHandleTable.end()) {
        HANDLE task = fHandleTable[thread];
        if (ffMMCSSFun3(task) == 0) {
            jack_error("AvRevertMmThreadCharacteristics error : %d", GetLastError());
        } else {
            jack_log("AvRevertMmThreadCharacteristics success");
        }
        return 0;
    } else {
        return -1;
    }
}

}
