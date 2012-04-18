/*
 * messagebuffer.h -- realtime-safe message interface for jackd.
 *
 *  This function is included in libjack so backend drivers can use
 *  it, *not* for external client processes.  The VERBOSE() and
 *  MESSAGE() macros are realtime-safe.
 */

/*
 *  Copyright (C) 2004 Rui Nuno Capela, Steve Harris
 *  Copyright (C) 2008 Nedko Arnaudov
 *  Copyright (C) 2008 Grame
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __JackMessageBuffer__
#define __JackMessageBuffer__

#include "JackPlatformPlug.h"
#include "JackMutex.h"
#include "JackAtomic.h"

namespace Jack
{

/* MB_NEXT() relies on the fact that MB_BUFFERS is a power of two */
#define MB_BUFFERS  128
#define MB_NEXT(index) ((index+1) & (MB_BUFFERS-1))
#define MB_BUFFERSIZE   256     /* message length limit */

struct JackMessage
{
    int level;
    char message[MB_BUFFERSIZE];
};

/*!
\brief Message buffer to be used from RT threads.
*/

class JackMessageBuffer : public JackRunnableInterface
{

    private:

        volatile JackThreadInitCallback fInit;
        void* fInitArg;
        JackMessage fBuffers[MB_BUFFERS];
        JackThread fThread;
        JackProcessSync fGuard;
        volatile unsigned int fInBuffer;
        volatile unsigned int fOutBuffer;
        SInt32 fOverruns;
        bool fRunning;

        void Flush();

        bool Start();
        bool Stop();

    public:

        JackMessageBuffer();
        ~JackMessageBuffer();

        // JackRunnableInterface interface
        bool Execute();

	    bool static Create();
	    bool static Destroy();

        void AddMessage(int level, const char *message);
        int SetInitCallback(JackThreadInitCallback callback, void *arg);

	    static JackMessageBuffer* fInstance;
};

#ifdef __cplusplus
extern "C"
{
#endif

void JackMessageBufferAdd(int level, const char *message);

#ifdef __cplusplus
}
#endif

};

#endif
