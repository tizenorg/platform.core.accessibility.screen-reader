#include <math.h>
#include <atspi/atspi.h>
#include "logger.h"
#include "navigator.h"
#include "window_tracker.h"
#include "keyboard_tracker.h"
#include "pivot_chooser.h"
#include "structural_navi.h"
#include "object_cache.h"
#include "flat_navi.h"
#include "app_tracker.h"
#include "smart_notification.h"
#include "screen_reader_system.h"
#include "screen_reader_haptic.h"

#define QUICKPANEL_DOWN TRUE
#define QUICKPANEL_UP FALSE

#define DISTANCE_NB 8

#define DEBUG_MODE

#define GERROR_CHECK(error)\
  if (error)\
   {\
     ERROR("Error_log:%s",error->message);\
     g_error_free(error);\
     error = NULL;\
   }

static AtspiAccessible *top_window;
static gboolean _window_cache_builded;
static FlatNaviContext *context;

static void
_current_highlight_object_set(AtspiAccessible *obj)
{
   //TODO
   //speak virtually focused widget
}

void test_debug(AtspiAccessible *current_widget)
{
    int jdx;
    int count_child;
    gchar *role;
    GError *err = NULL;
    AtspiAccessible *child_iter = NULL;
    AtspiAccessible *parent = atspi_accessible_get_parent(current_widget, &err);
    GERROR_CHECK(err)

    if(!parent)
      return;
    count_child = atspi_accessible_get_child_count(parent, &err);
    GERROR_CHECK(err)
    DEBUG("Total childs in parent: %d\n", count_child);
    if(!count_child)
        return;

    for(jdx = 0; jdx < count_child; jdx++)
    {
        child_iter = atspi_accessible_get_child_at_index(parent, jdx, &err);
        GERROR_CHECK(err)

        if(current_widget == child_iter)
          {
            role = atspi_accessible_get_role_name(parent, &err);
            DEBUG("Childen found in parent: %s at index: %d\n", role, jdx);
          }
        else
          {
            role = atspi_accessible_get_role_name(parent, &err);
            DEBUG("Childen not found in parent: %s at index: %d\n", role, jdx);
          }
        g_free(role);
        GERROR_CHECK(err)
    }
}

static void _focus_next(void)
{
   AtspiAccessible *obj;
   if (!context)
     {
        ERROR("No navigation context created");
        return;
     }
   obj = flat_navi_context_next(context);
   // try next line
   if (!obj)
        obj = flat_navi_context_line_next(context);
   // try 'cycle' objects in context
   if (!obj)
     {
        flat_navi_context_line_first(context);
        obj = flat_navi_context_first(context);
        smart_notification(FOCUS_CHAIN_END_NOTIFICATION_EVENT, 0, 0);
     }
   if (obj)
     _current_highlight_object_set(obj);
   else
     DEBUG("Next widget not found. Abort");
}

static void _focus_prev(void)
{
   AtspiAccessible *obj;
   if (!context)
     {
        ERROR("No navigation context created");
        return;
     }
   obj = flat_navi_context_prev(context);
   // try previous line
   if (!obj)
     {
        obj = flat_navi_context_line_prev(context);
        if (obj)
           obj = flat_navi_context_last(context);
     }
   // try 'cycle' objects in context
   if (!obj)
     {
        flat_navi_context_line_last(context);
        obj = flat_navi_context_last(context);
        smart_notification(FOCUS_CHAIN_END_NOTIFICATION_EVENT, 0, 0);
     }
   if (obj)
     _current_highlight_object_set(obj);
   else
     DEBUG("Previous widget not found. Abort");
}

static void
_on_cache_builded(void *data)
{
   DEBUG("Cache building");
   _window_cache_builded = TRUE;
   AtspiAccessible *pivot = NULL;
   if (context)
     {
        pivot = flat_navi_context_current_get(context);
        flat_navi_context_free(context);
     }
   context = flat_navi_context_create(top_window);

   // try to set previous object in new context
   if (flat_navi_context_current_set(context, pivot))
     _current_highlight_object_set(pivot);
   else
     _current_highlight_object_set(flat_navi_context_current_get(context));
   DEBUG("Cache building finished");
}

static void
_view_content_changed(AppTrackerEventType type, void *user_data)
{
   DEBUG("View content changed");
   _window_cache_builded = FALSE;
   if (top_window)
      object_cache_build_async(top_window, 5, _on_cache_builded, NULL);
}

static void on_window_activate(void *data, AtspiAccessible *window)
{
   gchar *name;
   DEBUG("... on window activate ...");

   app_tracker_callback_unregister(top_window, APP_TRACKER_EVENT_VIEW_CHANGED, _view_content_changed, NULL);

   if(window)
   {
      app_tracker_callback_register(window, APP_TRACKER_EVENT_VIEW_CHANGED, _view_content_changed, NULL);
      name = atspi_accessible_get_name(window, NULL);
      DEBUG("Window name: %s", name);
      _window_cache_builded = FALSE;
      object_cache_build_async(window, 5, _on_cache_builded, NULL);
      g_free(name);
   }
   else
   {
       ERROR("No top window found!");
   }
   top_window = window;
}

void kb_tracker (void *data, Key k)
{
   switch(k)
   {
      case KEY_LEFT:
          _focus_prev();
          break;
      case KEY_RIGHT:
          _focus_next();
          break;
      default:
          DEBUG("Key %d not supported \n", k);
   }
}

void navigator_init(void)
{
   window_tracker_init();
   window_tracker_register(on_window_activate, NULL);
   window_tracker_active_window_request();
   app_tracker_init();
   smart_notification_init();
   system_notifications_init();
   keyboard_tracker_init();
   keyboard_tracker_register(kb_tracker, NULL);
}

void navigator_shutdown(void)
{
   if (context)
     {
        flat_navi_context_free(context);
        context = NULL;
     }
   object_cache_shutdown();
   app_tracker_shutdown();
   window_tracker_shutdown();
   smart_notification_shutdown();
   system_notifications_shutdown();
   keyboard_tracker_shutdown();
}
