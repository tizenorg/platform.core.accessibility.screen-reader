#include <string.h>
#include "window_tracker.h"
#include "logger.h"

static Window_Tracker_Cb user_cb;
static void *user_data;
static AtspiEventListener *listener;
static AtspiAccessible *last_active_win;

static AtspiAccessible*
_get_window_object_from_given(AtspiAccessible *obj)
{
   if (!obj)
      return NULL;

   if (atspi_accessible_get_role(obj, NULL) != ATSPI_ROLE_DESKTOP_FRAME)
      return NULL;

   AtspiAccessible *win = NULL;
   AtspiAccessible *app = NULL;
   AtspiAccessible *ret = NULL;
   AtspiStateSet *st = NULL;
   int desktop_childs;
   int app_childs;
   int i,j;

   desktop_childs = atspi_accessible_get_child_count(obj, NULL);

   for (i=0; i < desktop_childs; i++)
      {
         app = atspi_accessible_get_child_at_index(obj, i, NULL);
         if (atspi_accessible_get_role(app, NULL) == ATSPI_ROLE_APPLICATION)
            {
               app_childs = atspi_accessible_get_child_count(app, NULL);
               for (j=0; j < app_childs; j++)
                  {
                     win = atspi_accessible_get_child_at_index(app, j, NULL);
                     if (win) {
                        st = atspi_accessible_get_state_set (win);
                        if (atspi_state_set_contains(st, ATSPI_STATE_ACTIVE)) {
                           return win;
                        }
                     }
                  }
            }
      }

   return ret;
}

static void
_on_atspi_window_cb(const AtspiEvent *event)
{
   if (event->source == last_active_win)
      {
         DEBUG("Window already handled");
         return;
      }

   if (!strcmp(event->type, "window:restore") ||
         !strcmp(event->type, "window:activate"))
      {
         if (user_cb) user_cb(user_data, event->source);
         last_active_win = event->source;
      }
   else
      {
         AtspiAccessible *obj = NULL;
         obj = _get_window_object_from_given(event->source);

         if (obj == last_active_win) {
            DEBUG("Window already handled");
            return;
         }

         if (obj)
            {
               if (!strcmp(event->type, "object:children-changed:add"))
                  {
                     if (user_cb) user_cb(user_data, obj);
                     last_active_win = obj;
                  }
            }
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
   atspi_event_listener_register(listener, "object:children-changed", NULL);
   atspi_event_listener_register(listener, "window:deactivate", NULL);
   atspi_event_listener_register(listener, "window:activate", NULL);
   atspi_event_listener_register(listener, "window:restore", NULL);
}

void window_tracker_shutdown(void)
{
   DEBUG("START");
   atspi_event_listener_deregister(listener, "object:children-changed", NULL);
   atspi_event_listener_deregister(listener, "window:deactivate", NULL);
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
