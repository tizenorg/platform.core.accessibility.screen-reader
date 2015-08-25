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

#include "elm_access_adapter.h"
#include "logger.h"

static void _get_root_coords(Ecore_X_Window win, int *x, int *y)
{
	Ecore_X_Window root = ecore_x_window_root_first_get();
	Ecore_X_Window parent = ecore_x_window_parent_get(win);
	int wx, wy;

	if (x)
		*x = 0;
	if (y)
		*y = 0;

	while (parent && (root != parent)) {
		ecore_x_window_geometry_get(parent, &wx, &wy, NULL, NULL);
		if (x)
			*x += wx;
		if (y)
			*y += wy;
		parent = ecore_x_window_parent_get(parent);
	}
}

static void _send_ecore_x_client_msg(Ecore_X_Window win, int x, int y, Eina_Bool activate)
{
	int x_win, y_win;
	long type;
	_get_root_coords(win, &x_win, &y_win);
	DEBUG("Window screen size:%d %d", x_win, y_win);
	DEBUG("activate keyboard: %d %d", x, y);

	if (activate)
		type = ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ACTIVATE;
	else
		type = ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ;

	ecore_x_client_message32_send(win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, win, type, x - x_win, y - y_win, 0);
}

void elm_access_adaptor_emit_activate(Ecore_X_Window win, int x, int y)
{
	_send_ecore_x_client_msg(win, x, y, EINA_TRUE);
}

void elm_access_adaptor_emit_read(Ecore_X_Window win, int x, int y)
{
	_send_ecore_x_client_msg(win, x, y, EINA_FALSE);
}
