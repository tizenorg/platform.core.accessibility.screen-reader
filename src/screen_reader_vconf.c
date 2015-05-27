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

bool set_langauge(Service_Data *sd, const char *new_language, int new_voice)
{
	DEBUG("START");

	Eina_List *l;
	Voice_Info *vi;

	if(strncmp(sd->language, new_language, LAN_NAME - 1) == 0 && sd->voice_type == new_voice)
	{
		DEBUG("No need to change accessibility language: %s(%d) -> %s(%d)",
				sd->language, sd->voice_type, new_language, new_voice);

		return true;
	}

	EINA_LIST_FOREACH(sd->available_languages, l, vi)
	{
		DEBUG("foreach %s <- %s", vi->language, new_language);
		if(strncmp(vi->language, new_language, LAN_NAME - 1) == 0 &&
				vi->voice_type == new_voice)
		{
			DEBUG("str_cpy %s (%d) -> %s (%d)", sd->language, sd->voice_type, vi->language, vi->voice_type);
			snprintf(sd->language, LAN_NAME, "%s", vi->language);
			sd->voice_type = vi->voice_type;
			DEBUG("after_str_cpy");

			DEBUG("ACCESSIBILITY LANGUAGE CHANGED");
			DEBUG("END");
			return true;
		}
	}

	DEBUG("ACCESSIBILITY LANGUAGE FAILED TO CHANGED");

	vconf_set_str("db/setting/accessibility/language", sd->language);
	vconf_set_int("db/setting/accessibility/voice", sd->voice_type);

	DEBUG("END");
	return false;
}

// ------------------------------ vconf callbacks----------------------

void information_level_cb(keynode_t *node, void *user_data)
{
	DEBUG("START");
	DEBUG("Information level set: %d", node->value.i);

	Service_Data *service_data = user_data;
	service_data->information_level = node->value.i;

	DEBUG("END");
}

void app_termination_cb(keynode_t *node, void *user_data)
{
	DEBUG("START");
	DEBUG("Application terminate %d", !node->value.i);

	Service_Data *service_data = user_data;
	service_data->run_service = node->value.i;

	if(service_data->run_service == 0)
	{
		elm_exit();
	}

	DEBUG("END");
}

void language_cb(keynode_t *node, void *user_data)
{
	DEBUG("START");
	DEBUG("Trying to set language to: %s", node->value.s);

	Service_Data *sd = user_data;

	int voice_type;

	vconf_get_int("db/setting/accessibility/voice", (int*)(&voice_type));
	set_langauge(sd, node->value.s, voice_type);

	DEBUG("END");
}

void voice_cb(keynode_t *node, void *user_data)
{
	DEBUG("START");
	DEBUG("Voice set to: %d", node->value.i);

	Service_Data *sd = user_data;

	const char *lang = vconf_get_str("db/setting/accessibility/language");
	set_langauge(sd, lang, (int)node->value.i);

	DEBUG("END");
}

void reading_speed_cb(keynode_t *node, void *user_data)
{
	DEBUG("START");
	DEBUG("Reading speed set to: %d", node->value.i);

	Service_Data *service_data = user_data;
	service_data->reading_speed = node->value.i;

	DEBUG("END");
}

// --------------------------------------------------------------------

int get_key_values(Service_Data *sd)
{
	DEBUG("START");

	char *language = vconf_get_str("db/setting/accessibility/language");

	if(sd->language == NULL)
	{
		DEBUG("FAILED TO SET LANGUAGE");
	}

	int ret = -1;
	int to_ret = 0;


	int voice;
	ret = vconf_get_int("db/setting/accessibility/voice", &voice);
	if(ret != 0)
	{
		to_ret -= -1;
		DEBUG("FAILED TO SET VOICE TYPE: %d", ret);
	}

	set_langauge(sd, language, voice);

	ret = vconf_get_int("db/setting/accessibility/speech_rate", &sd->reading_speed);
	if(ret != 0)
	{
		to_ret -= -2;
		DEBUG("FAILED TO SET READING SPEED: %d", ret);
	}

	ret = vconf_get_int("db/setting/accessibility/information_level", &sd->information_level);
	if(ret != 0)
	{
		to_ret -= -4;
		DEBUG("FAILED TO SET INFORMATION LEVEL: %d", ret);
	}

	DEBUG("SCREEN READER DATA SET TO: Language: %s; Voice: %d, Reading_Speed: %d, Information_Level: %d, Tracking signal: %s;",
			sd->language, sd->voice_type, sd->reading_speed, sd->information_level, sd->tracking_signal_name);

	DEBUG("END");
	return to_ret;
}

bool vconf_init(Service_Data *service_data)
{
	DEBUG( "--------------------- VCONF_init START ---------------------");
	int ret = 0;

	if(vconf_set(keys))
	{
		DEBUG("nothing is written\n");
	}
	else
	{
		DEBUG("everything is written\n");
	}

	vconf_keylist_free(keys);
	// ----------------------------------------------------------------------------------

	ret = get_key_values(service_data);
	if(ret != 0)
	{
		DEBUG("Could not set data from vconf: %d", ret);
	}

	ret = vconf_notify_key_changed("db/setting/accessibility/information_level", information_level_cb, service_data);
	if(ret != 0)
	{
		DEBUG("Could not add information level callback");
		return false;
	}

	ret = vconf_notify_key_changed("db/menu_widget/language", language_cb, service_data);
	if(ret != 0)
	{
		DEBUG("Could not add language callback");
		return false;
	}

	ret = vconf_notify_key_changed("db/setting/accessibility/language", language_cb, service_data);
	if(ret != 0)
	{
		DEBUG("Could not add language callback");
		return false;
	}

	ret = vconf_notify_key_changed("db/setting/accessibility/voice", voice_cb, service_data);
	if(ret != 0)
	{
		DEBUG("Could not add voice callback");
		return false;
	}

	ret = vconf_notify_key_changed("db/setting/accessibility/speech_rate", reading_speed_cb, service_data);
	if(ret != 0)
	{
		DEBUG("Could not add reading speed callback callback");
		return false;
	}

	DEBUG("ALL CALBACKS ADDED");

	DEBUG( "---------------------- VCONF_init END ----------------------\n\n");
	return true;
}
