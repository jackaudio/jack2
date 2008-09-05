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
#include <string.h>
#include <dbus/dbus.h>

#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <jack/driver.h>
#include <jack/engine.h>
#include "controller_internal.h"
#include "dbus.h"

/* XPath expression used for engine options selection */
#define XPATH_ENGINE_OPTIONS_EXPRESSION "/jack/engine/option"

/* XPath expression used for drivers selection */
#define XPATH_DRIVERS_EXPRESSION "/jack/drivers/driver"

/* XPath expression used for driver options selection */
#define XPATH_DRIVER_OPTIONS_EXPRESSION "/jack/drivers/driver[@name = '%s']/option"

bool
jack_controller_settings_init()
{
    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION;

    return true;
}

void
jack_controller_settings_uninit()
{
}

#define writer ((xmlTextWriterPtr)context)

bool
jack_controller_settings_write_option(
    void *context,
    const char *name,
    const char *content,
    void *dbus_call_context_ptr)
{
    if (xmlTextWriterStartElement(writer, BAD_CAST "option") == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        return false;
    }

    if (xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST name) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteAttribute() failed.");
        return false;
    }

    if (xmlTextWriterWriteString(writer, BAD_CAST content) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteString() failed.");
        return false;
    }

    if (xmlTextWriterEndElement(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterEndElement() failed.");
        return false;
    }

    return true;
}

#undef writer

bool
jack_controller_settings_write_engine(
    struct jack_controller * controller_ptr,
    xmlTextWriterPtr writer,
    void *dbus_call_context_ptr)
{
/*  jack_info("engine settings begin"); */

/*  if (xmlTextWriterWriteComment(writer, BAD_CAST "engine parameters") == -1) */
/*  { */
/*      jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteComment() failed."); */
/*      return false; */
/*  } */

    if (xmlTextWriterStartElement(writer, BAD_CAST "engine") == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        return false;
    }

    if (!jack_controller_settings_save_engine_options(writer, controller_ptr, dbus_call_context_ptr))
    {
        return false;
    }

    if (xmlTextWriterEndElement(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterEndElement() failed.");
        return false;
    }

/*  jack_info("engine settings end"); */
    return true;
}

bool
jack_controller_settings_write_driver(
    struct jack_controller * controller_ptr,
    xmlTextWriterPtr writer,
    jackctl_driver driver,
    void *dbus_call_context_ptr)
{
/*  if (xmlTextWriterWriteComment(writer, BAD_CAST "driver parameters") == -1) */
/*  { */
/*      jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteComment() failed."); */
/*      return false; */
/*  } */

    if (xmlTextWriterStartElement(writer, BAD_CAST "driver") == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        return false;
    }

    if (xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST jackctl_driver_get_name(driver)) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteAttribute() failed.");
        return false;
    }

    if (!jack_controller_settings_save_driver_options(writer, driver, dbus_call_context_ptr))
    {
        return false;
    }

    if (xmlTextWriterEndElement(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterEndElement() failed.");
        return false;
    }

    return true;
}

bool
jack_controller_settings_write_drivers(
    struct jack_controller * controller_ptr,
    xmlTextWriterPtr writer,
    void *dbus_call_context_ptr)
{
    const JSList * node_ptr;
    jackctl_driver driver;

    if (xmlTextWriterStartElement(writer, BAD_CAST "drivers") == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        return false;
    }

    node_ptr = jackctl_server_get_drivers_list(controller_ptr->server);

    while (node_ptr != NULL)
    {
        driver = (jackctl_driver)node_ptr->data;

        if (!jack_controller_settings_write_driver(
                controller_ptr,
                writer,
                driver,
                dbus_call_context_ptr))
        {
            return false;
        }

        node_ptr = jack_slist_next(node_ptr);
    }

    if (xmlTextWriterEndElement(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterEndElement() failed.");
        return false;
    }

    return true;
}

bool
jack_controller_settings_write_internal(
    struct jack_controller * controller_ptr,
    xmlTextWriterPtr writer,
    jackctl_internal internal,
    void *dbus_call_context_ptr)
{
/*  if (xmlTextWriterWriteComment(writer, BAD_CAST "driver parameters") == -1) */
/*  { */
/*      jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteComment() failed."); */
/*      return false; */
/*  } */

    if (xmlTextWriterStartElement(writer, BAD_CAST "internal") == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        return false;
    }

    if (xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST jackctl_internal_get_name(driver)) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteAttribute() failed.");
        return false;
    }

    if (!jack_controller_settings_save_internal_options(writer, internal, dbus_call_context_ptr))
    {
        return false;
    }

    if (xmlTextWriterEndElement(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterEndElement() failed.");
        return false;
    }

    return true;
}

bool
jack_controller_settings_write_internals(
    struct jack_controller * controller_ptr,
    xmlTextWriterPtr writer,
    void *dbus_call_context_ptr)
{
    const JSList * node_ptr;
    jackctl_driver internal;

    if (xmlTextWriterStartElement(writer, BAD_CAST "internals") == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        return false;
    }

    node_ptr = jackctl_server_get_internals_list(controller_ptr->server);

    while (node_ptr != NULL)
    {
        internal = (jackctl_internal)node_ptr->data;

        if (!jack_controller_settings_write_internal(
                controller_ptr,
                writer,
                internal,
                dbus_call_context_ptr))
        {
            return false;
        }

        node_ptr = jack_slist_next(node_ptr);
    }

    if (xmlTextWriterEndElement(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterEndElement() failed.");
        return false;
    }

    return true;
}

bool
jack_controller_settings_save(
    struct jack_controller * controller_ptr,
    void *dbus_call_context_ptr)
{
    xmlTextWriterPtr writer;
    char *filename;
    size_t conf_len;
    bool ret;
    time_t timestamp;
    char timestamp_str[28];

    time(&timestamp);
    timestamp_str[0] = ' ';
    ctime_r(&timestamp, timestamp_str + 1);
    timestamp_str[25] = ' ';

    ret = false;

    conf_len = strlen(JACKDBUS_CONF);

    filename = malloc(g_jackdbus_dir_len + conf_len + 1);
    if (filename == NULL)
    {
        jack_error("Out of memory.");
        goto fail;
    }

    memcpy(filename, g_jackdbus_dir, g_jackdbus_dir_len);
    memcpy(filename + g_jackdbus_dir_len, JACKDBUS_CONF, conf_len);
    filename[g_jackdbus_dir_len + conf_len] = 0;

    jack_info("saving settings to \"%s\"", filename);

    writer = xmlNewTextWriterFilename(filename, 0);
    if (writer == NULL)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Error creating the xml writer.");
        goto fail_free_filename;
    }

    if (xmlTextWriterSetIndent(writer, 1) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterSetIndent() failed.");
        goto fail_free_writter;
    }

    if (xmlTextWriterStartDocument(writer, NULL, NULL, NULL) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartDocument() failed.");
        goto fail_free_writter;
    }

    if (xmlTextWriterWriteComment(writer, BAD_CAST "\n" JACK_CONF_HEADER_TEXT) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteComment() failed.");
        goto fail_free_writter;
    }

    if (xmlTextWriterWriteComment(writer, BAD_CAST timestamp_str) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterWriteComment() failed.");
        goto fail_free_writter;
    }

    if (xmlTextWriterStartElement(writer, BAD_CAST "jack") == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        goto fail_free_writter;
    }

    if (!jack_controller_settings_write_engine(controller_ptr, writer, dbus_call_context_ptr))
    {
        goto fail_free_writter;
    }

    if (!jack_controller_settings_write_drivers(controller_ptr, writer, dbus_call_context_ptr))
    {
        goto fail_free_writter;
    }
    
    if (!jack_controller_settings_write_internals(controller_ptr, writer, dbus_call_context_ptr))
    {
        goto fail_free_writter;
    }

    if (xmlTextWriterEndElement(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterStartElement() failed.");
        goto fail_free_writter;
    }

    if (xmlTextWriterEndDocument(writer) == -1)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "xmlTextWriterEndDocument() failed.");
        goto fail_free_writter;
    }

    ret = true;

fail_free_writter:
    xmlFreeTextWriter(writer);

fail_free_filename:
    free(filename);

fail:
    return ret;
}

void
jack_controller_settings_read_engine(
    struct jack_controller * controller_ptr,
    xmlXPathContextPtr xpath_ctx_ptr)
{
    xmlXPathObjectPtr xpath_obj_ptr;
    xmlBufferPtr content_buffer_ptr;
    int i;
    const char *option_name;
    const char *option_value;

    /* Evaluate xpath expression */
    xpath_obj_ptr = xmlXPathEvalExpression((const xmlChar *)XPATH_ENGINE_OPTIONS_EXPRESSION, xpath_ctx_ptr);
    if (xpath_obj_ptr == NULL)
    {
        jack_error("Unable to evaluate XPath expression \"%s\"", XPATH_ENGINE_OPTIONS_EXPRESSION);
        goto exit;
    }

    if (xpath_obj_ptr->nodesetval == NULL || xpath_obj_ptr->nodesetval->nodeNr == 0)
    {
        jack_error("XPath \"%s\" evaluation returned no data", XPATH_ENGINE_OPTIONS_EXPRESSION);
        goto free_xpath_obj;
    }

    content_buffer_ptr = xmlBufferCreate();
    if (content_buffer_ptr == NULL)
    {
        jack_error("xmlBufferCreate() failed.");
        goto free_xpath_obj;
    }

    for (i = 0 ; i < xpath_obj_ptr->nodesetval->nodeNr ; i++)
    {
        //jack_info("engine option \"%s\" at index %d", xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name"), i);

        if (xmlNodeBufGetContent(content_buffer_ptr, xpath_obj_ptr->nodesetval->nodeTab[i]) == -1)
        {
            jack_error("xmlNodeBufGetContent() failed.");
            goto next_option;
        }

        option_name = (const char *)xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name");
        option_value = (const char *)xmlBufferContent(content_buffer_ptr);

        jack_controller_settings_set_engine_option(controller_ptr, option_name, option_value);

    next_option:
        xmlBufferEmpty(content_buffer_ptr);
    }

//free_buffer:
    xmlBufferFree(content_buffer_ptr);

free_xpath_obj:
    xmlXPathFreeObject(xpath_obj_ptr);

exit:
    return;
}

void
jack_controller_settings_read_driver(
    struct jack_controller * controller_ptr,
    xmlXPathContextPtr xpath_ctx_ptr,
    jackctl_driver driver)
{
    char *xpath;
    size_t xpath_len;
    xmlXPathObjectPtr xpath_obj_ptr;
    xmlBufferPtr content_buffer_ptr;
    int i;
    const char *option_name;
    const char *option_value;
    const char *driver_name;

    driver_name = jackctl_driver_get_name(driver);

    jack_info("reading options for driver \"%s\"", driver_name);

    xpath_len = snprintf(NULL, 0, XPATH_DRIVER_OPTIONS_EXPRESSION, driver_name);

    xpath = malloc(xpath_len);
    if (xpath == NULL)
    {
        jack_error("Out of memory.");
        goto exit;
    }

    snprintf(xpath, xpath_len, XPATH_DRIVER_OPTIONS_EXPRESSION, driver_name);

    //jack_info("xpath = \"%s\"", xpath);

    /* Evaluate xpath expression */
    xpath_obj_ptr = xmlXPathEvalExpression((const xmlChar *)xpath, xpath_ctx_ptr);
    if (xpath_obj_ptr == NULL)
    {
        jack_error("Unable to evaluate XPath expression \"%s\"", xpath);
        goto free_xpath;
    }

    if (xpath_obj_ptr->nodesetval == NULL || xpath_obj_ptr->nodesetval->nodeNr == 0)
    {
        //jack_info("XPath \"%s\" evaluation returned no data", xpath);
        goto free_xpath_obj;
    }

    content_buffer_ptr = xmlBufferCreate();
    if (content_buffer_ptr == NULL)
    {
        jack_error("xmlBufferCreate() failed.");
        goto free_xpath_obj;
    }

    for (i = 0 ; i < xpath_obj_ptr->nodesetval->nodeNr ; i++)
    {
        //jack_info("driver option \"%s\" at index %d", xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name"), i);

        if (xmlNodeBufGetContent(content_buffer_ptr, xpath_obj_ptr->nodesetval->nodeTab[i]) == -1)
        {
            jack_error("xmlNodeBufGetContent() failed.");
            goto next_option;
        }

        option_name = (const char *)xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name");
        option_value = (const char *)xmlBufferContent(content_buffer_ptr);

        jack_controller_settings_set_driver_option(driver, option_name, option_value);

    next_option:
        xmlBufferEmpty(content_buffer_ptr);
    }

//free_buffer:
    xmlBufferFree(content_buffer_ptr);

free_xpath_obj:
    xmlXPathFreeObject(xpath_obj_ptr);

free_xpath:
    free(xpath);

exit:
    return;
}

void
jack_controller_settings_read_internal(
    struct jack_controller * controller_ptr,
    xmlXPathContextPtr xpath_ctx_ptr,
    jackctl_internal internal)
{
    char *xpath;
    size_t xpath_len;
    xmlXPathObjectPtr xpath_obj_ptr;
    xmlBufferPtr content_buffer_ptr;
    int i;
    const char *option_name;
    const char *option_value;
    const char *internal_name;

    internal_name = jackctl_internal_get_name(internal);

    jack_info("reading options for internal \"%s\"", internal_name);

    xpath_len = snprintf(NULL, 0, XPATH_DRIVER_OPTIONS_EXPRESSION, internal_name);

    xpath = malloc(xpath_len);
    if (xpath == NULL)
    {
        jack_error("Out of memory.");
        goto exit;
    }

    snprintf(xpath, xpath_len, XPATH_DRIVER_OPTIONS_EXPRESSION, internal_name);

    //jack_info("xpath = \"%s\"", xpath);

    /* Evaluate xpath expression */
    xpath_obj_ptr = xmlXPathEvalExpression((const xmlChar *)xpath, xpath_ctx_ptr);
    if (xpath_obj_ptr == NULL)
    {
        jack_error("Unable to evaluate XPath expression \"%s\"", xpath);
        goto free_xpath;
    }

    if (xpath_obj_ptr->nodesetval == NULL || xpath_obj_ptr->nodesetval->nodeNr == 0)
    {
        //jack_info("XPath \"%s\" evaluation returned no data", xpath);
        goto free_xpath_obj;
    }

    content_buffer_ptr = xmlBufferCreate();
    if (content_buffer_ptr == NULL)
    {
        jack_error("xmlBufferCreate() failed.");
        goto free_xpath_obj;
    }

    for (i = 0 ; i < xpath_obj_ptr->nodesetval->nodeNr ; i++)
    {
        //jack_info("driver option \"%s\" at index %d", xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name"), i);

        if (xmlNodeBufGetContent(content_buffer_ptr, xpath_obj_ptr->nodesetval->nodeTab[i]) == -1)
        {
            jack_error("xmlNodeBufGetContent() failed.");
            goto next_option;
        }

        option_name = (const char *)xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name");
        option_value = (const char *)xmlBufferContent(content_buffer_ptr);

        jack_controller_settings_set_internal_option(internal, option_name, option_value);

    next_option:
        xmlBufferEmpty(content_buffer_ptr);
    }

//free_buffer:
    xmlBufferFree(content_buffer_ptr);

free_xpath_obj:
    xmlXPathFreeObject(xpath_obj_ptr);

free_xpath:
    free(xpath);

exit:
    return;
}

void
jack_controller_settings_read_drivers(
    struct jack_controller * controller_ptr,
    xmlXPathContextPtr xpath_ctx_ptr)
{
    xmlXPathObjectPtr xpath_obj_ptr;
    int i;
    const char *driver_name;
    jackctl_driver driver;

    /* Evaluate xpath expression */
    xpath_obj_ptr = xmlXPathEvalExpression((const xmlChar *)XPATH_DRIVERS_EXPRESSION, xpath_ctx_ptr);
    if (xpath_obj_ptr == NULL)
    {
        jack_error("Unable to evaluate XPath expression \"%s\"", XPATH_DRIVERS_EXPRESSION);
        goto exit;
    }

    if (xpath_obj_ptr->nodesetval == NULL || xpath_obj_ptr->nodesetval->nodeNr == 0)
    {
        jack_error("XPath \"%s\" evaluation returned no data", XPATH_DRIVERS_EXPRESSION);
        goto free_xpath_obj;
    }

    for (i = 0 ; i < xpath_obj_ptr->nodesetval->nodeNr ; i++)
    {
        driver_name = (const char *)xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name");

        driver = jack_controller_find_driver(controller_ptr->server, driver_name);
        if (driver == NULL)
        {
            jack_error("ignoring settings for unknown driver \"%s\"", driver_name);
        }
        else
        {
            jack_info("setting for driver \"%s\" found", driver_name);

            jack_controller_settings_read_driver(controller_ptr, xpath_ctx_ptr, driver);
        }
    }

free_xpath_obj:
    xmlXPathFreeObject(xpath_obj_ptr);

exit:
    return;
}

void
jack_controller_settings_read_internals(
    struct jack_controller * controller_ptr,
    xmlXPathContextPtr xpath_ctx_ptr)
{
    xmlXPathObjectPtr xpath_obj_ptr;
    int i;
    const char *internal_name;
    jackctl_internal internal;

    /* Evaluate xpath expression */
    xpath_obj_ptr = xmlXPathEvalExpression((const xmlChar *)XPATH_DRIVERS_EXPRESSION, xpath_ctx_ptr);
    if (xpath_obj_ptr == NULL)
    {
        jack_error("Unable to evaluate XPath expression \"%s\"", XPATH_DRIVERS_EXPRESSION);
        goto exit;
    }

    if (xpath_obj_ptr->nodesetval == NULL || xpath_obj_ptr->nodesetval->nodeNr == 0)
    {
        jack_error("XPath \"%s\" evaluation returned no data", XPATH_DRIVERS_EXPRESSION);
        goto free_xpath_obj;
    }

    for (i = 0 ; i < xpath_obj_ptr->nodesetval->nodeNr ; i++)
    {
        internal_name = (const char *)xmlGetProp(xpath_obj_ptr->nodesetval->nodeTab[i], BAD_CAST "name");

        driver = jack_controller_find_internal(controller_ptr->server, driver_name);
        if (driver == NULL)
        {
            jack_error("ignoring settings for unknown internal \"%s\"", internal_name);
        }
        else
        {
            jack_info("setting for internal \"%s\" found", internal_name);

            jack_controller_settings_read_internal(controller_ptr, xpath_ctx_ptr, driver);
        }
    }

free_xpath_obj:
    xmlXPathFreeObject(xpath_obj_ptr);

exit:
    return;
}


void
jack_controller_settings_load(
    struct jack_controller * controller_ptr)
{
    char *filename;
    size_t conf_len;
    xmlDocPtr doc_ptr;
    xmlXPathContextPtr xpath_ctx_ptr;

    conf_len = strlen(JACKDBUS_CONF);

    filename = malloc(g_jackdbus_dir_len + conf_len + 1);
    if (filename == NULL)
    {
        jack_error("Out of memory.");
        goto exit;
    }

    memcpy(filename, g_jackdbus_dir, g_jackdbus_dir_len);
    memcpy(filename + g_jackdbus_dir_len, JACKDBUS_CONF, conf_len);
    filename[g_jackdbus_dir_len + conf_len] = 0;

    jack_info("loading settings from \"%s\"", filename);

    doc_ptr = xmlParseFile(filename);
    if (doc_ptr == NULL)
    {
        jack_error("Failed to parse \"%s\"", filename);
        goto free_filename;
    }

    /* Create xpath evaluation context */
    xpath_ctx_ptr = xmlXPathNewContext(doc_ptr);
    if (xpath_ctx_ptr == NULL)
    {
        jack_error("Unable to create new XPath context");
        goto free_doc;
    }

    jack_controller_settings_read_engine(controller_ptr, xpath_ctx_ptr);
    jack_controller_settings_read_drivers(controller_ptr, xpath_ctx_ptr);
    jack_controller_settings_read_internals(controller_ptr, xpath_ctx_ptr);

    xmlXPathFreeContext(xpath_ctx_ptr);

free_doc:
    xmlFreeDoc(doc_ptr);

free_filename:
    free(filename);

exit:
    return;
}

void
jack_controller_settings_save_auto(
    struct jack_controller * controller_ptr)
{
    jack_controller_settings_save(controller_ptr, NULL);
}
