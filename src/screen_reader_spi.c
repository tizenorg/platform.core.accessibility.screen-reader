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
#include "navigator.h"
#ifdef RUN_IPC_TEST_SUIT
#include "test_suite/test_suite.h"
#endif

#define EPS 0.000000001

/** @brief Service_Data used as screen reader internal data struct*/
static Service_Data *service_data;
static Eina_Bool ignore_next_caret_move = EINA_FALSE;
static int last_caret_position = -1;

typedef struct {
	char *key;
	char *val;
} Attr;

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

Eina_Bool double_click_timer_cb(void *data)
{
	Service_Data *sd = data;
	sd->clicked_widget = NULL;

	return EINA_FALSE;
}

bool allow_recursive_name(AtspiAccessible * obj)
{
	AtspiRole r = atspi_accessible_get_role(obj, NULL);
	if (r == ATSPI_ROLE_FILLER)
		return true;
	return false;
}

char *generate_description_for_subtree(AtspiAccessible * obj)
{
	DEBUG("START");
	if (!allow_recursive_name(obj))
		return strdup("");

	if (!obj)
		return strdup("");
	int child_count = atspi_accessible_get_child_count(obj, NULL);

	DEBUG("There is %d children inside this filler", child_count);
	if (!child_count)
		return strdup("");

	int i;
	char *name = NULL;
	char *below = NULL;
	char ret[256] = "\0";
	AtspiAccessible *child = NULL;
	for (i = 0; i < child_count; i++) {
		child = atspi_accessible_get_child_at_index(obj, i, NULL);
		name = atspi_accessible_get_name(child, NULL);
		DEBUG("%d child name:%s", i, name);
		if (name && strncmp(name, "\0", 1)) {
			strncat(ret, name, sizeof(ret) - strlen(ret) - 1);
		}
		strncat(ret, " ", sizeof(ret) - strlen(ret) - 1);
		below = generate_description_for_subtree(child);
		if (strncmp(below, "\0", 1)) {
			strncat(ret, below, sizeof(ret) - strlen(ret) - 1);
		}
		g_object_unref(child);
		free(below);
		free(name);
	}
	return strdup(ret);
}

static char *spi_on_state_changed_get_text(AtspiEvent * event, void *user_data)
{
	Service_Data *sd = (Service_Data *) user_data;
	char *name;
	char *names = NULL;
	char *description;
	char *role_name;
	char *other;
	char ret[256] = "\0";
	sd->currently_focused = event->source;

	description = atspi_accessible_get_description(sd->currently_focused, NULL);
	name = atspi_accessible_get_name(sd->currently_focused, NULL);
	role_name = atspi_accessible_get_localized_role_name(sd->currently_focused, NULL);
	other = generate_description_for_subtree(sd->currently_focused);

	DEBUG("->->->->->-> WIDGET GAINED HIGHLIGHT: %s <-<-<-<-<-<-<-", name);
	DEBUG("->->->->->-> FROM SUBTREE HAS NAME:  %s <-<-<-<-<-<-<-", other);

	if (name && strncmp(name, "\0", 1))
		names = strdup(name);
	else if (other && strncmp(other, "\0", 1))
		names = strdup(other);

	if (names) {
		strncat(ret, names, sizeof(ret) - strlen(ret) - 1);
		strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
	}

	if (role_name)
		strncat(ret, role_name, sizeof(ret) - strlen(ret) - 1);

	if (description) {
		if (strncmp(description, "\0", 1))
			strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
		strncat(ret, description, sizeof(ret) - strlen(ret) - 1);
	}

	free(name);
	free(names);
	free(description);
	free(role_name);
	free(other);

	return strdup(ret);
}

static char *spi_on_caret_move_get_text(AtspiEvent * event, void *user_data)
{
	Service_Data *sd = (Service_Data *) user_data;
	sd->currently_focused = event->source;
	char *return_text;
	gchar *text = NULL;
	char *added_text = NULL;
	int ret = -1;

	if (last_caret_position == event->detail1) {	// ignore fake moves, selection moves etc.
		return NULL;
	}
	last_caret_position = event->detail1;

	if (ignore_next_caret_move) {
		ignore_next_caret_move = EINA_FALSE;
		return NULL;
	}

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

static int spi_get_text_interface_text_length(AtspiEvent * event, void *user_data)
{
	Service_Data *sd;
	AtspiText *text_interface;

	sd = (Service_Data *) user_data;
	sd->currently_focused = event->source;
	text_interface = atspi_accessible_get_text_iface(sd->currently_focused);
	if (text_interface) {
		return (int)atspi_text_get_character_count(text_interface, NULL);
	}
	return -1;
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

static char *spi_on_text_delete(AtspiEvent * event, void *user_data)
{
	char ret[TTS_MAX_TEXT_SIZE] = "\0";

	if (event->detail2 == 1) {
		strncpy(ret, g_value_get_string(&event->any_data), sizeof(ret) - 1);
		strncat(ret, _("IDS_REMOVED"), sizeof(ret) - strlen(ret) - 1);
	} else {
		strncpy(ret, _("IDS_TEXT_REMOVED"), sizeof(ret) - 1);
	}

	if (event->detail1 != last_caret_position) {
		if (event->detail1 == 0) {
			strncat(ret, _("IDS_REACHED_MIN_POS"), sizeof(ret) - strlen(ret) - 1);
		}
		ignore_next_caret_move = EINA_TRUE;
	} else if (event->detail1 == spi_get_text_interface_text_length(event, user_data)) {
		strncat(ret, _("IDS_REACHED_MAX_POS"), sizeof(ret) - strlen(ret) - 1);
	}

	return strdup(ret);
}

char *spi_event_get_text_to_read(AtspiEvent * event, void *user_data)
{
	DEBUG("START");
	Service_Data *sd = (Service_Data *) user_data;
	char *text_to_read = NULL;

	DEBUG("TRACK SIGNAL:%s", sd->tracking_signal_name);
	DEBUG("WENT EVENT:%s", event->type);

	if (!sd->tracking_signal_name) {
		ERROR("Invalid tracking signal name");
		return NULL;
	}

	if (!strcmp(event->type, sd->tracking_signal_name) && event->detail1 == 1) {
		text_to_read = spi_on_state_changed_get_text(event, user_data);

	} else if (!strcmp(event->type, CARET_MOVED_SIG)) {
		text_to_read = spi_on_caret_move_get_text(event, user_data);

	} else if (!strcmp(event->type, TEXT_INSERT_SIG)) {
		ignore_next_caret_move = EINA_TRUE;
		if (event->detail2 == 1) {
			text_to_read = strdup(g_value_get_string(&event->any_data));
		} else {
			text_to_read = strdup(_("IDS_TEXT_INSERTED"));
		}

	} else if (!strcmp(event->type, TEXT_DELETE_SIG)) {
		text_to_read = spi_on_text_delete(event, user_data);

	} else if (!strcmp(event->type, VALUE_CHANGED_SIG)) {
		text_to_read = spi_on_value_changed_get_text(event, user_data);

	} else if (!strcmp(event->type, STATE_FOCUSED_SIG)) {
		ignore_next_caret_move = EINA_TRUE;
		last_caret_position = -1;

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
		DEBUG("No text to read");
		return;
	}
	DEBUG("SPEAK: %s", text_to_read);
	tts_speak(text_to_read, EINA_TRUE);

	free(text_to_read);
	DEBUG("END");
}

/**
  * @brief Register new listener and log error if any
**/
static void spi_listener_register(AtspiEventListener *spi_listener, const char *signal_name)
{
	GError *error = NULL;

	if (atspi_event_listener_register(spi_listener, signal_name, &error) == false) {
		ERROR("FAILED TO REGISTER spi %s listener. Error: %s", signal_name, error ? error->message : "no error message");
		if (error)
			g_clear_error(&error);
	} else {
		DEBUG("spi %s listener REGISTERED", signal_name);
	}
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

	DEBUG(">>> Creating listeners <<<");

	sd->spi_listener = atspi_event_listener_new(spi_event_listener_cb, service_data, NULL);
	if (sd->spi_listener == NULL) {
		DEBUG("FAILED TO CREATE spi state changed listener");
	}
	// ---------------------------------------------------------------------------------------------------

	DEBUG("TRACKING SIGNAL:%s", sd->tracking_signal_name);

	spi_listener_register(sd->spi_listener, CARET_MOVED_SIG);
	spi_listener_register(sd->spi_listener, VALUE_CHANGED_SIG);
	spi_listener_register(sd->spi_listener, STATE_FOCUSED_SIG);
	spi_listener_register(sd->spi_listener, TEXT_INSERT_SIG);
	spi_listener_register(sd->spi_listener, TEXT_DELETE_SIG);

	DEBUG("---------------------- SPI_init END ----------------------\n\n");
}
