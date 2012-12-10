/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2008 Grame

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

#include "JackSystemDeps.h"
#include "JackGraphManager.h"
#include "JackInternalClient.h"
#include "JackServer.h"
#include "JackDebugClient.h"
#include "JackServerGlobals.h"
#include "JackTools.h"
#include "JackCompilerDeps.h"
#include "JackLockedEngine.h"

#ifdef __cplusplus
extern "C"
{
#endif

    jack_client_t* jack_client_new_aux(const char* client_name, jack_options_t options, jack_status_t* status);

    SERVER_EXPORT jack_client_t * jack_client_open (const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    SERVER_EXPORT int jack_client_close (jack_client_t *client);
    SERVER_EXPORT int jack_get_client_pid (const char *name);

#ifdef __cplusplus
}
#endif

using namespace Jack;

jack_client_t* jack_client_new_aux(const char* client_name, jack_options_t options, jack_status_t* status)
{
    jack_varargs_t va;          /* variable arguments */
    jack_status_t my_status;
    JackClient* client;

    if (client_name == NULL) {
        jack_error("jack_client_new called with a NULL client_name");
        return NULL;
    }

    jack_log("jack_client_new %s", client_name);

    if (status == NULL)         /* no status from caller? */
        status = &my_status;    /* use local status word */
    *status = (jack_status_t)0;

    /* validate parameters */
    if ((options & ~JackOpenOptions)) {
        int my_status1 = *status | (JackFailure | JackInvalidOption);
        *status = (jack_status_t)my_status1;
        return NULL;
    }

    /* parse variable arguments */
    jack_varargs_init(&va);

    if (!JackServerGlobals::Init()) { // jack server initialisation
        int my_status1 = (JackFailure | JackServerError);
        *status = (jack_status_t)my_status1;
        return NULL;
    }

    if (JACK_DEBUG) {
        client = new JackDebugClient(new JackInternalClient(JackServerGlobals::fInstance, GetSynchroTable())); // Debug mode
    } else {
        client = new JackInternalClient(JackServerGlobals::fInstance, GetSynchroTable());
    }

    int res = client->Open(va.server_name, client_name, va.session_id, options, status);
    if (res < 0) {
        delete client;
        JackServerGlobals::Destroy(); // jack server destruction
        int my_status1 = (JackFailure | JackServerError);
        *status = (jack_status_t)my_status1;
        return NULL;
    } else {
        return (jack_client_t*)client;
    }
}

jack_client_t* jack_client_open_aux(const char* client_name, jack_options_t options, jack_status_t* status, va_list ap)
{
    jack_varargs_t va;		/* variable arguments */
    jack_status_t my_status;
    JackClient* client;

    if (client_name == NULL) {
        jack_error("jack_client_open called with a NULL client_name");
        return NULL;
    }

    jack_log("jack_client_open %s", client_name);

    if (status == NULL)			/* no status from caller? */
        status = &my_status;	/* use local status word */
    *status = (jack_status_t)0;

    /* validate parameters */
    if ((options & ~JackOpenOptions)) {
        int my_status1 = *status | (JackFailure | JackInvalidOption);
        *status = (jack_status_t)my_status1;
        return NULL;
    }

    /* parse variable arguments */
    jack_varargs_parse(options, ap, &va);

    if (!JackServerGlobals::Init()) { // jack server initialisation
        int my_status1 = (JackFailure | JackServerError);
        *status = (jack_status_t)my_status1;
        return NULL;
    }

    if (JACK_DEBUG) {
        client = new JackDebugClient(new JackInternalClient(JackServerGlobals::fInstance, GetSynchroTable())); // Debug mode
    } else {
        client = new JackInternalClient(JackServerGlobals::fInstance, GetSynchroTable());
    }

    int res = client->Open(va.server_name, client_name, va.session_id, options, status);
    if (res < 0) {
        delete client;
        JackServerGlobals::Destroy(); // jack server destruction
        int my_status1 = (JackFailure | JackServerError);
        *status = (jack_status_t)my_status1;
        return NULL;
    } else {
        return (jack_client_t*)client;
    }
}

SERVER_EXPORT jack_client_t* jack_client_open(const char* ext_client_name, jack_options_t options, jack_status_t* status, ...)
{
    JackGlobals::CheckContext("jack_client_open");

    try {
        assert(JackGlobals::fOpenMutex);
        JackGlobals::fOpenMutex->Lock();
        va_list ap;
        va_start(ap, status);
        jack_client_t* res = jack_client_open_aux(ext_client_name, options, status, ap);
        va_end(ap);
        JackGlobals::fOpenMutex->Unlock();
        return res;
    } catch (std::bad_alloc& e) {
        jack_error("Memory allocation error...");
        return NULL;
    } catch (...) {
        jack_error("Unknown error...");
        return NULL;
    }
}

SERVER_EXPORT int jack_client_close(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_client_close");

    assert(JackGlobals::fOpenMutex);
    JackGlobals::fOpenMutex->Lock();
    int res = -1;
    jack_log("jack_client_close");
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_close called with a NULL client");
    } else {
        res = client->Close();
        delete client;
        JackServerGlobals::Destroy();   // jack server destruction
        jack_log("jack_client_close res = %d", res);
    }
    JackGlobals::fOpenMutex->Unlock();
    return res;
}

SERVER_EXPORT int jack_get_client_pid(const char *name)
{
    return (JackServerGlobals::fInstance != NULL)
        ? JackServerGlobals::fInstance->GetEngine()->GetClientPID(name)
        : 0;
}

