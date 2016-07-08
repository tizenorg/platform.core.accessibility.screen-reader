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
#include "window_tracker.h"
#include "logger.h"

#ifdef X11_ENABLED
#include <Ecore_X.h>
#include <Ecore_X_Atoms.h>
#endif

static Window_Tracker_Cb user_cb;
static void *user_data;
static AtspiEventListener *listener;
static AtspiAccessible *last_active_win;
AtspiAccessible *top_win;

static void _on_atspi_window_cb(const AtspiEvent * event)
{
	gchar *name = atspi_accessible_get_name(event->source, NULL);
	DEBUG("Event: %s: %s", event->type, name);

	if (!strcmp(event->type, "window:activate") && last_active_win != event->source)	//if we got activate 2 times
	{
		if (user_cb)
			user_cb(user_data, event->source);
		last_active_win = event->source;
		top_win = last_active_win;
	}

	if (!strcmp(event->type, "object:state-changed:visible") && !strcmp(name, "Quickpanel Window"))
	{
		if (event->detail1)
		{
			if (user_cb)
				user_cb(user_data, event->source);
			top_win = event->source;
		}
		else
		{
			if (user_cb)
				user_cb(user_data, last_active_win);
			top_win = last_active_win;
		}
	}

	g_free(name);
}

static AtspiAccessible *_get_active_win(void)
{
	DEBUG("START");
	int i, j, desktop_children_count, app_children_count;
	last_active_win = NULL;
	AtspiAccessible *desktop = atspi_get_desktop(0);
	if (!desktop) {
		ERROR("DESKTOP NOT FOUND");
		return NULL;
	}

	unsigned int active_window_pid = 0;
#ifdef X11_ENABLED
	Ecore_X_Window focus_window = ecore_x_window_focus_get();

	if (focus_window) {
		//invoking atspi_accessible_get_child_count for non active apps results in very long screen-reader startup
		//not active apps have low priority and dbus calls take a lot of time (a few hundred ms per call)
		//Hence we first try to determine accessible window using pid of currently focused window
		if (!ecore_x_window_prop_card32_get(focus_window, ECORE_X_ATOM_NET_WM_PID, &active_window_pid, 1))
			active_window_pid = 0;
		if (active_window_pid)
			DEBUG("First we will try filter apps by PID: %i", active_window_pid);
	}
#endif
	desktop_children_count = atspi_accessible_get_child_count(desktop, NULL);
	DEBUG("Desktop children count = %d", desktop_children_count);
	for (i = 0; i < desktop_children_count; i++) {
		AtspiAccessible *app = atspi_accessible_get_child_at_index(desktop, i, NULL);
		if (!app) continue;
		if (active_window_pid == 0 || active_window_pid == atspi_accessible_get_process_id(app, NULL))
			app_children_count = atspi_accessible_get_child_count(app, NULL);
		else
			app_children_count = 0;
		for (j = 0; j < app_children_count; j++) {
			AtspiAccessible *win = atspi_accessible_get_child_at_index(app, j, NULL);
			AtspiStateSet *states = atspi_accessible_get_state_set(win);
			AtspiRole role = atspi_accessible_get_role(win, NULL);
			if ((atspi_state_set_contains(states, ATSPI_STATE_ACTIVE)) && (role == ATSPI_ROLE_WINDOW))
				last_active_win = win;

			g_object_unref(states);
			g_object_unref(win);

			if (last_active_win)
				break;
		}
		g_object_unref(app);
		if (active_window_pid > 0 && (i == desktop_children_count - 1)) {
			// we are in last iteration and we should fall back to normal iteration over child windows
			// without filtering by focus windows PID
			i = -1;
			active_window_pid = 0;
		}
		if (last_active_win)
			break;
	}
	g_object_unref(desktop);
	DEBUG("END last_active_win: %p", last_active_win);
	top_win = last_active_win;
	return last_active_win;
}

void window_tracker_init(void)
{
	DEBUG("START");
	listener = atspi_event_listener_new_simple(_on_atspi_window_cb, NULL);
	atspi_event_listener_register(listener, "window:activate", NULL);
	atspi_event_listener_register(listener, "object:state-changed:visible", NULL);
}

void window_tracker_shutdown(void)
{
	DEBUG("START");
	atspi_event_listener_deregister(listener, "window:activate", NULL);
	atspi_event_listener_deregister(listener, "object:state-changed:visible", NULL);
	g_object_unref(listener);
	listener = NULL;
	user_cb = NULL;
	user_data = NULL;
	last_active_win = NULL;
	top_win = NULL;
}

void window_tracker_register(Window_Tracker_Cb cb, void *data)
{
	DEBUG("START");
	user_cb = cb;
	user_data = data;
}

void window_tracker_active_window_request(void)
{
	DEBUG("START");
	_get_active_win();
	if (user_cb)
		user_cb(user_data, last_active_win);
}
