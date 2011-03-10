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

#include "JackMessageBuffer.h"
#include "JackGlobals.h"
#include "JackError.h"

namespace Jack
{

JackMessageBuffer* JackMessageBuffer::fInstance = NULL;

JackMessageBuffer::JackMessageBuffer()
    :fInit(NULL),fInitArg(NULL),fThread(this),fInBuffer(0),fOutBuffer(0),fOverruns(0),fRunning(false)
{}

JackMessageBuffer::~JackMessageBuffer()
{}

void JackMessageBuffer::Start()
{
    fRunning = true;
    fThread.StartSync();
}

void JackMessageBuffer::Stop()
{
    if (fOverruns > 0) {
        jack_error("WARNING: %d message buffer overruns!", fOverruns);
    } else {
        jack_log("no message buffer overruns");
    }
    fGuard.Lock();
    fRunning = false;
    fGuard.Signal();
    fGuard.Unlock();
    fThread.Stop();
    Flush();
}

void JackMessageBuffer::Flush()
{
    while (fOutBuffer != fInBuffer) {
        jack_log_function(fBuffers[fOutBuffer].level, fBuffers[fOutBuffer].message);
        fOutBuffer = MB_NEXT(fOutBuffer);
    }
}

void JackMessageBuffer::AddMessage(int level, const char *message)
{
    if (fGuard.Trylock()) {
        fBuffers[fInBuffer].level = level;
        strncpy(fBuffers[fInBuffer].message, message, MB_BUFFERSIZE);
        fInBuffer = MB_NEXT(fInBuffer);
        fGuard.Signal();
        fGuard.Unlock();
    } else {            /* lock collision */
        INC_ATOMIC(&fOverruns);
    }
}

bool JackMessageBuffer::Execute()
{
    while (fRunning) {
        fGuard.Lock();
        fGuard.Wait();
        /* the client asked for all threads to run a thread
        initialization callback, which includes us.
        */
        if (fInit) {
            fInit(fInitArg);
            fInit = NULL;
            /* and we're done */
            fGuard.Signal();
        }
        Flush();
        fGuard.Unlock();
    }
    return false;
}

void JackMessageBuffer::Create()
{
    if (fInstance == NULL) {
        fInstance = new JackMessageBuffer();
        fInstance->Start();
    }
}

void JackMessageBuffer::Destroy()
{
    if (fInstance != NULL) {
        fInstance->Stop();
        delete fInstance;
        fInstance = NULL;
    }
}

void JackMessageBufferAdd(int level, const char *message)
{
    if (Jack::JackMessageBuffer::fInstance == NULL) {
        /* Unable to print message with realtime safety. Complain and print it anyway. */
        jack_log_function(LOG_LEVEL_ERROR, "messagebuffer not initialized, skip message");
    } else {
        Jack::JackMessageBuffer::fInstance->AddMessage(level, message);
    }
}

void JackMessageBuffer::SetInitCallback(JackThreadInitCallback callback, void *arg)
{
    fGuard.Lock();
    /* set up the callback */
    fInitArg = arg;
    fInit = callback;
    /* wake msg buffer thread */
    fGuard.Signal();
    /* wait for it to be done */
    fGuard.Wait();
    /* and we're done */
    fGuard.Unlock();
}

};

