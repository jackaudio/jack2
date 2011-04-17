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

#ifndef PARAMS_H__A23EDE06_C1C9_4489_B253_FD1B26B66929__INCLUDED
#define PARAMS_H__A23EDE06_C1C9_4489_B253_FD1B26B66929__INCLUDED

#include "jack/control.h"
#include "list.h"

#define PARAM_ADDRESS_SIZE 3

#define PTNODE_ENGINE    "engine"
#define PTNODE_DRIVER    "driver"
#define PTNODE_DRIVERS   "drivers"
#define PTNODE_INTERNALS "internals"

struct jack_parameter_vtable
{
    bool                          (* is_set)(void * obj);
    bool                          (* reset)(void * obj);
    union jackctl_parameter_value (* get_value)(void * obj);
    bool                          (* set_value)(void * obj, const union jackctl_parameter_value * value_ptr);
    union jackctl_parameter_value (* get_default_value)(void * obj);
};

#define JACK_CONSTRAINT_FLAG_VALID       ((uint32_t)1) /**< if not set, there is no constraint */
#define JACK_CONSTRAINT_FLAG_STRICT      ((uint32_t)2) /**< if set, constraint is strict, i.e. supplying non-matching value will not work */
#define JACK_CONSTRAINT_FLAG_FAKE_VALUE  ((uint32_t)4) /**< if set, values have no user meaningful meaning */

struct jack_parameter_enum
{
    union jackctl_parameter_value value;
    const char * short_desc;
};

struct jack_parameter
{
    void * obj;
    struct jack_parameter_vtable vtable;
    struct list_head siblings;
    bool ext;
    jackctl_param_type_t type;
    const char * name;
    const char * short_decr;
    const char * long_descr;

    uint32_t constraint_flags;  /**< JACK_CONSTRAINT_FLAG_XXX */
    bool constraint_range; /**< if true, constraint is a range (min-max), otherwise it is an enumeration */

    union
    {
        struct
        {
            union jackctl_parameter_value min;
            union jackctl_parameter_value max;
        } range; /**< valid when JACK_CONSTRAINT_FLAG_RANGE flag is set */

        struct
        {
            uint32_t count;
            struct jack_parameter_enum * possible_values_array;
        } enumeration; /**< valid when JACK_CONSTRAINT_FLAG_RANGE flag is not set */
    } constraint;
};

typedef struct _jack_params { int unused; } * jack_params_handle;

jack_params_handle jack_params_create(jackctl_server_t * server);
void jack_params_destroy(jack_params_handle params);

bool jack_params_set_driver(jack_params_handle params, const char * name);
jackctl_driver_t * jack_params_get_driver(jack_params_handle params);

bool jack_params_check_address(jack_params_handle params, const char * const * address, bool want_leaf);
bool jack_params_is_leaf_container(jack_params_handle params, const char * const * address);

bool
jack_params_iterate_container(
    jack_params_handle params,
    const char * const * address,
    bool (* callback)(void * context, const char * name),
    void * context);

bool
jack_params_iterate_params(
    jack_params_handle params,
    const char * const * address,
    bool (* callback)(void * context, const struct jack_parameter * param_ptr),
    void * context);

const struct jack_parameter * jack_params_get_parameter(jack_params_handle params, const char * const * address);

void jack_params_add_parameter(jack_params_handle params, const char * const * address, bool end, struct jack_parameter * param_ptr);

#endif /* #ifndef PARAMS_H__A23EDE06_C1C9_4489_B253_FD1B26B66929__INCLUDED */
