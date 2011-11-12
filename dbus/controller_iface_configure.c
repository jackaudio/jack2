/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008,2011 Nedko Arnaudov
    Copyright (C) 2007-2008 Juuso Alasuutari

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

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <dbus/dbus.h>

#include "jackdbus.h"
#include "controller_internal.h"
#include "xml.h"

unsigned char jack_controller_dbus_types[JACK_PARAM_MAX] =
{
    [JackParamInt] = DBUS_TYPE_INT32,
    [JackParamUInt] = DBUS_TYPE_UINT32,
    [JackParamChar] = DBUS_TYPE_BYTE,
    [JackParamString] = DBUS_TYPE_STRING,
    [JackParamBool] = DBUS_TYPE_BOOLEAN,
};

const char *jack_controller_dbus_type_signatures[JACK_PARAM_MAX] =
{
    [JackParamInt] = DBUS_TYPE_INT32_AS_STRING,
    [JackParamUInt] = DBUS_TYPE_UINT32_AS_STRING,
    [JackParamChar] = DBUS_TYPE_BYTE_AS_STRING,
    [JackParamString] = DBUS_TYPE_STRING_AS_STRING,
    [JackParamBool] = DBUS_TYPE_BOOLEAN_AS_STRING,
};

#define PARAM_TYPE_JACK_TO_DBUS(_) jack_controller_dbus_types[_]
#define PARAM_TYPE_JACK_TO_DBUS_SIGNATURE(_) jack_controller_dbus_type_signatures[_]

static
bool
jack_controller_jack_to_dbus_variant(
    jackctl_param_type_t type,
    const union jackctl_parameter_value *value_ptr,
    message_arg_t  *dbusv_ptr)
{
    switch (type)
    {
    case JackParamInt:
        dbusv_ptr->int32 = (dbus_int32_t)value_ptr->i;
        return true;
    case JackParamUInt:
        dbusv_ptr->uint32 = (dbus_uint32_t)value_ptr->ui;
        return true;
    case JackParamChar:
        dbusv_ptr->byte = value_ptr->c;
        return true;
    case JackParamString:
        dbusv_ptr->string = value_ptr->str;
        return true;
    case JackParamBool:
        dbusv_ptr->boolean = (dbus_bool_t)value_ptr->b;
        return true;
    }

    jack_error("Unknown JACK parameter type %i", (int)type);
    assert(0);
    return false;
}

static
bool
jack_controller_dbus_to_jack_variant(
    int type,
    const message_arg_t *dbusv_ptr,
    union jackctl_parameter_value *value_ptr)
{
    size_t len;

    switch (type)
    {
    case DBUS_TYPE_INT32:
        value_ptr->i = dbusv_ptr->int32;
        return true;
    case DBUS_TYPE_UINT32:
        value_ptr->ui = dbusv_ptr->uint32;
        return true;
    case DBUS_TYPE_BYTE:
        value_ptr->c = dbusv_ptr->byte;
        return true;
    case DBUS_TYPE_STRING:
        len = strlen(dbusv_ptr->string);
        if (len > JACK_PARAM_STRING_MAX)
        {
            jack_error("Parameter string value is too long (%u)", (unsigned int)len);
            return false;
        }
        memcpy(value_ptr->str, dbusv_ptr->string, len + 1);

        return true;
    case DBUS_TYPE_BOOLEAN:
        value_ptr->b = dbusv_ptr->boolean;
        return true;
    }

    jack_error("Unknown D-Bus parameter type %i", (int)type);
    return false;
}

/*
 * Construct a return message for a Get[Driver|Engine]ParameterValue method call.
 *
 * The operation can only fail due to lack of memory, in which case
 * there's no sense in trying to construct an error return. Instead,
 * call->reply will be set to NULL and handled in send_method_return().
 */
static void
jack_dbus_construct_method_return_parameter(
    struct jack_dbus_method_call * call,
    dbus_bool_t is_set,
    int type,
    const char *signature,
    message_arg_t default_value,
    message_arg_t value)
{
    DBusMessageIter iter;

    /* Create a new method return message. */
    call->reply = dbus_message_new_method_return (call->message);
    if (!call->reply)
    {
        goto fail;
    }

    dbus_message_iter_init_append (call->reply, &iter);

    /* Append the is_set argument. */
    if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN, (const void *) &is_set))
    {
        goto fail_unref;
    }

    /* Append the 'default' and 'value' arguments. */
    if (!jack_dbus_message_append_variant(&iter, type, signature, &default_value))
    {
        goto fail_unref;
    }
    if (!jack_dbus_message_append_variant(&iter, type, signature, &value))
    {
        goto fail_unref;
    }

    return;

fail_unref:
    dbus_message_unref (call->reply);
    call->reply = NULL;

fail:
    jack_error ("Ran out of memory trying to construct method return");
}

static
bool
jack_controller_dbus_get_parameter_address_ex(
    struct jack_dbus_method_call * call,
    DBusMessageIter * iter_ptr,
    const char ** address_array)
{
    const char * signature;
    DBusMessageIter array_iter;
    int type;
    int index;

    if (!dbus_message_iter_init(call->message, iter_ptr))
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid arguments to method '%s'. No input arguments found.",
            call->method_name);
        return false;
    }

    signature = dbus_message_iter_get_signature(iter_ptr);
    if (signature == NULL)
    {
        jack_error("dbus_message_iter_get_signature() failed");
        return false;
    }

    if (strcmp(signature, "as") != 0)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid arguments to method '%s'. Input arguments signature '%s', must begin with 'as'.",
            call->method_name,
            signature);
        return false;
    }

    dbus_message_iter_recurse(iter_ptr, &array_iter);

    index = 0;
    while ((type = dbus_message_iter_get_arg_type(&array_iter)) != DBUS_TYPE_INVALID)
    {
        if (index == PARAM_ADDRESS_SIZE)
        {
            jack_dbus_error(
                call,
                JACK_DBUS_ERROR_INVALID_ARGS,
                "Invalid arguments to method '%s'. Parameter address array must contain not more than %u elements.",
                (unsigned int)PARAM_ADDRESS_SIZE,
                call->method_name);
            return false;
        }

        ;
        if (type != DBUS_TYPE_STRING)
        {
            jack_dbus_error(
                call,
                JACK_DBUS_ERROR_FATAL,
                "Internal error when parsing parameter address of method '%s'. Address array element type '%c' is not string type.",
                call->method_name,
                type);
            return false;
        }

        dbus_message_iter_get_basic(&array_iter, address_array + index);
        //jack_info("address component: '%s'", address_array[index]);

        dbus_message_iter_next(&array_iter);
        index++;
    }

    while (index < PARAM_ADDRESS_SIZE)
    {
        address_array[index] = NULL;
        index++;
    }

    return true;
}

static
bool
jack_controller_dbus_get_parameter_address(
    struct jack_dbus_method_call * call,
    const char ** address_array)
{
    DBusMessageIter iter;
    bool ret;

    ret = jack_controller_dbus_get_parameter_address_ex(call, &iter, address_array);
    if (ret && dbus_message_iter_has_next(&iter))
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid arguments to method '%s'. Input arguments signature must be 'as'.",
            call->method_name);
        return false;
    }

    return ret;
}

#define controller_ptr ((struct jack_controller *)call->context)

static bool append_node_name(void * context, const char * name)
{
    return dbus_message_iter_append_basic(context, DBUS_TYPE_STRING, &name);
}

static
void
jack_controller_dbus_read_container(
    struct jack_dbus_method_call * call)
{
    const char * address[PARAM_ADDRESS_SIZE];
    dbus_bool_t leaf;
    DBusMessageIter iter;
    DBusMessageIter array_iter;

    //jack_info("jack_controller_dbus_read_container() called");

    if (!jack_controller_dbus_get_parameter_address(call, address))
    {
        /* The method call had invalid arguments meaning that
         * jack_controller_dbus_get_parameter_address() has
         * constructed an error for us. */
        return;
    }

    //jack_info("address is '%s':'%s':'%s'", address[0], address[1], address[2]);

    /* Create a new method return message. */
    call->reply = dbus_message_new_method_return(call->message);
    if (!call->reply)
    {
        goto oom;
    }

    dbus_message_iter_init_append(call->reply, &iter);

    if (!jack_params_check_address(controller_ptr->params, address, false))
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid container address '%s':'%s':'%s' supplied to method '%s'.",
            address[0],
            address[1],
            address[2],
            call->method_name);
        return;
    }

    leaf = jack_params_is_leaf_container(controller_ptr->params, address);
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &leaf))
    {
        goto oom_unref;
    }

    if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &array_iter))
    {
        goto oom_unref;
    }


    if (!jack_params_iterate_container(controller_ptr->params, address, append_node_name, &array_iter))
    {
        goto oom_close_unref;
    }

    dbus_message_iter_close_container(&iter, &array_iter);

    return;

oom_close_unref:
    dbus_message_iter_close_container(&iter, &array_iter);

oom_unref:
    dbus_message_unref(call->reply);
    call->reply = NULL;

oom:
    jack_error ("Ran out of memory trying to construct method return");
}

static bool append_parameter(void * context, const struct jack_parameter * param_ptr)
{
    DBusMessageIter struct_iter;
    unsigned char type;

    /* Open the struct. */
    if (!dbus_message_iter_open_container(context, DBUS_TYPE_STRUCT, NULL, &struct_iter))
    {
        goto fail;
    }

    /* Append parameter type. */
    type = PARAM_TYPE_JACK_TO_DBUS(param_ptr->type);
    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_BYTE, &type))
    {
        goto fail_close;
    }

    /* Append parameter name. */
    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &param_ptr->name))
    {
        goto fail_close;
    }

    /* Append parameter short description. */
    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &param_ptr->short_decr))
    {
        goto fail_close;
    }

    /* Append parameter long description. */
    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &param_ptr->long_descr))
    {
        goto fail_close;
    }

    /* Close the struct. */
    if (!dbus_message_iter_close_container(context, &struct_iter))
    {
        goto fail;
    }

    return true;

fail_close:
    dbus_message_iter_close_container(context, &struct_iter);

fail:
    return false;
}

static
void
jack_controller_dbus_get_parameters_info(
    struct jack_dbus_method_call * call)
{
    const char * address[PARAM_ADDRESS_SIZE];
    DBusMessageIter iter, array_iter;

    //jack_info("jack_controller_dbus_get_parameters_info() called");

    if (!jack_controller_dbus_get_parameter_address(call, address))
    {
        /* The method call had invalid arguments meaning that
         * jack_controller_dbus_get_parameter_address() has
         * constructed an error for us. */
        return;
    }

    //jack_info("address is '%s':'%s':'%s'", address[0], address[1], address[2]);

    if (!jack_params_check_address(controller_ptr->params, address, true))
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid container address '%s':'%s':'%s' supplied to method '%s'.",
            address[0],
            address[1],
            address[2],
            call->method_name);
        return;
    }

    call->reply = dbus_message_new_method_return (call->message);
    if (!call->reply)
    {
        goto fail;
    }

    dbus_message_iter_init_append (call->reply, &iter);

    /* Open the array. */
    if (!dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "(ysss)", &array_iter))
    {
        goto fail_unref;
    }

    if (!jack_params_iterate_params(controller_ptr->params, address, append_parameter, &array_iter))
    {
        goto fail_close_unref;
    }

    /* Close the array. */
    if (!dbus_message_iter_close_container (&iter, &array_iter))
    {
        goto fail_unref;
    }

    return;

fail_close_unref:
    dbus_message_iter_close_container (&iter, &array_iter);

fail_unref:
    dbus_message_unref (call->reply);
    call->reply = NULL;

fail:
    jack_error ("Ran out of memory trying to construct method return");
}

static
void
jack_controller_dbus_get_parameter_info(
    struct jack_dbus_method_call * call)
{
    const char * address[PARAM_ADDRESS_SIZE];
    const struct jack_parameter * param_ptr;
    DBusMessageIter iter;

    //jack_info("jack_controller_dbus_get_parameter_info() called");

    if (!jack_controller_dbus_get_parameter_address(call, address))
    {
        /* The method call had invalid arguments meaning that
         * jack_controller_dbus_get_parameter_address() has
         * constructed an error for us. */
        return;
    }

    //jack_info("address is '%s':'%s':'%s'", address[0], address[1], address[2]);

    param_ptr = jack_params_get_parameter(controller_ptr->params, address);
    if (param_ptr == NULL)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid container address '%s':'%s':'%s' supplied to method '%s'.",
            address[0],
            address[1],
            address[2],
            call->method_name);
        return;
    }

    call->reply = dbus_message_new_method_return(call->message);
    if (!call->reply)
    {
        goto fail;
    }

    dbus_message_iter_init_append(call->reply, &iter);

    if (!append_parameter(&iter, param_ptr))
    {
        goto fail_unref;
    }

    return;

fail_unref:
    dbus_message_unref(call->reply);
    call->reply = NULL;

fail:
    jack_error("Ran out of memory trying to construct method return");
}

static
void
jack_controller_dbus_get_parameter_constraint(
    struct jack_dbus_method_call * call)
{
    const char * address[PARAM_ADDRESS_SIZE];
    const struct jack_parameter * param_ptr;
    uint32_t index;
    DBusMessageIter iter, array_iter, struct_iter;
    const char * descr;
    message_arg_t value;

    //jack_info("jack_controller_dbus_get_parameter_constraint() called");

    if (!jack_controller_dbus_get_parameter_address(call, address))
    {
        /* The method call had invalid arguments meaning that
         * jack_controller_dbus_get_parameter_address() has
         * constructed an error for us. */
        return;
    }

    //jack_info("address is '%s':'%s':'%s'", address[0], address[1], address[2]);

    param_ptr = jack_params_get_parameter(controller_ptr->params, address);
    if (param_ptr == NULL)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid container address '%s':'%s':'%s' supplied to method '%s'.",
            address[0],
            address[1],
            address[2],
            call->method_name);
        return;
    }

    call->reply = dbus_message_new_method_return(call->message);
    if (!call->reply)
    {
        goto fail;
    }

    dbus_message_iter_init_append(call->reply, &iter);

    if ((param_ptr->constraint_flags & JACK_CONSTRAINT_FLAG_VALID) != 0)
    {
        value.boolean = param_ptr->constraint_range;
    }
    else
    {
        value.boolean = false;
    }

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &value))
    {
        goto fail_unref;
    }

    if ((param_ptr->constraint_flags & JACK_CONSTRAINT_FLAG_VALID) != 0)
    {
        value.boolean = (param_ptr->constraint_flags & JACK_CONSTRAINT_FLAG_STRICT) != 0;
    }

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &value))
    {
        goto fail_unref;
    }

    if ((param_ptr->constraint_flags & JACK_CONSTRAINT_FLAG_VALID) != 0)
    {
        value.boolean = (param_ptr->constraint_flags & JACK_CONSTRAINT_FLAG_FAKE_VALUE) != 0;
    }

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &value))
    {
        goto fail_unref;
    }

    /* Open the array. */
    if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(vs)", &array_iter))
    {
        goto fail_unref;
    }

    if ((param_ptr->constraint_flags & JACK_CONSTRAINT_FLAG_VALID) == 0)
    {
        goto close;
    }

    if (param_ptr->constraint_range)
    {
        /* Open the struct. */
        if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
        {
            goto fail_close_unref;
        }

        jack_controller_jack_to_dbus_variant(param_ptr->type, &param_ptr->constraint.range.min, &value);

        if (!jack_dbus_message_append_variant(
                &struct_iter,
                PARAM_TYPE_JACK_TO_DBUS(param_ptr->type),
                PARAM_TYPE_JACK_TO_DBUS_SIGNATURE(param_ptr->type),
                &value))
        {
            goto fail_close2_unref;
        }

        descr = "min";

        if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &descr))
        {
            goto fail_close2_unref;
        }

        /* Close the struct. */
        if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
        {
            goto fail_close_unref;
        }

        /* Open the struct. */
        if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
        {
            goto fail_close_unref;
        }

        jack_controller_jack_to_dbus_variant(param_ptr->type, &param_ptr->constraint.range.max, &value);
 
        if (!jack_dbus_message_append_variant(
                &struct_iter,
                PARAM_TYPE_JACK_TO_DBUS(param_ptr->type),
                PARAM_TYPE_JACK_TO_DBUS_SIGNATURE(param_ptr->type),
                &value))
        {
            goto fail_close2_unref;
        }

        descr = "max";

        if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &descr))
        {
            goto fail_close2_unref;
        }

        /* Close the struct. */
        if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
        {
            goto fail_close_unref;
        }
    }
    else
    {
        /* Append enum values to the array. */
        for (index = 0 ; index < param_ptr->constraint.enumeration.count ; index++)
        {
            descr = param_ptr->constraint.enumeration.possible_values_array[index].short_desc;

            jack_controller_jack_to_dbus_variant(param_ptr->type, &param_ptr->constraint.enumeration.possible_values_array[index].value, &value);

            /* Open the struct. */
            if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
            {
                goto fail_close_unref;
            }

            if (!jack_dbus_message_append_variant(
                    &struct_iter,
                    PARAM_TYPE_JACK_TO_DBUS(param_ptr->type),
                    PARAM_TYPE_JACK_TO_DBUS_SIGNATURE(param_ptr->type),
                    &value))
            {
                goto fail_close2_unref;
            }

            if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &descr))
            {
                goto fail_close2_unref;
            }

            /* Close the struct. */
            if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
            {
                goto fail_close_unref;
            }
        }
    }

close:
    /* Close the array. */
    if (!dbus_message_iter_close_container(&iter, &array_iter))
    {
        goto fail_unref;
    }

    return;

fail_close2_unref:
    dbus_message_iter_close_container(&array_iter, &struct_iter);

fail_close_unref:
    dbus_message_iter_close_container(&iter, &array_iter);

fail_unref:
    dbus_message_unref(call->reply);
    call->reply = NULL;

fail:
    jack_error ("Ran out of memory trying to construct method return");
}

static void
jack_controller_dbus_get_parameter_value(
    struct jack_dbus_method_call * call)
{
    const char * address[PARAM_ADDRESS_SIZE];
    const struct jack_parameter * param_ptr;
    union jackctl_parameter_value jackctl_value;
    union jackctl_parameter_value jackctl_default_value;
    message_arg_t value;
    message_arg_t default_value;

    //jack_info("jack_controller_dbus_get_parameter_value() called");

    if (!jack_controller_dbus_get_parameter_address(call, address))
    {
        /* The method call had invalid arguments meaning that
         * jack_controller_dbus_get_parameter_address() has
         * constructed an error for us. */
        return;
    }

    //jack_info("address is '%s':'%s':'%s'", address[0], address[1], address[2]);

    param_ptr = jack_params_get_parameter(controller_ptr->params, address);
    if (param_ptr == NULL)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid container address '%s':'%s':'%s' supplied to method '%s'.",
            address[0],
            address[1],
            address[2],
            call->method_name);
        return;
    }

    jackctl_default_value = param_ptr->vtable.get_default_value(param_ptr->obj);
    jackctl_value = param_ptr->vtable.get_value(param_ptr->obj);

    jack_controller_jack_to_dbus_variant(param_ptr->type, &jackctl_value, &value);
    jack_controller_jack_to_dbus_variant(param_ptr->type, &jackctl_default_value, &default_value);

    /* Construct the reply. */
    jack_dbus_construct_method_return_parameter(
        call,
        (dbus_bool_t)(param_ptr->vtable.is_set(param_ptr->obj) ? TRUE : FALSE),
        PARAM_TYPE_JACK_TO_DBUS(param_ptr->type),
        PARAM_TYPE_JACK_TO_DBUS_SIGNATURE(param_ptr->type),
        default_value,
        value);
}

static
void
jack_controller_dbus_set_parameter_value(
    struct jack_dbus_method_call * call)
{
    const char * address[PARAM_ADDRESS_SIZE];
    const struct jack_parameter * param_ptr;
    DBusMessageIter iter;
    DBusMessageIter variant_iter;
    message_arg_t arg;
    int arg_type;
    union jackctl_parameter_value value;

    //jack_info("jack_controller_dbus_set_parameter_value() called");

    if (!jack_controller_dbus_get_parameter_address_ex(call, &iter, address))
    {
        /* The method call had invalid arguments meaning that
         * jack_controller_dbus_get_parameter_address() has
         * constructed an error for us. */
        return;
    }

    //jack_info("address is '%s':'%s':'%s'", address[0], address[1], address[2]);

    param_ptr = jack_params_get_parameter(controller_ptr->params, address);
    if (param_ptr == NULL)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid container address '%s':'%s':'%s' supplied to method '%s'.",
            address[0],
            address[1],
            address[2],
            call->method_name);
        return;
    }

    dbus_message_iter_next(&iter);

    if (dbus_message_iter_has_next(&iter))
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid arguments to method '%s'. Too many arguments.",
            call->method_name);
        return;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid arguments to method '%s'. Value to set must be variant.",
            call->method_name);
        return;
    }

    dbus_message_iter_recurse (&iter, &variant_iter);
    dbus_message_iter_get_basic(&variant_iter, &arg);
    arg_type = dbus_message_iter_get_arg_type(&variant_iter);

    //jack_info("argument of type '%c'", arg_type);

    if (PARAM_TYPE_JACK_TO_DBUS(param_ptr->type) != arg_type)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Parameter value type mismatch: was expecting '%c', got '%c'",
            (char)PARAM_TYPE_JACK_TO_DBUS(param_ptr->type),
            (char)arg_type);
        return;
    }

    if (!jack_controller_dbus_to_jack_variant(
            arg_type,
            &arg,
            &value))
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Cannot convert parameter value");
        return;
    }

    param_ptr->vtable.set_value(param_ptr->obj, &value);

    jack_controller_pending_save(controller_ptr);

    jack_dbus_construct_method_return_empty(call);

}

static
void
jack_controller_dbus_reset_parameter_value(
    struct jack_dbus_method_call * call)
{
    const char * address[PARAM_ADDRESS_SIZE];
    const struct jack_parameter * param_ptr;

    //jack_info("jack_controller_dbus_reset_parameter_value() called");

    if (!jack_controller_dbus_get_parameter_address(call, address))
    {
        /* The method call had invalid arguments meaning that
         * jack_controller_dbus_get_parameter_address() has
         * constructed an error for us. */
        return;
    }

    //jack_info("address is '%s':'%s':'%s'", address[0], address[1], address[2]);

    param_ptr = jack_params_get_parameter(controller_ptr->params, address);
    if (param_ptr == NULL)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "Invalid container address '%s':'%s':'%s' supplied to method '%s'.",
            address[0],
            address[1],
            address[2],
            call->method_name);
        return;
    }

    param_ptr->vtable.reset(param_ptr->obj);

    jack_controller_pending_save(controller_ptr);

    jack_dbus_construct_method_return_empty(call);
}

#undef controller_ptr

JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(ReadContainer, "Get names of child parameters or containers")
    JACK_DBUS_METHOD_ARGUMENT_IN("parent", "as", "Address of parent container")
    JACK_DBUS_METHOD_ARGUMENT_OUT("leaf", "b", "Whether children are parameters (true) or containers (false)")
    JACK_DBUS_METHOD_ARGUMENT_OUT("children", "as", "Array of child names")
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(GetParametersInfo, "Retrieve info about parameters")
    JACK_DBUS_METHOD_ARGUMENT_IN("parent", "as", "Address of parameters parent")
    JACK_DBUS_METHOD_ARGUMENT_OUT("parameter_info_array", "a(ysss)", "Array of parameter info structs. Each info struct contains: type char, name, short and long description")
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(GetParameterInfo, "Retrieve info about parameter")
    JACK_DBUS_METHOD_ARGUMENT_IN("parameter", "as", "Address of parameter")
    JACK_DBUS_METHOD_ARGUMENT_OUT("parameter_info", "(ysss)", "Parameter info struct that contains: type char, name, short and long description")
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(GetParameterConstraint, "Get constraint of parameter")
    JACK_DBUS_METHOD_ARGUMENT_IN("parameter", "as", "Address of parameter")
    JACK_DBUS_METHOD_ARGUMENT_OUT("is_range", "b", "Whether constrinat is a range. If so, values parameter will contain two values, min and max")
    JACK_DBUS_METHOD_ARGUMENT_OUT("is_strict", "b", "Whether enum constraint is strict. I.e. value not listed in values array will not work")
    JACK_DBUS_METHOD_ARGUMENT_OUT("is_fake_value", "b", "Whether enum values are fake. I.e. have no user meaningful meaning")
    JACK_DBUS_METHOD_ARGUMENT_OUT("values", "a(vs)", "Values. If there is no constraint, this array will be empty. For range constraint there will be two values, min and max. For enum constraint there will be 2 or more values.")
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(GetParameterValue, "Get value of parameter")
    JACK_DBUS_METHOD_ARGUMENT_IN("parameter", "as", "Address of parameter")
    JACK_DBUS_METHOD_ARGUMENT_OUT("is_set", "b", "Whether parameter is set or its default value is used")
    JACK_DBUS_METHOD_ARGUMENT_OUT("default", "v", "Default value of parameter")
    JACK_DBUS_METHOD_ARGUMENT_OUT("value", "v", "Actual value of parameter")
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(SetParameterValue, "Set value of parameter")
    JACK_DBUS_METHOD_ARGUMENT_IN("parameter", "as", "Address of parameter")
    JACK_DBUS_METHOD_ARGUMENT_IN("value", "v", "New value for parameter")
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(ResetParameterValue, "Reset parameter to default value")
    JACK_DBUS_METHOD_ARGUMENT_IN("parameter", "as", "Address of parameter")
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHODS_BEGIN
    JACK_DBUS_METHOD_DESCRIBE(ReadContainer, jack_controller_dbus_read_container)
    JACK_DBUS_METHOD_DESCRIBE(GetParametersInfo, jack_controller_dbus_get_parameters_info)
    JACK_DBUS_METHOD_DESCRIBE(GetParameterInfo, jack_controller_dbus_get_parameter_info)
    JACK_DBUS_METHOD_DESCRIBE(GetParameterConstraint, jack_controller_dbus_get_parameter_constraint)
    JACK_DBUS_METHOD_DESCRIBE(GetParameterValue, jack_controller_dbus_get_parameter_value)
    JACK_DBUS_METHOD_DESCRIBE(SetParameterValue, jack_controller_dbus_set_parameter_value)
    JACK_DBUS_METHOD_DESCRIBE(ResetParameterValue, jack_controller_dbus_reset_parameter_value)
JACK_DBUS_METHODS_END

/*
 * Parameter addresses:
 *
 * "engine"
 * "engine", "driver"
 * "engine", "realtime"
 * "engine", ...more engine parameters
 *
 * "driver", "device"
 * "driver", ...more driver parameters
 *
 * "drivers", "alsa", "device"
 * "drivers", "alsa", ...more alsa driver parameters
 *
 * "drivers", ...more drivers
 *
 * "internals", "netmanager", "multicast_ip"
 * "internals", "netmanager", ...more netmanager parameters
 *
 * "internals", ...more internals
 *
 */

JACK_DBUS_IFACE_BEGIN(g_jack_controller_iface_configure, "org.jackaudio.Configure")
    JACK_DBUS_IFACE_EXPOSE_METHODS
JACK_DBUS_IFACE_END
