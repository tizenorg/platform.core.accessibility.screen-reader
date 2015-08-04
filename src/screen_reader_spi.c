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

#include <stdio.h>
#include "screen_reader_spi.h"
#include "screen_reader_tts.h"
#include "logger.h"
#ifdef RUN_IPC_TEST_SUIT
#include "test_suite/test_suite.h"
#endif

#define EPS 0.000000001

/** @brief Service_Data used as screen reader internal data struct*/
static Service_Data *service_data;

typedef struct
{
   char *key;
   char *val;
} Attr;


/**
 * @brief Debug function. Print current toolkit version/event
 * type/event source/event detail1/event detail2
 *
 * @param AtspiEvent instance
 *
 */
static void display_info(const AtspiEvent *event)
{
   AtspiAccessible  *source = event->source;
   gchar *name = atspi_accessible_get_name(source, NULL);
   gchar *role = atspi_accessible_get_localized_role_name(source, NULL);
   gchar *toolkit = atspi_accessible_get_toolkit_name(source, NULL);

   DEBUG("--------------------------------------------------------");
   DEBUG("Toolkit: %s; Event_type: %s; (%d, %d)", toolkit, event->type, event->detail1, event->detail2);
   DEBUG("Name: %s; Role: %s", name, role);
   DEBUG("--------------------------------------------------------");
}

Eina_Bool double_click_timer_cb(void *data)
{
   Service_Data *sd = data;
   sd->clicked_widget = NULL;

   return EINA_FALSE;
}

bool allow_recursive_name(AtspiAccessible *obj)
{
   AtspiRole r = atspi_accessible_get_role(obj, NULL);
   if (r == ATSPI_ROLE_FILLER)
      return true;
   return false;
}

char *generate_description_for_subtree(AtspiAccessible *obj)
{
   DEBUG("START");
   if (!allow_recursive_name(obj))
      return strdup("");


   if (!obj)
      return strdup("");
   int child_count = atspi_accessible_get_child_count(obj, NULL);

   DEBUG("There is %d children inside this filler", child_count);
   if (!child_count)
      return strdup("");

   int i;
   char *name = NULL;
   char *below = NULL;
   char ret[256] = "\0";
   AtspiAccessible *child = NULL;
   for (i=0; i < child_count; i++)
      {
         child = atspi_accessible_get_child_at_index(obj, i, NULL);
         name = atspi_accessible_get_name(child, NULL);
         DEBUG("%d child name:%s", i, name);
         if (name && strncmp(name, "\0", 1))
            {
               strncat(ret, name, sizeof(ret) - strlen(ret) - 1);
            }
         strncat(ret, " ", 1);
         below = generate_description_for_subtree(child);
         if (strncmp(below, "\0", 1))
            {
               strncat(ret, below, sizeof(ret) - strlen(ret) - 1);
            }
         g_object_unref(child);
         free(below);
         free(name);
      }
   return strdup(ret);
}

static char *spi_on_state_changed_get_text(AtspiEvent *event, void *user_data)
{
   Service_Data *sd = (Service_Data*)user_data;
   char *name;
   char *names = NULL;
   char *description;
   char *role_name;
   char *other;
   char ret[256] = "\0";
   sd->currently_focused = event->source;

   description = atspi_accessible_get_description(sd->currently_focused, NULL);
   name = atspi_accessible_get_name(sd->currently_focused, NULL);
   role_name = atspi_accessible_get_localized_role_name(sd->currently_focused, NULL);
   other = generate_description_for_subtree(sd->currently_focused);

   DEBUG("->->->->->-> WIDGET GAINED HIGHLIGHT: %s <-<-<-<-<-<-<-", name);
   DEBUG("->->->->->-> FROM SUBTREE HAS NAME:  %s <-<-<-<-<-<-<-", other);

   if (name && strncmp(name, "\0", 1))
      names = strdup(name);
   else if (other && strncmp(other, "\0", 1))
      names = strdup(other);

   if (names)
      {
         strncat(ret, names, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", 2);
      }

   if (role_name)
      strncat(ret, role_name, sizeof(ret) - strlen(ret) - 1);

   if (description)
      {
         if (strncmp(description, "\0", 1))
            strncat(ret, ", ", 2);
         strncat(ret, description, sizeof(ret) - strlen(ret) - 1);
      }

   free(name);
   free(names);
   free(description);
   free(role_name);
   free(other);

   return strdup(ret);
}

static char *spi_on_caret_move_get_text(AtspiEvent *event, void *user_data)
{
   Service_Data *sd = (Service_Data*)user_data;
   sd->currently_focused = event->source;
   char *return_text;

   AtspiText *text_interface = atspi_accessible_get_text_iface(sd->currently_focused);
   if(text_interface)
      {
         DEBUG("->->->->->-> WIDGET CARET MOVED: %s <-<-<-<-<-<-<-",
               atspi_accessible_get_name(sd->currently_focused, NULL));

         int char_count = (int)atspi_text_get_character_count(text_interface, NULL);
         int caret_pos = atspi_text_get_caret_offset(text_interface, NULL);
         if(!caret_pos)
            {
               DEBUG("MIN POSITION REACHED");
               if(asprintf(&return_text, "%s %s", (char*)atspi_text_get_text(text_interface, caret_pos, caret_pos + 1, NULL), _("IDS_REACHED_MIN_POS")) < 0)
                  {
                     ERROR(MEMORY_ERROR);
                     return NULL;
                  }
            }
         else if(char_count == caret_pos)
            {
               DEBUG("MAX POSITION REACHED");
               if(asprintf(&return_text, "%s %s", (char*)atspi_text_get_text(text_interface, caret_pos, caret_pos + 1, NULL), _("IDS_REACHED_MAX_POS")) < 0)
                  {
                     ERROR(MEMORY_ERROR);
                     return NULL;
                  }
            }
         else
            {
               if(asprintf(&return_text, "%s", (char*)atspi_text_get_text(text_interface, caret_pos, caret_pos + 1, NULL)) < 0)
                  {
                     ERROR(MEMORY_ERROR);
                     return NULL;
                  }
            }
      }
   else
      {
         ERROR(MEMORY_ERROR);
         return NULL;
      }
   return return_text;
}

static char *spi_on_value_changed_get_text(AtspiEvent *event, void *user_data)
{
   Service_Data *sd = (Service_Data*)user_data;
   char *text_to_read = NULL;

   sd->currently_focused = event->source;

   AtspiValue *value_interface = atspi_accessible_get_value_iface(sd->currently_focused);
   if(value_interface)
      {
         DEBUG("->->->->->-> WIDGET VALUE CHANGED: %s <-<-<-<-<-<-<-",
               atspi_accessible_get_name(sd->currently_focused, NULL));

         double current_temp_value = (double)atspi_value_get_current_value(value_interface, NULL);
         if(abs(current_temp_value - atspi_value_get_maximum_value(value_interface, NULL)) < EPS)
            {
               DEBUG("MAX VALUE REACHED");
               if(asprintf(&text_to_read, "%.2f %s", current_temp_value, _("IDS_REACHED_MAX_VAL")) < 0)
                  {
                     ERROR(MEMORY_ERROR);
                     return NULL;
                  }
            }
         else if(abs(current_temp_value - atspi_value_get_minimum_value(value_interface, NULL)) < EPS)
            {
               DEBUG("MIN VALUE REACHED");
               if(asprintf(&text_to_read, "%.2f %s", current_temp_value, _("IDS_REACHED_MIN_VAL")) < 0)
                  {
                     ERROR(MEMORY_ERROR);
                     return NULL;
                  }
            }
         else
            {
               if(asprintf(&text_to_read, "%.2f", current_temp_value) < 0)
                  {
                     ERROR(MEMORY_ERROR);
                     return NULL;
                  }
            }
      }

   return text_to_read;
}

char *spi_event_get_text_to_read(AtspiEvent *event, void *user_data)
{
   DEBUG("START");

   Service_Data *sd = (Service_Data*)user_data;
   char *text_to_read;

   DEBUG("TRACK SIGNAL:%s", sd->tracking_signal_name);
   DEBUG("WENT EVENT:%s", event->type);

   if(!sd->tracking_signal_name)
      {
         ERROR("Invalid tracking signal name");
         return NULL;
      }

   if(!strncmp(event->type, sd->tracking_signal_name, strlen(event->type)) && event->detail1 == 1)
      {
         text_to_read = spi_on_state_changed_get_text(event, user_data);
      }
   else if(!strncmp(event->type, CARET_MOVED_SIG, strlen(event->type)))
      {
         text_to_read = spi_on_caret_move_get_text(event, user_data);
      }
   else if(!strncmp(event->type, VALUE_CHANGED_SIG, strlen(event->type)))
      {
         text_to_read = spi_on_value_changed_get_text(event, user_data);
      }
   else
      {
         ERROR("Unknown event type");
         return NULL;
      }

   return text_to_read;
}

void spi_event_listener_cb(AtspiEvent *event, void *user_data)
{
   DEBUG("START")
   display_info(event);

   if(!user_data)
      {
         ERROR("Invalid parameter");
         return;
      }

   char *text_to_read = spi_event_get_text_to_read(event, user_data);
   if(!text_to_read)
      {
         ERROR("Can not prepare text to read");
         return;
      }
   DEBUG("SPEAK: %s", text_to_read)
   tts_speak(text_to_read, EINA_TRUE);

   tts_state_e state;
   Service_Data *sd = (Service_Data*)user_data;
   tts_get_state(sd->tts, &state);

   if (state == TTS_STATE_READY || state == TTS_STATE_PAUSED)
     {
        int ret = tts_play(sd->tts);
        if (TTS_ERROR_NONE != ret)
           ERROR("Fail to play TTS : ret(%d)", ret);
     }
   else
     {
           DEBUG("Current tts state: %d", state);
     }

   free(text_to_read);
   DEBUG("END")
}

/**
  * @brief Initializer for screen-reader atspi listeners
  *
  * @param user_data screen-reader internal data
  *
**/
void spi_init(Service_Data *sd)
{
   if(!sd)
      {
         ERROR("Invalid parameter");
         return;
      }
   DEBUG( "--------------------- SPI_init START ---------------------");
   service_data = sd;

   DEBUG( ">>> Creating listeners <<<");

   sd->spi_listener = atspi_event_listener_new(spi_event_listener_cb, service_data, NULL);
   if(sd->spi_listener == NULL)
      {
         DEBUG("FAILED TO CREATE spi state changed listener")
      }

   // ---------------------------------------------------------------------------------------------------

   DEBUG("TRACKING SIGNAL:%s", sd->tracking_signal_name);

   gboolean ret1 = atspi_event_listener_register(sd->spi_listener, sd->tracking_signal_name, NULL);
   if(ret1 == false)
      {
         DEBUG("FAILED TO REGISTER spi focus/highlight listener");
      }
   GError *error = NULL;
   gboolean ret2 = atspi_event_listener_register(sd->spi_listener, CARET_MOVED_SIG, &error);
   if(ret2 == false)
      {
         DEBUG("FAILED TO REGISTER spi caret moved listener: %s", error ? error->message : "no error message");
         if (error)
             g_clear_error(&error);
      }

   gboolean ret3 = atspi_event_listener_register(sd->spi_listener, VALUE_CHANGED_SIG, &error);
   if(ret3 == false)
      {
         DEBUG("FAILED TO REGISTER spi value changed listener: %s", error ? error->message : "no error message");
         if (error)
             g_clear_error(&error);
      }

   if(ret1 == true && ret2 == true && ret3 == true)
      {
         DEBUG("spi listener REGISTERED");
      }

   DEBUG( "---------------------- SPI_init END ----------------------\n\n");
}
