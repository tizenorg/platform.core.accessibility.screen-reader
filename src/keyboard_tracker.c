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

#include <string.h>
#include <atspi/atspi.h>
#include <Ecore.h>
#include <vconf.h>
#ifdef X11_ENABLED
#include <Ecore_X.h>
#endif
#include "keyboard_tracker.h"
#include "logger.h"
#include "screen_reader_tts.h"
static AtspiDeviceListener *listener;
static AtspiDeviceListener *async_listener;
#ifndef X11_ENABLED
static int keyboardX = 0;
static int keyboardY = 0;
static int keyboardW = 0;
static int keyboardH = 0;

#ifndef SCREEN_READER_TV
extern AtspiAccessible *top_win;
#endif

#define E_KEYBOARD_SERVICE_BUS_NAME "org.tizen.keyboard"
#define E_KEYBOARD_SERVICE_NAVI_IFC_NAME "org.tizen.KBGestureNavigation"
#define E_KEYBOARD_SERVICE_NAVI_OBJ_PATH "/org/tizen/KBGestureNavigation"

static Eldbus_Connection *conn = NULL;
static Eldbus_Service_Interface *iface = NULL;
#define KB_GESTURE_SIGNAL 0
static const Eldbus_Signal signals[] = {
   [KB_GESTURE_SIGNAL] = {"KBGestureDetected",
                                ELDBUS_ARGS({"i", "type"},{"i", "x"},{"i", "y"}),0},
                                { }
};

static const Eldbus_Service_Interface_Desc iface_desc = {
		E_KEYBOARD_SERVICE_NAVI_IFC_NAME, NULL, signals
};
#endif

#ifndef X11_ENABLED
static int prev_keyboard_state = VCONFKEY_ISF_INPUT_PANEL_STATE_HIDE;
#endif

#ifdef X11_ENABLED
static Ecore_Event_Handler *root_xwindow_property_changed_hld = NULL;
static Ecore_Event_Handler *active_xwindow_property_changed_hld = NULL;

static void _check_keyboard_state(Ecore_X_Window keyboard_win)
{
	Ecore_X_Virtual_Keyboard_State keyboard_state;
	static Ecore_X_Virtual_Keyboard_State last_keyboard_state = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;

	if (!keyboard_win)
	{
		return;
	}

	keyboard_state = ecore_x_e_virtual_keyboard_state_get(keyboard_win);
	if (keyboard_state == last_keyboard_state)
	{
		return;
	}

	if (keyboard_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
	{
		tts_speak (_("IDS_VISUAL_KEYBOARD_ENABLED"), EINA_FALSE);
		last_keyboard_state = keyboard_state;
	}
	else if (keyboard_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
	{
		tts_speak (_("IDS_VISUAL_KEYBOARD_DISABLED"), EINA_FALSE);
		last_keyboard_state = keyboard_state;
	}
}

static Eina_Bool _active_xwindow_property_changed_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Property *wp;
	wp = (Ecore_X_Event_Window_Property *)event;

	if (!wp)
	{
		return EINA_FALSE;
	}

	if (wp->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
	{
		DEBUG("keyboard state event");
		_check_keyboard_state(wp->win);
	}

	return EINA_TRUE;
}

void active_xwindow_property_tracker_register()
{
	Ecore_X_Window active_window = 0;
	ecore_x_window_prop_xid_get(ecore_x_window_root_first_get(), ECORE_X_ATOM_NET_ACTIVE_WINDOW, ECORE_X_ATOM_WINDOW, &active_window, 1);
	if (active_window)
	{
		ecore_x_event_mask_set(active_window, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
		active_xwindow_property_changed_hld = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _active_xwindow_property_changed_cb, NULL);
	}
}

void active_xwindow_property_tracker_unregister()
{
	if (active_xwindow_property_changed_hld)
	{
		ecore_event_handler_del(active_xwindow_property_changed_hld);
		active_xwindow_property_changed_hld = NULL;
	}
}

static Eina_Bool _root_xwindow_property_changed_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Property *wp;
	wp = (Ecore_X_Event_Window_Property *)event;

	if (!wp)
	{
		return EINA_FALSE;
	}

	if (wp->atom == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
	{
		DEBUG("active window change");
		active_xwindow_property_tracker_unregister();
		active_xwindow_property_tracker_register();
	}

	return EINA_TRUE;
}

void root_xwindow_property_tracker_register()
{
	Ecore_X_Window root_window;

	root_window = ecore_x_window_root_first_get();
	if (root_window)
	{
		ecore_x_event_mask_set(root_window, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
		root_xwindow_property_changed_hld = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _root_xwindow_property_changed_cb, NULL);
	}
}

void root_xwindow_property_tracker_unregister()
{
	if (root_xwindow_property_changed_hld)
	{
		ecore_event_handler_del(root_xwindow_property_changed_hld);
		root_xwindow_property_changed_hld = NULL;
	}
}
#endif
static gboolean async_keyboard_cb(const AtspiDeviceEvent * stroke, void *data)
{
	DEBUG("AT-SPI DEVICE EVENT: ID(%d) STRING(%s) TYPE(%d) HW_CODE(%d) MODIFIERS(%d) TIMESTAMP(%d)", stroke->id, stroke->event_string,stroke->type,stroke->hw_code, stroke->modifiers, stroke->timestamp);
	if (!strcmp(stroke->event_string, "XF86Back"))
	{
		tts_speak(_("IDS_BACK_BUTTON"), EINA_TRUE);
		return TRUE;
	}
	else
		return FALSE;
}

void keyboard_changed_cb(keynode_t * node, void *user_data)
{
	DEBUG("START");
	int keyboard_state;
	int ret = vconf_get_int(VCONFKEY_ISF_INPUT_PANEL_STATE, &keyboard_state);
	if (ret != 0) {
		ERROR("ret == %d", ret);
		return;
	}
	if (keyboard_state == prev_keyboard_state) return;
	if (keyboard_state == VCONFKEY_ISF_INPUT_PANEL_STATE_SHOW) {
		tts_speak (_("IDS_KEYBOARD_SHOWN"), EINA_FALSE);
		DEBUG("tts_speak Keyboard shown\n");
		prev_keyboard_state = keyboard_state;
	}
	else if (keyboard_state == VCONFKEY_ISF_INPUT_PANEL_STATE_HIDE) {
		tts_speak (_("IDS_KEYBOARD_HIDDEN"), EINA_FALSE);
		DEBUG("tts_speak keyboard hidden\n");
		prev_keyboard_state = keyboard_state;
	}
}

void _set_vconf_key_changed_callback_keyboard_status()
{
	DEBUG("START");
	int keyboard_state;
	int ret = vconf_get_int(VCONFKEY_ISF_INPUT_PANEL_STATE, &keyboard_state);
	if (ret != 0) {
		ERROR("ret == %d", ret);
		return;
	}
	//If keyboard is already open/hidden do not read.
	ret = vconf_notify_key_changed(VCONFKEY_ISF_INPUT_PANEL_STATE, keyboard_changed_cb, NULL);
	if (ret != 0)
		DEBUG("Could not add notify callback to VCONFKEY_ISF_INPUT_PANEL_STATE key");
	DEBUG("END");
}

void _unset_vconf_key_changed_callback_keyboard_status()
{
	DEBUG("START");
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_ISF_INPUT_PANEL_STATE, keyboard_changed_cb);
	if (ret != 0)
		DEBUG("Could not delete notify callback to VCONFKEY_ISF_INPUT_PANEL_STATE key");
	DEBUG("END");
}

void keyboard_tracker_init(void)
{
	async_listener = atspi_device_listener_new(async_keyboard_cb, NULL, NULL);
	atspi_register_keystroke_listener(async_listener, NULL, 0, 1 << ATSPI_KEY_RELEASED_EVENT, ATSPI_KEYLISTENER_NOSYNC, NULL);

#ifdef X11_ENABLED
	active_xwindow_property_tracker_register();
	root_xwindow_property_tracker_register();
#else
	eldbus_init();
	conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
	eldbus_name_request(conn, E_KEYBOARD_SERVICE_BUS_NAME,
						ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE, NULL, NULL);
	iface = eldbus_service_interface_register(conn, E_KEYBOARD_SERVICE_NAVI_OBJ_PATH, &iface_desc);
	_set_vconf_key_changed_callback_keyboard_status();
#endif
	DEBUG("keyboard tracker init");
}

void keyboard_tracker_shutdown(void)
{
	atspi_deregister_keystroke_listener(listener, NULL, 0, 1 << ATSPI_KEY_PRESSED, NULL);
	atspi_deregister_keystroke_listener(async_listener, NULL, 0, 1 << ATSPI_KEY_RELEASED_EVENT, NULL);
#ifdef X11_ENABLED
	root_xwindow_property_tracker_unregister();
	active_xwindow_property_tracker_unregister();
#else
	eldbus_name_release(conn, E_KEYBOARD_SERVICE_BUS_NAME, NULL, NULL);
	eldbus_connection_unref(conn);
	eldbus_shutdown();
	_unset_vconf_key_changed_callback_keyboard_status();
#endif
	DEBUG("keyboard tracker shutdown");
}

#ifndef X11_ENABLED
void keyboard_geometry_set(int x, int y, int width, int height)
{
	keyboardX = x;
	keyboardY = y;
	keyboardW = width;
	keyboardH = height;
}

void keyboard_geometry_get(int *x, int *y, int *width, int *height)
{
	*x = keyboardX;
	*y = keyboardY;
	*width = keyboardW;
	*height = keyboardH;
}

Eina_Bool keyboard_event_status(int x, int y)
{
#ifndef SCREEN_READER_TV
	gchar* name = NULL;
	if (prev_keyboard_state == VCONFKEY_ISF_INPUT_PANEL_STATE_SHOW) {
		if (top_win)
			name = atspi_accessible_get_name(top_win, NULL);
		if (name && strcmp(name, "Quickpanel Window")) {
			if ((y >= keyboardY) && (y <= (keyboardY + keyboardH)) && (x >= keyboardX) && (x <= (keyboardX + keyboardW))) {
				g_free(name);
				return EINA_TRUE;
			}
		}
		g_free(name);
	}
#endif
	return EINA_FALSE;
}

void keyboard_signal_emit(int type, int x, int y)
{
	if (!conn) return;
	if (!iface) return;

	eldbus_service_signal_emit(iface, KB_GESTURE_SIGNAL, type, x, y);
}
#endif
