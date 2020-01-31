/*
Copyright (C) 2001-2003 Paul Davis
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

#include "JackDebugClient.h"
#include "JackLibClient.h"
#include "JackChannel.h"
#include "JackLibGlobals.h"
#include "JackGlobals.h"
#include "JackCompilerDeps.h"
#include "JackTools.h"
#include "JackSystemDeps.h"
#include "JackServerLaunch.h"
#include "JackMetadata.h"
#include <assert.h>

using namespace Jack;

#ifdef __cplusplus
extern "C"
{
#endif

    jack_client_t * jack_client_new_aux (const char *client_name,
            jack_options_t options,
            jack_status_t *status);

    LIB_EXPORT jack_client_t * jack_client_open (const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    LIB_EXPORT int jack_client_close (jack_client_t *client);
    LIB_EXPORT int jack_get_client_pid (const char *name);

    LIB_EXPORT int jack_set_property(jack_client_t*, jack_uuid_t subject, const char* key, const char* value, const char* type);
    LIB_EXPORT int jack_get_property(jack_uuid_t subject, const char* key, char** value, char** type);
    LIB_EXPORT void jack_free_description(jack_description_t* desc, int free_description_itself);
    LIB_EXPORT int jack_get_properties(jack_uuid_t subject, jack_description_t* desc);
    LIB_EXPORT int jack_get_all_properties(jack_description_t** descs);
    LIB_EXPORT int jack_remove_property(jack_client_t* client, jack_uuid_t subject, const char* key);
    LIB_EXPORT int jack_remove_properties(jack_client_t* client, jack_uuid_t subject);
    LIB_EXPORT int jack_remove_all_properties(jack_client_t* client);
    LIB_EXPORT int jack_set_property_change_callback(jack_client_t* client, JackPropertyChangeCallback callback, void* arg);

#ifdef __cplusplus
}
#endif

static jack_client_t * jack_client_open_aux (const char *client_name,
            jack_options_t options,
            jack_status_t *status, va_list ap);

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

    JackLibGlobals *global = JackGlobalsManager::Instance()->CreateGlobal<JackLibGlobals>(va.server_name);
    if (global == nullptr) {
        jack_error("jack failed to create global context");
        return 0;
    }

    if (try_start_server(&va, options, status)) {
        jack_error("jack server is not running or cannot be started");
        JackGlobalsManager::Instance()->DestroyGlobal(va.server_name); // jack library destruction
        return 0;
    }

    if (JACK_DEBUG) {
        client = new JackDebugClient(new JackLibClient(global)); // Debug mode
    } else {
        client = new JackLibClient(global);
    }

    int res = client->Open(va.server_name, client_name, va.session_id, options, status);
    if (res < 0) {
        delete client;
        JackGlobalsManager::Instance()->DestroyGlobal(va.server_name); // jack library destruction
        int my_status1 = (JackFailure | JackServerError);
        *status = (jack_status_t)my_status1;
        return NULL;
    } else {
        return (jack_client_t*)client;
    }
}

static jack_client_t* jack_client_open_aux(const char* client_name, jack_options_t options, jack_status_t* status, va_list ap)
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

    JackLibGlobals *global = JackGlobalsManager::Instance()->CreateGlobal<JackLibGlobals>(va.server_name);
    if (global == nullptr) {
        jack_error("jack failed to create global context");
        return 0;
    }

    if (try_start_server(&va, options, status)) {
        jack_error("jack server is not running or cannot be started");
        JackGlobalsManager::Instance()->DestroyGlobal(va.server_name); // jack library destruction
        return 0;
    }

    if (JACK_DEBUG) {
        client = new JackDebugClient(new JackLibClient(global)); // Debug mode
    } else {
        client = new JackLibClient(global);
    }

    int res = client->Open(va.server_name, client_name, va.session_id, options, status);
    if (res < 0) {
        delete client;
        JackGlobalsManager::Instance()->DestroyGlobal(va.server_name); // jack library destruction
        int my_status1 = (JackFailure | JackServerError);
        *status = (jack_status_t)my_status1;
        return NULL;
    } else {
        return (jack_client_t*)client;
    }
}

LIB_EXPORT jack_client_t* jack_client_open(const char* ext_client_name, jack_options_t options, jack_status_t* status, ...)
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

LIB_EXPORT int jack_client_close(jack_client_t* ext_client)
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
        JackGlobalsManager::Instance()->DestroyGlobal(client->GetGlobal()->fServerName); // jack library destruction
        delete client;
        jack_log("jack_client_close res = %d", res);
    }
    JackGlobals::fOpenMutex->Unlock();
    return res;
}

LIB_EXPORT int jack_get_client_pid(const char *name)
{
    jack_error("jack_get_client_pid : not implemented on library side");
    return 0;
}

// Metadata API

LIB_EXPORT int jack_set_property(jack_client_t* ext_client, jack_uuid_t subject, const char* key, const char* value, const char* type)
{
    JackGlobals::CheckContext("jack_set_property");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_set_property ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_property called with a NULL client");
        return -1;
    } else {
        JackMetadata* metadata = GetMetadata();
        return (metadata ? metadata->SetProperty(client, subject, key, value, type) : -1);
    }
}

LIB_EXPORT int jack_get_property(jack_uuid_t subject, const char* key, char** value, char** type)
{
    JackGlobals::CheckContext("jack_get_property");

    JackMetadata* metadata = GetMetadata();
    return (metadata ? metadata->GetProperty(subject, key, value, type) : -1);
}

LIB_EXPORT void jack_free_description(jack_description_t* desc, int free_actual_description_too)
{
    JackGlobals::CheckContext("jack_free_description");

    JackMetadata* metadata = GetMetadata();
    if (metadata)
        metadata->FreeDescription(desc, free_actual_description_too);
}

LIB_EXPORT int jack_get_properties(jack_uuid_t subject, jack_description_t* desc)
{
    JackGlobals::CheckContext("jack_get_properties");

    JackMetadata* metadata = GetMetadata();
    return (metadata ? metadata->GetProperties(subject, desc) : -1);
}

LIB_EXPORT int jack_get_all_properties(jack_description_t** descriptions)
{
    JackGlobals::CheckContext("jack_get_all_properties");

    JackMetadata* metadata = GetMetadata();
    return (metadata ? metadata->GetAllProperties(descriptions) : -1);
}

LIB_EXPORT int jack_remove_property(jack_client_t* ext_client, jack_uuid_t subject, const char* key)
{
    JackGlobals::CheckContext("jack_remove_property");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_remove_property ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_remove_property called with a NULL client");
        return -1;
    } else {
        JackMetadata* metadata = GetMetadata();
        return (metadata ? metadata->RemoveProperty(client, subject, key) : -1);
    }
}

LIB_EXPORT int jack_remove_properties(jack_client_t* ext_client, jack_uuid_t subject)
{
    JackGlobals::CheckContext("jack_remove_properties");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_remove_properties ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_remove_properties called with a NULL client");
        return -1;
    } else {
        JackMetadata* metadata = GetMetadata();
        return (metadata ? metadata->RemoveProperties(client, subject) : -1);
    }
}

LIB_EXPORT int jack_remove_all_properties(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_remove_all_properties");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_remove_all_properties ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_remove_all_properties called with a NULL client");
        return -1;
    } else {
        JackMetadata* metadata = GetMetadata();
        return (metadata ? metadata->RemoveAllProperties(client) : -1);
    }
}

LIB_EXPORT int jack_set_property_change_callback(jack_client_t* ext_client, JackPropertyChangeCallback callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_property_change_callback");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_set_property_change_callback ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_property_change_callback called with a NULL client");
        return -1;
    } else {
        return client->SetPropertyChangeCallback(callback, arg);
    }
}
