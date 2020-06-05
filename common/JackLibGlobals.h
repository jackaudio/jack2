/*
Copyright (C) 2005 Grame

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

#ifndef __JackLibGlobals__
#define __JackLibGlobals__

#include "JackShmMem.h"
#include "JackEngineControl.h"
#include "JackGlobals.h"
#include "JackPlatformPlug.h"
#include "JackGraphManager.h"
#include "JackMessageBuffer.h"
#include "JackTime.h"
#include "JackClient.h"
#include "JackError.h"
#include <assert.h>
#include <signal.h>

#ifdef WIN32
#ifdef __MINGW32__
#include <sys/types.h>
typedef _sigset_t sigset_t;
#else
typedef HANDLE sigset_t;
#endif
#endif

namespace Jack
{

class JackClient;

/*!
\brief Global library static structure: singleton kind of pattern.
*/

class JackLibGlobals : public JackGlobals
{

    /* This object is managed by JackGlobalsManager */
    friend class JackGlobalsManager;

    private:

        /* is called when first context for specific server is created */
        JackLibGlobals(const std::string &server_name)
            : JackGlobals(server_name)
            , fMetadata(server_name.c_str())
        {
            fGraphManager = -1;
            fEngineControl = -1;
            fClientCount = 0;

            InitTime();
        }

        /* is called when last context for specific server is removed */
        ~JackLibGlobals() override
        {
            EndTime();

            for (int i = 0; i < CLIENT_NUM; i++) {
                fSynchroTable[i].Disconnect();
            }
        }

    public:

        JackShmReadWritePtr<JackGraphManager> fGraphManager;	/*! Shared memory Port manager */
        JackShmReadWritePtr<JackEngineControl> fEngineControl;	/*! Shared engine control */  // transport engine has to be writable
        JackSynchro fSynchroTable[CLIENT_NUM];                  /*! Shared synchro table */
        JackMetadata fMetadata;                                 /*! Shared metadata base */
        sigset_t fProcessSignals;

        int fClientCount;

        /* is called each time a context for specific server is added */
        bool AddContext(const uint32_t &context_num, const std::string &server_name) override
        {
            if (!fServerRunning && fClientCount > 0) {
                // Cleanup remaining clients
                jack_error("Jack server was closed but clients are still allocated, cleanup...");
                for (int i = 0; i < CLIENT_NUM; i++) {
                    JackClient* client = fClientTable[i];
                    if (client) {
                        jack_error("Cleanup client ref = %d", i);
                        client->Close();
                        delete client;
                    }
                }

                // run destructors for all included objects, keeping the behaviour to previous revisions of JackGlobals source
                this->~JackLibGlobals();
                // construct in place
                new(this) JackLibGlobals(server_name);
            }

            fClientCount++;

            if (context_num == 0 && !JackMessageBuffer::Create()) {
                jack_error("Cannot create message buffer");
            }

            // Filter SIGPIPE to avoid having client get a SIGPIPE when trying to access a died server.
#ifdef WIN32
            // TODO
#else
            sigset_t signals;
            sigemptyset(&signals);
            sigaddset(&signals, SIGPIPE);
            sigprocmask(SIG_BLOCK, &signals, &fProcessSignals);
#endif
            return true;
        }

        /* is called each time a context for specific server is removed */
        bool DelContext(const uint32_t &context_num) override
        {
#ifdef WIN32
           // TODO
#else
            sigprocmask(SIG_BLOCK, &fProcessSignals, nullptr);
#endif
            if (context_num == 0 && !JackMessageBuffer::Destroy()) {
                jack_error("Cannot destroy message buffer");
            }

            return --fClientCount == 0;
        }

        JackGraphManager *GetGraphManager() override
        {
            return fGraphManager;
        }

        JackEngineControl *GetEngineControl() override
        {
            return fEngineControl;
        }

        JackSynchro *GetSynchroTable() override
        {
            return fSynchroTable;
        }

        /* must not be used in JackLibGlobals */
        JackServer *GetServer() override
        {
            assert(false);
            return nullptr;
        }

        JackMetadata* GetMetadata()
        {
            return &fMetadata;
        }
};

} // end of namespace

#endif

