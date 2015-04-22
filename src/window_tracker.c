#include <string.h>
#include "window_tracker.h"
#include "logger.h"

static Window_Tracker_Cb user_cb;
static void *user_data;
static AtspiEventListener *listener;
static AtspiAccessible *last_active_win;

static void
_on_atspi_window_cb(const AtspiEvent *event)
{
   DEBUG("START");
   DEBUG("%s", event->type);
   if (!strcmp(event->type, "window:activate") ||
       !strcmp(event->type, "window:restore") ||
       !strcmp(event->type, "window:create"))
     {
        if (last_active_win != event->source)
            {
              if (user_cb) user_cb(user_data, event->source);
              last_active_win = event->source;
            }
     }
   else if (!strcmp(event->type, "window:deactivate") ||
            !strcmp(event->type, "window:destroy"))
     {
        if ((last_active_win == event->source) &&
            user_cb)
          user_cb(user_data, NULL);
        last_active_win = NULL;
     }
}

static AtspiAccessible*
_get_active_win(void)
{
   int i, j;
   last_active_win = NULL;
   AtspiAccessible *desktop = atspi_get_desktop(0);
   DEBUG("START");
   if (desktop) {
       DEBUG("DESKTOP FOUND");
   }
   else {
       ERROR("DESKTOP NOT FOUND");
   }

   for (i = 0; i < atspi_accessible_get_child_count(desktop, NULL); i++) {
       AtspiAccessible *app = atspi_accessible_get_child_at_index(desktop, i, NULL);
       for (j = 0; j < atspi_accessible_get_child_count(app, NULL); j++) {
          AtspiAccessible *win = atspi_accessible_get_child_at_index(app, j, NULL);
          AtspiStateSet *states = atspi_accessible_get_state_set(win);
          AtspiRole role = atspi_accessible_get_role(win, NULL);

          if ((atspi_state_set_contains(states, ATSPI_STATE_ACTIVE)) && (role == ATSPI_ROLE_WINDOW))
            {
               g_object_unref(states);
               last_active_win = win;
               break;
            }
          g_object_unref(states);
       }
   }
   return last_active_win;
}

void window_tracker_init(void)
{
   DEBUG("START");
   listener = atspi_event_listener_new_simple(_on_atspi_window_cb, NULL);
   atspi_event_listener_register(listener, "window:activate", NULL);
   atspi_event_listener_register(listener, "window:create", NULL);
   atspi_event_listener_register(listener, "window:deactivate", NULL);
   atspi_event_listener_register(listener, "window:restore", NULL);
   atspi_event_listener_register(listener, "window:destroy", NULL);
}

void window_tracker_shutdown(void)
{
   atspi_event_listener_deregister(listener, "window:activate", NULL);
   atspi_event_listener_deregister(listener, "window:create", NULL);
   atspi_event_listener_deregister(listener, "window:deactivate", NULL);
   atspi_event_listener_deregister(listener, "window:restore", NULL);
   atspi_event_listener_deregister(listener, "window:destroy", NULL);
   g_object_unref(listener);
   listener = NULL;
   user_cb = NULL;
   user_data = NULL;
   last_active_win = NULL;
}

void window_tracker_register(Window_Tracker_Cb cb, void *data)
{
   user_cb = cb;
   user_data = data;
}

void window_tracker_active_window_request(void)
{
   DEBUG("START");
   _get_active_win();
   if (user_cb) user_cb(user_data, last_active_win);
}
