/*
Copyright (C) 2001-2003 Paul Davis
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

#ifdef WIN32 
#pragma warning (disable : 4786)
#endif

#include "JackInternalClient.h"
#include "JackGraphManager.h"
#include "JackServer.h"
#include "JackDebugClient.h"
#include "JackServerGlobals.h"
#include "JackError.h"
#include "varargs.h"

/*
TODO: 
 
- implement the "jack_client_new", "jack_client_open", "jack_client_close" API so that we can provide a libjackdmp shared library
to be used by clients for direct access.
 
- automatic launch of the jack server with the first client open, automatic close when the last client exit. Use of a jackd.rsc configuration file.
 
*/

#ifdef WIN32
	#define	EXPORT __declspec(dllexport)
#else
	#define	EXPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    EXPORT jack_client_t* my_jack_internal_client_new(const char* client_name);
    EXPORT void my_jack_internal_client_close(jack_client_t* ext_client);

    EXPORT jack_client_t * jack_client_open (const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    EXPORT jack_client_t * jack_client_new (const char *client_name);
    EXPORT int jack_client_close (jack_client_t *client);

#ifdef __cplusplus
}
#endif

using namespace Jack;

EXPORT jack_client_t* my_jack_internal_client_new(const char* client_name, jack_options_t options, jack_status_t* status)
{
    JackLog("jack_internal_client_new %s", client_name);
    if (client_name == NULL) {
        jack_error("jack_internal_client_new called with a NULL client_name");
        return NULL;
    }
#ifdef __CLIENTDEBUG__
    JackClient* client = new JackDebugClient(new JackInternalClient(JackServer::fInstance, GetSynchroTable())); // Debug mode
#else
    JackClient* client = new JackInternalClient(JackServer::fInstance, GetSynchroTable()); // To improve...
#endif

    int res = client->Open(client_name, options, status);
    if (res < 0) {
        delete client;
        return NULL;
    } else {
        return (jack_client_t*)client;
    }
}

EXPORT void my_jack_internal_client_close(jack_client_t* ext_client)
{
    JackLog("jack_internal_client_close");
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_internal_client_close called with a NULL client");
    } else {
		client->Close();
        delete client;
        JackLog("jack_internal_client_close OK");
    }
}

EXPORT jack_client_t* jack_client_new(const char* client_name)
{
    int options = JackUseExactName;
    if (getenv("JACK_START_SERVER") == NULL)
        options |= JackNoStartServer;

    return jack_client_open(client_name, (jack_options_t)options, NULL);
}

// TO BE IMPLEMENTED PROPERLY
EXPORT jack_client_t* jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
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

    JackServerGlobals::Init(); // jack server initialisation

#ifdef __CLIENTDEBUG__
    JackClient* client = new JackDebugClient(new JackInternalClient(JackServer::fInstance, GetSynchroTable())); // Debug mode
#else
    JackClient* client = new JackInternalClient(JackServer::fInstance, GetSynchroTable()); // To improve...
#endif

    int res = client->Open(client_name, options, status);
    if (res < 0) {
        delete client;
        JackServerGlobals::Destroy(); // jack server destruction
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
		JackServerGlobals::Destroy(); // jack library destruction
		return res;
	}
}

