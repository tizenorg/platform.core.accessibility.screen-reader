#ifndef SCREEN_READER_TTS_H_
#define SCREEN_READER_TTS_H_

#include <tts.h>
#include "screen_reader.h"

extern tts_h h_tts;
extern char* language;
extern int voice;
extern int speed;

typedef struct {
    char* text;
    Eina_Bool discardable;
    Eina_Bool want_discard_previous_reading;
    Eina_Bool is_playing;
    AtspiAccessible *obj;
} Read_Command;

bool tts_init(void *data);
void tts_shutdown(void *data);
void state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data);
/**
 * @brief Requests discardable, asynchronous (queued) reading of given text.
 *
 * @param text_to_speak a text that would be read by the Text To Speach (TTS) service using default TTS language
 * @param want_discard_previous_reading determines if this reading wants the preceding reading to be discarded
 * @return Read_Command* a pointer to the reading command suppporting this reading request
 *
 */
Read_Command *tts_speak(char *text_to_speak, Eina_Bool want_discard_previous_reading);

//TODO: add support for i18
/**
 * @brief Requests asynchronous (queued) reading of given text allowing to control,
 * if it can be discarded by other reading requests
 *
 * @param text_to_speak a text that would be read by the Text To Speach (TTS) service using default TTS language
 * @param want_discard_previous_reading determines if this reading wants the preceding reading to be discarded
 * @param discardable determines if this reading request can be discarded by subsequent reading requests
 * @param obj an accessible object associated with this reading request, which will be notified when the reading is stopped or canceled
 * @return Read_Command* a pointer to the reading command suppporting this reading request
 *
 */
Read_Command *tts_speak_customized(char *text_to_speak, Eina_Bool want_discard_previous_reading, Eina_Bool discardable, AtspiAccessible *obj);
void spi_stop(void *data);
void tts_stop_set(void);
Eina_Bool tts_pause_get(void);
Eina_Bool tts_pause_set(Eina_Bool pause_switch);
void set_utterance_cb( void(*utter_cb)(void));

#endif /* SCREEN_READER_TTS_H_ */
