#ifndef SCREEN_READER_VCONF_H_
#define SCREEN_READER_VCONF_H_

#include "screen_reader.h"
#include "logger.h"

bool vconf_init(Service_Data * service_data);
void vconf_exit();
void screen_reader_vconf_update_tts_language(Service_Data *service_data);

#endif /* SCREEN_READER_VCONF_H_ */
