/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008,2011 Nedko Arnaudov
    
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
#include <assert.h>
#include <dbus/dbus.h>

#include "controller_internal.h"

void
jack_controller_deserialize_parameter_value(
    struct jack_controller *controller_ptr,
    const char * const * address,
    const char * option_value)
{
    const struct jack_parameter * param_ptr;
    union jackctl_parameter_value value;
    size_t size;

    param_ptr = jack_params_get_parameter(controller_ptr->params, address);
    if (param_ptr == NULL)
    {
        jack_error("Unknown parameter");
        goto ignore;
    }

    jack_info("setting parameter '%s':'%s':'%s' to value \"%s\"", address[0], address[1], address[2], option_value);

    switch (param_ptr->type)
    {
    case JackParamInt:
        value.i = atoi(option_value);
        break;
    case JackParamUInt:
        value.ui = strtoul(option_value, NULL, 10);
        break;
    case JackParamChar:
        if (option_value[0] == 0 || option_value[1] != 0)
        {
            jack_error("invalid char option value \"%s\"", option_value);
            goto ignore;
        }
        value.c = *option_value;
        break;
    case JackParamString:
        size = strlen(option_value);
        if (size >= sizeof(value.str))
        {
            jack_error("string option value \"%s\" is too long, max is %zu chars (including terminating zero)", option_value, sizeof(value.str));
            goto ignore;
        }

        strcpy(value.str, option_value);
        break;
    case JackParamBool:
        if (strcmp(option_value, "true") == 0)
        {
            value.b = true;
        }
        else if (strcmp(option_value, "false") == 0)
        {
            value.b = false;
        }
        else
        {
            jack_error("ignoring unknown bool value \"%s\"", option_value);
            goto ignore;
        }
        break;
    default:
        jack_error("Unknown type %d", (int)param_ptr->type);
        goto ignore;
    }

    if (param_ptr->vtable.set_value(param_ptr->obj, &value))
    {
        return;
    }

    jack_error("Parameter set failed");

ignore:
    jack_error("Ignoring restore attempt of parameter '%s':'%s':'%s'", address[0], address[1], address[2]);
}

void
jack_controller_serialize_parameter_value(
    const struct jack_parameter * param_ptr,
    char * value_buffer)
{
    union jackctl_parameter_value value;

    value = param_ptr->vtable.get_value(param_ptr->obj);

    switch (param_ptr->type)
    {
    case JackParamInt:
        sprintf(value_buffer, "%d", (int)value.i);
        return;
    case JackParamUInt:
        sprintf(value_buffer, "%u", (unsigned int)value.ui);
        return;
    case JackParamChar:
        sprintf(value_buffer, "%c", (char)value.c);
        return;
    case JackParamString:
        strcpy(value_buffer, value.str);
        return;
    case JackParamBool:
        strcpy(value_buffer, value.b ? "true" : "false");
        return;
    }

    jack_error("parameter of unknown type %d", (int)param_ptr->type);
    assert(false);
    *value_buffer = 0;
}
