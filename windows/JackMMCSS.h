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



#ifndef __JackMMCSS__
#define __JackMMCSS__

#include "JackSystemDeps.h"
#include "JackCompilerDeps.h"
#include <windows.h>
#include <map>

namespace Jack
{

typedef enum _AVRT_PRIORITY {
  AVRT_PRIORITY_LOW = -1,
  AVRT_PRIORITY_NORMAL,		/* 0 */
  AVRT_PRIORITY_HIGH,		/* 1 */
  AVRT_PRIORITY_CRITICAL	/* 2 */
} AVRT_PRIORITY, *PAVRT_PRIORITY;

#define BASE_REALTIME_PRIORITY 90

typedef HANDLE (WINAPI *avSetMmThreadCharacteristics)(LPCTSTR, LPDWORD);
typedef BOOL (WINAPI *avRevertMmThreadCharacteristics)(HANDLE);
typedef BOOL (WINAPI *avSetMmThreadPriority)(HANDLE, AVRT_PRIORITY);

/*!
\brief MMCSS services.
*/

class SERVER_EXPORT JackMMCSS
{

    private:

        static JACK_HANDLE fAvrtDll;
        static avSetMmThreadCharacteristics ffMMCSSFun1;
        static avSetMmThreadPriority ffMMCSSFun2;
        static avRevertMmThreadCharacteristics ffMMCSSFun3;
        static std::map<jack_native_thread_t, HANDLE> fHandleTable;

    public:

        JackMMCSS();
        ~JackMMCSS();

        static int MMCSSAcquireRealTime(jack_native_thread_t thread, int priority);
        static int MMCSSDropRealTime(jack_native_thread_t thread);

};

} // end of namespace

#endif

