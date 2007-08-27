/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2006 Grame

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

#include "JackDebugClient.h"
#include "JackLibClient.h"
#include "JackChannel.h"
#include "JackLibGlobals.h"
#include "JackGlobals.h"
#include "JackServerLaunch.h"

using namespace Jack;

#ifdef WIN32
	#define	EXPORT __declspec(dllexport)
#else
	#define	EXPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    EXPORT jack_client_t * jack_client_open (const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    EXPORT jack_client_t * jack_client_new (const char *client_name);
    EXPORT int jack_client_close (jack_client_t *client);

#ifdef __cplusplus
}
#endif

JackLibGlobals* JackLibGlobals::fGlobals = NULL;
long JackLibGlobals::fClientCount = 0;

static inline bool CheckPort(jack_port_id_t port_index)
{
    return (port_index < PORT_NUM);
}

static jack_client_t* jack_client_open_aux(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
    va_list ap;				/* variable argument pointer */
    jack_varargs_t va;		/* variable arguments */
    jack_status_t my_status;

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
    va_start(ap, status);
    jack_varargs_parse(options, ap, &va);
    va_end(ap);

    JackLog("jack_client_open %s\n", client_name);
    if (client_name == NULL) {
        jack_error("jack_client_new called with a NULL client_name");
        return NULL;
    }

    JackLibGlobals::Init(); // jack library initialisation
	
#ifndef WIN32
	if (try_start_server(&va, options, status)) {
		jack_error("jack server is not running or cannot be started");
		JackLibGlobals::Destroy(); // jack library destruction
		return 0;
	}
#endif	

#ifdef __CLIENTDEBUG__
    JackClient* client = new JackDebugClient(new JackLibClient(GetSynchroTable())); // Debug mode
#else
    JackClient* client = new JackLibClient(GetSynchroTable());
#endif

    int res = client->Open(client_name, options, status);
    if (res < 0) {
        delete client;
        JackLibGlobals::Destroy(); // jack library destruction
		int my_status1 = (JackFailure|JackServerError);
        *status = (jack_status_t)my_status1;
        return NULL;
    } else {
        return (jack_client_t*)client;
    }
}

EXPORT jack_client_t* jack_client_new(const char* client_name)
{
    int options = JackUseExactName;
    if (getenv("JACK_START_SERVER") == NULL)
        options |= JackNoStartServer;

    return jack_client_open_aux(client_name, (jack_options_t)options, NULL);
}

EXPORT jack_client_t* jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
    va_list ap;
	va_start(ap, status);
	jack_client_t* res = jack_client_open_aux(client_name, options, status, ap);
	va_end(ap);
	return res;
}

EXPORT int jack_client_close(jack_client_t* ext_client)
{
    JackLog("jack_client_close\n");
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_close called with a NULL client");
        return -1;
    } else {
		int res = client->Close();
		delete client;
		JackLog("jack_client_close OK\n");
		JackLibGlobals::Destroy(); // jack library destruction
		return res;
	}
}


