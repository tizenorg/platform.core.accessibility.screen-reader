#ifndef SCREEN_READER_TTS_H_
#define SCREEN_READER_TTS_H_

#include <Eina.h>
#include <tts.h>
#include "screen_reader.h"

extern tts_h h_tts;
extern char* language;
extern int voice;
extern int speed;

bool tts_init(void *data);
void state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data);
void tts_speak(char *text_to_speak, Eina_Bool flush_switch);
void tts_speak_customized(char *text_to_speak, Eina_Bool flush_switch, Eina_Bool interruptable);
void tts_shutdown(void *data);

void tts_stop_set(void);
Eina_Bool tts_pause_get(void);
Eina_Bool tts_pause_set(Eina_Bool pause_switch);

void set_utterance_cb( void(*utter_cb)(void));

typedef struct {
    char* text;
    Eina_Bool interruptable;
    Eina_Bool protected;
} Read_Command;

#endif /* SCREEN_READER_TTS_H_ */
