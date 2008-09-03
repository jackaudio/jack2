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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <expat.h>
#include <dbus/dbus.h>

#include "controller_internal.h"
#include "jackdbus.h"

bool
jack_controller_settings_init()
{
    return true;
}

void
jack_controller_settings_uninit()
{
}

#define PARSE_CONTEXT_ROOT        0
#define PARSE_CONTEXT_JACK        1
#define PARSE_CONTEXT_ENGINE      1
#define PARSE_CONTEXT_DRIVERS     2
#define PARSE_CONTEXT_DRIVER      3
#define PARSE_CONTEXT_OPTION      4
#define PARSE_CONTEXT_INTERNALS   5
#define PARSE_CONTEXT_INTERNAL    6

#define MAX_STACK_DEPTH       10

struct parse_context
{
    struct jack_controller *controller_ptr;
    XML_Bool error;
    unsigned int element[MAX_STACK_DEPTH];
    signed int depth;
    jackctl_driver_t *driver;
    jackctl_internal_t *internal;
    char option[JACK_PARAM_STRING_MAX+1];
    int option_used;
    char *name;
};

#define context_ptr ((struct parse_context *)data)

void
jack_controller_settings_callback_chrdata(void *data, const XML_Char *s, int len)
{
    if (context_ptr->error)
    {
        return;
    }

    if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_OPTION)
    {
        if (context_ptr->option_used + len >= JACK_PARAM_STRING_MAX)
        {
            jack_error("xml parse max char data length reached");
            context_ptr->error = XML_TRUE;
            return;
        }

        memcpy(context_ptr->option + context_ptr->option_used, s, len);
        context_ptr->option_used += len;
    }
}

void
jack_controller_settings_callback_elstart(void *data, const char *el, const char **attr)
{
    jackctl_driver_t *driver;
    jackctl_internal_t *internal;

    if (context_ptr->error)
    {
        return;
    }

    if (context_ptr->depth + 1 >= MAX_STACK_DEPTH)
    {
        jack_error("xml parse max stack depth reached");
        context_ptr->error = XML_TRUE;
        return;
    }

    if (strcmp(el, "jack") == 0)
    {
        //jack_info("<jack>");
        context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_JACK;
        return;
    }

    if (strcmp(el, "engine") == 0)
    {
        //jack_info("<engine>");
        context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_ENGINE;
        return;
    }

    if (strcmp(el, "drivers") == 0)
    {
        //jack_info("<drivers>");
        context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_DRIVERS;
        return;
    }
    
    if (strcmp(el, "internals") == 0)
    {
        //jack_info("<internals>");
        context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_INTERNALS;
        return;
    }

    if (strcmp(el, "driver") == 0)
    {
        if ((attr[0] == NULL || attr[2] != NULL) || strcmp(attr[0], "name") != 0)
        {
            jack_error("<driver> XML element must contain exactly one attribute, named \"name\"");
            context_ptr->error = XML_TRUE;
            return;
        }

        //jack_info("<driver>");
        context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_DRIVER;

        driver = jack_controller_find_driver(context_ptr->controller_ptr->server, attr[1]);
        if (driver == NULL)
        {
            jack_error("ignoring settings for unknown driver \"%s\"", attr[1]);
        }
        else
        {
            jack_info("setting for driver \"%s\" found", attr[1]);
        }

        context_ptr->driver = driver;

        return;
    }
    
    if (strcmp(el, "internal") == 0)
    {
        if ((attr[0] == NULL || attr[2] != NULL) || strcmp(attr[0], "name") != 0)
        {
            jack_error("<internal> XML element must contain exactly one attribute, named \"name\"");
            context_ptr->error = XML_TRUE;
            return;
        }

        //jack_info("<internal>");
        context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_INTERNAL;

        internal = jack_controller_find_internal(context_ptr->controller_ptr->server, attr[1]);
        if (internal == NULL)
        {
            jack_error("ignoring settings for unknown internal \"%s\"", attr[1]);
        }
        else
        {
            jack_info("setting for internal \"%s\" found", attr[1]);
        }

        context_ptr->internal = internal;

        return;
    }


    if (strcmp(el, "option") == 0)
    {
        //jack_info("<option>");
        if ((attr[0] == NULL || attr[2] != NULL) || strcmp(attr[0], "name") != 0)
        {
            jack_error("<option> XML element must contain exactly one attribute, named \"name\"");
            context_ptr->error = XML_TRUE;
            return;
        }

        context_ptr->name = strdup(attr[1]);
        if (context_ptr->name == NULL)
        {
            jack_error("strdup() failed");
            context_ptr->error = XML_TRUE;
            return;
        }

        context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_OPTION;
        context_ptr->option_used = 0;
        return;
    }

    jack_error("unknown element \"%s\"", el);
    context_ptr->error = XML_TRUE;
}

void
jack_controller_settings_callback_elend(void *data, const char *el)
{
    if (context_ptr->error)
    {
        return;
    }

    //jack_info("element end (depth = %d, element = %u)", context_ptr->depth, context_ptr->element[context_ptr->depth]);

    if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_OPTION)
    {
        context_ptr->option[context_ptr->option_used] = 0;

        if (context_ptr->depth == 2 &&
            context_ptr->element[0] == PARSE_CONTEXT_JACK &&
            context_ptr->element[1] == PARSE_CONTEXT_ENGINE)
        {
            jack_controller_settings_set_engine_option(context_ptr->controller_ptr, context_ptr->name, context_ptr->option);
        }

        if (context_ptr->depth == 3 &&
            context_ptr->element[0] == PARSE_CONTEXT_JACK &&
            context_ptr->element[1] == PARSE_CONTEXT_DRIVERS &&
            context_ptr->element[2] == PARSE_CONTEXT_DRIVER &&
            context_ptr->driver != NULL)
        {
            jack_controller_settings_set_driver_option(context_ptr->driver, context_ptr->name, context_ptr->option);
        }
        
        if (context_ptr->depth == 3 &&
            context_ptr->element[0] == PARSE_CONTEXT_JACK &&
            context_ptr->element[1] == PARSE_CONTEXT_INTERNALS &&
            context_ptr->element[2] == PARSE_CONTEXT_INTERNAL &&
            context_ptr->internal != NULL)
        {
            jack_controller_settings_set_internal_option(context_ptr->internal, context_ptr->name, context_ptr->option);
        }
    }

    context_ptr->depth--;

    if (context_ptr->name != NULL)
    {
        free(context_ptr->name);
        context_ptr->name = NULL;
    }
}

#undef context_ptr

void
jack_controller_settings_load(
    struct jack_controller * controller_ptr)
{
    XML_Parser parser;
    int bytes_read;
    void *buffer;
    char *filename;
    size_t conf_len;
    struct stat st;
    int fd;
    enum XML_Status xmls;
    struct parse_context context;

    conf_len = strlen(JACKDBUS_CONF);

    filename = malloc(g_jackdbus_config_dir_len + conf_len + 1);
    if (filename == NULL)
    {
        jack_error("Out of memory.");
        goto exit;
    }

    memcpy(filename, g_jackdbus_config_dir, g_jackdbus_config_dir_len);
    memcpy(filename + g_jackdbus_config_dir_len, JACKDBUS_CONF, conf_len);
    filename[g_jackdbus_config_dir_len + conf_len] = 0;

    jack_info("Loading settings from \"%s\" using %s ...", filename, XML_ExpatVersion());

    if (stat(filename, &st) != 0)
    {
        jack_error("failed to stat \"%s\", error is %d (%s)", filename, errno, strerror(errno));
    }

    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        jack_error("open() failed to open conf filename.");
        goto exit_free_filename;
    }

    parser = XML_ParserCreate(NULL);
    if (parser == NULL)
    {
        jack_error("XML_ParserCreate() failed to create parser object.");
        goto exit_close_file;
    }

    //jack_info("conf file size is %llu bytes", (unsigned long long)st.st_size);

    /* we are expecting that conf file has small enough size to fit in memory */

    buffer = XML_GetBuffer(parser, st.st_size);
    if (buffer == NULL)
    {
        jack_error("XML_GetBuffer() failed.");
        goto exit_free_parser;
    }

    bytes_read = read(fd, buffer, st.st_size);
    if (bytes_read != st.st_size)
    {
        jack_error("read() returned unexpected result.");
        goto exit_free_parser;
    }

    context.controller_ptr = controller_ptr;
    context.error = XML_FALSE;
    context.depth = -1;
    context.name = NULL;

    XML_SetElementHandler(parser, jack_controller_settings_callback_elstart, jack_controller_settings_callback_elend);
    XML_SetCharacterDataHandler(parser, jack_controller_settings_callback_chrdata);
    XML_SetUserData(parser, &context);

    xmls = XML_ParseBuffer(parser, bytes_read, XML_TRUE);
    if (xmls == XML_STATUS_ERROR)
    {
        jack_error("XML_ParseBuffer() failed.");
        goto exit_free_parser;
    }

exit_free_parser:
    XML_ParserFree(parser);

exit_close_file:
    close(fd);

exit_free_filename:
    free(filename);

exit:
    return;
}
