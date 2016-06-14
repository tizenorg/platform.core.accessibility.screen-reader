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
#ifdef X11_ENABLED
#include <Ecore_X.h>
#endif
#include "keyboard_tracker.h"
#include "logger.h"
#include "screen_reader_tts.h"
static AtspiDeviceListener *listener;
static AtspiDeviceListener *async_listener;

#ifndef X11_ENABLED
static AtspiEventListener *keyboard_listener;
static Eina_Bool prev_keyboard_state = EINA_FALSE;
extern AtspiAccessible* top_window;
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
		tts_speak (_("IDS_KEYBOARD_SHOWN"), EINA_FALSE);
		last_keyboard_state = keyboard_state;
	}
	else if (keyboard_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
	{
		tts_speak (_("IDS_KEYBOARD_HIDDEN"), EINA_FALSE);
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
	if (!strcmp(stroke->event_string, "XF86Back"))
	{
		tts_speak(_("IDS_BACK_BUTTON"), EINA_TRUE);
		return TRUE;
	}
	else
		return FALSE;
}

static void _keyboard_cb(const AtspiEvent * event)
{
	Eina_Bool keyboard_state = EINA_FALSE;
	if (strcmp(atspi_accessible_get_name(top_window, NULL), atspi_accessible_get_name(event->source, NULL))) return;
	if (!strcmp(event->type, "object:state-changed:keyboard") &&
			(atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_WINDOW)) {
		keyboard_state = (Eina_Bool)event->detail1;
		if (keyboard_state == prev_keyboard_state) return;
		if (keyboard_state) {
			tts_speak (_("IDS_KEYBOARD_SHOWN"), EINA_FALSE);
			DEBUG("tts_speak Keyboard shown\n");
			prev_keyboard_state = keyboard_state;
		}
		else {
			tts_speak (_("IDS_KEYBOARD_HIDDEN"), EINA_FALSE);
			DEBUG("tts_speak keyboard hidden\n");
			prev_keyboard_state = keyboard_state;
		}
	}
}

void keyboard_tracker_init(void)
{
	//Below two lines causes issue when keyboard is opened, back key press gets handled by both keyboard and lower object, for e.g: naviframe pop also happens
	//along with hiding keyboard.
	async_listener = atspi_device_listener_new(async_keyboard_cb, NULL, NULL);
	atspi_register_keystroke_listener(async_listener, NULL, 0, 1 << ATSPI_KEY_RELEASED_EVENT, ATSPI_KEYLISTENER_NOSYNC, NULL);
#ifdef X11_ENABLED
	active_xwindow_property_tracker_register();
	root_xwindow_property_tracker_register();
#else
	keyboard_listener = atspi_event_listener_new_simple(_keyboard_cb, NULL);
	atspi_event_listener_register(keyboard_listener, "object:state-changed:keyboard", NULL);
#endif
	DEBUG("keyboard tracker init");
}

void keyboard_tracker_shutdown(void)
{
	//Below two lines causes issue when keyboard is opened, back key press gets handled by both keyboard and lower object, for e.g: naviframe pop also happens
	//along with hiding keyboard.
	atspi_deregister_keystroke_listener(listener, NULL, 0, 1 << ATSPI_KEY_PRESSED, NULL);
	atspi_deregister_keystroke_listener(async_listener, NULL, 0, 1 << ATSPI_KEY_RELEASED_EVENT, NULL);
#ifdef X11_ENABLED
	root_xwindow_property_tracker_unregister();
	active_xwindow_property_tracker_unregister();
#else
	atspi_event_listener_deregister(keyboard_listener, "object:state-changed:keyboard", NULL);
	g_object_unref(keyboard_listener);
	keyboard_listener = NULL;
#endif
	DEBUG("keyboard tracker shutdown");
}
