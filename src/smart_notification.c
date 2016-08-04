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

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <atspi/atspi.h>
#include "logger.h"
#include "screen_reader_tts.h"
#include "screen_reader_haptic.h"
#include "smart_notification.h"
#include <wav_player.h>
#include <sound_manager.h>

#define RED  "\x1B[31m"
#define RESET "\033[0m"

static Eina_Bool status = EINA_FALSE;

static void _smart_notification_focus_chain_end(void);
static void _smart_notification_realized_items(int start_idx, int end_idx);
static void _smart_notification_highlight(void);
static void _smart_notification_highlight_actionable(void);
static void _smart_notification_action(void);
static void _smart_notification_contextual_menu(void);
static void _smart_notification_long_press(void);
static void _smart_notification_state_change(void);

static void
_playback_completed_cb(int id, void *user_data)
{
	const char* path = (const char*)user_data;
	DEBUG("WAV Player", "Completed! [id:%d, path:%s]", id, path);
}

static char *_get_sound_path(const char *file_path)
{
	char *dir;
	dir = strdup(_TZ_SYS_RO_APP"/org.tizen.screen-reader/res/sounds");
	char *path = g_strconcat(dir, "/", file_path, NULL);
	free(dir);
	DEBUG("path: (%s)", path);
	return path;
}
/**
 * @brief Smart Notifications event handler
 *
 * @param nt Notification event type
 * @param start_index int first visible items index smart_notification_realized_items
 * @param end_index int last visible items index used for smart_notification_realized_items
 */
void smart_notification(Notification_Type nt, int start_index, int end_index)
{
	DEBUG("START");
	if (!status)
		return;

	switch (nt) {
	case FOCUS_CHAIN_END_NOTIFICATION_EVENT:
		_smart_notification_focus_chain_end();
		break;
	case REALIZED_ITEMS_NOTIFICATION_EVENT:
		_smart_notification_realized_items(start_index, end_index);
		break;
	case HIGHLIGHT_NOTIFICATION_EVENT:
		_smart_notification_highlight();
		break;
	case HIGHLIGHT_NOTIFICATION_EVENT_ACTIONABLE:
		_smart_notification_highlight_actionable();
		break;
	case ACTION_NOTIFICATION_EVENT:
		_smart_notification_action();
		break;
	case CONTEXTUAL_MENU_NOTIFICATION_EVENT:
		_smart_notification_contextual_menu();
		break;
	case LONG_PRESS_NOTIFICATION_EVENT:
		_smart_notification_long_press();
		break;
	case WINDOW_STATE_CHANGE_NOTIFICATION_EVENT:
		_smart_notification_state_change();
		break;
	default:
		DEBUG("Gesture type %d not handled in switch", nt);
	}
}

/**
 * @brief Used for getting first and last index of visible items
 *
 * @param scrollable_object AtspiAccessible object on which scroll was triggered
 * @param start_index int first visible items index smart_notification_realized_items
 * @param end_index int last visible items index used for smart_notification_realized_items
 */
void get_realized_items_count(AtspiAccessible * scrollable_object, int *start_idx, int *end_idx)
{
	DEBUG("START");
	int count_child, jdx;
	AtspiAccessible *child_iter;
	AtspiStateType state = ATSPI_STATE_SHOWING;

	if (!scrollable_object) {
		DEBUG("No scrollable object");
		return;
	}

	count_child = atspi_accessible_get_child_count(scrollable_object, NULL);

	for (jdx = 0; jdx < count_child; jdx++) {
		child_iter = atspi_accessible_get_child_at_index(scrollable_object, jdx, NULL);
		if (!child_iter)
			continue;

		AtspiStateSet *state_set = atspi_accessible_get_state_set(child_iter);

		gboolean is_visible = atspi_state_set_contains(state_set, state);
		if (is_visible) {
			*start_idx = jdx;
			DEBUG("Item with index %d is visible", jdx);
		} else
			DEBUG("Item with index %d is NOT visible", jdx);
	}
	*end_idx = *start_idx + 8;
}

/**
 * @brief Scroll-start/Scroll-end event callback
 *
 * @param event AtspiEvent
 * @param user_data UNUSED
 */

static void _scroll_event_cb(AtspiEvent * event, gpointer user_data)
{
	if (!status)
		return;

	int start_index, end_index;
	start_index = 0;
	end_index = 0;

	gchar *role_name = atspi_accessible_get_role_name(event->source, NULL);
	fprintf(stderr, "Event: %s: %d, obj: %p: role: %s\n", event->type, event->detail1, event->source, role_name);
	g_free(role_name);

	if (!strcmp(event->type, "object:scroll-start")) {
		DEBUG("Scrolling started");
		tts_speak(_("IDS_SCROLLING_STARTED"), EINA_TRUE);
	} else if (!strcmp(event->type, "object:scroll-end")) {
		DEBUG("Scrolling finished");
		tts_speak(_("IDS_SCROLLING_FINISHED"), EINA_FALSE);
		get_realized_items_count((AtspiAccessible *) event->source, &start_index, &end_index);
		_smart_notification_realized_items(start_index, end_index);
	}
}

/**
 * @brief Initializer for smart notifications
 *
 *
 */
void smart_notification_init(void)
{
	DEBUG("Smart Notification init!");

	AtspiEventListener *listener;

	listener = atspi_event_listener_new(_scroll_event_cb, NULL, NULL);
	atspi_event_listener_register(listener, "object:scroll-start", NULL);
	atspi_event_listener_register(listener, "object:scroll-end", NULL);

#ifndef SCREEN_READER_TV
	haptic_module_init();
#endif

	status = EINA_TRUE;
}

/**
 * @brief Smart notifications shutdown
 *
 */
void smart_notification_shutdown(void)
{
#ifndef SCREEN_READER_TV
	haptic_module_disconnect();
#endif
	status = EINA_FALSE;
}

/**
 * @brief Smart notifications focus chain event handler
 *
 */
static void _smart_notification_focus_chain_end(void)
{
	if (!status)
		return;

	DEBUG(RED "Smart notification - FOCUS CHAIN END" RESET);
	int wav_player_id;
	//sound to play when reached start/end not given hence using below.
	gchar* wav_path = _get_sound_path("focus_general.ogg");

	DEBUG("Smart notification - HIGHLIGHT" RESET);
	wav_player_start(wav_path, SOUND_TYPE_MEDIA, _playback_completed_cb, (void*)wav_path, &wav_player_id);
	g_free(wav_path);
}

/**
 * @brief Smart notifications hightlight event handler
 *
 */
static void _smart_notification_highlight(void)
{
	if (!status)
		return;
	int wav_player_id;
	gchar* wav_path = _get_sound_path("focus_general.ogg");

	DEBUG("Smart notification - HIGHLIGHT" RESET);
	wav_player_start(wav_path, SOUND_TYPE_MEDIA, _playback_completed_cb, (void*)wav_path, &wav_player_id);
	g_free(wav_path);
}

static void _smart_notification_highlight_actionable(void)
{
	if (!status)
		return;
	int wav_player_id;

	gchar* wav_path = _get_sound_path("focus_actionable.ogg");

	DEBUG("Smart notification - HIGHLIGHT" RESET);
	wav_player_start(wav_path, SOUND_TYPE_MEDIA, _playback_completed_cb, (void*)wav_path, &wav_player_id);
	g_free(wav_path);
}

static void _smart_notification_action(void)
{
	if (!status)
		return;
	int wav_player_id;
	gchar* wav_path = _get_sound_path("clicked.ogg");

	DEBUG("Smart notification - ACTION" RESET);
	wav_player_start(wav_path, SOUND_TYPE_MEDIA, _playback_completed_cb, (void*)wav_path, &wav_player_id);
	g_free(wav_path);
}

static void _smart_notification_contextual_menu(void)
{
	if (!status)
		return;
	int wav_player_id;
	gchar* wav_path = _get_sound_path("contextual_menu.ogg");

	DEBUG("Smart notification - context menu" RESET);
	wav_player_start(wav_path, SOUND_TYPE_MEDIA, _playback_completed_cb, (void*)wav_path, &wav_player_id);
	g_free(wav_path);
}

static void _smart_notification_long_press(void)
{
	if (!status)
		return;
	int wav_player_id;
	gchar* wav_path = _get_sound_path("long_clicked.ogg");

	DEBUG("Smart notification - long press" RESET);
	wav_player_start(wav_path, SOUND_TYPE_MEDIA, _playback_completed_cb, (void*)wav_path, &wav_player_id);
	g_free(wav_path);
}

static void _smart_notification_state_change(void)
{
	if (!status)
		return;
	int wav_player_id;
	gchar* wav_path = _get_sound_path("window_state_changed.ogg");

	DEBUG("Smart notification - view change" RESET);
	wav_player_start(wav_path, SOUND_TYPE_MEDIA, _playback_completed_cb, (void*)wav_path, &wav_player_id);
	g_free(wav_path);
}

/**
 * @brief Smart notifications realized items event handler
 *
 */
static void _smart_notification_realized_items(int start_idx, int end_idx)
{
	if (!status)
		return;

	if (start_idx == end_idx)
		return;

	DEBUG(RED "Smart notification - Visible items notification" RESET);

	char buf[256];

	snprintf(buf, sizeof(buf), _("IDS_REACHED_ITEMS_NOTIFICATION"), start_idx, end_idx);

	tts_speak(buf, EINA_FALSE);
}
