/*
Copyright (C) 2001 Paul Davis 
Copyright (C) 2004-2006 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/*
	Copyright: 	© Copyright 2002 Apple Computer, Inc. All rights reserved.
 
	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
	("Apple") in consideration of your agreement to the following terms, and your
	use, installation, modification or redistribution of this Apple software
	constitutes acceptance of these terms.  If you do not agree with these terms,
	please do not use, install, modify or redistribute this Apple software.
 
	In consideration of your agreement to abide by the following terms, and subject
	to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
	copyrights in this original Apple software (the "Apple Software"), to use,
	reproduce, modify and redistribute the Apple Software, with or without
	modifications, in source and/or binary forms; provided that if you redistribute
	the Apple Software in its entirety and without modifications, you must retain
	this notice and the following text and disclaimers in all such redistributions of
	the Apple Software.  Neither the name, trademarks, service marks or logos of
	Apple Computer, Inc. may be used to endorse or promote products derived from the
	Apple Software without specific prior written permission from Apple.  Except as
	expressly stated in this notice, no other rights or licenses, express or implied,
	are granted by Apple herein, including but not limited to any patent rights that
	may be infringed by your derivative works or by other works in which the Apple
	Software may be incorporated.
 
	The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
	WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
	WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
	PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
	COMBINATION WITH YOUR PRODUCTS.
 
	IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
	OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
	(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __JackMachThread__
#define __JackMachThread__

#include "JackPosixThread.h"
#import <CoreServices/../Frameworks/CarbonCore.framework/Headers/MacTypes.h>

#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <CoreAudio/HostTime.h>

#define THREAD_SET_PRIORITY         0
#define THREAD_SCHEDULED_PRIORITY   1

namespace Jack
{

/*!
\brief Darwin threads. Real-time threads are actually "time constraint" threads.
*/

class JackMachThread : public JackPosixThread
{

    private:

        UInt64 fPeriod;
        UInt64 fComputation;
        UInt64 fConstraint;

        static UInt32 GetThreadSetPriority(pthread_t thread);
        static UInt32 GetThreadScheduledPriority(pthread_t thread);
        static UInt32 GetThreadPriority(pthread_t thread, int inWhichPriority);

    public:

        JackMachThread(JackRunnableInterface* runnable, UInt64 period, UInt64 computation, UInt64 constraint)
                : JackPosixThread(runnable), fPeriod(period), fComputation(computation), fConstraint(constraint)
        {}

        JackMachThread(JackRunnableInterface* runnable)
                : JackPosixThread(runnable), fPeriod(0), fComputation(0), fConstraint(0)
        {}
        JackMachThread(JackRunnableInterface* runnable, int cancellation)
                : JackPosixThread(runnable, cancellation), fPeriod(0), fComputation(0), fConstraint(0)
        {}

        virtual ~JackMachThread()
        {}

        int Kill();

        int AcquireRealTime();
        int DropRealTime();
        void SetParams(UInt64 period, UInt64 computation, UInt64 constraint);
        static int GetParams(UInt64* period, UInt64* computation, UInt64* constraint);
        static int SetThreadToPriority(pthread_t thread, UInt32 inPriority, Boolean inIsFixed, UInt64 period, UInt64 computation, UInt64 constraint);
		
		static int AcquireRealTimeImp(pthread_t thread, UInt64 period, UInt64 computation, UInt64 constraint);
		static int DropRealTimeImp(pthread_t thread);
};

} // end of namespace

#endif
