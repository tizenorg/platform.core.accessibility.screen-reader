/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dbus_direct_reading_adapter.h"
#include "logger.h"

#include <Eldbus.h>

#define BUS "org.tizen.ScreenReader"
#define PATH "/org/tizen/DirectReading"
#define INTERFACE "org.tizen.DirectReading"

static Eldbus_Connection *conn;
static Eldbus_Service_Interface *iface;

static Eldbus_Message *
_hello(const Eldbus_Service_Interface *iface EINA_UNUSED, const Eldbus_Message *message)
{
	DEBUG("[START]");
	Eldbus_Message *reply = eldbus_message_method_return_new(message);
	eldbus_message_arguments_append(reply, "s", "Hello World");
	ERROR("TEST MESSAGE\n");
	return reply;
}

static Eldbus_Message *
_consume_read_command(const Eldbus_Service_Interface *iface EINA_UNUSED, const Eldbus_Message *msg)
{
	DEBUG("[START] CONSUMING READ COMMAND");
	Eldbus_Message *reply = eldbus_message_method_return_new(msg);
	Eina_Bool interruptable = EINA_TRUE;
	char* text = "";
	// read the parameters
	if (!eldbus_message_arguments_get(msg, "sb", &text, &interruptable))
		printf("eldbus_message_arguments_get() error\n");
	DEBUG("RECIVED DBUS MESSAGE: text(%s), interruptable(%d)", text, interruptable);
	tts_speak_customized(text,EINA_TRUE,interruptable);
	eldbus_message_arguments_append(reply, "sb", text, interruptable);
	DEBUG("[END] CONSUMING READ COMMAND");
	return reply;
}

static const Eldbus_Method methods[] = {
	{"Hello", ELDBUS_ARGS({"s", "message"}), NULL, _hello},
	{"ReadCommand", ELDBUS_ARGS({"s", "text"},{"b","interruptable"}), NULL, _consume_read_command},
	{ }
};

static const Eldbus_Service_Interface_Desc iface_desc = {
	INTERFACE, methods, NULL
};

static void
on_name_request(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
	unsigned int reply;
	DEBUG("[START]");

	if (eldbus_message_error_get(msg, NULL, NULL)) {
		ERROR("error on on_name_request\n");
		return;
	}

	if (!eldbus_message_arguments_get(msg, "u", &reply)) {
		ERROR("error geting arguments on on_name_request\n");
		return;
	}

	if (reply != ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER) {
		ERROR("error name already in use\n");
		return;
	}

	DEBUG("[END]");
}

void _get_a11y_address(void *data, const Eldbus_Message * msg, Eldbus_Pending * pending)
{
	const char *sock_addr;
	const char *errname, *errmsg;
	Eldbus_Connection *session;

	DEBUG("[START]");
        session = data;
	if (eldbus_message_error_get(msg, &errname, &errmsg)) {
		ERROR("GetAddress failed: %s %s", errname, errmsg);
		return;
	}

	if (!eldbus_message_arguments_get(msg, "s", &sock_addr) || !sock_addr) {
		ERROR("Could not get A11Y Bus socket address.");
		goto end;
	}

	DEBUG("A11 socket %s",sock_addr);

	if (!(conn = eldbus_address_connection_get(sock_addr))) {
		ERROR("Failed to connect to %s", sock_addr);
		goto end;
	}

	iface = eldbus_service_interface_register(conn, PATH, &iface_desc);
	if (!iface) {
		ERROR("Failed to register %s interface", INTERFACE);
		eldbus_connection_unref(conn);
		conn = NULL;
		goto end;
	}

	eldbus_name_request(conn, BUS, ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE,
			on_name_request, NULL);

 end:
	eldbus_connection_unref(session);
	DEBUG("[END]");
}

void dbus_direct_reading_init(void)
{
	Eldbus_Connection *session;
	Eldbus_Message *msg;

	DEBUG("[START]");
	eldbus_init();

   	session = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
	if (!session) {
		ERROR("Unable to get session bus");
		return;
	}

 	msg = eldbus_message_method_call_new("org.a11y.Bus", "/org/a11y/bus", "org.a11y.Bus", "GetAddress");
	if (!msg) {
		ERROR("DBus message allocation failed");
		goto fail_msg;
	}

	if (!eldbus_connection_send(session, msg, _get_a11y_address, session, -1)) {
		ERROR("Message send failed");
		goto fail_send;
	}

	DEBUG("[END]");
	return;

 fail_send:
	eldbus_message_unref(msg);
 fail_msg:
	eldbus_connection_unref(session);
}

void dbus_direct_reading_shutdown(void)
{
	DEBUG("[START]");
	if (iface)
		eldbus_service_object_unregister(iface);
	if (conn)
		eldbus_connection_unref(conn);

	conn = NULL;
	iface = NULL;

	DEBUG("[END]");
	eldbus_shutdown();
}
