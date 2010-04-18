/*
    Copyright (C) 2001 Paul Davis
    Copyright (C) 2004 Jack O'Quin
    Copyright (C) 2010 Torben Hohn
    
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

#ifndef __jack_session_h__
#define __jack_session_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <jack/types.h>
#include <jack/weakmacros.h>

/**
 * @defgroup SessionClientFunctions Session API for clients.
 * @{
 */


/**
 * session event types.
 *
 * if a client cant save templates, i might just do a normal save.
 *
 * the rationale, why there is no quit without save, is that a client
 * might refuse to quit when it has unsaved data.
 * however some other clients might have already quit.
 * this results in too much confusion, so we just dont support that.
 * the session manager can check, if the saved state is different from a previous
 * save, and just remove the saved stuff.
 *
 * (an inquiry function, whether a quit is ok, followed by a quit event
 *  would have a race)
 */
enum JackSessionEventType {
    JackSessionSave = 1,
    JackSessionSaveAndQuit = 2,
    JackSessionSaveTemplate = 3
};

typedef enum JackSessionEventType jack_session_event_type_t;

enum JackSessionFlags {
    /**
     * an error occured while saving.
     */
    JackSessionSaveError = 0x01,

    /**
     * this reply indicates that a client is part of a multiclient application.
     * the command reply is left empty. but the session manager should still
     * consider this client part of a session. it will come up due to invocation of another
     * client.
     */
    JackSessionChildClient = 0x02
};

typedef enum JackSessionFlags jack_session_flags_t;

struct _jack_session_event {
    /**
     * the actual type of this session event.
     */
    jack_session_event_type_t type;

    /**
     * session_directory with trailing separator
     * this is per client. so the client can do whatever it likes in here.
     */
    const char *session_dir;

    /**
     * client_uuid which must be specified to jack_client_open on session reload.
     * client can specify it in the returned commandline as an option, or just save it
     * with the state file.
     */
    const char *client_uuid;

    /**
     * the command_line is the reply of the client.
     * it specifies in a platform dependent way, how the client must be restarted upon session reload.
     *
     * probably it should contain ${SESSION_DIR} instead of the actual session dir.
     * this would basically make the session dir moveable.
     *
     * ownership of the memory is handed to jack.
     * initially set to NULL by jack;
     */
    char *command_line;

    /**
     * flags to be set by the client. normally left 0.
     */
    jack_session_flags_t flags;
};

typedef struct _jack_session_event jack_session_event_t;

/**
 * Prototype for the client supplied function that is called
 * whenever a session notification is sent via jack_session_notify().
 *
 * The session_id must be passed to jack_client_open on session reload (this can be
 * done by specifying it somehow on the returned command line).
 *
 * @param event the event_structure.
 * @param arg pointer to a client supplied structure
 */
typedef void (*JackSessionCallback)(jack_session_event_t *event, void *arg);

/**
 * Tell the JACK server to call @a save_callback the session handler wants
 * to save.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_session_callback(jack_client_t *client,
			    JackSessionCallback session_callback,
			    void *arg) JACK_WEAK_EXPORT;

/**
 * reply to a session_event
 *
 * this can either be called directly from the callback, or later from a different thread.
 * so its possible to just stick the event pointer into a pipe and execute the save code
 * from the gui thread.
 *
 * @return 0 on success, otherwise a non-zero error code
 */

int jack_session_reply( jack_client_t *client, jack_session_event_t *event ) JACK_WEAK_EXPORT;


/**
 * free memory used by a jack_session_event_t
 * this also frees the memory used by the command_line pointer.
 * if its non NULL.
 */

void jack_session_event_free (jack_session_event_t *event);

/*@}*/


/**
 * @defgroup JackSessionManagerAPI  this API is intended for a sessionmanager.
 *				    this API could be server specific. if we dont reach consensus here,
 *				    we can just drop it.
 *				    i know its a bit clumsy.
 *				    but this api isnt required to be as stable as the client api.
 * @{
 */

typedef struct  {
	const char *uuid;
	const char *client_name;
	const char *command;
	jack_session_flags_t flags;
} jack_session_command_t;

/**
 * send a save or quit event, to all clients listening for session
 * callbacks. the returned strings of the clients are accumulated and
 * returned as an array of jack_session_command_t.
 * its terminated by ret[i].uuid == NULL
 * target == NULL means send to all interested clients. otherwise a clientname
 */

jack_session_command_t *jack_session_notify (jack_client_t* client,
					     const char *target,
					     jack_session_event_type_t type,
					     const char *path ) JACK_WEAK_EXPORT;

/**
 * free the memory allocated by a session command.
 */

void jack_session_commands_free (jack_session_command_t *cmds) JACK_WEAK_EXPORT;

/**
 * get the sessionid for a client name.
 * the sessionmanager needs this to reassociate a client_name to the session_id.
 */

char *jack_get_uuid_for_client_name( jack_client_t *client, const char *client_name ) JACK_WEAK_EXPORT;

/**
 * get the client name for a session_id.
 * in order to snapshot the graph connections, the sessionmanager needs to map
 * session_ids to client names.
 */

char *jack_get_client_name_by_uuid( jack_client_t *client, const char *client_uuid ) JACK_WEAK_EXPORT;

/**
 * reserve a client name and associate it to a uuid.
 * when a client later call jack_client_open() and specifies the uuid,
 * jackd will assign the reserved name.
 * this allows a session manager to know in advance under which client name
 * its managed clients will appear.
 *
 * @return 0 on success, otherwise a non-zero error code
 */

int
jack_reserve_client_name( jack_client_t *client, const char *name, const char *uuid ) JACK_WEAK_EXPORT;

#ifdef __cplusplus
}
#endif
#endif
