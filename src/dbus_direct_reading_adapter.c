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

#define E_A11Y_SERVICE_BUS_NAME "org.tizen.ScreenReader"
#define E_A11Y_SERVICE_DIRECT_READING_IFC_NAME "org.tizen.DirectReading"
#define E_A11Y_SERVICE_DIRECT_READING_OBJ_PATH "/org/tizen/DirectReading"

static Eldbus_Connection *conn;
static Eldbus_Service_Interface *iface;

static char *get_signal_name(const Signal signal) {

	switch (signal) {
	case READING_STARTED:
		{ return "ReadingStarted";}
	case READING_STOPPED:
		{ return "ReadingStopped";}
	case READING_CANCELLED:
		{ return "ReadingCancelled";}
	default:
		{ return "UnknownSignal";}
	}
}

static const Eldbus_Signal _signals[] = {
	[READING_STARTED] = {"ReadingStarted", ELDBUS_ARGS({"so", NULL}), 0},
	[READING_STOPPED] = {"ReadingStopped", ELDBUS_ARGS({"so", NULL}), 0},
	[READING_CANCELLED] = {"ReadingCancelled", ELDBUS_ARGS({"so", NULL}), 0},
	{ }
};

static Eldbus_Message *_consume_read_command(const Eldbus_Service_Interface *iface EINA_UNUSED, const Eldbus_Message *msg) {
	DEBUG("dbus_direct_reading_adapter CONSUMING READ COMMAND: BEGIN");
	Eldbus_Message *reply = eldbus_message_method_return_new(msg);
	bool interruptable = true;
	char* text = "";
	// read the parameters
	if (!eldbus_message_arguments_get(msg, "sb", &text, &interruptable))
		printf("eldbus_message_arguments_get() error\n");
	DEBUG("RECIVED DBUS MESSAGE: text(%s), interruptable(%d)", text, interruptable);
	tts_speak_customized(text,EINA_TRUE,interruptable);
	eldbus_message_arguments_append(reply, "sb", text, interruptable);
	return reply;
}

static const Eldbus_Method _methods[] = {
	{"ReadCommand", ELDBUS_ARGS({"s", "text"},{"b","interruptable"}), NULL, _consume_read_command},
	{ }
};

static const Eldbus_Service_Interface_Desc _iface_desc = {
	E_A11Y_SERVICE_DIRECT_READING_IFC_NAME, _methods, _signals
};

void _on_name_request(void *data, const Eldbus_Message * msg, Eldbus_Pending * pending EINA_UNUSED) {
	Eldbus_Service_Interface *iface;
	unsigned int reply;
	const char *errname;
	const char *errmsg;
	iface = data;

	if (eldbus_message_error_get(msg, &errname, &errmsg)) {
		ERROR("_on_name_get failed: %s %s", errname, errmsg);
		return;
	}

	if (!eldbus_message_arguments_get(msg, "u", &reply)) {
		ERROR("Could not get argument of _on_name_get.");
		return;
	}
	DEBUG("_on_name_get argument: %d", reply);
}

void dbus_direct_reading_adapter_init(void)
{

	eldbus_init();

	if (!(conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM))) {
		ERROR("Unable to get system connection");
		return;
	}

	if (!(iface = eldbus_service_interface_register(conn, E_A11Y_SERVICE_DIRECT_READING_OBJ_PATH, &_iface_desc))) {
		ERROR("Failed to register %s interface", E_A11Y_SERVICE_DIRECT_READING_IFC_NAME);
		eldbus_connection_unref(conn);
		conn = NULL;
		return;
	}

	eldbus_name_request(conn, E_A11Y_SERVICE_BUS_NAME, ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE, _on_name_request, iface);

}

void dbus_direct_reading_adapter_shutdown(void)
{
	if (iface)
		eldbus_service_object_unregister(iface);
	if (conn)
		eldbus_connection_unref(conn);

	conn = NULL;
	iface = NULL;

	eldbus_shutdown();
}

void dbus_direct_reading_adapter_emit(const Signal signal, const char *bus_name, const char *object_path)
{
	if (!iface)
		return;
	if (!bus_name || !object_path)
		return;

	if (!eldbus_service_signal_emit(iface, signal, bus_name, object_path)) {
		ERROR("Unable to send %s signal", get_signal_name(signal));
	} else
		DEBUG("Successfull send %s singal", get_signal_name(signal));
}
