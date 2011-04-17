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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
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

struct parse_context
{
    struct jack_controller *controller_ptr;
    XML_Bool error;
    bool option_value_capture;
    char option[JACK_PARAM_STRING_MAX+1];
    int option_used;
    const char * address[PARAM_ADDRESS_SIZE];
    int address_index;
    char * container;
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

    if (context_ptr->option_value_capture)
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
    if (context_ptr->error)
    {
        return;
    }

    if (context_ptr->address_index >= PARAM_ADDRESS_SIZE)
    {
        assert(context_ptr->address_index == PARAM_ADDRESS_SIZE);
        jack_error("xml param address max depth reached");
        context_ptr->error = XML_TRUE;
        return;
    }

    //jack_info("<%s>", el);

    if (strcmp(el, "jack") == 0)
    {
        return;
    }

    if (strcmp(el, PTNODE_ENGINE) == 0)
    {
        context_ptr->address[context_ptr->address_index++] = PTNODE_ENGINE;
        return;
    }

    if (strcmp(el, PTNODE_DRIVERS) == 0)
    {
        context_ptr->address[context_ptr->address_index++] = PTNODE_DRIVERS;
        return;
    }
    
    if (strcmp(el, PTNODE_INTERNALS) == 0)
    {
        context_ptr->address[context_ptr->address_index++] = PTNODE_INTERNALS;
        return;
    }

    if (strcmp(el, "driver") == 0 ||
        strcmp(el, "internal") == 0)
    {
        if ((attr[0] == NULL || attr[2] != NULL) || strcmp(attr[0], "name") != 0)
        {
            jack_error("<%s> XML element must contain exactly one attribute, named \"name\"", el);
            context_ptr->error = XML_TRUE;
            return;
        }

        context_ptr->container = strdup(attr[1]);
        if (context_ptr->container == NULL)
        {
            jack_error("strdup() failed");
            context_ptr->error = XML_TRUE;
            return;
        }

        context_ptr->address[context_ptr->address_index++] = context_ptr->container;

        return;
    }
    
    if (strcmp(el, "option") == 0)
    {
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

        context_ptr->address[context_ptr->address_index++] = context_ptr->name;
        context_ptr->option_value_capture = true;
        context_ptr->option_used = 0;
        return;
    }

    jack_error("unknown element \"%s\"", el);
    context_ptr->error = XML_TRUE;
}

void
jack_controller_settings_callback_elend(void *data, const char *el)
{
    int i;

    if (context_ptr->error)
    {
        return;
    }

    //jack_info("</%s> (depth = %d)", el, context_ptr->address_index);

    if (strcmp(el, "option") == 0)
    {
        assert(context_ptr->option_value_capture);
        context_ptr->option[context_ptr->option_used] = 0;

        for (i = context_ptr->address_index; i < PARAM_ADDRESS_SIZE; i++)
        {
            context_ptr->address[context_ptr->address_index] = NULL;
        }

        jack_controller_deserialize_parameter_value(context_ptr->controller_ptr, context_ptr->address, context_ptr->option);

        free(context_ptr->name);
        context_ptr->name = NULL;
        context_ptr->option_value_capture = false;
        context_ptr->address_index--;
    }
    else if (context_ptr->container != NULL)
    {
        //jack_info("'%s'", context_ptr->container);
        free(context_ptr->container);
        context_ptr->container = NULL;
        context_ptr->address_index--;
    }
    else if (strcmp(el, PTNODE_ENGINE) == 0 ||
             strcmp(el, PTNODE_DRIVERS) == 0 ||
             strcmp(el, PTNODE_INTERNALS) == 0)
    {
        context_ptr->address_index--;
    }
    else
    {
        //jack_info("no depth decrement");
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
    context.option_value_capture = false;
    context.address_index = 0;
    context.name = NULL;
    context.container = NULL;

    XML_SetElementHandler(parser, jack_controller_settings_callback_elstart, jack_controller_settings_callback_elend);
    XML_SetCharacterDataHandler(parser, jack_controller_settings_callback_chrdata);
    XML_SetUserData(parser, &context);

    xmls = XML_ParseBuffer(parser, bytes_read, XML_TRUE);

    free(context.name);
    free(context.container);

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
