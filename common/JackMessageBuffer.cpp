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

#include "JackMessageBuffer.h"
#include "JackGlobals.h"
#include "JackError.h"

namespace Jack
{

JackMessageBuffer* JackMessageBuffer::fInstance = NULL;

JackMessageBuffer::JackMessageBuffer():fInBuffer(0),fOutBuffer(0),fOverruns(0)
{
    fThread = JackGlobals::MakeThread(this); 
    fSignal = JackGlobals::MakeInterProcessSync(); 
    fMutex = new JackMutex(); 
    fThread->StartSync();
}

JackMessageBuffer::~JackMessageBuffer()
{
    if (fOverruns > 0) {
        jack_error("WARNING: %d message buffer overruns!", fOverruns); 
    } else {
        jack_info("no message buffer overruns"); 
    }
    fThread->SetStatus(JackThread::kIdle);
    fSignal->Signal();
    fThread->Stop();
    Flush();
    delete fThread;
    delete fMutex;
    delete fSignal;
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
    if (fMutex->Trylock()) {
        fBuffers[fInBuffer].level = level;
        strncpy(fBuffers[fInBuffer].message, message, MB_BUFFERSIZE);
        fInBuffer = MB_NEXT(fInBuffer);
        fSignal->SignalAll();
        fMutex->Unlock();
    } else {            /* lock collision */
        INC_ATOMIC(&fOverruns);
    }
}
         
bool JackMessageBuffer::Execute()
{
    fSignal->Wait();	
    fMutex->Lock();
    Flush();
    fMutex->Unlock();
    return true;	
}

void JackMessageBuffer::Create() 
{
    if (fInstance == NULL) {
        fInstance = new JackMessageBuffer();
    }
}

void JackMessageBuffer::Destroy() 
{
    if (fInstance != NULL) {
        delete fInstance;
        fInstance = NULL;
    }
}

void JackMessageBufferAdd(int level, const char *message) 
{
    if (Jack::JackMessageBuffer::fInstance == NULL) {
        /* Unable to print message with realtime safety.
         * Complain and print it anyway. */
        jack_log_function(LOG_LEVEL_ERROR, "messagebuffer not initialized, skip message");
    } else {
        Jack::JackMessageBuffer::fInstance->AddMessage(level, message);
    }
}

};

