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

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <atspi/atspi.h>
#include <tone_player.h>
#include "logger.h"
#include "screen_reader_tts.h"
#include "screen_reader_haptic.h"
#include "smart_notification.h"

#define RED  "\x1B[31m"
#define RESET "\033[0m"

static Eina_Bool status = EINA_FALSE;

static void _smart_notification_focus_chain_end(void);
static void _smart_notification_realized_items(int start_idx, int end_idx);

/**
 * @brief Smart Notifications event handler
 *
 * @param nt Notification event type
 * @param start_index int first visible items index smart_notification_realized_items
 * @param end_index int last visible items index used for smart_notification_realized_items
 */
void smart_notification(Notification_Type nt, int start_index, int end_index)
{
   DEBUG("START");
   if(!status)
      return;

   switch(nt)
      {
      case FOCUS_CHAIN_END_NOTIFICATION_EVENT:
         _smart_notification_focus_chain_end();
         break;
      case REALIZED_ITEMS_NOTIFICATION_EVENT:
         _smart_notification_realized_items(start_index, end_index);
         break;
      default:
         DEBUG("Gesture type %d not handled in switch", nt);
      }
}

/**
 * @brief Used for getting first and last index of visible items
 *
 * @param scrollable_object AtspiAccessible object on which scroll was triggered
 * @param start_index int first visible items index smart_notification_realized_items
 * @param end_index int last visible items index used for smart_notification_realized_items
 */
void get_realized_items_count(AtspiAccessible *scrollable_object, int *start_idx, int *end_idx)
{
   DEBUG("START");
   int count_child, jdx;
   AtspiAccessible *child_iter;
   AtspiStateType state =  ATSPI_STATE_SHOWING;

   if(!scrollable_object)
      {
         DEBUG("No scrollable object");
         return;
      }

   count_child = atspi_accessible_get_child_count(scrollable_object, NULL);

   for(jdx = 0; jdx < count_child; jdx++)
      {
         child_iter = atspi_accessible_get_child_at_index(scrollable_object, jdx, NULL);
         if (!child_iter) continue;

         AtspiStateSet* state_set = atspi_accessible_get_state_set(child_iter);

         gboolean is_visible = atspi_state_set_contains(state_set, state);
         if(is_visible)
            {
               *start_idx = jdx;
               DEBUG("Item with index %d is visible", jdx);
            }
         else
            DEBUG("Item with index %d is NOT visible", jdx);
      }
   *end_idx = *start_idx + 8;
}

/**
 * @brief Scroll-start/Scroll-end event callback
 *
 * @param event AtspiEvent
 * @param user_data UNUSED
 */

static void
_scroll_event_cb(AtspiEvent *event, gpointer user_data)
{
   if(!status)
      return;

   int start_index, end_index;
   start_index = 0;
   end_index = 0;

   gchar *role_name = atspi_accessible_get_role_name(event->source, NULL);
   fprintf(stderr, "Event: %s: %d, obj: %p: role: %s\n",
           event->type, event->detail1, event->source,
           role_name);
   g_free(role_name);

   if (!strcmp(event->type, "object:scroll-start"))
      {
         DEBUG("Scrolling started");
         tts_speak("Scrolling started", EINA_TRUE);
      }
   else if (!strcmp(event->type, "object:scroll-end"))
      {
         DEBUG("Scrolling finished");
         tts_speak("Scrolling finished", EINA_FALSE);
         get_realized_items_count((AtspiAccessible *)event->source, &start_index, &end_index);
         _smart_notification_realized_items(start_index, end_index);
      }
}

/**
 * @brief Initializer for smart notifications
 *
 *
 */
void smart_notification_init(void)
{
   DEBUG("Smart Notification init!");

   AtspiEventListener *listener;

   listener = atspi_event_listener_new(_scroll_event_cb, NULL, NULL);
   atspi_event_listener_register(listener, "object:scroll-start", NULL);
   atspi_event_listener_register(listener, "object:scroll-end", NULL);

   haptic_module_init();

   status = EINA_TRUE;
}

/**
 * @brief Smart notifications shutdown
 *
 */
void smart_notification_shutdown(void)
{
   status = EINA_FALSE;
}

/**
 * @brief Smart notifications focus chain event handler
 *
 */
static void _smart_notification_focus_chain_end(void)
{
   if(!status)
      return;

   DEBUG(RED"Smart notification - FOCUS CHAIN END"RESET);

   tone_player_stop(0);
   tone_player_start(TONE_TYPE_SUP_CONFIRM, SOUND_TYPE_MEDIA, 200, NULL);
}

/**
 * @brief Smart notifications realized items event handler
 *
 */
static void _smart_notification_realized_items(int start_idx, int end_idx)
{
   if(!status)
      return;

   if(start_idx == end_idx)
      return;

   DEBUG(RED"Smart notification - Visible items notification"RESET);

   char buf[256];

   snprintf(buf, sizeof(buf), _("IDS_REACHED_ITEMS_NOTIFICATION"), start_idx, end_idx);

   tts_speak(strdup(buf), EINA_FALSE);
}
