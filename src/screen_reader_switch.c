#include "logger.h"
#include <Eldbus.h>

Eina_Bool screen_reader_switch_enabled_get(Eina_Bool * value)
{
	Eldbus_Connection *conn;
	Eldbus_Object *dobj;
	Eldbus_Proxy *proxy;
	Eldbus_Message *req, *reply;
	const char *errname = NULL, *errmsg = NULL;
	Eina_Bool ret = EINA_FALSE;
	Eldbus_Message_Iter *iter;

	eldbus_init();

	if (!(conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION))) {
		ERROR("Connection to session bus failed");
		return EINA_FALSE;
	}
	if (!(dobj = eldbus_object_get(conn, "org.a11y.Bus", "/org/a11y/bus"))) {
		ERROR("Failed to create eldbus object for /org/a11y/bus");
		goto fail_obj;
	}
	if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.DBus.Properties"))) {
		ERROR("Failed to create proxy object for 'org.freedesktop.DBus.Properties'");
		goto fail_proxy;
	}
	if (!(req = eldbus_proxy_method_call_new(proxy, "Get"))) {
		ERROR("Failed to create method call on org.freedesktop.DBus.Properties.Get");
		goto fail_proxy;
	}
	eldbus_message_ref(req);

	if (!eldbus_message_arguments_append(req, "ss", "org.a11y.Status", "ScreenReaderEnabled")) {
		ERROR("Failed to append message args");
		goto fail_msg;
	}

	reply = eldbus_proxy_send_and_block(proxy, req, 100);
	if (!reply || eldbus_message_error_get(reply, &errname, &errmsg)) {
		ERROR("Unable to call method org.freedesktop.DBus.Properties.Get: %s %s", errname, errmsg);
		goto fail_msg;
	}

	if (!eldbus_message_arguments_get(reply, "v", &iter)) {
		ERROR("Invalid answer signature");
		goto fail_msg;
	} else {
		if (!eldbus_message_iter_arguments_get(iter, "b", value)) {
			ERROR("Invalid variant signature");
		} else
			ret = EINA_TRUE;
	}

 fail_msg:
	eldbus_message_unref(req);
 fail_proxy:
	eldbus_object_unref(dobj);
 fail_obj:
	eldbus_connection_unref(conn);

	eldbus_shutdown();

	return ret;
}

Eina_Bool screen_reader_switch_enabled_set(Eina_Bool value)
{
	Eldbus_Connection *conn;
	Eldbus_Object *dobj;
	Eldbus_Proxy *proxy;
	Eldbus_Message *req, *reply;
	const char *errname = NULL, *errmsg = NULL;
	Eina_Bool ret = EINA_FALSE;
	Eldbus_Message_Iter *iter;

	eldbus_init();

	if (!(conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION))) {
		ERROR("Connection to session bus failed");
		return EINA_FALSE;
	}
	if (!(dobj = eldbus_object_get(conn, "org.a11y.Bus", "/org/a11y/bus"))) {
		ERROR("Failed to create eldbus object");
		goto fail_obj;
	}
	if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.DBus.Properties"))) {
		ERROR("Failed to create proxy object for 'org.freedesktop.DBus.Properties'");
		goto fail_proxy;
	}
	if (!(req = eldbus_proxy_method_call_new(proxy, "Set"))) {
		ERROR("Failed to create method call on org.freedesktop.DBus.Properties.Set");
		goto fail_proxy;
	}
	eldbus_message_ref(req);

	if (!eldbus_message_arguments_append(req, "ss", "org.a11y.Status", "ScreenReaderEnabled")) {
		ERROR("Failed to append message args");
		goto fail_msg;
	}
	if (!(iter = eldbus_message_iter_container_new(eldbus_message_iter_get(req), 'v', "b"))) {
		ERROR("Unable to create variant iterator");
		goto fail_msg;
	}
	if (!eldbus_message_iter_arguments_append(iter, "b", value)) {
		ERROR("Unable to append to variant iterator");
		goto fail_msg;
	}
	if (!eldbus_message_iter_container_close(eldbus_message_iter_get(req), iter)) {
		ERROR("Failed to close variant iterator");
		goto fail_msg;
	}
	eldbus_proxy_send(proxy, req, NULL, NULL, -1.0);
	ret = EINA_TRUE;

 fail_msg:
	eldbus_message_unref(req);
 fail_proxy:
	eldbus_object_unref(dobj);
 fail_obj:
	eldbus_connection_unref(conn);

	eldbus_shutdown();

	return ret;
}
