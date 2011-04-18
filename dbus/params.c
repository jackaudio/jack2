/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2011 Nedko Arnaudov

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

#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <dbus/dbus.h>

#include "params.h"
#include "controller_internal.h"

#define PTNODE_ENGINE    "engine"
#define PTNODE_DRIVER    "driver"
#define PTNODE_DRIVERS   "drivers"
#define PTNODE_INTERNALS "internals"

struct jack_parameter_container
{
    struct list_head siblings;
    char * name;
    struct jack_parameter_container * symlink;
    bool leaf;
    struct list_head children;
    void * obj;
};

struct jack_params
{
    jackctl_server_t * server;
    struct jack_parameter_container root;
    struct list_head * drivers_ptr;
    uint32_t drivers_count;
    struct jack_parameter_container * driver_ptr;
    bool driver_set;            /* whether driver is manually set, if false - DEFAULT_DRIVER is auto set */
};

static bool controlapi_parameter_is_set(void * obj)
{
    return jackctl_parameter_is_set((jackctl_parameter_t *)obj);
}

static bool controlapi_parameter_reset(void * obj)
{
    return jackctl_parameter_reset((jackctl_parameter_t *)obj);
}

union jackctl_parameter_value controlapi_parameter_get_value(void * obj)
{
    return jackctl_parameter_get_value((jackctl_parameter_t *)obj);
}

bool controlapi_parameter_set_value(void * obj, const union jackctl_parameter_value * value_ptr)
{
    return jackctl_parameter_set_value((jackctl_parameter_t *)obj, value_ptr);
}

union jackctl_parameter_value controlapi_parameter_get_default_value(void * obj)
{
    return jackctl_parameter_get_default_value((jackctl_parameter_t *)obj);
}

static struct jack_parameter_container * create_container(struct list_head * parent_list_ptr, const char * name)
{
    struct jack_parameter_container * container_ptr;

    container_ptr = malloc(sizeof(struct jack_parameter_container));
    if (container_ptr == NULL)
    {
        jack_error("Ran out of memory trying to allocate struct jack_parameter_container");
        goto fail;
    }

    container_ptr->name = strdup(name);
    if (container_ptr->name == NULL)
    {
        jack_error("Ran out of memory trying to strdup parameter container name");
        goto free;
    }

    container_ptr->leaf = false;
    container_ptr->symlink = NULL;
    container_ptr->obj = NULL;
    INIT_LIST_HEAD(&container_ptr->children);
    list_add_tail(&container_ptr->siblings, parent_list_ptr);

    return container_ptr;

free:
    free(container_ptr);
fail:
    return NULL;
}

static bool add_controlapi_param(struct list_head * parent_list_ptr, jackctl_parameter_t * param)
{
    struct jack_parameter * param_ptr;
    uint32_t i;

    param_ptr = malloc(sizeof(struct jack_parameter));
    if (param_ptr == NULL)
    {
        jack_error("Ran out of memory trying to allocate struct jack_parameter");
        goto fail;
    }

    param_ptr->ext = false;
    param_ptr->obj = param;
    param_ptr->vtable.is_set = controlapi_parameter_is_set;
    param_ptr->vtable.reset = controlapi_parameter_reset;
    param_ptr->vtable.get_value = controlapi_parameter_get_value;
    param_ptr->vtable.set_value = controlapi_parameter_set_value;
    param_ptr->vtable.get_default_value = controlapi_parameter_get_default_value;

    param_ptr->type = jackctl_parameter_get_type(param);
    param_ptr->name = jackctl_parameter_get_name(param);
    param_ptr->short_decr = jackctl_parameter_get_short_description(param);
    param_ptr->long_descr = jackctl_parameter_get_long_description(param);

    if (jackctl_parameter_has_range_constraint(param))
    {
        param_ptr->constraint_flags = JACK_CONSTRAINT_FLAG_VALID;
        param_ptr->constraint_range = true;
        jackctl_parameter_get_range_constraint(param, &param_ptr->constraint.range.min, &param_ptr->constraint.range.max);
    }
    else if (jackctl_parameter_has_enum_constraint(param))
    {
        param_ptr->constraint_flags = JACK_CONSTRAINT_FLAG_VALID;
        param_ptr->constraint_range = false;
        param_ptr->constraint.enumeration.count = jackctl_parameter_get_enum_constraints_count(param);
        param_ptr->constraint.enumeration.possible_values_array = malloc(sizeof(struct jack_parameter_enum) * param_ptr->constraint.enumeration.count);
        if (param_ptr->constraint.enumeration.possible_values_array == NULL)
        {
            goto free;
        }

        for (i = 0; i < param_ptr->constraint.enumeration.count; i++)
        {
            param_ptr->constraint.enumeration.possible_values_array[i].value = jackctl_parameter_get_enum_constraint_value(param, i);
            param_ptr->constraint.enumeration.possible_values_array[i].short_desc = jackctl_parameter_get_enum_constraint_description(param, i);
        }
    }
    else
    {
        param_ptr->constraint_flags = 0;
        goto add;
    }

    if (jackctl_parameter_constraint_is_strict(param))
    {
        param_ptr->constraint_flags |= JACK_CONSTRAINT_FLAG_STRICT;
    }

    if (jackctl_parameter_constraint_is_fake_value(param))
    {
        param_ptr->constraint_flags |= JACK_CONSTRAINT_FLAG_FAKE_VALUE;
    }

add:
    list_add_tail(&param_ptr->siblings, parent_list_ptr);
    return true;

free:
    free(param_ptr);
fail:
    return false;
}

static void free_params(struct list_head * parent_list_ptr)
{
    struct jack_parameter * param_ptr;

    while (!list_empty(parent_list_ptr))
    {
        param_ptr = list_entry(parent_list_ptr->next, struct jack_parameter, siblings);
        list_del(&param_ptr->siblings);

        if (param_ptr->ext)
        {
            continue;
        }

        if ((param_ptr->constraint_flags & JACK_CONSTRAINT_FLAG_VALID) != 0 &&
            !param_ptr->constraint_range &&
            param_ptr->constraint.enumeration.possible_values_array != NULL)
        {
            free(param_ptr->constraint.enumeration.possible_values_array);
        }

        free(param_ptr);
    }
}

static void free_containers(struct list_head * parent_list_ptr)
{
    struct jack_parameter_container * container_ptr;

    while (!list_empty(parent_list_ptr))
    {
        container_ptr = list_entry(parent_list_ptr->next, struct jack_parameter_container, siblings);
        list_del(&container_ptr->siblings);

        if (container_ptr->leaf)
        {
            free_params(&container_ptr->children);
        }
        else
        {
            free_containers(&container_ptr->children);
        }

        free(container_ptr->name);
        free(container_ptr);
    }
}

static struct jack_parameter_container * find_container(struct jack_parameter_container * parent_ptr, const char * const * address, int max_depth)
{
    struct list_head * node_ptr;
    struct jack_parameter_container * container_ptr;

    if (max_depth == 0 || *address == NULL)
    {
        return parent_ptr;
    }

    if (parent_ptr->leaf)
    {
        return NULL;
    }

    if (max_depth > 0)
    {
        max_depth--;
    }

    list_for_each(node_ptr, &parent_ptr->children)
    {
        container_ptr = list_entry(node_ptr, struct jack_parameter_container, siblings);
        if (strcmp(container_ptr->name, *address) == 0)
        {
            if (container_ptr->symlink != NULL)
            {
                container_ptr = container_ptr->symlink;
            }

            return find_container(container_ptr, address + 1, max_depth);
        }
    }

    return NULL;
}

static bool init_leaf(struct list_head * parent_list_ptr, const char * name, const JSList * params_list, void * obj)
{
    struct jack_parameter_container * container_ptr;

    container_ptr = create_container(parent_list_ptr, name);
    if (container_ptr == NULL)
    {
        return false;
    }

    container_ptr->leaf = true;
    container_ptr->obj = obj;

    while (params_list)
    {
        if (!add_controlapi_param(&container_ptr->children, params_list->data))
        {
            return false;
        }

        params_list = jack_slist_next(params_list);
    }

    return true;
}

static bool init_engine(struct jack_params * params_ptr)
{
    return init_leaf(&params_ptr->root.children, PTNODE_ENGINE, jackctl_server_get_parameters(params_ptr->server), NULL);
}

static bool init_drivers(struct jack_params * params_ptr)
{
    const JSList * list;
    struct jack_parameter_container * container_ptr;

    container_ptr = create_container(&params_ptr->root.children, PTNODE_DRIVERS);
    if (container_ptr == NULL)
    {
        return false;
    }

    params_ptr->drivers_ptr = &container_ptr->children;
    params_ptr->drivers_count = 0;

    list = jackctl_server_get_drivers_list(params_ptr->server);
    while (list)
    {
        if (!init_leaf(&container_ptr->children, jackctl_driver_get_name(list->data), jackctl_driver_get_parameters(list->data), list->data))
        {
            return false;
        }

        params_ptr->drivers_count++;

        list = jack_slist_next(list);
    }

    return true;
}

static bool init_internals(struct jack_params * params_ptr)
{
    const JSList * list;
    struct jack_parameter_container * container_ptr;

    container_ptr = create_container(&params_ptr->root.children, PTNODE_INTERNALS);
    if (container_ptr == NULL)
    {
        return false;
    }

    list = jackctl_server_get_internals_list(params_ptr->server);
    while (list)
    {
        if (!init_leaf(&container_ptr->children, jackctl_internal_get_name(list->data), jackctl_internal_get_parameters(list->data), NULL))
        {
            return false;
        }

        list = jack_slist_next(list);
    }

    return true;
}

static bool init_driver(struct jack_params * params_ptr)
{
    struct jack_parameter_container * container_ptr;

    container_ptr = create_container(&params_ptr->root.children, PTNODE_DRIVER);
    if (container_ptr == NULL)
    {
        return false;
    }

    params_ptr->driver_ptr = container_ptr;

    return true;
}

#define params_ptr ((struct jack_params *)obj)

static bool engine_driver_parameter_is_set(void * obj)
{
    return params_ptr->driver_set;
}

static bool engine_driver_parameter_reset(void * obj)
{
    if (!jack_params_set_driver(obj, DEFAULT_DRIVER))
    {
        return false;
    }

    params_ptr->driver_set = false;

    return true;
}

union jackctl_parameter_value engine_driver_parameter_get_value(void * obj)
{
    union jackctl_parameter_value value;

    strcpy(value.str, params_ptr->driver_ptr->symlink->name);

    return value;
}

bool engine_driver_parameter_set_value(void * obj, const union jackctl_parameter_value * value_ptr)
{
    return jack_params_set_driver(obj, value_ptr->str);
}

union jackctl_parameter_value engine_driver_parameter_get_default_value(void * obj)
{
    union jackctl_parameter_value value;

    strcpy(value.str, DEFAULT_DRIVER);

    return value;
}

#undef params_ptr

static bool add_engine_driver_enum_constraint(void * context, const char * name)
{
    strcpy((*((struct jack_parameter_enum **)context))->value.str, name);
    (*((struct jack_parameter_enum **)context))->short_desc = name;
    (*((struct jack_parameter_enum **)context))++;
    return true;
}

static bool init_engine_driver_parameter(struct jack_params * params_ptr)
{
    struct jack_parameter * param_ptr;
    const char * address[PARAM_ADDRESS_SIZE] = {PTNODE_ENGINE, NULL};
    struct jack_parameter_container * engine_ptr;
    struct jack_parameter_enum * possible_value;

    engine_ptr = find_container(&params_ptr->root, address, PARAM_ADDRESS_SIZE);
    if (engine_ptr == NULL)
    {
        return false;
    }

    param_ptr = malloc(sizeof(struct jack_parameter));
    if (param_ptr == NULL)
    {
        jack_error("Ran out of memory trying to allocate struct jack_parameter");
        goto fail;
    }

    param_ptr->ext = false;
    param_ptr->obj = params_ptr;
    param_ptr->vtable.is_set = engine_driver_parameter_is_set;
    param_ptr->vtable.reset = engine_driver_parameter_reset;
    param_ptr->vtable.get_value = engine_driver_parameter_get_value;
    param_ptr->vtable.set_value = engine_driver_parameter_set_value;
    param_ptr->vtable.get_default_value = engine_driver_parameter_get_default_value;

    param_ptr->type = JackParamString;
    param_ptr->name = "driver";
    param_ptr->short_decr = "Driver to use";
    param_ptr->long_descr = "";

    param_ptr->constraint_flags = JACK_CONSTRAINT_FLAG_VALID | JACK_CONSTRAINT_FLAG_STRICT | JACK_CONSTRAINT_FLAG_FAKE_VALUE;
    param_ptr->constraint_range = false;
    param_ptr->constraint.enumeration.count = params_ptr->drivers_count;
    param_ptr->constraint.enumeration.possible_values_array = malloc(sizeof(struct jack_parameter_enum) * params_ptr->drivers_count);
    if (param_ptr->constraint.enumeration.possible_values_array == NULL)
    {
        goto free;
    }

    address[0] = PTNODE_DRIVERS;
    possible_value = param_ptr->constraint.enumeration.possible_values_array;
    jack_params_iterate_container((jack_params_handle)params_ptr, address, add_engine_driver_enum_constraint, &possible_value);

    list_add(&param_ptr->siblings, &engine_ptr->children);
    return true;

free:
    free(param_ptr);
fail:
    return false;
}

jack_params_handle jack_params_create(jackctl_server_t * server)
{
    struct jack_params * params_ptr;

    params_ptr = malloc(sizeof(struct jack_params));
    if (params_ptr == NULL)
    {
        jack_error("Ran out of memory trying to allocate struct jack_params");
        return NULL;
    }

    params_ptr->server = server;
    INIT_LIST_HEAD(&params_ptr->root.children);
    params_ptr->root.leaf = false;
    params_ptr->root.name = NULL;

    if (!init_engine(params_ptr) ||
        !init_drivers(params_ptr) ||
        !init_driver(params_ptr) ||
        !init_engine_driver_parameter(params_ptr) ||
        !jack_params_set_driver((jack_params_handle)params_ptr, DEFAULT_DRIVER) ||
        !init_internals(params_ptr))
    {
        jack_params_destroy((jack_params_handle)params_ptr);
        return NULL;
    }

    params_ptr->driver_set = false;

    assert(strcmp(params_ptr->driver_ptr->symlink->name, DEFAULT_DRIVER) == 0);

    return (jack_params_handle)params_ptr;
}

#define params_ptr ((struct jack_params *)params)

void jack_params_destroy(jack_params_handle params)
{
    free_containers(&params_ptr->root.children);
    free(params);
}

bool jack_params_set_driver(jack_params_handle params, const char * name)
{
    struct list_head * node_ptr;
    struct jack_parameter_container * container_ptr;

    list_for_each(node_ptr, params_ptr->drivers_ptr)
    {
        container_ptr = list_entry(node_ptr, struct jack_parameter_container, siblings);
        if (strcmp(container_ptr->name, name) == 0)
        {
            params_ptr->driver_ptr->symlink = container_ptr;
            params_ptr->driver_set = true;
            return true;
        }
    }

    return false;
}

jackctl_driver_t * jack_params_get_driver(jack_params_handle params)
{
    return params_ptr->driver_ptr->symlink->obj;
}

bool jack_params_check_address(jack_params_handle params, const char * const * address, bool want_leaf)
{
    struct jack_parameter_container * container_ptr;

    container_ptr = find_container(&params_ptr->root, address, PARAM_ADDRESS_SIZE);
    if (container_ptr == NULL)
    {
        return false;
    }

    if (want_leaf && !container_ptr->leaf)
    {
        return false;
    }

    return true;
}

bool jack_params_is_leaf_container(jack_params_handle params, const char * const * address)
{
    struct jack_parameter_container * container_ptr;

    container_ptr = find_container(&params_ptr->root, address, PARAM_ADDRESS_SIZE);
    if (container_ptr == NULL)
    {
        assert(false);
        return false;
    }

    return container_ptr->leaf;
}

bool
jack_params_iterate_container(
    jack_params_handle params,
    const char * const * address,
    bool (* callback)(void * context, const char * name),
    void * context)
{
    struct jack_parameter_container * container_ptr;
    struct list_head * node_ptr;
    const char * name;

    container_ptr = find_container(&params_ptr->root, address, PARAM_ADDRESS_SIZE);
    if (container_ptr == NULL)
    {
        assert(false);
        return true;
    }

    list_for_each(node_ptr, &container_ptr->children)
    {
        if (container_ptr->leaf)
        {
            name = list_entry(node_ptr, struct jack_parameter, siblings)->name;
        }
        else
        {
            name = list_entry(node_ptr, struct jack_parameter_container, siblings)->name;
        }

        if (!callback(context, name))
        {
            return false;
        }
    }

    return true;
}

bool
jack_params_iterate_params(
    jack_params_handle params,
    const char * const * address,
    bool (* callback)(void * context, const struct jack_parameter * param_ptr),
    void * context)
{
    struct jack_parameter_container * container_ptr;
    struct list_head * node_ptr;
    struct jack_parameter * param_ptr;

    container_ptr = find_container(&params_ptr->root, address, PARAM_ADDRESS_SIZE);
    if (container_ptr == NULL || !container_ptr->leaf)
    {
        assert(false);
        return true;
    }

    list_for_each(node_ptr, &container_ptr->children)
    {
        param_ptr = list_entry(node_ptr, struct jack_parameter, siblings);
        if (!callback(context, param_ptr))
        {
            return false;
        }
    }

    return true;
}

const struct jack_parameter * jack_params_get_parameter(jack_params_handle params, const char * const * address)
{
    int depth;
    struct jack_parameter_container * container_ptr;
    struct list_head * node_ptr;
    struct jack_parameter * param_ptr;

    for (depth = 0; depth < PARAM_ADDRESS_SIZE; depth++)
    {
        if (address[depth] == NULL)
        {
            break;
        }
    }

    depth--;

    container_ptr = find_container(&params_ptr->root, address, depth);
    if (container_ptr == NULL || !container_ptr->leaf)
    {
        return NULL;
    }

    list_for_each(node_ptr, &container_ptr->children)
    {
        param_ptr = list_entry(node_ptr, struct jack_parameter, siblings);
        if (strcmp(param_ptr->name, address[depth]) == 0)
        {
            return param_ptr;
        }
    }

    return NULL;
}

void jack_params_add_parameter(jack_params_handle params, const char * const * address, bool end, struct jack_parameter * param_ptr)
{
    struct jack_parameter_container * container_ptr;

    container_ptr = find_container(&params_ptr->root, address, PARAM_ADDRESS_SIZE);
    if (container_ptr == NULL || !container_ptr->leaf)
    {
        assert(false);
        return;
    }

    param_ptr->ext = true;

    if (end)
    {
        list_add_tail(&param_ptr->siblings, &container_ptr->children);
    }
    else
    {
        list_add(&param_ptr->siblings, &container_ptr->children);
    }

    return;
}

#undef params_ptr
