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
#include "keyboard_tracker.h"
#include "logger.h"

static AtspiDeviceListener *listener;
static Keyboard_Tracker_Cb user_cb;
static void *user_data;

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
	atspi_init();
	listener = atspi_device_listener_new(device_cb, NULL, NULL);
	atspi_register_keystroke_listener(listener, NULL, 0, ATSPI_KEY_PRESSED, ATSPI_KEYLISTENER_SYNCHRONOUS | ATSPI_KEYLISTENER_CANCONSUME, NULL);
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
	DEBUG("keyboard tracker shutdown");
}
