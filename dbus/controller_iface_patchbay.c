/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2008 Nedko Arnaudov
    Copyright (C) 2008 Juuso Alasuutari

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

#define _GNU_SOURCE            /* PTHREAD_MUTEX_RECURSIVE */

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dbus/dbus.h>
#include <pthread.h>

#include "jackdbus.h"
#include "controller_internal.h"
#include "list.h"

#define JACK_DBUS_IFACE_NAME "org.jackaudio.JackPatchbay"

/* FIXME: these need to be retrieved from common headers */
#define JACK_CLIENT_NAME_SIZE 64
#define JACK_PORT_NAME_SIZE 256

struct jack_graph
{
    uint64_t version;
    struct list_head clients;
    struct list_head ports;
    struct list_head connections;
};

struct jack_graph_client
{
    uint64_t id;
    char * name;
    int pid;
    struct list_head siblings;
    struct list_head ports;
};

struct jack_graph_port
{
    uint64_t id;
    char * name;
    uint32_t flags;
    uint32_t type;
    struct list_head siblings_graph;
    struct list_head siblings_client;
    struct jack_graph_client * client;
};

struct jack_graph_connection
{
    uint64_t id;
    struct jack_graph_port * port1;
    struct jack_graph_port * port2;
    struct list_head siblings;
};

struct jack_controller_patchbay
{
    pthread_mutex_t lock;
    struct jack_graph graph;
    uint64_t next_client_id;
    uint64_t next_port_id;
    uint64_t next_connection_id;
};

void
jack_controller_patchbay_send_signal_graph_changed(
    dbus_uint64_t new_graph_version)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "GraphChanged",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_INVALID);
}

void
jack_controller_patchbay_send_signal_client_appeared(
    dbus_uint64_t new_graph_version,
    dbus_uint64_t client_id,
    const char * client_name)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "ClientAppeared",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_UINT64,
        &client_id,
        DBUS_TYPE_STRING,
        &client_name,
        DBUS_TYPE_INVALID);
}

void
jack_controller_patchbay_send_signal_client_disappeared(
    dbus_uint64_t new_graph_version,
    dbus_uint64_t client_id,
    const char * client_name)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "ClientDisappeared",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_UINT64,
        &client_id,
        DBUS_TYPE_STRING,
        &client_name,
        DBUS_TYPE_INVALID);
}

void
jack_controller_patchbay_send_signal_port_appeared(
    dbus_uint64_t new_graph_version,
    dbus_uint64_t client_id,
    const char * client_name,
    dbus_uint64_t port_id,
    const char * port_name,
    dbus_uint32_t port_flags,
    dbus_uint32_t port_type)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "PortAppeared",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_UINT64,
        &client_id,
        DBUS_TYPE_STRING,
        &client_name,
        DBUS_TYPE_UINT64,
        &port_id,
        DBUS_TYPE_STRING,
        &port_name,
        DBUS_TYPE_UINT32,
        &port_flags,
        DBUS_TYPE_UINT32,
        &port_type,
        DBUS_TYPE_INVALID);
}

void
jack_controller_patchbay_send_signal_port_disappeared(
    dbus_uint64_t new_graph_version,
    dbus_uint64_t client_id,
    const char * client_name,
    dbus_uint64_t port_id,
    const char * port_name)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "PortDisappeared",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_UINT64,
        &client_id,
        DBUS_TYPE_STRING,
        &client_name,
        DBUS_TYPE_UINT64,
        &port_id,
        DBUS_TYPE_STRING,
        &port_name,
        DBUS_TYPE_INVALID);
}

void
jack_controller_patchbay_send_signal_ports_connected(
    dbus_uint64_t new_graph_version,
    dbus_uint64_t client1_id,
    const char * client1_name,
    dbus_uint64_t port1_id,
    const char * port1_name,
    dbus_uint64_t client2_id,
    const char * client2_name,
    dbus_uint64_t port2_id,
    const char * port2_name,
    dbus_uint64_t connection_id)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "PortsConnected",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_UINT64,
        &client1_id,
        DBUS_TYPE_STRING,
        &client1_name,
        DBUS_TYPE_UINT64,
        &port1_id,
        DBUS_TYPE_STRING,
        &port1_name,
        DBUS_TYPE_UINT64,
        &client2_id,
        DBUS_TYPE_STRING,
        &client2_name,
        DBUS_TYPE_UINT64,
        &port2_id,
        DBUS_TYPE_STRING,
        &port2_name,
        DBUS_TYPE_UINT64,
        &connection_id,
        DBUS_TYPE_INVALID);
}

void
jack_controller_patchbay_send_signal_ports_disconnected(
    dbus_uint64_t new_graph_version,
    dbus_uint64_t client1_id,
    const char * client1_name,
    dbus_uint64_t port1_id,
    const char * port1_name,
    dbus_uint64_t client2_id,
    const char * client2_name,
    dbus_uint64_t port2_id,
    const char * port2_name,
    dbus_uint64_t connection_id)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "PortsDisconnected",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_UINT64,
        &client1_id,
        DBUS_TYPE_STRING,
        &client1_name,
        DBUS_TYPE_UINT64,
        &port1_id,
        DBUS_TYPE_STRING,
        &port1_name,
        DBUS_TYPE_UINT64,
        &client2_id,
        DBUS_TYPE_STRING,
        &client2_name,
        DBUS_TYPE_UINT64,
        &port2_id,
        DBUS_TYPE_STRING,
        &port2_name,
        DBUS_TYPE_UINT64,
        &connection_id,
        DBUS_TYPE_INVALID);
}

void
jack_controller_patchbay_send_signal_port_renamed(
    dbus_uint64_t new_graph_version,
    dbus_uint64_t client_id,
    const char * client_name,
    dbus_uint64_t port_id,
    const char * port_old_name,
    const char * port_new_name)
{

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "PortRenamed",
        DBUS_TYPE_UINT64,
        &new_graph_version,
        DBUS_TYPE_UINT64,
        &client_id,
        DBUS_TYPE_STRING,
        &client_name,
        DBUS_TYPE_UINT64,
        &port_id,
        DBUS_TYPE_STRING,
        &port_old_name,
        DBUS_TYPE_STRING,
        &port_new_name,
        DBUS_TYPE_INVALID);
}

static
struct jack_graph_client *
jack_controller_patchbay_find_client(
    struct jack_controller_patchbay *patchbay_ptr,
    const char *client_name,    /* not '\0' terminated */
    size_t client_name_len)     /* without terminating '\0' */
{
    struct list_head *node_ptr;
    struct jack_graph_client *client_ptr;

    list_for_each(node_ptr, &patchbay_ptr->graph.clients)
    {
        client_ptr = list_entry(node_ptr, struct jack_graph_client, siblings);
        if (strlen(client_ptr->name) == client_name_len && strncmp(client_ptr->name, client_name, client_name_len) == 0)
        {
            return client_ptr;
        }
    }

    return NULL;
}

static
struct jack_graph_client *
jack_controller_patchbay_find_client_by_id(
    struct jack_controller_patchbay *patchbay_ptr,
    uint64_t id)
{
    struct list_head *node_ptr;
    struct jack_graph_client *client_ptr;

    list_for_each(node_ptr, &patchbay_ptr->graph.clients)
    {
        client_ptr = list_entry(node_ptr, struct jack_graph_client, siblings);
        if (client_ptr->id == id)
        {
            return client_ptr;
        }
    }

    return NULL;
}

static
struct jack_graph_client *
jack_controller_patchbay_create_client(
    struct jack_controller_patchbay *patchbay_ptr,
    const char *client_name,    /* not '\0' terminated */
    size_t client_name_len)     /* without terminating '\0' */
{
    struct jack_graph_client * client_ptr;

    client_ptr = malloc(sizeof(struct jack_graph_client));
    if (client_ptr == NULL)
    {
        jack_error("Memory allocation of jack_graph_client structure failed.");
        goto fail;
    }

    client_ptr->name = malloc(client_name_len + 1);
    if (client_ptr->name == NULL)
    {
        jack_error("malloc() failed to allocate memory for client name.");
        goto fail_free_client;
    }

    memcpy(client_ptr->name, client_name, client_name_len);
    client_ptr->name[client_name_len] = 0;

    client_ptr->pid = jack_get_client_pid(client_ptr->name);
    jack_info("New client '%s' with PID %d", client_ptr->name, client_ptr->pid);

    client_ptr->id = patchbay_ptr->next_client_id++;
    INIT_LIST_HEAD(&client_ptr->ports);


    pthread_mutex_lock(&patchbay_ptr->lock);
    list_add_tail(&client_ptr->siblings, &patchbay_ptr->graph.clients);
    patchbay_ptr->graph.version++;
    jack_controller_patchbay_send_signal_client_appeared(patchbay_ptr->graph.version, client_ptr->id, client_ptr->name);
    jack_controller_patchbay_send_signal_graph_changed(patchbay_ptr->graph.version);
    pthread_mutex_unlock(&patchbay_ptr->lock);

    return client_ptr;

fail_free_client:
    free(client_ptr);

fail:
    return NULL;
}

static
void
jack_controller_patchbay_destroy_client(
    struct jack_controller_patchbay *patchbay_ptr,
    struct jack_graph_client *client_ptr)
{
    jack_info("Client '%s' with PID %d is out", client_ptr->name, client_ptr->pid);

    pthread_mutex_lock(&patchbay_ptr->lock);
    list_del(&client_ptr->siblings);
    patchbay_ptr->graph.version++;
    jack_controller_patchbay_send_signal_client_disappeared(patchbay_ptr->graph.version, client_ptr->id, client_ptr->name);
    jack_controller_patchbay_send_signal_graph_changed(patchbay_ptr->graph.version);
    pthread_mutex_unlock(&patchbay_ptr->lock);

    free(client_ptr->name);
    free(client_ptr);
}

static
void
jack_controller_patchbay_destroy_client_by_name(
    struct jack_controller_patchbay *patchbay_ptr,
    const char *client_name)    /* '\0' terminated */
{
    struct jack_graph_client *client_ptr;

    client_ptr = jack_controller_patchbay_find_client(patchbay_ptr, client_name, strlen(client_name));
    if (client_ptr == NULL)
    {
        jack_error("Cannot destroy unknown client '%s'", client_name);
        return;
    }

    jack_controller_patchbay_destroy_client(patchbay_ptr, client_ptr);
}

static
void
jack_controller_patchbay_new_port(
    struct jack_controller_patchbay *patchbay_ptr,
    const char *port_full_name,
    uint32_t port_flags,
    uint32_t port_type)
{
    struct jack_graph_client *client_ptr;
    struct jack_graph_port *port_ptr;
    const char *port_short_name;
    size_t client_name_len;

    //jack_info("new port: %s", port_full_name);

    port_short_name = strchr(port_full_name, ':');
    if (port_short_name == NULL)
    {
        jack_error("port name '%s' does not contain ':' separator char", port_full_name);
        return;
    }

    port_short_name++;          /* skip ':' separator char */

    client_name_len = port_short_name - port_full_name - 1; /* without terminating '\0' */

    client_ptr = jack_controller_patchbay_find_client(patchbay_ptr, port_full_name, client_name_len);
    if (client_ptr == NULL)
    {
        client_ptr = jack_controller_patchbay_create_client(patchbay_ptr, port_full_name, client_name_len);
        if (client_ptr == NULL)
        {
            jack_error("Creation of new jack_graph client failed.");
            return;
        }
    }

    port_ptr = malloc(sizeof(struct jack_graph_port));
    if (port_ptr == NULL)
    {
        jack_error("Memory allocation of jack_graph_port structure failed.");
        return;
    }

    port_ptr->name = strdup(port_short_name);
    if (port_ptr->name == NULL)
    {
        jack_error("strdup() call for port name '%s' failed.", port_short_name);
        free(port_ptr);
        return;
    }

    port_ptr->id = patchbay_ptr->next_port_id++;
    port_ptr->flags = port_flags;
    port_ptr->type = port_type;
    port_ptr->client = client_ptr;

    pthread_mutex_lock(&patchbay_ptr->lock);
    list_add_tail(&port_ptr->siblings_client, &client_ptr->ports);
    list_add_tail(&port_ptr->siblings_graph, &patchbay_ptr->graph.ports);
    patchbay_ptr->graph.version++;
    jack_controller_patchbay_send_signal_port_appeared(
        patchbay_ptr->graph.version,
        client_ptr->id,
        client_ptr->name,
        port_ptr->id,
        port_ptr->name,
        port_ptr->flags,
        port_ptr->type);
    jack_controller_patchbay_send_signal_graph_changed(patchbay_ptr->graph.version);
    pthread_mutex_unlock(&patchbay_ptr->lock);
}

static
void
jack_controller_patchbay_remove_port(
    struct jack_controller_patchbay *patchbay_ptr,
    struct jack_graph_port *port_ptr)
{
    //jack_info("remove port: %s", port_ptr->name);

    pthread_mutex_lock(&patchbay_ptr->lock);
    list_del(&port_ptr->siblings_client);
    list_del(&port_ptr->siblings_graph);
    patchbay_ptr->graph.version++;
    jack_controller_patchbay_send_signal_port_disappeared(patchbay_ptr->graph.version, port_ptr->client->id, port_ptr->client->name, port_ptr->id, port_ptr->name);
    jack_controller_patchbay_send_signal_graph_changed(patchbay_ptr->graph.version);
    pthread_mutex_unlock(&patchbay_ptr->lock);

    free(port_ptr->name);
    free(port_ptr);
}

static
struct jack_graph_port *
jack_controller_patchbay_find_port_by_id(
    struct jack_controller_patchbay *patchbay_ptr,
    uint64_t port_id)
{
    struct list_head *node_ptr;
    struct jack_graph_port *port_ptr;

    list_for_each(node_ptr, &patchbay_ptr->graph.ports)
    {
        port_ptr = list_entry(node_ptr, struct jack_graph_port, siblings_graph);
        if (port_ptr->id == port_id)
        {
            return port_ptr;
        }
    }

    return NULL;
}

static
struct jack_graph_port *
jack_controller_patchbay_find_client_port_by_name(
    struct jack_controller_patchbay *patchbay_ptr,
    struct jack_graph_client *client_ptr,
    const char *port_name)
{
    struct list_head *node_ptr;
    struct jack_graph_port *port_ptr;

    list_for_each(node_ptr, &client_ptr->ports)
    {
        port_ptr = list_entry(node_ptr, struct jack_graph_port, siblings_client);
        if (strcmp(port_ptr->name, port_name) == 0)
        {
            return port_ptr;
        }
    }

    return NULL;
}

static
struct jack_graph_port *
jack_controller_patchbay_find_port_by_full_name(
    struct jack_controller_patchbay *patchbay_ptr,
    const char *port_full_name)
{
    const char *port_short_name;
    size_t client_name_len;
    struct jack_graph_client *client_ptr;

    //jack_info("name: %s", port_full_name);

    port_short_name = strchr(port_full_name, ':');
    if (port_short_name == NULL)
    {
        jack_error("port name '%s' does not contain ':' separator char", port_full_name);
        return NULL;
    }

    port_short_name++;          /* skip ':' separator char */

    client_name_len = port_short_name - port_full_name - 1; /* without terminating '\0' */

    client_ptr = jack_controller_patchbay_find_client(patchbay_ptr, port_full_name, client_name_len);
    if (client_ptr == NULL)
    {
        jack_error("cannot find client of port '%s'", port_full_name);
        return NULL;
    }

    return jack_controller_patchbay_find_client_port_by_name(patchbay_ptr, client_ptr, port_short_name);
}

static
struct jack_graph_port *
jack_controller_patchbay_find_port_by_names(
    struct jack_controller_patchbay *patchbay_ptr,
    const char *client_name,
    const char *port_name)
{
    struct jack_graph_client *client_ptr;

    client_ptr = jack_controller_patchbay_find_client(patchbay_ptr, client_name, strlen(client_name));
    if (client_ptr == NULL)
    {
        jack_error("cannot find client '%s'", client_name);
        return NULL;
    }

    return jack_controller_patchbay_find_client_port_by_name(patchbay_ptr, client_ptr, port_name);
}

static
struct jack_graph_connection *
jack_controller_patchbay_create_connection(
    struct jack_controller_patchbay *patchbay_ptr,
    struct jack_graph_port *port1_ptr,
    struct jack_graph_port *port2_ptr)
{
    struct jack_graph_connection * connection_ptr;

    connection_ptr = malloc(sizeof(struct jack_graph_connection));
    if (connection_ptr == NULL)
    {
        jack_error("Memory allocation of jack_graph_connection structure failed.");
        return NULL;
    }

    connection_ptr->id = patchbay_ptr->next_connection_id++;
    connection_ptr->port1 = port1_ptr;
    connection_ptr->port2 = port2_ptr;

    pthread_mutex_lock(&patchbay_ptr->lock);
    list_add_tail(&connection_ptr->siblings, &patchbay_ptr->graph.connections);
    patchbay_ptr->graph.version++;
    jack_controller_patchbay_send_signal_ports_connected(
        patchbay_ptr->graph.version,
        port1_ptr->client->id,
        port1_ptr->client->name,
        port1_ptr->id,
        port1_ptr->name,
        port2_ptr->client->id,
        port2_ptr->client->name,
        port2_ptr->id,
        port2_ptr->name,
        connection_ptr->id);
    jack_controller_patchbay_send_signal_graph_changed(patchbay_ptr->graph.version);
    pthread_mutex_unlock(&patchbay_ptr->lock);

    return connection_ptr;
}

static
void
jack_controller_patchbay_destroy_connection(
    struct jack_controller_patchbay *patchbay_ptr,
    struct jack_graph_connection *connection_ptr)
{
    pthread_mutex_lock(&patchbay_ptr->lock);
    list_del(&connection_ptr->siblings);
    patchbay_ptr->graph.version++;
    jack_controller_patchbay_send_signal_ports_disconnected(
        patchbay_ptr->graph.version,
        connection_ptr->port1->client->id,
        connection_ptr->port1->client->name,
        connection_ptr->port1->id,
        connection_ptr->port1->name,
        connection_ptr->port2->client->id,
        connection_ptr->port2->client->name,
        connection_ptr->port2->id,
        connection_ptr->port2->name,
        connection_ptr->id);
    jack_controller_patchbay_send_signal_graph_changed(patchbay_ptr->graph.version);
    pthread_mutex_unlock(&patchbay_ptr->lock);

    free(connection_ptr);
}

static
struct jack_graph_connection *
jack_controller_patchbay_find_connection(
    struct jack_controller_patchbay *patchbay_ptr,
    struct jack_graph_port *port1_ptr,
    struct jack_graph_port *port2_ptr)
{
    struct list_head *node_ptr;
    struct jack_graph_connection *connection_ptr;

    list_for_each(node_ptr, &patchbay_ptr->graph.connections)
    {
        connection_ptr = list_entry(node_ptr, struct jack_graph_connection, siblings);
        if ((connection_ptr->port1 == port1_ptr &&
             connection_ptr->port2 == port2_ptr) ||
            (connection_ptr->port1 == port2_ptr &&
             connection_ptr->port2 == port1_ptr))
        {
            return connection_ptr;
        }
    }

    return NULL;
}

static
struct jack_graph_connection *
jack_controller_patchbay_find_connection_by_id(
    struct jack_controller_patchbay *patchbay_ptr,
    uint64_t connection_id)
{
    struct list_head *node_ptr;
    struct jack_graph_connection *connection_ptr;

    list_for_each(node_ptr, &patchbay_ptr->graph.connections)
    {
        connection_ptr = list_entry(node_ptr, struct jack_graph_connection, siblings);
        if (connection_ptr->id == connection_id)
        {
            return connection_ptr;
        }
    }

    return NULL;
}

static
bool
jack_controller_patchbay_connect(
    struct jack_dbus_method_call *dbus_call_ptr,
    struct jack_controller *controller_ptr,
    struct jack_graph_port *port1_ptr,
    struct jack_graph_port *port2_ptr)
{
    int ret;
    char port1_name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char port2_name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];

    sprintf(port1_name, "%s:%s", port1_ptr->client->name, port1_ptr->name);
    sprintf(port2_name, "%s:%s", port2_ptr->client->name, port2_ptr->name);

    ret = jack_connect(controller_ptr->client, port1_name, port2_name);
    if (ret != 0)
    {
        jack_dbus_error(dbus_call_ptr, JACK_DBUS_ERROR_GENERIC, "jack_connect() failed with %d", ret);
        return false;
    }

    return true;
}

static
bool
jack_controller_patchbay_disconnect(
    struct jack_dbus_method_call *dbus_call_ptr,
    struct jack_controller *controller_ptr,
    struct jack_graph_port *port1_ptr,
    struct jack_graph_port *port2_ptr)
{
    int ret;
    char port1_name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char port2_name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];

    sprintf(port1_name, "%s:%s", port1_ptr->client->name, port1_ptr->name);
    sprintf(port2_name, "%s:%s", port2_ptr->client->name, port2_ptr->name);

    ret = jack_disconnect(controller_ptr->client, port1_name, port2_name);
    if (ret != 0)
    {
        jack_dbus_error(dbus_call_ptr, JACK_DBUS_ERROR_GENERIC, "jack_disconnect() failed with %d", ret);
        return false;
    }

    return true;
}

#define controller_ptr ((struct jack_controller *)call->context)
#define patchbay_ptr ((struct jack_controller_patchbay *)controller_ptr->patchbay_context)

static
void
jack_controller_dbus_get_all_ports(
    struct jack_dbus_method_call * call)
{
    struct list_head * client_node_ptr;
    struct list_head * port_node_ptr;
    struct jack_graph_client * client_ptr;
    struct jack_graph_port * port_ptr;
    DBusMessageIter iter, sub_iter;
    char fullname[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char *fullname_var = fullname;

    if (!controller_ptr->started)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_SERVER_NOT_RUNNING,
            "Can't execute this method with stopped JACK server");
        return;
    }

    call->reply = dbus_message_new_method_return (call->message);
    if (!call->reply)
    {
        goto fail;
    }

    dbus_message_iter_init_append (call->reply, &iter);

    if (!dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "s", &sub_iter))
    {
        goto fail_unref;
    }

    pthread_mutex_lock(&patchbay_ptr->lock);

    list_for_each(client_node_ptr, &patchbay_ptr->graph.clients)
    {
        client_ptr = list_entry(client_node_ptr, struct jack_graph_client, siblings);

        list_for_each(port_node_ptr, &client_ptr->ports)
        {
            port_ptr = list_entry(port_node_ptr, struct jack_graph_port, siblings_client);

            jack_info("%s:%s", client_ptr->name, port_ptr->name);
            sprintf(fullname, "%s:%s", client_ptr->name, port_ptr->name);
            if (!dbus_message_iter_append_basic (&sub_iter, DBUS_TYPE_STRING, &fullname_var))
            {
                pthread_mutex_unlock(&patchbay_ptr->lock);
                dbus_message_iter_close_container (&iter, &sub_iter);
                goto fail_unref;
            }
        }
    }

    pthread_mutex_unlock(&patchbay_ptr->lock);

    if (!dbus_message_iter_close_container (&iter, &sub_iter))
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
void
jack_controller_dbus_get_graph(
    struct jack_dbus_method_call * call)
{
    struct list_head * client_node_ptr;
    struct list_head * port_node_ptr;
    struct list_head * connection_node_ptr;
    struct jack_graph_client * client_ptr;
    struct jack_graph_port * port_ptr;
    struct jack_graph_connection * connection_ptr;
    DBusMessageIter iter;
    DBusMessageIter clients_array_iter;
    DBusMessageIter client_struct_iter;
    DBusMessageIter ports_array_iter;
    DBusMessageIter port_struct_iter;
    dbus_uint64_t version;
    DBusMessageIter connections_array_iter;
    DBusMessageIter connection_struct_iter;

    if (!controller_ptr->started)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_SERVER_NOT_RUNNING,
            "Can't execute this method with stopped JACK server");
        return;
    }

    if (!jack_dbus_get_method_args(call, DBUS_TYPE_UINT64, &version, DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that
         * jack_dbus_get_method_args() has constructed an error for us.
         */
        goto exit;
    }

    //jack_info("Getting graph, know version is %" PRIu32, version);

    call->reply = dbus_message_new_method_return(call->message);
    if (!call->reply)
    {
        jack_error("Ran out of memory trying to construct method return");
        goto exit;
    }

    dbus_message_iter_init_append (call->reply, &iter);

    pthread_mutex_lock(&patchbay_ptr->lock);

    if (version > patchbay_ptr->graph.version)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_INVALID_ARGS,
            "known graph version %" PRIu64 " is newer than actual version %" PRIu64,
            version,
            patchbay_ptr->graph.version);
        pthread_mutex_unlock(&patchbay_ptr->lock);
        goto exit;
    }

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &patchbay_ptr->graph.version))
    {
        goto nomem_unlock;
    }

    if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(tsa(tsuu))", &clients_array_iter))
    {
        goto nomem_unlock;
    }

    if (version < patchbay_ptr->graph.version)
    {
        list_for_each(client_node_ptr, &patchbay_ptr->graph.clients)
        {
            client_ptr = list_entry(client_node_ptr, struct jack_graph_client, siblings);

            if (!dbus_message_iter_open_container (&clients_array_iter, DBUS_TYPE_STRUCT, NULL, &client_struct_iter))
            {
                goto nomem_close_clients_array;
            }

            if (!dbus_message_iter_append_basic(&client_struct_iter, DBUS_TYPE_UINT64, &client_ptr->id))
            {
                goto nomem_close_client_struct;
            }

            if (!dbus_message_iter_append_basic(&client_struct_iter, DBUS_TYPE_STRING, &client_ptr->name))
            {
                goto nomem_close_client_struct;
            }

            if (!dbus_message_iter_open_container(&client_struct_iter, DBUS_TYPE_ARRAY, "(tsuu)", &ports_array_iter))
            {
                goto nomem_close_client_struct;
            }

            list_for_each(port_node_ptr, &client_ptr->ports)
            {
                port_ptr = list_entry(port_node_ptr, struct jack_graph_port, siblings_client);

                if (!dbus_message_iter_open_container(&ports_array_iter, DBUS_TYPE_STRUCT, NULL, &port_struct_iter))
                {
                    goto nomem_close_ports_array;
                }

                if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_UINT64, &port_ptr->id))
                {
                    goto nomem_close_port_struct;
                }

                if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_STRING, &port_ptr->name))
                {
                    goto nomem_close_port_struct;
                }

                if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_UINT32, &port_ptr->flags))
                {
                    goto nomem_close_port_struct;
                }

                if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_UINT32, &port_ptr->type))
                {
                    goto nomem_close_port_struct;
                }

                if (!dbus_message_iter_close_container(&ports_array_iter, &port_struct_iter))
                {
                    goto nomem_close_ports_array;
                }
            }

            if (!dbus_message_iter_close_container(&client_struct_iter, &ports_array_iter))
            {
                goto nomem_close_client_struct;
            }

            if (!dbus_message_iter_close_container(&clients_array_iter, &client_struct_iter))
            {
                goto nomem_close_clients_array;
            }
        }
    }

    if (!dbus_message_iter_close_container(&iter, &clients_array_iter))
    {
        goto nomem_unlock;
    }

    if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(tstststst)", &connections_array_iter))
    {
        goto nomem_unlock;
    }

    if (version < patchbay_ptr->graph.version)
    {
        list_for_each(connection_node_ptr, &patchbay_ptr->graph.connections)
        {
            connection_ptr = list_entry(connection_node_ptr, struct jack_graph_connection, siblings);

            if (!dbus_message_iter_open_container(&connections_array_iter, DBUS_TYPE_STRUCT, NULL, &connection_struct_iter))
            {
                goto nomem_close_connections_array;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port1->client->id))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port1->client->name))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port1->id))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port1->name))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port2->client->id))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port2->client->name))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port2->id))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port2->name))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->id))
            {
                goto nomem_close_connection_struct;
            }

            if (!dbus_message_iter_close_container(&connections_array_iter, &connection_struct_iter))
            {
                goto nomem_close_connections_array;
            }
        }
    }

    if (!dbus_message_iter_close_container(&iter, &connections_array_iter))
    {
        goto nomem_unlock;
    }

    pthread_mutex_unlock(&patchbay_ptr->lock);

    return;

nomem_close_connection_struct:
    dbus_message_iter_close_container(&connections_array_iter, &connection_struct_iter);

nomem_close_connections_array:
    dbus_message_iter_close_container(&iter, &connections_array_iter);
    goto nomem_unlock;

nomem_close_port_struct:
    dbus_message_iter_close_container(&ports_array_iter, &port_struct_iter);

nomem_close_ports_array:
    dbus_message_iter_close_container(&client_struct_iter, &ports_array_iter);

nomem_close_client_struct:
    dbus_message_iter_close_container(&clients_array_iter, &client_struct_iter);

nomem_close_clients_array:
    dbus_message_iter_close_container(&iter, &clients_array_iter);

nomem_unlock:
    pthread_mutex_unlock(&patchbay_ptr->lock);

//nomem:
    dbus_message_unref(call->reply);
    call->reply = NULL;
    jack_error("Ran out of memory trying to construct method return");

exit:
    return;
}

static
void
jack_controller_dbus_connect_ports_by_name(
    struct jack_dbus_method_call * call)
{
    const char * client1_name;
    const char * port1_name;
    const char * client2_name;
    const char * port2_name;
    struct jack_graph_port *port1_ptr;
    struct jack_graph_port *port2_ptr;

/*  jack_info("jack_controller_dbus_connect_ports_by_name() called."); */

    if (!controller_ptr->started)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_SERVER_NOT_RUNNING,
            "Can't execute this method with stopped JACK server");
        return;
    }

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_STRING,
            &client1_name,
            DBUS_TYPE_STRING,
            &port1_name,
            DBUS_TYPE_STRING,
            &client2_name,
            DBUS_TYPE_STRING,
            &port2_name,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that
         * jack_dbus_get_method_args() has constructed an error for us.
         */
        return;
    }

/*  jack_info("connecting %s:%s and %s:%s", client1_name, port1_name, client2_name, port2_name); */

    pthread_mutex_lock(&patchbay_ptr->lock);

    port1_ptr = jack_controller_patchbay_find_port_by_names(patchbay_ptr, client1_name, port1_name);
    if (port1_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port '%s':'%s'", client1_name, port1_name);
        goto unlock;
    }

    port2_ptr = jack_controller_patchbay_find_port_by_names(patchbay_ptr, client2_name, port2_name);
    if (port2_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port '%s':'%s'", client2_name, port2_name);
        goto unlock;
    }

    if (!jack_controller_patchbay_connect(
            call,
            controller_ptr,
            port1_ptr,
            port2_ptr))
    {
        /* jack_controller_patchbay_connect() constructed error reply */
        goto unlock;
    }

    jack_dbus_construct_method_return_empty(call);

unlock:
    pthread_mutex_unlock(&patchbay_ptr->lock);
}

static
void
jack_controller_dbus_connect_ports_by_id(
    struct jack_dbus_method_call * call)
{
    dbus_uint64_t port1_id;
    dbus_uint64_t port2_id;
    struct jack_graph_port *port1_ptr;
    struct jack_graph_port *port2_ptr;

/*     jack_info("jack_controller_dbus_connect_ports_by_id() called."); */

    if (!controller_ptr->started)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_SERVER_NOT_RUNNING,
            "Can't execute this method with stopped JACK server");
        return;
    }

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_UINT64,
            &port1_id,
            DBUS_TYPE_UINT64,
            &port2_id,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that
         * jack_dbus_get_method_args() has constructed an error for us.
         */
        return;
    }

    pthread_mutex_lock(&patchbay_ptr->lock);

    port1_ptr = jack_controller_patchbay_find_port_by_id(patchbay_ptr, port1_id);
    if (port1_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port %" PRIu64, port1_id);
        goto unlock;
    }

    port2_ptr = jack_controller_patchbay_find_port_by_id(patchbay_ptr, port2_id);
    if (port2_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port %" PRIu64, port2_id);
        goto unlock;
    }

    if (!jack_controller_patchbay_connect(
            call,
            controller_ptr,
            port1_ptr,
            port2_ptr))
    {
        /* jack_controller_patchbay_connect() constructed error reply */
        goto unlock;
    }

    jack_dbus_construct_method_return_empty(call);

unlock:
    pthread_mutex_unlock(&patchbay_ptr->lock);
}

static
void
jack_controller_dbus_disconnect_ports_by_name(
    struct jack_dbus_method_call * call)
{
    const char * client1_name;
    const char * port1_name;
    const char * client2_name;
    const char * port2_name;
    struct jack_graph_port *port1_ptr;
    struct jack_graph_port *port2_ptr;

/*  jack_info("jack_controller_dbus_disconnect_ports_by_name() called."); */

    if (!controller_ptr->started)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_SERVER_NOT_RUNNING,
            "Can't execute this method with stopped JACK server");
        return;
    }

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_STRING,
            &client1_name,
            DBUS_TYPE_STRING,
            &port1_name,
            DBUS_TYPE_STRING,
            &client2_name,
            DBUS_TYPE_STRING,
            &port2_name,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that
         * jack_dbus_get_method_args() has constructed an error for us.
         */
        return;
    }

/*  jack_info("disconnecting %s:%s and %s:%s", client1_name, port1_name, client2_name, port2_name); */

    pthread_mutex_lock(&patchbay_ptr->lock);

    port1_ptr = jack_controller_patchbay_find_port_by_names(patchbay_ptr, client1_name, port1_name);
    if (port1_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port '%s':'%s'", client1_name, port1_name);
        goto unlock;
    }

    port2_ptr = jack_controller_patchbay_find_port_by_names(patchbay_ptr, client2_name, port2_name);
    if (port2_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port '%s':'%s'", client2_name, port2_name);
        goto unlock;
    }

    if (!jack_controller_patchbay_disconnect(
            call,
            controller_ptr,
            port1_ptr,
            port2_ptr))
    {
        /* jack_controller_patchbay_connect() constructed error reply */
        goto unlock;
    }

    jack_dbus_construct_method_return_empty(call);

unlock:
    pthread_mutex_unlock(&patchbay_ptr->lock);
}

static
void
jack_controller_dbus_disconnect_ports_by_id(
    struct jack_dbus_method_call * call)
{
    dbus_uint64_t port1_id;
    dbus_uint64_t port2_id;
    struct jack_graph_port *port1_ptr;
    struct jack_graph_port *port2_ptr;

/*     jack_info("jack_controller_dbus_disconnect_ports_by_id() called."); */

    if (!controller_ptr->started)
    {
        jack_dbus_error(
            call,
            JACK_DBUS_ERROR_SERVER_NOT_RUNNING,
            "Can't execute this method with stopped JACK server");
        return;
    }

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_UINT64,
            &port1_id,
            DBUS_TYPE_UINT64,
            &port2_id,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that
         * jack_dbus_get_method_args() has constructed an error for us.
         */
        return;
    }

    pthread_mutex_lock(&patchbay_ptr->lock);

    port1_ptr = jack_controller_patchbay_find_port_by_id(patchbay_ptr, port1_id);
    if (port1_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port %" PRIu64, port1_id);
        goto unlock;
    }

    port2_ptr = jack_controller_patchbay_find_port_by_id(patchbay_ptr, port2_id);
    if (port2_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find port %" PRIu64, port2_id);
        goto unlock;
    }

    if (!jack_controller_patchbay_disconnect(
            call,
            controller_ptr,
            port1_ptr,
            port2_ptr))
    {
        /* jack_controller_patchbay_connect() constructed error reply */
        goto unlock;
    }

    jack_dbus_construct_method_return_empty(call);

unlock:
    pthread_mutex_unlock(&patchbay_ptr->lock);
}

static
void
jack_controller_dbus_disconnect_ports_by_connection_id(
    struct jack_dbus_method_call * call)
{
    dbus_uint64_t connection_id;
    struct jack_graph_connection *connection_ptr;

/*     jack_info("jack_controller_dbus_disconnect_ports_by_id() called."); */

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_UINT64,
            &connection_id,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that
         * jack_dbus_get_method_args() has constructed an error for us.
         */
        return;
    }

    pthread_mutex_lock(&patchbay_ptr->lock);

    connection_ptr = jack_controller_patchbay_find_connection_by_id(patchbay_ptr, connection_id);
    if (connection_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find connection %" PRIu64, connection_id);
        goto unlock;
    }

    if (!jack_controller_patchbay_disconnect(
            call,
            controller_ptr,
            connection_ptr->port1,
            connection_ptr->port2))
    {
        /* jack_controller_patchbay_connect() constructed error reply */
        goto unlock;
    }

    jack_dbus_construct_method_return_empty(call);

unlock:
    pthread_mutex_unlock(&patchbay_ptr->lock);
}

static
void
jack_controller_dbus_get_client_pid(
    struct jack_dbus_method_call * call)
{
    dbus_uint64_t client_id;
    struct jack_graph_client *client_ptr;
    message_arg_t arg;

/*     jack_info("jack_controller_dbus_get_client_pid() called."); */

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_UINT64,
            &client_id,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that
         * jack_dbus_get_method_args() has constructed an error for us.
         */
        return;
    }

    pthread_mutex_lock(&patchbay_ptr->lock);

    client_ptr = jack_controller_patchbay_find_client_by_id(patchbay_ptr, client_id);
    if (client_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "cannot find client %" PRIu64, client_id);
        goto unlock;
    }

    arg.int64 = client_ptr->pid;

    jack_dbus_construct_method_return_single(call, DBUS_TYPE_INT64, arg);

unlock:
    pthread_mutex_unlock(&patchbay_ptr->lock);
}

#undef controller_ptr
#define controller_ptr ((struct jack_controller *)context)

static
int
jack_controller_graph_order_callback(
    void *context)
{
    const char **ports;
    int i;
    jack_port_t *port_ptr;
    
    if (patchbay_ptr->graph.version > 1)
    {
        /* we use this only for initial catchup */
        return 0;
    }
    
    ports = jack_get_ports(controller_ptr->client, NULL, NULL, 0);
    if (ports)
    {
        for (i = 0;  ports[i]; ++i)
        {
            jack_info("graph reorder: new port '%s'", ports[i]);
            port_ptr = jack_port_by_name(controller_ptr->client, ports[i]);;
            jack_controller_patchbay_new_port(patchbay_ptr, ports[i], jack_port_flags(port_ptr), jack_port_type_id(port_ptr));
        }

        free(ports);
    }

    if (patchbay_ptr->graph.version == 1)
    {
        /* we have empty initial graph, increment graph version,
           so we dont do jack_get_ports() again,
           on next next graph change */
        patchbay_ptr->graph.version++;
    }

    return 0;
}

void
jack_controller_client_registration_callback(
    const char *client_name,
    int created,
    void *context)
{
    if (created)
    {
        jack_log("client '%s' created", client_name);
        jack_controller_patchbay_create_client(patchbay_ptr, client_name, strlen(client_name));
    }
    else
    {
        jack_log("client '%s' destroyed", client_name);
        jack_controller_patchbay_destroy_client_by_name(patchbay_ptr, client_name);
    }
}

void
jack_controller_port_registration_callback(
    jack_port_id_t port_id,
    int created,
    void *context)
{
    jack_port_t *port_ptr;
    struct jack_graph_port *graph_port_ptr;
    const char *port_name;

    port_ptr = jack_port_by_id(controller_ptr->client, port_id);
    port_name = jack_port_name(port_ptr);

    if (created)
    {
        jack_log("port '%s' created", port_name);
        jack_controller_patchbay_new_port(patchbay_ptr, port_name, jack_port_flags(port_ptr), jack_port_type_id(port_ptr));
    }
    else
    {
        jack_log("port '%s' destroyed", port_name);
        graph_port_ptr = jack_controller_patchbay_find_port_by_full_name(patchbay_ptr, port_name);
        if (graph_port_ptr == NULL)
        {
            jack_error("Failed to find port '%s' to destroy", port_name);
            return;
        }

        jack_controller_patchbay_remove_port(patchbay_ptr, graph_port_ptr);
    }
}

void
jack_controller_port_connect_callback(
    jack_port_id_t port1_id,
    jack_port_id_t port2_id,
    int connect,
    void *context)
{
    jack_port_t *port1;
    jack_port_t *port2;
    const char *port1_name;
    const char *port2_name;
    struct jack_graph_port *port1_ptr;
    struct jack_graph_port *port2_ptr;
    struct jack_graph_connection *connection_ptr;

    port1 = jack_port_by_id(controller_ptr->client, port1_id);
    port2 = jack_port_by_id(controller_ptr->client, port2_id);

    port1_name = jack_port_name(port1);
    port2_name = jack_port_name(port2);

    port1_ptr = jack_controller_patchbay_find_port_by_full_name(patchbay_ptr, port1_name);
    if (port1_ptr == NULL)
    {
        jack_error("Failed to find port '%s' to [dis]connect", port1_name);
        return;
    }

    port2_ptr = jack_controller_patchbay_find_port_by_full_name(patchbay_ptr, port2_name);
    if (port2_ptr == NULL)
    {
        jack_error("Failed to find port '%s' to [dis]connect", port2_name);
        return;
    }

    if (connect)
    {
        jack_info("Connecting '%s' to '%s'", port1_name, port2_name);
        connection_ptr = jack_controller_patchbay_find_connection(patchbay_ptr, port1_ptr, port2_ptr);
        if (connection_ptr != NULL)
        {
            jack_error("'%s' and '%s' are already connected", port1_name, port2_name);
            return;
        }

        jack_controller_patchbay_create_connection(patchbay_ptr, port1_ptr, port2_ptr);
    }
    else
    {
        jack_info("Disconnecting '%s' from '%s'", port1_name, port2_name);
        connection_ptr = jack_controller_patchbay_find_connection(patchbay_ptr, port1_ptr, port2_ptr);
        if (connection_ptr == NULL)
        {
            jack_error("Cannot find connection being removed");
            return;
        }

        jack_controller_patchbay_destroy_connection(patchbay_ptr, connection_ptr);
    }
}

int jack_controller_port_rename_callback(jack_port_id_t port, const char * old_name, const char * new_name, void * context)
{
    struct jack_graph_port * port_ptr;
    const char * port_new_short_name;
    const char * port_old_short_name;
    char * name_buffer;

    jack_info("port renamed: '%s' -> '%s'", old_name, new_name);

    port_new_short_name = strchr(new_name, ':');
    if (port_new_short_name == NULL)
    {
        jack_error("renamed port new name '%s' does not contain ':' separator char", new_name);
        return -1;
    }

    port_new_short_name++;      /* skip ':' separator char */

    port_old_short_name = strchr(old_name, ':');
    if (port_old_short_name == NULL)
    {
        jack_error("renamed port old name '%s' does not contain ':' separator char", old_name);
        return -1;
    }

    port_old_short_name++;      /* skip ':' separator char */

    port_ptr = jack_controller_patchbay_find_port_by_full_name(patchbay_ptr, old_name);
    if (port_ptr == NULL)
    {
        jack_error("renamed port '%s' not found", old_name);
        return -1;
    }

    name_buffer = strdup(port_new_short_name);
    if (name_buffer == NULL)
    {
        jack_error("strdup() call for port name '%s' failed.", port_new_short_name);
        return 1;
    }

    free(port_ptr->name);
    port_ptr->name = name_buffer;

    pthread_mutex_lock(&patchbay_ptr->lock);
    patchbay_ptr->graph.version++;
    jack_controller_patchbay_send_signal_port_renamed(
        patchbay_ptr->graph.version,
        port_ptr->client->id,
        port_ptr->client->name,
        port_ptr->id,
        port_old_short_name,
        port_ptr->name);
    jack_controller_patchbay_send_signal_graph_changed(patchbay_ptr->graph.version);
    pthread_mutex_unlock(&patchbay_ptr->lock);

    return 0;
}

#undef controller_ptr

void
jack_controller_patchbay_uninit(
    struct jack_controller * controller_ptr)
{
    struct jack_graph_client *client_ptr;
    struct jack_graph_port *port_ptr;

/*     jack_info("jack_controller_patchbay_uninit() called"); */

    while (!list_empty(&patchbay_ptr->graph.ports))
    {
        port_ptr = list_entry(patchbay_ptr->graph.ports.next, struct jack_graph_port, siblings_graph);
        jack_controller_patchbay_remove_port(patchbay_ptr, port_ptr);
    }

    while (!list_empty(&patchbay_ptr->graph.clients))
    {
        client_ptr = list_entry(patchbay_ptr->graph.clients.next, struct jack_graph_client, siblings);
        jack_controller_patchbay_destroy_client(patchbay_ptr, client_ptr);
    }

    pthread_mutex_destroy(&patchbay_ptr->lock);
}

#undef patchbay_ptr

bool
jack_controller_patchbay_init(
    struct jack_controller * controller_ptr)
{
    int ret;
    struct jack_controller_patchbay * patchbay_ptr;
    pthread_mutexattr_t attr;

/*     jack_info("jack_controller_patchbay_init() called"); */

    patchbay_ptr = malloc(sizeof(struct jack_controller_patchbay));
    if (patchbay_ptr == NULL)
    {
        jack_error("Memory allocation of jack_controller_patchbay structure failed.");
        goto fail;
    }

    ret = pthread_mutexattr_init(&attr);
    if (ret != 0)
    {
        goto fail;
    }

    ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (ret != 0)
    {
        goto fail;
    }

    pthread_mutex_init(&patchbay_ptr->lock, &attr);
    INIT_LIST_HEAD(&patchbay_ptr->graph.clients);
    INIT_LIST_HEAD(&patchbay_ptr->graph.ports);
    INIT_LIST_HEAD(&patchbay_ptr->graph.connections);
    patchbay_ptr->graph.version = 1;
    patchbay_ptr->next_client_id = 1;
    patchbay_ptr->next_port_id = 1;
    patchbay_ptr->next_connection_id = 1;

    controller_ptr->patchbay_context = patchbay_ptr;

    ret = jack_set_graph_order_callback(controller_ptr->client, jack_controller_graph_order_callback, controller_ptr);
    if (ret != 0)
    {
        jack_error("jack_set_graph_order_callback() failed with error %d", ret);
        goto fail_uninit_mutex;
    }

    ret = jack_set_client_registration_callback(controller_ptr->client, jack_controller_client_registration_callback, controller_ptr);
    if (ret != 0)
    {
        jack_error("jack_set_client_registration_callback() failed with error %d", ret);
        goto fail_uninit_mutex;
    }

    ret = jack_set_port_registration_callback(controller_ptr->client, jack_controller_port_registration_callback, controller_ptr);
    if (ret != 0)
    {
        jack_error("jack_set_port_registration_callback() failed with error %d", ret);
        goto fail_uninit_mutex;
    }

    ret = jack_set_port_connect_callback(controller_ptr->client, jack_controller_port_connect_callback, controller_ptr);
    if (ret != 0)
    {
        jack_error("jack_set_port_connect_callback() failed with error %d", ret);
        goto fail_uninit_mutex;
    }

    ret = jack_set_port_rename_callback(controller_ptr->client, jack_controller_port_rename_callback, controller_ptr);
    if (ret != 0)
    {
        jack_error("jack_set_port_rename_callback() failed with error %d", ret);
        goto fail_uninit_mutex;
    }

    return true;

fail_uninit_mutex:
    pthread_mutex_destroy(&patchbay_ptr->lock);

fail:
    return false;
}

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(GetAllPorts)
    JACK_DBUS_METHOD_ARGUMENT("ports_list", "as", true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(GetGraph)
    JACK_DBUS_METHOD_ARGUMENT("known_graph_version", DBUS_TYPE_UINT64_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("current_graph_version", DBUS_TYPE_UINT64_AS_STRING, true)
    JACK_DBUS_METHOD_ARGUMENT("clients_and_ports", "a(tsa(tsuu))", true)
    JACK_DBUS_METHOD_ARGUMENT("connections", "a(tstststst)", true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(ConnectPortsByName)
    JACK_DBUS_METHOD_ARGUMENT("client1_name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("port1_name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("client2_name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("port2_name", DBUS_TYPE_STRING_AS_STRING, false)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(ConnectPortsByID)
    JACK_DBUS_METHOD_ARGUMENT("port1_id", DBUS_TYPE_UINT64_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("port2_id", DBUS_TYPE_UINT64_AS_STRING, false)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(DisconnectPortsByName)
    JACK_DBUS_METHOD_ARGUMENT("client1_name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("port1_name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("client2_name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("port2_name", DBUS_TYPE_STRING_AS_STRING, false)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(DisconnectPortsByID)
    JACK_DBUS_METHOD_ARGUMENT("port1_id", DBUS_TYPE_UINT64_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("port2_id", DBUS_TYPE_UINT64_AS_STRING, false)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(DisconnectPortsByConnectionID)
    JACK_DBUS_METHOD_ARGUMENT("connection_id", DBUS_TYPE_UINT64_AS_STRING, false)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(GetClientPID)
    JACK_DBUS_METHOD_ARGUMENT("client_id", DBUS_TYPE_UINT64_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("process_id", DBUS_TYPE_INT64_AS_STRING, true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHODS_BEGIN
    JACK_DBUS_METHOD_DESCRIBE(GetAllPorts, jack_controller_dbus_get_all_ports)
    JACK_DBUS_METHOD_DESCRIBE(GetGraph, jack_controller_dbus_get_graph)
    JACK_DBUS_METHOD_DESCRIBE(ConnectPortsByName, jack_controller_dbus_connect_ports_by_name)
    JACK_DBUS_METHOD_DESCRIBE(ConnectPortsByID, jack_controller_dbus_connect_ports_by_id)
    JACK_DBUS_METHOD_DESCRIBE(DisconnectPortsByName, jack_controller_dbus_disconnect_ports_by_name)
    JACK_DBUS_METHOD_DESCRIBE(DisconnectPortsByID, jack_controller_dbus_disconnect_ports_by_id)
    JACK_DBUS_METHOD_DESCRIBE(DisconnectPortsByConnectionID, jack_controller_dbus_disconnect_ports_by_connection_id)
    JACK_DBUS_METHOD_DESCRIBE(GetClientPID, jack_controller_dbus_get_client_pid)
JACK_DBUS_METHODS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(GraphChanged)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(ClientAppeared)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_name", DBUS_TYPE_STRING_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(ClientDisappeared)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_name", DBUS_TYPE_STRING_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(PortAppeared)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_flags", DBUS_TYPE_UINT32_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_type", DBUS_TYPE_UINT32_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(PortDisappeared)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_name", DBUS_TYPE_STRING_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(PortsConnected)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client1_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client1_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port1_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port1_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client2_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client2_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port2_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port2_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("connection_id", DBUS_TYPE_UINT64_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(PortsDisconnected)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client1_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client1_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port1_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port1_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client2_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client2_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port2_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port2_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("connection_id", DBUS_TYPE_UINT64_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(PortRenamed)
    JACK_DBUS_SIGNAL_ARGUMENT("new_graph_version", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_id", DBUS_TYPE_UINT64_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("client_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_old_name", DBUS_TYPE_STRING_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("port_new_name", DBUS_TYPE_STRING_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_SIGNALS_BEGIN
    JACK_DBUS_SIGNAL_DESCRIBE(GraphChanged)
    JACK_DBUS_SIGNAL_DESCRIBE(ClientAppeared)
    JACK_DBUS_SIGNAL_DESCRIBE(ClientDisappeared)
    JACK_DBUS_SIGNAL_DESCRIBE(PortAppeared)
    JACK_DBUS_SIGNAL_DESCRIBE(PortDisappeared)
    JACK_DBUS_SIGNAL_DESCRIBE(PortsConnected)
    JACK_DBUS_SIGNAL_DESCRIBE(PortsDisconnected)
    JACK_DBUS_SIGNAL_DESCRIBE(PortRenamed)
JACK_DBUS_SIGNALS_END

JACK_DBUS_IFACE_BEGIN(g_jack_controller_iface_patchbay, JACK_DBUS_IFACE_NAME)
    JACK_DBUS_IFACE_EXPOSE_METHODS
    JACK_DBUS_IFACE_EXPOSE_SIGNALS
JACK_DBUS_IFACE_END
