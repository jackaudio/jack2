/*
  Copyright (C) 2003 Bob Ham <rah@bash.sh>
  Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>

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

#ifndef __jack_driver_interface_h__
#define __jack_driver_interface_h__

#ifdef __cplusplus
extern "C"
{
#endif

#include <limits.h>
#include "jslist.h"
#include "JackCompilerDeps.h"
#include "JackSystemDeps.h"

#define JACK_DRIVER_NAME_MAX          15
#define JACK_DRIVER_PARAM_NAME_MAX    15
#define JACK_DRIVER_PARAM_STRING_MAX  127
#define JACK_DRIVER_PARAM_DESC        255
#define JACK_PATH_MAX                 511

#define JACK_CONSTRAINT_FLAG_RANGE       ((uint32_t)1) /**< if set, constraint is a range (min-max) */
#define JACK_CONSTRAINT_FLAG_STRICT      ((uint32_t)2) /**< if set, constraint is strict, i.e. supplying non-matching value will not work */
#define JACK_CONSTRAINT_FLAG_FAKE_VALUE  ((uint32_t)4) /**< if set, values have no user meaningful meaning */

    /** Driver parameter types */
    typedef enum
    {
        JackDriverParamInt = 1,
        JackDriverParamUInt,
        JackDriverParamChar,
        JackDriverParamString,
        JackDriverParamBool
    } jack_driver_param_type_t;

    /** Driver parameter value */
    typedef union
    {
        uint32_t ui;
        int32_t i;
        char c;
        char str[JACK_DRIVER_PARAM_STRING_MAX + 1];
    } jack_driver_param_value_t;

    typedef struct {
        jack_driver_param_value_t value;
        char short_desc[64];               /**< A short (~30 chars) description for the user */
    } jack_driver_param_value_enum_t;

    typedef struct {
        uint32_t flags;         /**< JACK_CONSTRAINT_FLAG_XXX */

        union {
            struct {
                jack_driver_param_value_t min;
                jack_driver_param_value_t max;
            } range;            /**< valid when JACK_CONSTRAINT_FLAG_RANGE flag is set */

            struct {
                uint32_t count;
                jack_driver_param_value_enum_t * possible_values_array;
            } enumeration;      /**< valid when JACK_CONSTRAINT_FLAG_RANGE flag is not set */
        } constraint;
    } jack_driver_param_constraint_desc_t;

    /** A driver parameter descriptor */
    typedef struct {
        char name[JACK_DRIVER_NAME_MAX + 1]; /**< The parameter's name */
        char character;                    /**< The parameter's character (for getopt, etc) */
        jack_driver_param_type_t type;     /**< The parameter's type */
        jack_driver_param_value_t value;   /**< The parameter's (default) value */
        jack_driver_param_constraint_desc_t * constraint; /**< Pointer to parameter constraint descriptor. NULL if there is no constraint */
        char short_desc[64];               /**< A short (~30 chars) description for the user */
        char long_desc[1024];              /**< A longer description for the user */
    }
    jack_driver_param_desc_t;

    /** A driver parameter */
    typedef struct {
        char character;
        jack_driver_param_value_t value;
    }
    jack_driver_param_t;


    /** A struct for describing a jack driver */
    typedef struct {
        char name[JACK_DRIVER_NAME_MAX + 1];      /**< The driver's canonical name */
        char desc[JACK_DRIVER_PARAM_DESC + 1];    /**< The driver's extended description */
        char file[JACK_PATH_MAX + 1];             /**< The filename of the driver's shared object file */
        uint32_t nparams;                         /**< The number of parameters the driver has */
        jack_driver_param_desc_t * params;        /**< An array of parameter descriptors */
    }
    jack_driver_desc_t;


SERVER_EXPORT int jack_parse_driver_params (jack_driver_desc_t * desc, int argc, char* argv[], JSList ** param_ptr);

#ifdef __cplusplus
}
#endif

#endif /* __jack_driver_interface_h__ */


