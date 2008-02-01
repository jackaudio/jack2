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

#ifdef WIN32 
#pragma warning (disable : 4786)
#endif

#include "JackGraphManager.h"
#include "JackInternalClient.h"
#include "JackServer.h"
#include "JackDebugClient.h"
#include "JackServerGlobals.h"
#include "JackError.h"
#include "JackServerLaunch.h"
#include "JackTools.h"

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
    EXPORT int jack_client_close (jack_client_t *client);

#ifdef __cplusplus
}
#endif

using namespace Jack;

EXPORT jack_client_t* jack_client_open(const char* ext_client_name, jack_options_t options, jack_status_t* status, ...)
{
    va_list ap;				/* variable argument pointer */
    jack_varargs_t va;		/* variable arguments */
    jack_status_t my_status;
	JackClient* client;
	char client_name[JACK_CLIENT_NAME_SIZE];
	
	JackTools::RewriteName(ext_client_name, client_name); 

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

    if (!JackServerGlobals::Init()) { // jack server initialisation
		int my_status1 = (JackFailure | JackServerError);
        *status = (jack_status_t)my_status1;
		return NULL;
	}

#ifndef WIN32
	char* jack_debug = getenv("JACK_CLIENT_DEBUG");
	if (jack_debug && strcmp(jack_debug, "on") == 0)
		client = new JackDebugClient(new JackInternalClient(JackServer::fInstance, GetSynchroTable())); // Debug mode
	else
		client = new JackInternalClient(JackServer::fInstance, GetSynchroTable()); 
#else
	client = new JackInternalClient(JackServer::fInstance, GetSynchroTable());
#endif 

    int res = client->Open(va.server_name, client_name, options, status);
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
		JackServerGlobals::Destroy();	// jack server destruction
		return res;
	}
}

