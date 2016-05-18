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
    Eina_Bool interruptable;
    Eina_Bool is_playing;
    AtspiAccessible *obj;
} Read_Command;

bool tts_init(void *data);
void tts_shutdown(void *data);
void state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data);
/**
 * @brief Requests interruptible, asynchronous (queued) reading of given text.
 *
 * @param text_to_speak a text that would be read by the Text To Speach (TTS) service
 * @param flush_switch a switch that determines if the preciding reading requests has to be removed from reading queue
 * @return Read_Command* a pointer to the reading command suppporting this reading request
 *
 */
Read_Command *tts_speak(char *text_to_speak, Eina_Bool flush_switch);
/**
 * @brief Requests asynchronous (queued) reading of given text allowing to controll,
 * if it can be interrupted by other reading requests or not and.
 *
 * @param text_to_speak a text that would be read by the Text To Speach (TTS) service
 * @param flush_switch determines if the preciding reading requests has to be removed from reading queue
 * @param interruptible determines if this reading request can be interrupted by subsequent reading requests or not
 * @param obj an accessible object associated with this reading request, which will be notified when the reading is stopped or canceled
 * @return Read_Command* a pointer to the reading command suppporting this reading request
 *
 */
Read_Command *tts_speak_customized(char *text_to_speak, Eina_Bool flush_switch, Eina_Bool interruptible, AtspiAccessible *obj);
void spi_stop(void *data);
void tts_stop_set(void);
Eina_Bool tts_pause_get(void);
Eina_Bool tts_pause_set(Eina_Bool pause_switch);
void set_utterance_cb( void(*utter_cb)(void));

#endif /* SCREEN_READER_TTS_H_ */
