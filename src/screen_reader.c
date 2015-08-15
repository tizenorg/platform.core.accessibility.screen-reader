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

Service_Data service_data =
{
   //Set by vconf
   .run_service = 1,
#ifdef SCREEN_READER_MOBILE
   .tracking_signal_name = HIGHLIGHT_CHANGED_SIG,
#else
   .tracking_signal_name = FOCUS_CHANGED_SIG,
#endif

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
   Service_Data *service_data = data;

   vconf_init(service_data);
   tts_init(service_data);

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

   tts_stop(service_data->tts);
   tts_unprepare(service_data->tts);
   tts_destroy(service_data->tts);
   service_data->text_from_dbus = NULL;
   service_data->current_value = NULL;

   return 0;
}
