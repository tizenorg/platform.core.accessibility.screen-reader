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
#include <Ecore_X.h>
#include "keyboard_tracker.h"
#include "logger.h"

static AtspiDeviceListener *listener;
static Keyboard_Tracker_Cb user_cb;
static void *user_data;
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

static gboolean device_cb(const AtspiDeviceEvent * stroke, void *data)
{
	Key k;
	if (!strcmp(stroke->event_string, "KP_Up"))
		k = KEY_UP;
	else if (!strcmp(stroke->event_string, "KP_Down"))
		k = KEY_DOWN;
	else if (!strcmp(stroke->event_string, "KP_Left"))
		k = KEY_LEFT;
	else if (!strcmp(stroke->event_string, "KP_Right"))
		k = KEY_RIGHT;
	else
		return FALSE;

	if (user_cb)
		user_cb(user_data, k);

	return TRUE;
}

void keyboard_tracker_init(void)
{
	listener = atspi_device_listener_new(device_cb, NULL, NULL);
	atspi_register_keystroke_listener(listener, NULL, 0, ATSPI_KEY_PRESSED, ATSPI_KEYLISTENER_SYNCHRONOUS | ATSPI_KEYLISTENER_CANCONSUME, NULL);
	active_xwindow_property_tracker_register();
	root_xwindow_property_tracker_register();
	DEBUG("keyboard tracker init");
}

void keyboard_tracker_register(Keyboard_Tracker_Cb cb, void *data)
{
	user_cb = cb;
	user_data = data;
}

void keyboard_tracker_shutdown(void)
{
	atspi_deregister_keystroke_listener(listener, NULL, 0, ATSPI_KEY_PRESSED, NULL);
	root_xwindow_property_tracker_unregister();
	active_xwindow_property_tracker_unregister();
	DEBUG("keyboard tracker shutdown");
}
