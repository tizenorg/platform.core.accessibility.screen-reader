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

#include <Ecore.h>
#include <atspi/atspi.h>
#include "screen_reader_tts.h"
#include "screen_reader_vconf.h"
#include "dbus_direct_reading_adapter.h"
#include "logger.h"

// ---------------------------- DEBUG HELPERS ------------------------------

#define REMOVE_PRECEDING_READ_COMMANDS_LIMIT 1

#define GERROR_CHECK(error)\
  GError *_error = error;\
  if (_error)\
   {\
     ERROR("Error_log:%s", _error->message);\
     g_error_free(_error);\
     _error = NULL;\
   }

static int last_utt_id;
static Eina_Bool pause_state = EINA_FALSE;
static Eina_List *read_command_queue = NULL;
static Read_Command *last_read_command = NULL;

static void (*on_utterance_end) (void);

static char *get_tts_error(int r)
{
	switch (r) {
	case TTS_ERROR_NONE:
		{
			return "no error";
		}
	case TTS_ERROR_INVALID_PARAMETER:
		{
			return "inv param";
		}
	case TTS_ERROR_OUT_OF_MEMORY:
		{
			return "out of memory";
		}
	case TTS_ERROR_OPERATION_FAILED:
		{
			return "oper failed";
		}
	case TTS_ERROR_INVALID_STATE:
		{
			return "inv state";
		}
	default:
		{
			return "uknown error";
		}
	}
}

static char *get_tts_state(tts_state_e r)
{
	switch (r) {
	case TTS_STATE_CREATED:
		{
			return "created";
		}
	case TTS_STATE_READY:
		{
			return "ready";
		}
	case TTS_STATE_PLAYING:
		{
			return "playing";
		}
	case TTS_STATE_PAUSED:
		{
			return "pause";
		}
	default:
		{
			return "uknown state";
		}
	}
}

//-------------------------------------------------------------------------------------------------

void set_utterance_cb(void (*uter_cb) (void))
{
	on_utterance_end = uter_cb;
}

bool get_supported_voices_cb(tts_h tts, const char *language, int voice_type, void *user_data)
{
	DEBUG("LANG: %s; TYPE: %d", language, voice_type);

	Service_Data *sd = user_data;
	Voice_Info *vi = calloc(1, sizeof(Voice_Info));
	if (!vi) {
		ERROR(MEMORY_ERROR);
		return ECORE_CALLBACK_CANCEL;
	}

	if (asprintf(&vi->language, "%s", language) < 0) {
		free(vi);
		ERROR(MEMORY_ERROR);
		return ECORE_CALLBACK_CANCEL;
	}

	vi->voice_type = voice_type;

	sd->available_languages = eina_list_append(sd->available_languages, vi);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _do_atspi_action_by_name(AtspiAccessible *obj, const char *name) {
	Eina_Bool action_done_successfully = EINA_FALSE;
	if (obj) {
		GError *err = NULL;
		gchar *action_name = NULL;
		gint number = 0;
		gint i = 0;
		AtspiAction *action_iface = atspi_accessible_get_action_iface(obj);
		DEBUG("ACTION_IFACE:%p", action_iface);
		if (action_iface) {
			number = atspi_action_get_n_actions(action_iface, &err);
			DEBUG("ACTION#:%d", number);
			Eina_Bool found = EINA_FALSE;
			while (i < number && !found) {
				action_name = atspi_action_get_action_name(action_iface, i, &err);
				DEBUG("LOOKING FOR ACTION:%s FOUND:%s", name, action_name);
				if (action_name && !strcmp(name, action_name)) {
					found = EINA_TRUE;
				} else {
					i++;
				}
				g_free(action_name);
			}
			if (found) {
				DEBUG("PERFORMING ATSPI ACTION NO.%d", i);
				action_done_successfully = atspi_action_do_action(action_iface, i, &err);
			} else {
				DEBUG("NOT FOUND ATSPI ACTION:%s IN OBJECT:%p", name, obj);
			}
			g_object_unref(action_iface);
			GERROR_CHECK(err);
		}
	}
	return action_done_successfully;
}

static void  _reading_status_notify(Signal signal)
{
	DEBUG("[START] reading status notify for LAST_READ_COMMAND: %p", last_read_command);
	if (last_read_command && last_read_command->obj) {
		//do AT-SPI action
		if (!_do_atspi_action_by_name(last_read_command->obj, get_signal_name(signal)))
			ERROR("Failed to do AT-SPI action by name:%s for read command:%p with accessible object:%p",
				get_signal_name(signal), last_read_command, last_read_command->obj);
	} else {
		//broadcast dbus signal
		if (dbus_direct_reading_adapter_emit(signal, last_read_command) != 0)
			ERROR("Failed to broadcast dbus signal:%d for read command:%p", signal, last_read_command);
	}
}

void overwrite_last_read_command_with(Read_Command *new_value)
{
	if (last_read_command != NULL) {
		if (last_read_command->text != NULL) {
			if (last_read_command->is_playing)
				_reading_status_notify(READING_CANCELLED);
			else
				_reading_status_notify(READING_STOPPED);
			free(last_read_command->text);
		}
		free(last_read_command);
	}
	last_read_command = new_value;
}

Eina_Bool
can_discard(const Read_Command *prev, const Read_Command *next)
{
	DEBUG("[START] checking if can discard: prev (%p), next(%p)", prev, next);
	if (prev != NULL)
		DEBUG("CAN_DISCARD: prev->discardable(%d)", prev->discardable);
	return next != NULL ? (prev != NULL ? (prev->discardable && next->want_discard_previous_reading)  : EINA_TRUE) : EINA_FALSE;
}

Eina_Bool can_be_discarded(const Read_Command *prev)
{
   Eina_List *l;
   Read_Command *command;

   EINA_LIST_FOREACH(read_command_queue, l, command) {
      if (can_discard(prev, command))
          return EINA_TRUE;
   }
   return can_discard(prev, NULL);
}

//TODO: consider handling the case of discarding well-formed read command by subsequent malformed command
Read_Command *get_read_command_from_queue() {
	Read_Command *command = NULL;
		DEBUG("CLEAN UP THE QUEUE HEAD(%p) # of elements in queue:%d", read_command_queue, eina_list_count(read_command_queue));
	do {
		if (command) {
			DEBUG("skipping read command with text: %s", command->text);
			if (command->text)
				free(command->text);
			// signal ReadingSkipped
			if (command->obj) {
				//do AT-SPI action
				if (!_do_atspi_action_by_name(command->obj, get_signal_name(READING_SKIPPED)))
					ERROR("Failed to do AT-SPI action by name:%s for read command:%p with accessible object:%p",
						get_signal_name(READING_SKIPPED), command, command->obj);
			} else {
				//broadcast dbus signal
				if (dbus_direct_reading_adapter_emit(READING_SKIPPED, command) != 0)
					ERROR("Failed to broadcast dbus signal:%d for read command:%p", READING_SKIPPED, command);
			}
			free(command);
		}
		command = read_command_queue->data;
		DEBUG("GOT READ COMMAND:%p", command);
		read_command_queue = eina_list_remove(read_command_queue, command);
		DEBUG("REMOVED COMMAND FROM QUEUE. NEW HEAD: %p", read_command_queue);
	} while ((eina_list_count(read_command_queue) > 0 && can_be_discarded(command))
			|| command->text == NULL);
	return command;
}

enum _Play_Read_Command_Status {
    SUCCESS = 0,
    FAILURE_RECOVERABLE = 1,
    FAILURE_NOT_RECOVERABLE = 2
};

typedef enum _Play_Read_Command_Status Play_Read_Command_Status;

Play_Read_Command_Status play_read_command_with_tts(Service_Data *sd, Read_Command *command, tts_state_e state) {
	if (state == TTS_STATE_READY) {
		DEBUG("Passing TEXT: %s to TTS", command->text);
		int speak_id;
		int ret = 0;
		if ((ret = tts_add_text(sd->tts, command->text, NULL, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &speak_id))) {
			switch (ret) {
			case TTS_ERROR_INVALID_PARAMETER:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_INVALID_PARAMETER");
				return FAILURE_NOT_RECOVERABLE;
			case TTS_ERROR_INVALID_STATE:
				dlog_print(DLOG_FATAL, NULL, "FAILED tts_add_text: error: TTS_ERROR_INVALID_STATE, tts_state: %d", state);
				return FAILURE_NOT_RECOVERABLE;
			case TTS_ERROR_INVALID_VOICE:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_INVALID_VOICE");
				return FAILURE_NOT_RECOVERABLE;
			case TTS_ERROR_OPERATION_FAILED:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_OPERATION_FAILED");
				return FAILURE_NOT_RECOVERABLE;
			case TTS_ERROR_NOT_SUPPORTED:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_NOT_SUPPORTED");
				return FAILURE_NOT_RECOVERABLE;
			default:
				DEBUG("FAILED tts_add_text: error: not recognized");
				return FAILURE_NOT_RECOVERABLE;
			}
		}
		DEBUG("tts_speak_do: added id to:%d\n", speak_id);
		last_utt_id = speak_id;
		overwrite_last_read_command_with(command);
		command->is_playing = EINA_TRUE;
		tts_play(sd->tts);
		return SUCCESS;
	}
	return FAILURE_RECOVERABLE;
}

void tts_speak_do()
{
	DEBUG("[START] tts_speak_do");
	int ret = 0;
	Service_Data *sd = get_pointer_to_service_data_struct();
	if (!sd || eina_list_count(read_command_queue) == 0)
		return;
	Read_Command *command = NULL;
	Play_Read_Command_Status status = SUCCESS;
	do {
		command = get_read_command_from_queue();
		DEBUG("READ COMMAND text: %s", command->text);
		tts_state_e state;
		tts_get_state(sd->tts, &state);
		DEBUG("TTS_STATE: %d", state);
		DEBUG("TTS_STATE_NAME: %s", get_tts_state(state));
		DEBUG("START COMMAND PROCESSING");
		status = play_read_command_with_tts(sd, command, state);
		if (status == FAILURE_RECOVERABLE) {
			DEBUG("RETURNING READ COMMAND TO QUEUE: %p", command);
			read_command_queue = eina_list_prepend(read_command_queue, command);
			if ((state == TTS_STATE_PLAYING || state == TTS_STATE_PAUSED) && can_discard(last_read_command, command)) {
				DEBUG("Stopping TTS");
				ret = tts_stop(sd->tts);
				if (TTS_ERROR_NONE != ret)
					DEBUG("Fail to stop TTS: resultl(%d)", ret);
			}
		}
	} while (eina_list_count(read_command_queue) > 0 && status == FAILURE_NOT_RECOVERABLE);
	DEBUG("[END] tts_speak_do");
}

Read_Command *
tts_speak_customized(char *text_to_speak, Eina_Bool want_discard_previous_reading,
			Eina_Bool discardable, AtspiAccessible *obj)
{
	if (text_to_speak == NULL)
		return NULL;
	Service_Data *sd = get_pointer_to_service_data_struct();
	if (!sd)
		return NULL;
	tts_state_e state;
	tts_get_state(sd->tts, &state);
	DEBUG("READ COMMAND PARAMS, TEXT: %s, DISCARDABLE: %d, ATSPI_OBJECT: %p", text_to_speak, discardable, obj);
	Read_Command *rc = calloc(1, sizeof(Read_Command));
	rc->text = strdup(text_to_speak);
	rc->discardable = discardable;
	rc->want_discard_previous_reading = want_discard_previous_reading;
	rc->is_playing = EINA_FALSE;
	rc->obj = obj;
	DEBUG("BEFORE ADD: %p", read_command_queue);
	read_command_queue = eina_list_append(read_command_queue, rc);
	DEBUG("AFTER ADD: %p", read_command_queue);
	tts_speak_do();
	DEBUG("tts_speak_customized: END");
	return rc;
}

Read_Command *tts_speak(char *text_to_speak, Eina_Bool want_discard_previous_reading)
{
	return tts_speak_customized(text_to_speak, want_discard_previous_reading, EINA_TRUE, NULL);
}

static void __tts_test_utt_started_cb(tts_h tts, int utt_id, void *user_data)
{
	DEBUG("Utterance started : utt id(%d) \n", utt_id);
	return;
}

static void __tts_test_utt_completed_cb(tts_h tts, int utt_id, void *user_data)
{
	DEBUG("Utterance completed : utt id(%d) \n", utt_id);
#ifndef SCREEN_READER_TV
	if (last_utt_id == utt_id) {
		DEBUG("LAST UTTERANCE");
		pause_state = EINA_FALSE;
		on_utterance_end();
	}
#endif
	if (last_read_command) last_read_command->is_playing = EINA_FALSE;
	overwrite_last_read_command_with(NULL);
	tts_speak_do(EINA_FALSE);
	return;
}

bool tts_init(void *data)
{
	DEBUG("--------------------- TTS_init START ---------------------");
	Service_Data *sd = data;

	int r = tts_create(&sd->tts);
	DEBUG("Create tts %d (%s)", r, get_tts_error(r));

	r = tts_set_mode(sd->tts, TTS_MODE_SCREEN_READER);
	DEBUG("Set tts mode SR %d (%s)", r, get_tts_error(r));

	r = tts_prepare(sd->tts);
	DEBUG("Prepare tts %d (%s)", r, get_tts_error(r));

	tts_set_state_changed_cb(sd->tts, state_changed_cb, sd);

	tts_set_utterance_started_cb(sd->tts, __tts_test_utt_started_cb, sd);
	tts_set_utterance_completed_cb(sd->tts, __tts_test_utt_completed_cb, sd);

	DEBUG("---------------------- TTS_init END ----------------------\n\n");
	return true;
}

void tts_shutdown(void *data)
{
	Service_Data *sd = data;
	if (!sd) {
		ERROR("Invalid parameter");
		return;
	}
	sd->update_language_list = false;
	free(sd->current_value);
	sd->current_value = NULL;
	tts_stop(sd->tts);
}

Eina_Bool tts_pause_get(void)
{
	DEBUG("PAUSE STATE: %d", pause_state);
	return pause_state;
}

void tts_stop_set(void)
{
	Service_Data *sd = get_pointer_to_service_data_struct();
	tts_stop(sd->tts);
	overwrite_last_read_command_with(NULL);
}

Eina_Bool tts_pause_set(Eina_Bool pause_switch)
{
	Service_Data *sd = get_pointer_to_service_data_struct();
	if (!sd)
		return EINA_FALSE;

	if (pause_switch) {
		pause_state = EINA_TRUE;

		if (tts_pause(sd->tts)) {
			pause_state = EINA_FALSE;
			return EINA_FALSE;
		}
	} else if (!pause_switch) {
		pause_state = EINA_FALSE;

		if (tts_play(sd->tts)) {
			pause_state = EINA_TRUE;
			return EINA_FALSE;
		}
	}
	return EINA_TRUE;
}

Eina_Bool update_supported_voices(void *data)
{
	DEBUG("START");
	tts_state_e state;

	Service_Data *sd = data;

	int res = tts_get_state(sd->tts, &state);

	if (res != TTS_ERROR_NONE) {
		DEBUG("CANNOT RETRIVE STATE");
		return EINA_FALSE;
	}

	if (state == TTS_STATE_READY) {
		tts_foreach_supported_voices(sd->tts, get_supported_voices_cb, sd);
	} else {
		sd->update_language_list = EINA_TRUE;
	}

	DEBUG("END");
	return EINA_TRUE;
}

void state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void *user_data)
{
	if (pause_state) {
		DEBUG("TTS is currently paused. Resume to start reading");
		return;
	}

	DEBUG("++++++++++++++++state_changed_cb\n++++++++++++++++++");
	DEBUG("current state:%s and previous state:%s\n", get_tts_state(current), get_tts_state(previous));
	Service_Data *sd = user_data;

	if (TTS_STATE_READY == current) {
		update_supported_voices(sd);
		overwrite_last_read_command_with(NULL);
		tts_speak_do(EINA_FALSE);
	}
}
