#define _GNU_SOURCE

#include <Ecore.h>
#include "screen_reader_tts.h"
#include "screen_reader_vconf.h"
#include "logger.h"

// ---------------------------- DEBUG HELPERS ------------------------------

#define FLUSH_LIMIT 1

static int last_utt_id;
static Eina_Bool pause_state = EINA_FALSE;
static Eina_Bool flush_flag = EINA_FALSE;
static Eina_Strbuf *txt_keep_buff = NULL;

static void(*on_utterance_end)(void);

static void _text_keep(const char *txt)
{
   if (!txt_keep_buff) return;
   if (eina_strbuf_length_get(txt_keep_buff) > 0) eina_strbuf_append(txt_keep_buff, ", ");
   eina_strbuf_append(txt_keep_buff, txt);
}

static char * get_tts_error( int r )
{
   switch( r )
      {
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

static char * get_tts_state( tts_state_e r )
{
   switch( r )
      {
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

void set_utterance_cb( void(*uter_cb)(void))
{
   on_utterance_end = uter_cb;
}


bool get_supported_voices_cb(tts_h tts, const char* language, int voice_type, void* user_data)
{
   DEBUG("LANG: %s; TYPE: %d", language, voice_type);

   Service_Data *sd = user_data;
   Voice_Info *vi = calloc(1, sizeof(Voice_Info));
   if(!vi)
      {
         ERROR(MEMORY_ERROR);
         return  ECORE_CALLBACK_CANCEL;
      }

   if(asprintf(&vi->language, "%s",language) < 0)
      {
         free(vi);
         ERROR(MEMORY_ERROR);
         return  ECORE_CALLBACK_CANCEL;
      }

   vi->voice_type = voice_type;

   sd->available_languages = eina_list_append(sd->available_languages, vi);

   return ECORE_CALLBACK_RENEW;
}

static void __tts_test_utt_started_cb(tts_h tts, int utt_id, void* user_data)
{
   DEBUG("Utterance started : utt id(%d) \n", utt_id);
   return;
}

static void __tts_test_utt_completed_cb(tts_h tts, int utt_id, void* user_data)
{
   DEBUG("Utterance completed : utt id(%d) \n", utt_id);
   if(last_utt_id - utt_id > FLUSH_LIMIT)
      flush_flag = EINA_TRUE;
   else
      {
         if(flush_flag)
            flush_flag = EINA_FALSE;
      }

   if(last_utt_id == utt_id)
      {
         DEBUG("LAST UTTERANCE");
         pause_state = EINA_FALSE;
         on_utterance_end();

      }

   return;
}

bool tts_init(void *data)
{
   DEBUG( "--------------------- TTS_init START ---------------------");
   Service_Data *sd = data;

   int r = tts_create( &sd->tts );
   DEBUG( "Create tts %d (%s)", r, get_tts_error( r ) );

   r = tts_set_mode( sd->tts, TTS_MODE_DEFAULT );
   DEBUG( "Set tts mode SR %d (%s)", r, get_tts_error( r ) );

   r = tts_prepare( sd->tts );
   DEBUG( "Prepare tts %d (%s)", r, get_tts_error( r ) );

   tts_set_state_changed_cb(sd->tts, state_changed_cb, sd);

   tts_set_utterance_started_cb(sd->tts, __tts_test_utt_started_cb, sd);
   tts_set_utterance_completed_cb(sd->tts,  __tts_test_utt_completed_cb,  sd);

   DEBUG( "---------------------- TTS_init END ----------------------\n\n");
   txt_keep_buff = eina_strbuf_new();
   return true;
}

Eina_Bool tts_pause_get(void)
{
   DEBUG( "PAUSE STATE: %d", pause_state);
   return pause_state;
}


Eina_Bool tts_pause_set(Eina_Bool pause_switch)
{
   Service_Data *sd = get_pointer_to_service_data_struct();
   if(!sd)
      return EINA_FALSE;

   if(pause_switch)
      {
         pause_state = EINA_TRUE;

         if(tts_pause(sd->tts))
            {
               pause_state = EINA_FALSE;
               return EINA_FALSE;
            }
      }
   else if(!pause_switch)
      {
         pause_state = EINA_FALSE;

         if(tts_play(sd->tts))
            {
               pause_state = EINA_TRUE;
               return EINA_FALSE;
            }
      }
   return EINA_TRUE;
}

void tts_speak(char *text_to_speak, Eina_Bool flush_switch)
{
   Service_Data *sd = get_pointer_to_service_data_struct();
   int speak_id;

   if(!sd)
      return;
   tts_state_e state;
   tts_get_state(sd->tts, &state);

   if (state != TTS_STATE_PLAYING &&
          state != TTS_STATE_PAUSED &&
          state != TTS_STATE_READY)
     {
        if (text_to_speak) _text_keep(text_to_speak);
        return;
     }

   if(flush_flag || flush_switch)
      tts_stop(sd->tts);

   DEBUG( "tts_speak\n");
   DEBUG( "text to say:%s\n", text_to_speak);
   if ( !text_to_speak ) return;
   if ( !text_to_speak[0] ) return;

   if(tts_add_text( sd->tts, text_to_speak, NULL, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &speak_id))
      return;

   DEBUG("added id to:%d\n", speak_id);
   last_utt_id = speak_id;
}

Eina_Bool update_supported_voices(void *data)
{
   DEBUG("START");
   tts_state_e state;

   Service_Data *sd = data;

   int res = tts_get_state(sd->tts, &state);

   if(res != TTS_ERROR_NONE)
      {
         DEBUG("CANNOT RETRIVE STATE");
         return EINA_FALSE;
      }

   if(state == TTS_STATE_READY)
      {
         tts_foreach_supported_voices(sd->tts, get_supported_voices_cb, sd);
      }
   else
      {
         sd->update_language_list = EINA_TRUE;
      }

   DEBUG("END")
   return EINA_TRUE;
}

void state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data)
{
   if(pause_state)
      {
         DEBUG("TTS is currently paused. Resume to start reading");
         return;
      }

   DEBUG("++++++++++++++++state_changed_cb\n++++++++++++++++++");
   DEBUG("current state:%s and previous state:%s\n", get_tts_state(current), get_tts_state(previous));
   Service_Data *sd = user_data;

   if (TTS_STATE_CREATED == previous && TTS_STATE_READY == current)
     {

       update_supported_voices(sd);

       char *txt;

       if (!txt_keep_buff) return;
       if (!eina_strbuf_length_get(txt_keep_buff)) return;

       txt = eina_strbuf_string_steal(txt_keep_buff);
       eina_strbuf_free(txt_keep_buff);
       txt_keep_buff = NULL;

       if (txt && strlen(txt) == 0) return;

       tts_speak(txt, EINA_FALSE);
       tts_play(sd->tts);
       free(txt);
     }
   else if (current == TTS_STATE_READY || current == TTS_STATE_PAUSED)
     {
         DEBUG("TTS state == %s!", get_tts_state(current));
         tts_play(sd->tts);
     }
   else
      {
         DEBUG("TTS state != ready or paused!\n");
      }
}

void spi_stop( void *data)
{
   if(!data)
      {
         ERROR("Invalid parameter");
         return;
      }

   Service_Data *sd = data;
   sd->update_language_list = false;
   free((char*)sd->text_from_dbus);
   free(sd->current_value);
   sd->text_from_dbus = NULL;
   sd->current_value = NULL;
   tts_stop(sd->tts);
}
