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

#include <Elementary.h>
#include <vconf.h>
#include "screen_reader_vconf.h"
#include "screen_reader_spi.h"
#include "logger.h"

#ifdef RUN_IPC_TEST_SUIT
#include "test_suite/test_suite.h"
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SCREEN READER VCONF"

keylist_t *keys = NULL;

// ------------------------------ vconf callbacks----------------------

void app_termination_cb(keynode_t * node, void *user_data)
{
	DEBUG("START");
	DEBUG("Application terminate %d", !node->value.i);

	Service_Data *service_data = user_data;
	service_data->run_service = node->value.i;

	if (service_data->run_service == 0) {
		elm_exit();
	}

	DEBUG("END");
}

void display_language_cb(keynode_t * node, void *user_data)
{
	DEBUG("START");
	DEBUG("Trying to set LC_MESSAGES to: %s", node->value.s);

	Service_Data *sd = user_data;
	snprintf(sd->display_language, LANGUAGE_NAME_SIZE, "%s", node->value.s);
	//to make gettext work
	setenv("LC_MESSAGES", sd->display_language, 1);

	DEBUG("END");
}

// --------------------------------------------------------------------

int get_key_values(Service_Data * sd)
{
	DEBUG("START");
	int to_ret = 0;

	char *display_language = vconf_get_str("db/menu_widget/language");
	if (display_language) {
		snprintf(sd->display_language, LANGUAGE_NAME_SIZE, "%s", display_language);
		//to make gettext work
		setenv("LC_MESSAGES", sd->display_language, 1);
		free(display_language);
	} else
		WARNING("Can't get db/menu_widget/language value");

	DEBUG("SCREEN READER DATA SET TO: Display_Language: %s, Tracking signal: %s;", sd->display_language, sd->tracking_signal_name);

	DEBUG("END");
	return to_ret;
}

int _set_vconf_callback_and_print_message_on_error_and_return_error_code(const char *in_key, vconf_callback_fn cb, void *user_data)
{
	int ret = vconf_notify_key_changed(in_key, cb, user_data);
	if (ret != 0)
		DEBUG("Could not add notify callback to %s key", in_key);

	return ret;
}

bool vconf_init(Service_Data * service_data)
{
	DEBUG("--------------------- VCONF_init START ---------------------");
	int ret = 0;

	if (vconf_set(keys)) {
		DEBUG("nothing is written\n");
	} else {
		DEBUG("everything is written\n");
	}

	vconf_keylist_free(keys);
	// ----------------------------------------------------------------------------------

	ret = get_key_values(service_data);
	if (ret != 0) {
		DEBUG("Could not set data from vconf: %d", ret);
	}

	_set_vconf_callback_and_print_message_on_error_and_return_error_code("db/menu_widget/language", display_language_cb, service_data);

	DEBUG("---------------------- VCONF_init END ----------------------\n\n");
	return true;
}
