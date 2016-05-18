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
#include "screen_reader_tts.h"
#include "logger.h"

#include <Eldbus.h>
#include <assert.h>
#define BUS "org.tizen.ScreenReader"
#define PATH "/org/tizen/DirectReading"
#define INTERFACE "org.tizen.DirectReading"

static Eldbus_Connection *conn;
static Eldbus_Service_Interface *iface;

const char *get_signal_name(const Signal signal) {

	switch (signal) {
	case READING_STARTED:
		{ return "ReadingStarted";}
	case READING_STOPPED:
		{ return "ReadingStopped";}
	case READING_CANCELLED:
		{ return "ReadingCancelled";}
	case READING_SKIPPED:
		{ return "ReadingSkipped";}
	default:
		{ return "UnknownSignal";}
	}
}

static const Eldbus_Signal _signals[] = {
/*    [READING_STARTED] = {"ReadingStarted", ELDBUS_ARGS({"i", NULL}), 0},
    [READING_STOPPED] = {"ReadingStopped", ELDBUS_ARGS({"i", NULL}), 0},
    [READING_CANCELLED] = {"ReadingCancelled", ELDBUS_ARGS({"i", NULL}), 0},*/
    [0] = {"ReadingStateChanged", ELDBUS_ARGS({"is", NULL}), 0},
    { }
};

static Eldbus_Message *
_consume_read_command(const Eldbus_Service_Interface *iface EINA_UNUSED, const Eldbus_Message *msg)
{
	DEBUG("[START] CONSUMING READ COMMAND");
	assert(msg);
	Eldbus_Message *reply = eldbus_message_method_return_new(msg);
	if (reply) {
		Eina_Bool discardable = EINA_TRUE;
		char *text = "";
		void *rc_id = NULL;
		// read the parameters
		if (!eldbus_message_arguments_get(msg, "sb", &text, &discardable)) {
			ERROR("eldbus_message_arguments_get() failed to parse arguments of type String and Boolean from message msg=%p", msg);
		} else {
			DEBUG("RECIVED DBUS MESSAGE WITH ARGS: text(%s), discardable(%d)", text, discardable);
			rc_id = (void *)tts_speak_customized(text, EINA_TRUE, discardable, NULL);
		}
		if (!eldbus_message_arguments_append(reply, "sbi", text, discardable, rc_id))
			ERROR("eldbus_message_arguments_append() has failed to append arguments to reply message reply=%p", reply);
	} else {
		ERROR("eldbus_message_method_return_new(%p) has failed to create a reply message", msg);
	}
	DEBUG("[END] CONSUMING READ COMMAND");
	return reply;
}

static const Eldbus_Method _methods[] = {
	{"ReadCommand", ELDBUS_ARGS({"s", "text"}, {"b", "discardable"}), ELDBUS_ARGS({"s", "text"}, {"b", "discardable"}, {"i", "rc_id"}, {"i", "err_id"}), _consume_read_command},
	{ }
};

static const Eldbus_Service_Interface_Desc iface_desc = {
	INTERFACE, _methods, _signals
};

static void
on_name_request(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
	assert(msg);
	unsigned int reply;
	const char *errname, *errmsg;
	DEBUG("[START] on_name_request");

	if (eldbus_message_error_get(msg, &errname, &errmsg)) {
		ERROR("NameRequest failed: %s %s", errname, errmsg);
		return;
	}

	if (!eldbus_message_arguments_get(msg, "u", &reply)) {
		ERROR("error geting arguments from NameRequest message");
		return;
	}

	if (reply != ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER) {
		ERROR("error name already in use");
		return;
	}

	DEBUG("[END] on_name_request");
}

void _get_a11y_address(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
	assert(msg);
	const char *sock_addr;
	const char *errname, *errmsg;
	Eldbus_Connection *session = data;

	DEBUG("[START]");
	conn = NULL;

	if (eldbus_message_error_get(msg, &errname, &errmsg)) {
		ERROR("GetAddress failed: %s %s", errname, errmsg);
		return;
	}

	if (eldbus_message_arguments_get(msg, "s", &sock_addr) && sock_addr) {
		DEBUG("A11 socket %s", sock_addr);
		if ((conn = eldbus_address_connection_get(sock_addr))) {
			iface = eldbus_service_interface_register(conn, PATH, &iface_desc);
			if (iface) {
				DEBUG("RequestName start");
				eldbus_name_request(conn, BUS, ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE,
					on_name_request, NULL);
				DEBUG("RequestName end");
			} else {
				ERROR("Failed to register %s interface", INTERFACE);
			}
		} else {
			ERROR("Failed to connect to %s", sock_addr);
		}
	} else {
		ERROR("Could not get A11Y Bus socket address.");
	}

	if (iface == NULL && conn != NULL) {
		eldbus_connection_unref(conn);
		conn = NULL;
	}
	if (session != NULL)
		 eldbus_connection_unref(session);
	DEBUG("[END]");
}

int dbus_direct_reading_init(void)
{
	Eldbus_Connection *session = NULL;
	Eldbus_Message *msg = NULL;
	int ret = -1;

	DEBUG("[START]");
	if (eldbus_init() == 0) {
		ERROR("Unable to initialize eldbus module");
		return ret;
	};

	session = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
	if (session) {
		msg = eldbus_message_method_call_new("org.a11y.Bus", "/org/a11y/bus", "org.a11y.Bus", "GetAddress");
		if (msg) {
			if (eldbus_connection_send(session, msg, _get_a11y_address, session, -1)) {
				ret = 0;
			} else {
				ERROR("Message send failed");
			}
		}
	}
	// successfull call of eldbus_connection_send should already release resources
	if (msg != NULL && ret != 0)
		eldbus_message_unref(msg);
	if (session != NULL && ret != 0)
		eldbus_connection_unref(session);
	return ret;
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


int dbus_direct_reading_adapter_emit(const Signal signal, const void *data)
{
	if (!iface) {
		ERROR("Interface: %s not found on dbus", INTERFACE);
		return -1;
	}
	if (!strcmp("UnknownSignal", get_signal_name(signal))) {
		ERROR("Trying to emit unknown signal(%d)", signal);
		return -1;
	}
	if (!data) {
		ERROR("Trying to emit signal: %s which is not associated with any reading command", get_signal_name(signal));
		return -1;
	}
	if (!eldbus_service_signal_emit(iface, 0, data, get_signal_name(signal))) {
		ERROR("Unable to send %s signal", get_signal_name(signal));
		return -1;
	} else {
		DEBUG("Successfully send %s singal", get_signal_name(signal));
		return 0;
	}
}