/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008 Nedko Arnaudov
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <dbus/dbus.h>

#include "controller_internal.h"

void
jack_controller_settings_set_bool_option(
    const char *value_str,
    int *value_ptr)
{
    if (strcmp(value_str, "true") == 0)
    {
        *value_ptr = true;
    }
    else if (strcmp(value_str, "false") == 0)
    {
        *value_ptr = false;
    }
    else
    {
        jack_error("ignoring unknown bool value \"%s\"", value_str);
    }
}

void
jack_controller_settings_set_sint_option(
    const char *value_str,
    int *value_ptr)
{
    *value_ptr = atoi(value_str);
}

void
jack_controller_settings_set_uint_option(
    const char *value_str,
    unsigned int *value_ptr)
{
    *value_ptr = strtoul(value_str, NULL, 10);
}

void
jack_controller_settings_set_char_option(
    const char *value_str,
    char *value_ptr)
{
    if (value_str[0] == 0 || value_str[1] != 0)
    {
        jack_error("invalid char option value \"%s\"", value_str);
        return;
    }

    *value_ptr = *value_str;
}

void
jack_controller_settings_set_string_option(
    const char *value_str,
    char *value_ptr,
    size_t max_size)
{
    size_t size;

    size = strlen(value_str);

    if (size >= max_size)
    {
        jack_error("string option value \"%s\" is too long, max is %u chars (including terminating zero)", value_str, (unsigned int)max_size);
        return;
    }

    strcpy(value_ptr, value_str);
}

void
jack_controller_settings_set_driver_option(
    jackctl_driver_t *driver,
    const char *option_name,
    const char *option_value)
{
    jackctl_parameter_t *parameter;
    jackctl_param_type_t type;
    int value_int = 0;
    unsigned int value_uint = 0;
    union jackctl_parameter_value value;

    jack_info("setting driver option \"%s\" to value \"%s\"", option_name, option_value);

    parameter = jack_controller_find_parameter(jackctl_driver_get_parameters(driver), option_name);
    if (parameter == NULL)
    {
        jack_error(
            "Unknown parameter \"%s\" of driver \"%s\"",
            option_name,
            jackctl_driver_get_name(driver));
        return;
    }

    type = jackctl_parameter_get_type(parameter);

    switch (type)
    {
    case JackParamInt:
        jack_controller_settings_set_sint_option(option_value, &value_int);
        value.i = value_int;
        break;
    case JackParamUInt:
        jack_controller_settings_set_uint_option(option_value, &value_uint);
        value.ui = value_uint;
        break;
    case JackParamChar:
        jack_controller_settings_set_char_option(option_value, &value.c);
        break;
    case JackParamString:
        jack_controller_settings_set_string_option(option_value, value.str, sizeof(value.str));
        break;
    case JackParamBool:
        jack_controller_settings_set_bool_option(option_value, &value_int);
        value.i = value_int;
        break;
    default:
        jack_error("Parameter \"%s\" of driver \"%s\" is of unknown type %d",
               jackctl_parameter_get_name(parameter),
               jackctl_driver_get_name(driver),
               type);
    }

    jackctl_parameter_set_value(parameter, &value);
}

void
jack_controller_settings_set_internal_option(
    jackctl_internal_t *internal,
    const char *option_name,
    const char *option_value)
{
    jackctl_parameter_t *parameter;
    jackctl_param_type_t type;
    int value_int = 0;
    unsigned int value_uint = 0;
    union jackctl_parameter_value value;

    jack_info("setting internal option \"%s\" to value \"%s\"", option_name, option_value);

    parameter = jack_controller_find_parameter(jackctl_internal_get_parameters(internal), option_name);
    if (parameter == NULL)
    {
        jack_error(
            "Unknown parameter \"%s\" of internal \"%s\"",
            option_name,
            jackctl_internal_get_name(internal));
        return;
    }

    type = jackctl_parameter_get_type(parameter);

    switch (type)
    {
    case JackParamInt:
        jack_controller_settings_set_sint_option(option_value, &value_int);
        value.i = value_int;
        break;
    case JackParamUInt:
        jack_controller_settings_set_uint_option(option_value, &value_uint);
        value.ui = value_uint;
        break;
    case JackParamChar:
        jack_controller_settings_set_char_option(option_value, &value.c);
        break;
    case JackParamString:
        jack_controller_settings_set_string_option(option_value, value.str, sizeof(value.str));
        break;
    case JackParamBool:
        jack_controller_settings_set_bool_option(option_value, &value_int);
        value.i = value_int;
        break;
    default:
        jack_error("Parameter \"%s\" of internal \"%s\" is of unknown type %d",
               jackctl_parameter_get_name(parameter),
               jackctl_internal_get_name(internal),
               type);
    }

    jackctl_parameter_set_value(parameter, &value);
}

void
jack_controller_settings_set_engine_option(
    struct jack_controller *controller_ptr,
    const char *option_name,
    const char *option_value)
{
    jackctl_parameter_t *parameter;
    jackctl_param_type_t type;
    int value_int = 0;
    unsigned int value_uint = 0;
    union jackctl_parameter_value value;

    jack_info("setting engine option \"%s\" to value \"%s\"", option_name, option_value);

    if (strcmp(option_name, "driver") == 0)
    {
        if (!jack_controller_select_driver(controller_ptr, option_value))
        {
            jack_error("unknown driver '%s'", option_value);
        }

        return;
    }

    parameter = jack_controller_find_parameter(jackctl_server_get_parameters(controller_ptr->server), option_name);
    if (parameter == NULL)
    {
        jack_error(
            "Unknown engine parameter \"%s\"",
            option_name);
        return;
    }

    type = jackctl_parameter_get_type(parameter);

    switch (type)
    {
    case JackParamInt:
        jack_controller_settings_set_sint_option(option_value, &value_int);
        value.i = value_int;
        break;
    case JackParamUInt:
        jack_controller_settings_set_uint_option(option_value, &value_uint);
        value.ui = value_uint;
        break;
    case JackParamChar:
        jack_controller_settings_set_char_option(option_value, &value.c);
        break;
    case JackParamString:
        jack_controller_settings_set_string_option(option_value, value.str, sizeof(value.str));
        break;
    case JackParamBool:
        jack_controller_settings_set_bool_option(option_value, &value_int);
        value.i = value_int;
        break;
    default:
        jack_error("Engine parameter \"%s\" is of unknown type %d",
               jackctl_parameter_get_name(parameter),
               type);
    }

    jackctl_parameter_set_value(parameter, &value);
}

static
bool
jack_controller_settings_save_options(
    void *context,
    const JSList * parameters_list,
    void *dbus_call_context_ptr)
{
    jackctl_parameter_t *parameter;
    jackctl_param_type_t type;
    union jackctl_parameter_value value;
    const char * name;
    char value_str[50];

    while (parameters_list != NULL)
    {
        parameter = (jackctl_parameter_t *)parameters_list->data;

        if (jackctl_parameter_is_set(parameter))
        {
            type = jackctl_parameter_get_type(parameter);
            value = jackctl_parameter_get_value(parameter);
            name = jackctl_parameter_get_name(parameter);
        
            switch (type)
            {
            case JackParamInt:
                sprintf(value_str, "%d", (int)value.i);
                if (!jack_controller_settings_write_option(context, name, value_str, dbus_call_context_ptr))
                {
                    return false;
                }
                break;
            case JackParamUInt:
                sprintf(value_str, "%u", (unsigned int)value.ui);
                if (!jack_controller_settings_write_option(context, name, value_str, dbus_call_context_ptr))
                {
                    return false;
                }
                break;
            case JackParamChar:
                sprintf(value_str, "%c", (char)value.c);
                if (!jack_controller_settings_write_option(context, name, value_str, dbus_call_context_ptr))
                {
                    return false;
                }
                break;
            case JackParamString:
                if (!jack_controller_settings_write_option(context, name, value.str, dbus_call_context_ptr))
                {
                    return false;
                }
                break;
            case JackParamBool:
                if (!jack_controller_settings_write_option(context, name, value.b ? "true" : "false", dbus_call_context_ptr))
                {
                    return false;
                }
                break;
            default:
                jack_error("parameter of unknown type %d", type);
            }
        }

        parameters_list = jack_slist_next(parameters_list);
    }

    return true;
}

bool
jack_controller_settings_save_engine_options(
    void *context,
    struct jack_controller *controller_ptr,
    void *dbus_call_context_ptr)
{
    if (controller_ptr->driver != NULL)
    {
        if (!jack_controller_settings_write_option(
                context,
                "driver",
                jackctl_driver_get_name(controller_ptr->driver),
                dbus_call_context_ptr))
        {
            return false;
        }
    }

    return jack_controller_settings_save_options(context, jackctl_server_get_parameters(controller_ptr->server), dbus_call_context_ptr);
}

bool
jack_controller_settings_save_driver_options(
    void *context,
    jackctl_driver_t *driver,
    void *dbus_call_context_ptr)
{
    return jack_controller_settings_save_options(context, jackctl_driver_get_parameters(driver), dbus_call_context_ptr);
}

bool
jack_controller_settings_save_internal_options(
    void *context,
    jackctl_internal_t *internal,
    void *dbus_call_context_ptr)
{
    return jack_controller_settings_save_options(context, jackctl_internal_get_parameters(internal), dbus_call_context_ptr);
}
