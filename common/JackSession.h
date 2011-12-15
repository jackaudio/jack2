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

#ifndef __jack_session_int_h__
#define __jack_session_int_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum JackSessionEventType {
    JackSessionSave = 1,
    JackSessionSaveAndQuit = 2,
    JackSessionSaveTemplate = 3
};

typedef enum JackSessionEventType jack_session_event_type_t;

enum JackSessionFlags {
    JackSessionSaveError = 0x01,
    JackSessionNeedTerminal = 0x02
};

typedef enum JackSessionFlags jack_session_flags_t;

struct _jack_session_event {
    jack_session_event_type_t type;
    const char *session_dir;
    const char *client_uuid;
    char *command_line;
    jack_session_flags_t flags;
    uint32_t future;
};

typedef struct _jack_session_event jack_session_event_t;

typedef void (*JackSessionCallback)(jack_session_event_t *event,
                                    void                 *arg);

typedef struct  {
	const char           *uuid;
	const char           *client_name;
	const char           *command;
	jack_session_flags_t  flags;
} jack_session_command_t;

#ifdef __cplusplus
}
#endif
#endif
