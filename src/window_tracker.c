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
   ERROR("Event: %s: %s", event->type, atspi_accessible_get_name(event->source, NULL));

   if ( !strcmp(event->type, "window:activate") &&
         last_active_win != event->source) //if we got activate 2 times
      {

         if (user_cb) user_cb(user_data, event->source);
         last_active_win = event->source;
      }
}

static AtspiAccessible*
_get_active_win(void)
{
   DEBUG("START");
   int i, j;
   last_active_win = NULL;
   AtspiAccessible *desktop = atspi_get_desktop(0);
   if (!desktop)
      ERROR("DESKTOP NOT FOUND");

   for (i = 0; i < atspi_accessible_get_child_count(desktop, NULL); i++)
      {
         AtspiAccessible *app = atspi_accessible_get_child_at_index(desktop, i, NULL);
         for (j = 0; j < atspi_accessible_get_child_count(app, NULL); j++)
            {
               AtspiAccessible *win = atspi_accessible_get_child_at_index(app, j, NULL);
               AtspiStateSet *states = atspi_accessible_get_state_set(win);
               AtspiRole role = atspi_accessible_get_role(win, NULL);

               if ((atspi_state_set_contains(states, ATSPI_STATE_ACTIVE)) && (role == ATSPI_ROLE_WINDOW))
                  {
                     g_object_unref(states);
                     last_active_win = win;
                     DEBUG("END");
                     return last_active_win;
                  }
            }
      }
   ERROR("END");
   return NULL;
}

void window_tracker_init(void)
{
   DEBUG("START");
   listener = atspi_event_listener_new_simple(_on_atspi_window_cb, NULL);
   atspi_event_listener_register(listener, "window:create", NULL);
   atspi_event_listener_register(listener, "window:activate", NULL);
   atspi_event_listener_register(listener, "window:restore", NULL);
}

void window_tracker_shutdown(void)
{
   DEBUG("START");
   atspi_event_listener_deregister(listener, "window:create", NULL);
   atspi_event_listener_deregister(listener, "window:activate", NULL);
   atspi_event_listener_deregister(listener, "window:restore", NULL);
   g_object_unref(listener);
   listener = NULL;
   user_cb = NULL;
   user_data = NULL;
   last_active_win = NULL;
}

void window_tracker_register(Window_Tracker_Cb cb, void *data)
{
   DEBUG("START");
   user_cb = cb;
   user_data = data;
}

void window_tracker_active_window_request(void)
{
   DEBUG("START");
   _get_active_win();
   if (user_cb) user_cb(user_data, last_active_win);
}
