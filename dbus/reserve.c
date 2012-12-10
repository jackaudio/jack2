/***
  Copyright 2009 Lennart Poettering

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
***/

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "reserve.h"
#include "jack/control.h"

#define RESERVE_ERROR_NO_MEMORY            "org.freedesktop.ReserveDevice1.Error.NoMemory"
#define RESERVE_ERROR_PROTOCOL_VIOLATION   "org.freedesktop.ReserveDevice1.Error.Protocol"
#define RESERVE_ERROR_RELEASE_DENIED       "org.freedesktop.ReserveDevice1.Error.ReleaseDenied"

struct rd_device {
	int ref;

	char *device_name;
	char *application_name;
	char *application_device_name;
	char *service_name;
	char *object_path;
	int32_t priority;

	DBusConnection *connection;

	int owning:1;
	int registered:1;
	int filtering:1;
	int gave_up:1;

	rd_request_cb_t request_cb;
	void *userdata;
};


#define SERVICE_PREFIX "org.freedesktop.ReserveDevice1."
#define OBJECT_PREFIX "/org/freedesktop/ReserveDevice1/"

static const char introspection[] =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>"
	" <!-- If you are looking for documentation make sure to check out\n"
	"      http://git.0pointer.de/?p=reserve.git;a=blob;f=reserve.txt -->\n"
	" <interface name=\"org.freedesktop.ReserveDevice1\">"
	"  <method name=\"RequestRelease\">"
	"   <arg name=\"priority\" type=\"i\" direction=\"in\"/>"
	"   <arg name=\"result\" type=\"b\" direction=\"out\"/>"
	"  </method>"
	"  <property name=\"Priority\" type=\"i\" access=\"read\"/>"
	"  <property name=\"ApplicationName\" type=\"s\" access=\"read\"/>"
	"  <property name=\"ApplicationDeviceName\" type=\"s\" access=\"read\"/>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Properties\">"
	"  <method name=\"Get\">"
	"   <arg name=\"interface\" direction=\"in\" type=\"s\"/>"
	"   <arg name=\"property\" direction=\"in\" type=\"s\"/>"
	"   <arg name=\"value\" direction=\"out\" type=\"v\"/>"
	"  </method>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Introspectable\">"
	"  <method name=\"Introspect\">"
	"   <arg name=\"data\" type=\"s\" direction=\"out\"/>"
	"  </method>"
	" </interface>"
	"</node>";

static dbus_bool_t add_variant(
	DBusMessage *m,
	int type,
	const void *data) {

	DBusMessageIter iter, sub;
	char t[2];

	t[0] = (char) type;
	t[1] = 0;

	dbus_message_iter_init_append(m, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, t, &sub))
		return FALSE;

	if (!dbus_message_iter_append_basic(&sub, type, data))
		return FALSE;

	if (!dbus_message_iter_close_container(&iter, &sub))
		return FALSE;

	return TRUE;
}

static DBusHandlerResult object_handler(
	DBusConnection *c,
	DBusMessage *m,
	void *userdata) {

	rd_device *d;
	DBusError error;
	DBusMessage *reply = NULL;

	dbus_error_init(&error);

	d = (rd_device*)userdata;
	assert(d->ref >= 1);

	if (dbus_message_is_method_call(
		    m,
		    "org.freedesktop.ReserveDevice1",
		    "RequestRelease")) {

		int32_t priority;
		dbus_bool_t ret;

		if (!dbus_message_get_args(
			    m,
			    &error,
			    DBUS_TYPE_INT32, &priority,
			    DBUS_TYPE_INVALID))
			goto invalid;

		ret = FALSE;

		if (priority > d->priority && d->request_cb) {
			d->ref++;

			if (d->request_cb(d, 0) > 0) {
				ret = TRUE;
				d->gave_up = 1;
			}

			rd_release(d);
		}

		if (!(reply = dbus_message_new_method_return(m)))
			goto oom;

		if (!dbus_message_append_args(
			    reply,
			    DBUS_TYPE_BOOLEAN, &ret,
			    DBUS_TYPE_INVALID))
			goto oom;

		if (!dbus_connection_send(c, reply, NULL))
			goto oom;

		dbus_message_unref(reply);

		return DBUS_HANDLER_RESULT_HANDLED;

	} else if (dbus_message_is_method_call(
			   m,
			   "org.freedesktop.DBus.Properties",
			   "Get")) {

		const char *interface, *property;

		if (!dbus_message_get_args(
			    m,
			    &error,
			    DBUS_TYPE_STRING, &interface,
			    DBUS_TYPE_STRING, &property,
			    DBUS_TYPE_INVALID))
			goto invalid;

		if (strcmp(interface, "org.freedesktop.ReserveDevice1") == 0) {
			const char *empty = "";

			if (strcmp(property, "ApplicationName") == 0 && d->application_name) {
				if (!(reply = dbus_message_new_method_return(m)))
					goto oom;

				if (!add_variant(
					    reply,
					    DBUS_TYPE_STRING,
					    d->application_name ? (const char**) &d->application_name : &empty))
					goto oom;

			} else if (strcmp(property, "ApplicationDeviceName") == 0) {
				if (!(reply = dbus_message_new_method_return(m)))
					goto oom;

				if (!add_variant(
					    reply,
					    DBUS_TYPE_STRING,
					    d->application_device_name ? (const char**) &d->application_device_name : &empty))
					goto oom;

			} else if (strcmp(property, "Priority") == 0) {
				if (!(reply = dbus_message_new_method_return(m)))
					goto oom;

				if (!add_variant(
					    reply,
					    DBUS_TYPE_INT32,
					    &d->priority))
					goto oom;
			} else {
				if (!(reply = dbus_message_new_error_printf(
					      m,
					      DBUS_ERROR_UNKNOWN_METHOD,
					      "Unknown property %s",
					      property)))
					goto oom;
			}

			if (!dbus_connection_send(c, reply, NULL))
				goto oom;

			dbus_message_unref(reply);

			return DBUS_HANDLER_RESULT_HANDLED;
		}

	} else if (dbus_message_is_method_call(
			   m,
			   "org.freedesktop.DBus.Introspectable",
			   "Introspect")) {
			    const char *i = introspection;

		if (!(reply = dbus_message_new_method_return(m)))
			goto oom;

		if (!dbus_message_append_args(
			    reply,
			    DBUS_TYPE_STRING,
			    &i,
			    DBUS_TYPE_INVALID))
			goto oom;

		if (!dbus_connection_send(c, reply, NULL))
			goto oom;

		dbus_message_unref(reply);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

invalid:
	if (reply)
		dbus_message_unref(reply);

	if (!(reply = dbus_message_new_error(
		      m,
		      DBUS_ERROR_INVALID_ARGS,
		      "Invalid arguments")))
		goto oom;

	if (!dbus_connection_send(c, reply, NULL))
		goto oom;

	dbus_message_unref(reply);

	dbus_error_free(&error);

	return DBUS_HANDLER_RESULT_HANDLED;

oom:
	if (reply)
		dbus_message_unref(reply);

	dbus_error_free(&error);

	return DBUS_HANDLER_RESULT_NEED_MEMORY;
}

static DBusHandlerResult filter_handler(
	DBusConnection *c,
	DBusMessage *m,
	void *userdata) {

	DBusMessage *reply;
	rd_device *d;
	DBusError error;

	dbus_error_init(&error);

	d = (rd_device*)userdata;

	if (dbus_message_is_signal(m, "org.freedesktop.DBus", "NameLost")) {
		const char *name;

		if (!dbus_message_get_args(
			    m,
			    &error,
			    DBUS_TYPE_STRING, &name,
			    DBUS_TYPE_INVALID))
			goto invalid;

		if (strcmp(name, d->service_name) == 0 && d->owning) {
			d->owning = 0;

			if (!d->gave_up)  {
				d->ref++;

				if (d->request_cb)
					d->request_cb(d, 1);
				d->gave_up = 1;

				rd_release(d);
			}

			return DBUS_HANDLER_RESULT_HANDLED;
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

invalid:
	if (!(reply = dbus_message_new_error(
		      m,
		      DBUS_ERROR_INVALID_ARGS,
		      "Invalid arguments")))
		goto oom;

	if (!dbus_connection_send(c, reply, NULL))
		goto oom;

	dbus_message_unref(reply);

	dbus_error_free(&error);

	return DBUS_HANDLER_RESULT_HANDLED;

oom:
	if (reply)
		dbus_message_unref(reply);

	dbus_error_free(&error);

	return DBUS_HANDLER_RESULT_NEED_MEMORY;
}

static DBusObjectPathVTable vtable;

int rd_acquire(
	rd_device **_d,
	DBusConnection *connection,
	const char *device_name,
	const char *application_name,
	int32_t priority,
	rd_request_cb_t request_cb,
	DBusError *error) {

	rd_device *d = NULL;
	int r, k;
	DBusError _error;
	DBusMessage *m = NULL, *reply = NULL;
	dbus_bool_t good;
    vtable.message_function = object_handler;

	if (!error) {
		error = &_error;
		dbus_error_init(error);
	}

	if (!_d) {
		assert(0);
		r = -EINVAL;
		goto fail;
	}

	if (!connection) {
		assert(0);
		r = -EINVAL;
		goto fail;
	}

	if (!device_name) {
		assert(0);
		r = -EINVAL;
		goto fail;
	}

	if (!request_cb && priority != INT32_MAX) {
		assert(0);
		r = -EINVAL;
		goto fail;
	}

	if (!(d = (rd_device *)calloc(sizeof(rd_device), 1))) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot allocate memory for rd_device struct");
		r = -ENOMEM;
		goto fail;
	}

	d->ref = 1;

	if (!(d->device_name = strdup(device_name))) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot duplicate device name string");
		r = -ENOMEM;
		goto fail;
	}

	if (!(d->application_name = strdup(application_name))) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot duplicate application name string");
		r = -ENOMEM;
		goto fail;
	}

	d->priority = priority;
	d->connection = dbus_connection_ref(connection);
	d->request_cb = request_cb;

	if (!(d->service_name = (char*)malloc(sizeof(SERVICE_PREFIX) + strlen(device_name)))) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot allocate memory for service name string");
		r = -ENOMEM;
		goto fail;
	}
	sprintf(d->service_name, SERVICE_PREFIX "%s", d->device_name);

	if (!(d->object_path = (char*)malloc(sizeof(OBJECT_PREFIX) + strlen(device_name)))) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot allocate memory for object path string");
		r = -ENOMEM;
		goto fail;
	}
	sprintf(d->object_path, OBJECT_PREFIX "%s", d->device_name);

	if ((k = dbus_bus_request_name(
		     d->connection,
		     d->service_name,
		     DBUS_NAME_FLAG_DO_NOT_QUEUE|
		     (priority < INT32_MAX ? DBUS_NAME_FLAG_ALLOW_REPLACEMENT : 0),
		     error)) < 0) {
		jack_error("dbus_bus_request_name() failed. (1)");
		r = -EIO;
		goto fail;
	}

	switch (k) {
	case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
	case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
		goto success;
	case DBUS_REQUEST_NAME_REPLY_EXISTS:
		break;
	case DBUS_REQUEST_NAME_REPLY_IN_QUEUE : /* DBUS_NAME_FLAG_DO_NOT_QUEUE was specified */
	default:								/* unknown reply returned */
		jack_error("request name reply with unexpected value %d.", k);
		assert(0);
		r = -EIO;
		goto fail;
	}

	if (priority <= INT32_MIN) {
		r = -EBUSY;
		dbus_set_error(error, RESERVE_ERROR_RELEASE_DENIED, "Device reservation request with priority %"PRIi32" denied for \"%s\"", priority, device_name);
		goto fail;
	}

	if (!(m = dbus_message_new_method_call(
		      d->service_name,
		      d->object_path,
		      "org.freedesktop.ReserveDevice1",
		      "RequestRelease"))) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot allocate memory for RequestRelease method call");
		r = -ENOMEM;
		goto fail;
	}

	if (!dbus_message_append_args(
		    m,
		    DBUS_TYPE_INT32, &d->priority,
		    DBUS_TYPE_INVALID)) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot append args for RequestRelease method call");
		r = -ENOMEM;
		goto fail;
	}

	if (!(reply = dbus_connection_send_with_reply_and_block(
		      d->connection,
		      m,
		      5000, /* 5s */
		      error))) {

		if (dbus_error_has_name(error, DBUS_ERROR_TIMED_OUT) ||
		    dbus_error_has_name(error, DBUS_ERROR_UNKNOWN_METHOD) ||
		    dbus_error_has_name(error, DBUS_ERROR_NO_REPLY)) {
			/* This must be treated as denied. */
			jack_info("Device reservation request with priority %"PRIi32" denied for \"%s\": %s (%s)", priority, device_name, error->name, error->message);
			r = -EBUSY;
			goto fail;
		}

		jack_error("dbus_connection_send_with_reply_and_block(RequestRelease) failed.");
		r = -EIO;
		goto fail;
	}

	if (!dbus_message_get_args(
		    reply,
		    error,
		    DBUS_TYPE_BOOLEAN, &good,
		    DBUS_TYPE_INVALID)) {
		jack_error("RequestRelease() reply is invalid.");
		r = -EIO;
		goto fail;
	}

	if (!good) {
        dbus_set_error(error, RESERVE_ERROR_RELEASE_DENIED, "Device reservation request with priority %"PRIi32" denied for \"%s\" via RequestRelease()", priority, device_name);
		r = -EBUSY;
		goto fail;
	}

	if ((k = dbus_bus_request_name(
		     d->connection,
		     d->service_name,
		     DBUS_NAME_FLAG_DO_NOT_QUEUE|
		     (priority < INT32_MAX ? DBUS_NAME_FLAG_ALLOW_REPLACEMENT : 0)|
		     DBUS_NAME_FLAG_REPLACE_EXISTING,
		     error)) < 0) {
		jack_error("dbus_bus_request_name() failed. (2)");
		r = -EIO;
		goto fail;
	}

	if (k != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        /* this is racy, another contender may have acquired the device */
		dbus_set_error(error, RESERVE_ERROR_PROTOCOL_VIOLATION, "request name reply is not DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER but %d.", k);
		r = -EIO;
		goto fail;
	}

success:
	d->owning = 1;

	if (!(dbus_connection_try_register_object_path(
		      d->connection,
		      d->object_path,
		      &vtable,
		      d,
		      error))) {
		jack_error("cannot register object path \"%s\": %s", d->object_path, error->message);
		r = -ENOMEM;
		goto fail;
	}

	d->registered = 1;

	if (!dbus_connection_add_filter(
		    d->connection,
		    filter_handler,
		    d,
		    NULL)) {
		dbus_set_error(error, RESERVE_ERROR_NO_MEMORY, "Cannot add filter");
		r = -ENOMEM;
		goto fail;
	}

	d->filtering = 1;

	*_d = d;
	return 0;

fail:
	if (m)
		dbus_message_unref(m);

	if (reply)
		dbus_message_unref(reply);

	if (&_error == error)
		dbus_error_free(&_error);

	if (d)
		rd_release(d);

	return r;
}

void rd_release(
	rd_device *d) {

	if (!d)
		return;

	assert(d->ref > 0);

	if (--d->ref)
		return;


	if (d->filtering)
		dbus_connection_remove_filter(
			d->connection,
			filter_handler,
			d);

	if (d->registered)
		dbus_connection_unregister_object_path(
			d->connection,
			d->object_path);

	if (d->owning) {
		DBusError error;
		dbus_error_init(&error);

		dbus_bus_release_name(
			d->connection,
			d->service_name,
			&error);

		dbus_error_free(&error);
	}

	free(d->device_name);
	free(d->application_name);
	free(d->application_device_name);
	free(d->service_name);
	free(d->object_path);

	if (d->connection)
		dbus_connection_unref(d->connection);

	free(d);
}

int rd_set_application_device_name(rd_device *d, const char *n) {
	char *t;

	if (!d)
		return -EINVAL;

	assert(d->ref > 0);

	if (!(t = strdup(n)))
		return -ENOMEM;

	free(d->application_device_name);
	d->application_device_name = t;
	return 0;
}

void rd_set_userdata(rd_device *d, void *userdata) {

	if (!d)
		return;

	assert(d->ref > 0);
	d->userdata = userdata;
}

void* rd_get_userdata(rd_device *d) {

	if (!d)
		return NULL;

	assert(d->ref > 0);

	return d->userdata;
}
