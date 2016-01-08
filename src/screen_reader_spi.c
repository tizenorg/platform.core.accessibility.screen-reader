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

#define _GNU_SOURCE

#include <stdio.h>
#include "screen_reader_spi.h"
#include "screen_reader_tts.h"
#include "logger.h"
#include "lua_engine.h"
#ifdef RUN_IPC_TEST_SUIT
#include "test_suite/test_suite.h"
#endif

#define EPS 0.000000001

/** @brief Service_Data used as screen reader internal data struct*/
static Service_Data *service_data;

/**
 * @brief Debug function. Print current toolkit version/event
 * type/event source/event detail1/event detail2
 *
 * @param AtspiEvent instance
 *
 */
static void display_info(const AtspiEvent * event)
{
	AtspiAccessible *source = event->source;
	gchar *name = atspi_accessible_get_name(source, NULL);
	gchar *role = atspi_accessible_get_localized_role_name(source, NULL);
	gchar *toolkit = atspi_accessible_get_toolkit_name(source, NULL);

	DEBUG("--------------------------------------------------------");
	DEBUG("Toolkit: %s; Event_type: %s; (%d, %d)", toolkit, event->type, event->detail1, event->detail2);
	DEBUG("Name: %s; Role: %s", name, role);
	DEBUG("--------------------------------------------------------");
}

static char *spi_on_state_changed_get_text(AtspiEvent * event, void *user_data)
{
	Service_Data *sd = (Service_Data *) user_data;
	sd->currently_focused = event->source;

	return lua_engine_describe_object(sd->currently_focused);
}

static char *spi_on_caret_move_get_text(AtspiEvent * event, void *user_data)
{
	Service_Data *sd = (Service_Data *) user_data;
	sd->currently_focused = event->source;
	char *return_text;
	gchar *text = NULL;
	char *added_text = NULL;
	int ret = -1;

	AtspiText *text_interface = atspi_accessible_get_text_iface(sd->currently_focused);
	if (text_interface) {
		DEBUG("->->->->->-> WIDGET CARET MOVED: %s <-<-<-<-<-<-<-", atspi_accessible_get_name(sd->currently_focused, NULL));

		int char_count = (int)atspi_text_get_character_count(text_interface, NULL);
		int caret_pos = atspi_text_get_caret_offset(text_interface, NULL);
		text = atspi_text_get_text(text_interface, caret_pos, caret_pos + 1, NULL);
		if (!caret_pos) {
			DEBUG("MIN POSITION REACHED");
			added_text = _("IDS_REACHED_MIN_POS");
		} else if (char_count == caret_pos) {
			DEBUG("MAX POSITION REACHED");
			added_text = _("IDS_REACHED_MAX_POS");
		}
		if (added_text)	ret = asprintf(&return_text, "%s %s", (char *)text, added_text);
		else ret = asprintf(&return_text, "%s", (char *)text);
		g_free(text);
	}
	if (ret < 0) {
		ERROR(MEMORY_ERROR);
		return NULL;
	}
	return return_text;
}

static char *spi_on_value_changed_get_text(AtspiEvent * event, void *user_data)
{
	Service_Data *sd = (Service_Data *) user_data;
	char *text_to_read = NULL;

	sd->currently_focused = event->source;

	AtspiValue *value_interface = atspi_accessible_get_value_iface(sd->currently_focused);
	if (value_interface) {
		DEBUG("->->->->->-> WIDGET VALUE CHANGED: %s <-<-<-<-<-<-<-", atspi_accessible_get_name(sd->currently_focused, NULL));

		double current_temp_value = (double)atspi_value_get_current_value(value_interface, NULL);
		if (abs(current_temp_value - atspi_value_get_maximum_value(value_interface, NULL)) < EPS) {
			DEBUG("MAX VALUE REACHED");
			if (asprintf(&text_to_read, "%.2f %s", current_temp_value, _("IDS_REACHED_MAX_VAL")) < 0) {
				ERROR(MEMORY_ERROR);
				return NULL;
			}
		} else if (abs(current_temp_value - atspi_value_get_minimum_value(value_interface, NULL)) < EPS) {
			DEBUG("MIN VALUE REACHED");
			if (asprintf(&text_to_read, "%.2f %s", current_temp_value, _("IDS_REACHED_MIN_VAL")) < 0) {
				ERROR(MEMORY_ERROR);
				return NULL;
			}
		} else {
			if (asprintf(&text_to_read, "%.2f", current_temp_value) < 0) {
				ERROR(MEMORY_ERROR);
				return NULL;
			}
		}
	}

	return text_to_read;
}

char *spi_event_get_text_to_read(AtspiEvent * event, void *user_data)
{
	DEBUG("START");
	Service_Data *sd = (Service_Data *) user_data;
	char *text_to_read;

	DEBUG("TRACK SIGNAL:%s", sd->tracking_signal_name);
	DEBUG("WENT EVENT:%s", event->type);

	if (!sd->tracking_signal_name) {
		ERROR("Invalid tracking signal name");
		return NULL;
	}

	if (!strncmp(event->type, sd->tracking_signal_name, strlen(event->type)) && event->detail1 == 1) {
		text_to_read = spi_on_state_changed_get_text(event, user_data);
	} else if (!strncmp(event->type, CARET_MOVED_SIG, strlen(event->type))) {
		text_to_read = spi_on_caret_move_get_text(event, user_data);
	} else if (!strncmp(event->type, VALUE_CHANGED_SIG, strlen(event->type))) {
		text_to_read = spi_on_value_changed_get_text(event, user_data);
	} else {
		ERROR("Unknown event type");
		return NULL;
	}

	return text_to_read;
}

void spi_event_listener_cb(AtspiEvent * event, void *user_data)
{
	DEBUG("START");
	display_info(event);

	if (!user_data) {
		ERROR("Invalid parameter");
		return;
	}

	char *text_to_read = spi_event_get_text_to_read(event, user_data);
	if (!text_to_read) {
		ERROR("Can not prepare text to read");
		return;
	}
	DEBUG("SPEAK: %s", text_to_read);
	tts_speak(text_to_read, EINA_TRUE);

	free(text_to_read);
	DEBUG("END");
}

/**
  * @brief Initializer for screen-reader atspi listeners
  *
  * @param user_data screen-reader internal data
  *
**/
void spi_init(Service_Data * sd)
{
	if (!sd) {
		ERROR("Invalid parameter");
		return;
	}
	DEBUG("--------------------- SPI_init START ---------------------");
	service_data = sd;

	DEBUG(">>> Init lua engine<<<");
	if (lua_engine_init(sd->lua_script_path))
		ERROR("Failed to init lua engine.");

	DEBUG(">>> Creating listeners <<<");

	sd->spi_listener = atspi_event_listener_new(spi_event_listener_cb, service_data, NULL);
	if (sd->spi_listener == NULL) {
		DEBUG("FAILED TO CREATE spi state changed listener");
	}
	// ---------------------------------------------------------------------------------------------------

	DEBUG("TRACKING SIGNAL:%s", sd->tracking_signal_name);

	gboolean ret1 = atspi_event_listener_register(sd->spi_listener, sd->tracking_signal_name, NULL);
	if (ret1 == false) {
		DEBUG("FAILED TO REGISTER spi focus/highlight listener");
	}
	GError *error = NULL;
	gboolean ret2 = atspi_event_listener_register(sd->spi_listener, CARET_MOVED_SIG, &error);
	if (ret2 == false) {
		DEBUG("FAILED TO REGISTER spi caret moved listener: %s", error ? error->message : "no error message");
		if (error)
			g_clear_error(&error);
	}

	gboolean ret3 = atspi_event_listener_register(sd->spi_listener, VALUE_CHANGED_SIG, &error);
	if (ret3 == false) {
		DEBUG("FAILED TO REGISTER spi value changed listener: %s", error ? error->message : "no error message");
		if (error)
			g_clear_error(&error);
	}

	if (ret1 == true && ret2 == true && ret3 == true) {
		DEBUG("spi listener REGISTERED");
	}

	DEBUG("---------------------- SPI_init END ----------------------\n\n");
}

void spi_shutdown(Service_Data * sd)
{
	lua_engine_shutdown();
}
