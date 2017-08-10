/*
  JACK control API

  Copyright (C) 2008 Nedko Arnaudov
  Copyright (C) 2008 Grame

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __JackControlAPI__
#define __JackControlAPI__

#include "jslist.h"
#include "JackCompilerDeps.h"

/** Parameter types, intentionally similar to jack_driver_param_type_t */
typedef enum
{
    JackParamInt = 1,			/**< @brief value type is a signed integer */
    JackParamUInt,				/**< @brief value type is an unsigned integer */
    JackParamChar,				/**< @brief value type is a char */
    JackParamString,			/**< @brief value type is a string with max size of ::JACK_PARAM_STRING_MAX+1 chars */
    JackParamBool,				/**< @brief value type is a boolean */
} jackctl_param_type_t;

/** Driver types, intentionally similar to jack_driver_type_t */
typedef enum
{
    JackMaster = 1,         /**< @brief master driver */
    JackSlave,              /**< @brief slave driver */
} jackctl_driver_type_t;

/** @brief Max value that jackctl_param_type_t type can have */
#define JACK_PARAM_MAX (JackParamBool + 1)

/** @brief Max length of string parameter value, excluding terminating nul char */
#define JACK_PARAM_STRING_MAX  127

/** @brief Type for parameter value */
/* intentionally similar to jack_driver_param_value_t */
union jackctl_parameter_value
{
    uint32_t ui;				/**< @brief member used for ::JackParamUInt */
    int32_t i;					/**< @brief member used for ::JackParamInt */
    char c;						/**< @brief member used for ::JackParamChar */
    char str[JACK_PARAM_STRING_MAX + 1]; /**< @brief member used for ::JackParamString */
    bool b;				/**< @brief member used for ::JackParamBool */
};

/** opaque type for server object */
typedef struct jackctl_server jackctl_server_t;

/** opaque type for driver object */
typedef struct jackctl_driver jackctl_driver_t;

/** opaque type for internal client object */
typedef struct jackctl_internal jackctl_internal_t;

/** opaque type for parameter object */
typedef struct jackctl_parameter jackctl_parameter_t;

/** opaque type for sigmask object */
typedef struct jackctl_sigmask jackctl_sigmask_t;

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

SERVER_EXPORT jackctl_sigmask_t *
jackctl_setup_signals(
    unsigned int flags);

SERVER_EXPORT void
jackctl_wait_signals(
    jackctl_sigmask_t * signals);

SERVER_EXPORT jackctl_server_t *
jackctl_server_create(
    bool (* on_device_acquire)(const char * device_name),
    void (* on_device_release)(const char * device_name));

SERVER_EXPORT void
jackctl_server_destroy(
	jackctl_server_t * server);

SERVER_EXPORT const JSList *
jackctl_server_get_drivers_list(
	jackctl_server_t * server);

SERVER_EXPORT bool
jackctl_server_open(
    jackctl_server_t * server,
    jackctl_driver_t * driver);

SERVER_EXPORT bool
jackctl_server_start(
    jackctl_server_t * server);

SERVER_EXPORT bool
jackctl_server_stop(
    jackctl_server_t * server);

SERVER_EXPORT bool
jackctl_server_close(
    jackctl_server_t * server);

SERVER_EXPORT const JSList *
jackctl_server_get_parameters(
	jackctl_server_t * server);

SERVER_EXPORT const char *
jackctl_driver_get_name(
	jackctl_driver_t * driver);

SERVER_EXPORT jackctl_driver_type_t
jackctl_driver_get_type(
	jackctl_driver_t * driver);

SERVER_EXPORT const JSList *
jackctl_driver_get_parameters(
	jackctl_driver_t * driver);

SERVER_EXPORT const char *
jackctl_parameter_get_name(
	jackctl_parameter_t * parameter);

SERVER_EXPORT const char *
jackctl_parameter_get_short_description(
	jackctl_parameter_t * parameter);

SERVER_EXPORT const char *
jackctl_parameter_get_long_description(
	jackctl_parameter_t * parameter);

SERVER_EXPORT jackctl_param_type_t
jackctl_parameter_get_type(
	jackctl_parameter_t * parameter);

SERVER_EXPORT char
jackctl_parameter_get_id(
	jackctl_parameter_t * parameter);

SERVER_EXPORT bool
jackctl_parameter_is_set(
	jackctl_parameter_t * parameter);

SERVER_EXPORT bool
jackctl_parameter_reset(
	jackctl_parameter_t * parameter);

SERVER_EXPORT union jackctl_parameter_value
jackctl_parameter_get_value(
	jackctl_parameter_t * parameter);

SERVER_EXPORT bool
jackctl_parameter_set_value(
	jackctl_parameter_t * parameter,
	const union jackctl_parameter_value * value_ptr);

SERVER_EXPORT union jackctl_parameter_value
jackctl_parameter_get_default_value(
	jackctl_parameter_t * parameter);

SERVER_EXPORT union jackctl_parameter_value
jackctl_parameter_get_default_value(
    jackctl_parameter *parameter_ptr);

SERVER_EXPORT bool
jackctl_parameter_has_range_constraint(
	jackctl_parameter_t * parameter_ptr);

SERVER_EXPORT bool
jackctl_parameter_has_enum_constraint(
	jackctl_parameter_t * parameter_ptr);

SERVER_EXPORT uint32_t
jackctl_parameter_get_enum_constraints_count(
	jackctl_parameter_t * parameter_ptr);

SERVER_EXPORT union jackctl_parameter_value
jackctl_parameter_get_enum_constraint_value(
	jackctl_parameter_t * parameter_ptr,
	uint32_t index);

SERVER_EXPORT const char *
jackctl_parameter_get_enum_constraint_description(
	jackctl_parameter_t * parameter_ptr,
	uint32_t index);

SERVER_EXPORT void
jackctl_parameter_get_range_constraint(
	jackctl_parameter_t * parameter_ptr,
	union jackctl_parameter_value * min_ptr,
	union jackctl_parameter_value * max_ptr);

SERVER_EXPORT bool
jackctl_parameter_constraint_is_strict(
	jackctl_parameter_t * parameter_ptr);

SERVER_EXPORT bool
jackctl_parameter_constraint_is_fake_value(
	jackctl_parameter_t * parameter_ptr);

SERVER_EXPORT const JSList *
jackctl_server_get_internals_list(
    jackctl_server *server_ptr);

SERVER_EXPORT const char *
jackctl_internal_get_name(
    jackctl_internal *internal_ptr);

SERVER_EXPORT const JSList *
jackctl_internal_get_parameters(
    jackctl_internal *internal_ptr);

SERVER_EXPORT bool jackctl_server_load_internal(
    jackctl_server * server,
    jackctl_internal * internal);

SERVER_EXPORT bool jackctl_server_unload_internal(
    jackctl_server * server,
    jackctl_internal * internal);

SERVER_EXPORT bool jackctl_server_load_session_file(
    jackctl_server * server_ptr,
    const char * file);

SERVER_EXPORT bool jackctl_server_add_slave(jackctl_server_t * server,
                            jackctl_driver_t * driver);

SERVER_EXPORT bool jackctl_server_remove_slave(jackctl_server_t * server,
                            jackctl_driver_t * driver);

SERVER_EXPORT bool
jackctl_server_switch_master(jackctl_server_t * server,
                            jackctl_driver_t * driver);

SERVER_EXPORT int
jackctl_parse_driver_params(jackctl_driver * driver_ptr, int argc, char* argv[]);

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

