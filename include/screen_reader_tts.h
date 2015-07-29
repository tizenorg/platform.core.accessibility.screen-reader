#ifndef SCREEN_READER_TTS_H_
#define SCREEN_READER_TTS_H_

#include <tts.h>
#include "screen_reader.h"

extern tts_h h_tts;
extern char* language;
extern int voice;
extern int speed;

bool tts_init(void *data);
void state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data);
void tts_speak(char *text_to_speak, Eina_Bool flush_switch);
void spi_stop(void *data);

void tts_stop_set(void);
Eina_Bool tts_pause_get(void);
Eina_Bool tts_pause_set(Eina_Bool pause_switch);

void set_utterance_cb( void(*utter_cb)(void));

#endif /* SCREEN_READER_TTS_H_ */
