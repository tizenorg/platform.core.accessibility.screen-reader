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

#define FLUSH_LIMIT 1

#define GERROR_CHECK(error)\
  if (error)\
   {\
     ERROR("Error_log:%s",error->message);\
     g_error_free(error);\
     error = NULL;\
   }

static int last_utt_id;
static Eina_Bool pause_state = EINA_FALSE;
static Eina_Bool flush_flag = EINA_FALSE;
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

void overwrite_last_read_command_with(Read_Command *new_value)
{
	if (last_read_command != NULL) {
		if (last_read_command->text != NULL)
			free(last_read_command->text);
		free(last_read_command);
	}
	last_read_command = new_value;
}

Eina_Bool
can_interrupt(const Read_Command *prev, const Read_Command *next)
{
	DEBUG("[START] checking if can interrupt: prev (%p), next(%p)", prev, next);
	if (prev != NULL)
		DEBUG("CAN_INTERRUPT: prev->interruptable(%d)", prev->interruptable);
	return next != NULL ? (prev != NULL ? prev->interruptable : EINA_TRUE) : EINA_FALSE;
}

void tts_speak_do(Eina_Bool flush_switch)
{
	DEBUG("[START] tts_speak_do");
	int ret = 0;
	Service_Data *sd = get_pointer_to_service_data_struct();
	int speak_id;

	if (!sd || eina_list_count(read_command_queue) == 0)
		return;

	Read_Command *command = NULL;
	do {
		DEBUG("CLEAN UP THE QUEUE HEAD(%p)",read_command_queue);
		command = read_command_queue->data;
		DEBUG("GOT READ COMMAND:%p", command);
		read_command_queue = eina_list_remove(read_command_queue, command);
		DEBUG("REMOVED COMMAND FROM QUEUE. NEW HEAD: %p", read_command_queue);
	} while (eina_list_count(read_command_queue) > 0 && can_interrupt(command,read_command_queue->data));

	if (command->text == NULL) {
		free(command);
		return;
	}
	DEBUG("READ COMMAND text: %s", command->text);
	tts_state_e state;
	tts_get_state(sd->tts, &state);
	DEBUG("TTS_STATE: %d", state);
	DEBUG("TTS_STATE_NAME: %s", get_tts_state(state));
	DEBUG("START COMMAND PROCESSING");
	if (state == TTS_STATE_READY || ((state == TTS_STATE_PLAYING || state == TTS_STATE_PAUSED) && can_interrupt(last_read_command, command))) {
		DEBUG("ACTION REQUIRED. TTS state: %d",state);
		if (flush_flag || flush_switch) {
			if (state == TTS_STATE_PLAYING || state == TTS_STATE_PAUSED) {
				ret = tts_stop(sd->tts);
				if (TTS_ERROR_NONE != ret)
					DEBUG("Fail to stop TTS: resultl(%d)", ret);
			}
		}
		if ((ret = tts_add_text(sd->tts, command->text, NULL, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &speak_id))) {
			switch (ret) {
			case TTS_ERROR_INVALID_PARAMETER:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_INVALID_PARAMETER");
				break;
			case TTS_ERROR_INVALID_STATE:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_INVALID_STATE, tts_state: %d", state);
				break;
			case TTS_ERROR_INVALID_VOICE:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_INVALID_VOICE");
				break;
			case TTS_ERROR_OPERATION_FAILED:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_OPERATION_FAILED");
					break;
			case TTS_ERROR_NOT_SUPPORTED:
				DEBUG("FAILED tts_add_text: error: TTS_ERROR_NOT_SUPPORTED");
				break;
			default:
				DEBUG("FAILED tts_add_text: error: not recognized");
			}
		return;
		}

		DEBUG("tts_speak_do: added id to:%d\n", speak_id);
		last_utt_id = speak_id;
		overwrite_last_read_command_with(command);
		tts_play(sd->tts);
	} else if (command != NULL) {
		DEBUG("RETURNING READ COMMAND: %p",command);
		read_command_queue = eina_list_prepend(read_command_queue, command);
	}
	DEBUG("[END] tts_speak_do");
}

Read_Command *tts_speak_customized(char *text_to_speak, Eina_Bool flush_switch, Eina_Bool interruptable)
{
	if (text_to_speak == NULL)
		return NULL;
	Service_Data *sd = get_pointer_to_service_data_struct();
	tts_state_e state;
	tts_get_state(sd->tts, &state);
	DEBUG("READ COMMAND PARAMS, TEXT: %s, INTERRUPTABLE: %d", text_to_speak, interruptable);
	Read_Command *rc = calloc(1,sizeof(Read_Command));
	rc->text = strdup(text_to_speak);
	rc->interruptable = interruptable;
	DEBUG("BEFORE ADD: %p", read_command_queue);
	read_command_queue = eina_list_append(read_command_queue, rc);
	DEBUG("AFTER ADD: %p", read_command_queue);
	tts_speak_do(flush_switch);
	DEBUG("tts_speak_customized: END");
	return rc;
}

Read_Command *tts_speak(char *text_to_speak, Eina_Bool flush_switch)
{
	return tts_speak_customized(text_to_speak, flush_switch, EINA_TRUE);
}

static void __tts_test_utt_started_cb(tts_h tts, int utt_id, void *user_data)
{
	DEBUG("Utterance started : utt id(%d) \n", utt_id);
	return;
}

static void __tts_test_utt_completed_cb(tts_h tts, int utt_id, void *user_data)
{
	DEBUG("Utterance completed : utt id(%d) \n", utt_id);
	if (last_utt_id - utt_id > FLUSH_LIMIT)
		flush_flag = EINA_TRUE;
	else {
		if (flush_flag)
			flush_flag = EINA_FALSE;
	}

#ifndef SCREEN_READER_TV
	if (last_utt_id == utt_id) {
		DEBUG("LAST UTTERANCE");
		pause_state = EINA_FALSE;
		on_utterance_end();
	}
#endif
	Signal signal;
	if (can_interrupt(last_read_command, read_command_queue->data)) {
		signal = READING_CANCELLED;
	} else {
		signal = READING_STOPPED;
	}
	if (last_read_command->obj) {
		//do AT-SPI action
		GError *err = NULL;
		gchar *action_name = NULL;
		gint number = 0;
		gint i = 0;
		AtspiAction *action_iface = atspi_accessible_get_action_iface(last_read_command->obj);
		if (action_iface) {
			number = atspi_action_get_n_actions(action_iface, &err);
			Eina_Bool found = EINA_FALSE;
			while (i < number && !found) {
				action_name = atspi_action_get_action_name(action_iface, i, &err);
				if (action_name && !strcmp(get_signal_name(signal), action_name)) {
					found = EINA_TRUE;
				} else {
					i++;
				}
				g_free(action_name);
			}
			if (found) {
				DEBUG("PERFORMING ATSPI ACTION NO.%d", i);
				atspi_action_do_action(action_iface, i, &err);
			}
			g_object_unref(action_iface);
			GERROR_CHECK(err);
		}
	} else {
		//broadcast dbus signal
		dbus_direct_reading_adapter_emit(signal, last_read_command);
	}
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
	if (!data) {
		ERROR("Invalid parameter");
		return;
	}

	Service_Data *sd = data;
	sd->update_language_list = false;
	free((char *)sd->text_from_dbus);
	free(sd->current_value);
	sd->text_from_dbus = NULL;
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