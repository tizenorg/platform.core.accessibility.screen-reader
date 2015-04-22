#include "screen_reader.h"
#include "screen_reader_tts.h"
#include "screen_reader_vconf.h"
#include "screen_reader_spi.h"
#include <vconf.h>
#include "logger.h"

#ifdef RUN_IPC_TEST_SUIT
	#include "test_suite/test_suite.h"
#endif

#define BUF_SIZE 1024

Service_Data service_data = {
		//Set by vconf
		.information_level = 1,
		.run_service = 1,
		.language = "en_US",
		.voice_type = TTS_VOICE_TYPE_FEMALE,
		.reading_speed = 2,
		.tracking_signal_name = FOCUS_CHANGED_SIG,


		//Set by tts
		.tts = NULL,
		.available_languages = NULL,

		//Actions to do when tts state is 'ready'
		.update_language_list = false,

		.text_to_say_info = NULL
};

Service_Data *get_pointer_to_service_data_struct()
{
  return &service_data;
}

int screen_reader_create_service(void *data)
{
	DEBUG("Service Create Callback \n");

	Service_Data *service_data = data;

	tts_init(service_data);
	vconf_init(service_data);
	spi_init(service_data);


	/* XML TEST */

	#ifdef RUN_IPC_TEST_SUIT
		run_xml_tests();
		test_suite_init();
	#endif

	return 0;
}

int screen_reader_terminate_service(void *data)
{
	DEBUG("Service Terminate Callback \n");

	Service_Data *service_data = data;

	int vconf_ret = vconf_set_bool("db/setting/accessibility/screen_reader", FALSE);
	if(vconf_ret == 0)
	{
		DEBUG("TTS key set to false");
	}
	else
	{
		DEBUG("COULD NOT SET tts key to 0");
	}

	
	vconf_ret = vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, FALSE);
	if(vconf_ret == 0)
	{
		DEBUG("TTS key set to false");
	}
	else
	{
		DEBUG("COULD NOT SET tts key to 0");
	}

	tts_stop(service_data->tts);
	tts_unprepare(service_data->tts);
	tts_destroy(service_data->tts);
	service_data->text_from_dbus = NULL;
	service_data->current_value = NULL;

	return 0;
}
