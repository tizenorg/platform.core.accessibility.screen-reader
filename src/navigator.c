#include <Ecore_X.h>
#include <Ecore.h>
#include <math.h>
#include <atspi/atspi.h>
#include "logger.h"
#include "navigator.h"
#include "gesture_tracker.h"
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

typedef struct
{
  int x,y;
} last_focus_t;

static last_focus_t last_focus = {-1,-1};
static AtspiAccessible *current_obj;
static AtspiAccessible *top_window;
//static AtspiScrollable *scrolled_obj;
static Eina_Bool _window_cache_builded;
static FlatNaviContext *context;

static void
_current_highlight_object_set(AtspiAccessible *obj)
{
   DEBUG("START");
   GError *err = NULL;
   if (!obj)
     {
        DEBUG("Clearing focus object");
        current_obj = NULL;
        return;
     }
   if (current_obj == obj)
     {
        DEBUG("Object already focuseded");
        DEBUG("Object name:%s", atspi_accessible_get_name(obj, NULL));
        return;
     }
    if (obj && ATSPI_IS_COMPONENT(obj))
      {
         DEBUG("OBJ && IS COMPONENT");
         AtspiComponent *comp = atspi_accessible_get_component(obj);
         if (!comp)
           {
             GError *err = NULL;
             gchar *role = atspi_accessible_get_role_name(obj, &err);
             ERROR("AtspiComponent *comp NULL, [%s]", role);
             GERROR_CHECK(err);
             g_free(role);
             return;
           }
         atspi_component_grab_focus(comp, &err);
         GERROR_CHECK(err)
         gchar *name;
         gchar *role;

         current_obj = obj;
         const ObjectCache *oc = object_cache_get(obj);

         if (oc) {
             name = atspi_accessible_get_name(obj, &err);
             GERROR_CHECK(err)
             role = atspi_accessible_get_role_name(obj, &err);
             GERROR_CHECK(err)
             DEBUG("New focused object: %s, role: %s, (%d %d %d %d)",
               name,
               role,
               oc->bounds->x, oc->bounds->y, oc->bounds->width, oc->bounds->height);
             haptic_vibrate_start();
         }
         else {
           name = atspi_accessible_get_name(obj, &err);
           GERROR_CHECK(err)
           role = atspi_accessible_get_role_name(obj, &err);
           GERROR_CHECK(err)
           DEBUG("New focused object: %s, role: %s",
               name,
               role
               );
           haptic_vibrate_start();
               }
         g_free(role);
         g_free(name);
      }
    else
       DEBUG("Unable to focus on object");
   DEBUG("END");
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

#if 0
static void object_get_x_y(AtspiAccessible * accessibleObject, int* x, int* y)
{
    GError * error = NULL;

   if ( ATSPI_IS_COMPONENT(accessibleObject) )
   {
       AtspiComponent * component = ATSPI_COMPONENT(accessibleObject);
       AtspiPoint *position = atspi_component_get_position (component, ATSPI_COORD_TYPE_SCREEN, &error);
       if ( error != NULL )
       {
           g_error_free(error);
       }
       if ( position != NULL )
       {
           *x = position->x;
           *y = position->y;
           g_free ( position );
       }
   }
}

static void object_get_wh(AtspiAccessible * accessibleObject, int* width, int* height)
{
    GError * error = NULL;

   if ( ATSPI_IS_COMPONENT(accessibleObject) )
   {
       AtspiComponent * component = ATSPI_COMPONENT(accessibleObject);
       AtspiPoint * size = atspi_component_get_size (component, &error);
       if ( error != NULL )
       {
           g_error_free(error);
       }
       if ( size != NULL )
       {
           *width = size->x;
           *height = size->y;
           g_free ( size );
       }
   }
}

static void find_objects(AtspiAccessible* parent, gint x, gint y, gint radius, double* distance, AtspiAccessible **find_app)
{
    AtspiAccessible* app = NULL;
    GError* err = NULL;
    int jdx, kdx;

    int count_child = atspi_accessible_get_child_count(parent, &err);

    for(jdx = 0; jdx < count_child; jdx++)
    {
        app = atspi_accessible_get_child_at_index(parent, jdx, &err);

        AtspiStateSet* state_set = atspi_accessible_get_state_set (app);
        AtspiStateType state =  ATSPI_STATE_VISIBLE;

        int height = 0, width = 0, xpos1 = 0, ypos1 = 0, xpos2 = 0, ypos2 = 0;

        object_get_wh(app, &width, &height); object_get_x_y(app, &xpos1, &ypos1);

        gboolean is_visile = atspi_state_set_contains(state_set, state);

        if(is_visile == TRUE && width > 0 && height > 0)
        {
            xpos2 = xpos1 + width;
            ypos2 = ypos1 + height;

            double set_distance[DISTANCE_NB] = {0};
            double min_distance = DBL_MAX;

            set_distance[0] = pow((x - xpos1), 2) + pow((y - ypos1), 2);
            set_distance[1] = pow((x - xpos2), 2) + pow((y - ypos1), 2);
            set_distance[2] = pow((x - xpos1), 2) + pow((y - ypos2), 2);
            set_distance[3] = pow((x - xpos2), 2) + pow((y - ypos2), 2);
            set_distance[4] = DBL_MAX;
            set_distance[5] = DBL_MAX;
            set_distance[6] = DBL_MAX;
            set_distance[7] = DBL_MAX;

            if(x >= fmin(xpos1, xpos2) && x <= fmax(xpos1, xpos2))
            {
                set_distance[4] = pow((y - ypos1), 2);
                set_distance[5] = pow((y - ypos2), 2);
            }

            if(y >= fmin(ypos1, ypos2) && y <= fmax(ypos1, ypos2))
            {
                set_distance[6] = pow((x - xpos1), 2);
                set_distance[7] = pow((x - xpos2), 2);
            }

            for(kdx = 0; kdx < DISTANCE_NB; kdx++)
            {
                if(set_distance[kdx] < min_distance)
                    min_distance = set_distance[kdx];
            }

            if(min_distance <= *distance && (radius < 0 || (radius >= 0 && min_distance <= radius)))
            {
                *distance = min_distance;
                *find_app = app;
            }
            find_objects(app, x, y, radius, distance, find_app);
        }
    }
}

static AtspiAccessible *get_nearest_widget(AtspiAccessible* app_obj, gint x_cord, gint y_cord, gint radius)
{
    int xn = 0, yn = 0;
    GError *err = NULL;
    AtspiAccessible* f_app_obj = app_obj;
    double distance = DBL_MAX;
    int jdx = 0;

    int count_child = atspi_accessible_get_child_count(app_obj, &err);

    find_objects(app_obj, x_cord, y_cord, radius, &distance, &f_app_obj);

    return f_app_obj;
}
#endif

static void _focus_widget(Gesture_Info *info)
{
    DEBUG("START");
    AtspiAccessible *target_widget;
    AtspiComponent *window_component;
    GError *err = NULL;

    window_component = atspi_accessible_get_component(top_window);
    if(!window_component)
        return;
    if ((last_focus.x == info->x_begin) && (last_focus.y == info->y_begin))
      return;

    target_widget = atspi_component_get_accessible_at_point(window_component, info->x_begin, info->y_begin, ATSPI_COORD_TYPE_WINDOW, &err);
    GERROR_CHECK(err)
    if (target_widget) {
         DEBUG("WIDGET FOUND:%s", atspi_accessible_get_name(target_widget, NULL));
         if (flat_navi_context_current_set(context, target_widget))
           {
             _current_highlight_object_set(target_widget);
             last_focus.x = info->x_begin;
             last_focus.y = info->y_begin;
           }
         else
           ERROR("Hoveed object not found in window context[%dx%d][%s]", info->x_begin, info->y_begin, atspi_accessible_get_role_name(top_window, &err));
    }
    else
      DEBUG("NO widget under (%d, %d) found",
            info->x_begin, info->y_begin);
    DEBUG("END");
}

static void _focus_next(void)
{
   DEBUG("START");
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
   DEBUG("END");
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

static void _value_inc_widget(void)
{
    AtspiAccessible* current_widget = NULL;
    AtspiText *text_interface;
    gint current_offset;
    gboolean ret;
    GError *err = NULL;
    gchar *role;

    if(!current_obj)
      return;

    current_widget = current_obj;

    role = atspi_accessible_get_role_name(current_widget, &err);
    GERROR_CHECK(err)
    if(!strcmp(role, "entry"))
    {
        text_interface = atspi_accessible_get_text(current_widget);
        if(text_interface)
        {
            current_offset = atspi_text_get_caret_offset(text_interface, &err);
            GERROR_CHECK(err)
            ret = atspi_text_set_caret_offset(text_interface, current_offset + 1, &err);
            GERROR_CHECK(err)
            if(ret)
            {
                ERROR("Caret position increment done");
            }
            else
            {
                ERROR("Caret position increment error");
            }
        }
        else
            ERROR("No text interface supported!");
        g_free(role);
        return;
    }
    g_free(role);
    AtspiValue *value_interface = atspi_accessible_get_value(current_widget);
    if(value_interface)
    {
        ERROR("Value interface supported!\n");
        gdouble current_val = atspi_value_get_current_value(value_interface, &err);
        GERROR_CHECK(err)
        ERROR("Current value: %f\n ", (double)current_val);
        gdouble minimum_inc = atspi_value_get_minimum_increment(value_interface, &err);
        ERROR("Minimum increment: %f\n ", (double)minimum_inc);
        GERROR_CHECK(err)
        atspi_value_set_current_value(value_interface, current_val + minimum_inc, &err);
        GERROR_CHECK(err)
    }
    else
        ERROR("No value interface supported!\n");
}

static void _value_dec_widget(void)
{
    AtspiAccessible* current_widget = NULL;
    AtspiText *text_interface;
    gint current_offset;
    GError *err = NULL;
    gboolean ret;
    gchar *role;

    if(!current_obj)
      return;
    current_widget = current_obj;

    role = atspi_accessible_get_role_name(current_widget, &err);
    GERROR_CHECK(err)
    if(!strcmp(role, "entry"))
    {
        text_interface = atspi_accessible_get_text(current_widget);
        if(text_interface)
        {
            current_offset = atspi_text_get_caret_offset(text_interface, &err);
            GERROR_CHECK(err)
            ret = atspi_text_set_caret_offset(text_interface, current_offset - 1, &err);
            GERROR_CHECK(err)
            if(ret)
            {
                ERROR("Caret position decrement done");
            }
            else
            {
                ERROR("Caret position decrement error");
            }
        }
        else
            ERROR("No text interface supported!");
        g_free(role);
        return;
    }
    g_free(role);

    AtspiValue *value_interface = atspi_accessible_get_value(current_widget);
    if(value_interface)
    {
        ERROR("Value interface supported!\n");
        gdouble current_val = atspi_value_get_current_value(value_interface, &err);
        GERROR_CHECK(err)
        ERROR("Current value: %f\n ", (double)current_val);
        gdouble minimum_inc = atspi_value_get_minimum_increment(value_interface, &err);
        GERROR_CHECK(err)
        ERROR("Minimum increment: %f\n ", (double)minimum_inc);
        atspi_value_set_current_value(value_interface, current_val - minimum_inc, &err);
        GERROR_CHECK(err)
    }
    else
        ERROR("No value interface supported!\n");
}

static void _activate_widget(void)
{
    //activate the widget
    //only if activate mean click
    //special behavior for entry, caret should move from first/last last/first
    DEBUG("START");
    AtspiAccessible *current_widget = NULL;
    AtspiComponent *focus_component;
    GError *err = NULL;

    if(!current_obj)
      return;

    current_widget = current_obj;

    gchar *roleName;
    gchar *actionName;
    roleName = atspi_accessible_get_role_name(current_widget, &err);
    GERROR_CHECK(err)
    ERROR("Widget role prev: %s\n", roleName);

    if(!strcmp(roleName, "entry"))
    {
        focus_component = atspi_accessible_get_component(current_widget);
        if (focus_component != NULL)
        {
            if (atspi_component_grab_focus(focus_component, &err) == TRUE)
              {
                ERROR("Entry activated\n");
                GERROR_CHECK(err)
              }
            g_free(roleName);
            return;
        }
    }
    g_free(roleName);

    AtspiAction *action;
    gint number;
    int i;
    int k;

    action = atspi_accessible_get_action(current_widget);

    if(!action)
      {
        ERROR("Action null");
        return;
      }
    number = atspi_action_get_n_actions(action, &err);
    ERROR("Number of available action = %d\n", number);
    GERROR_CHECK(err)
        GArray *array = atspi_accessible_get_interfaces(current_widget);
        ERROR("TAB LEN = %d \n", array->len);

        for (k=0; k < array->len; k++)
            ERROR("Interface = %s\n", g_array_index( array, gchar *, k ));

        for (i=0; i<number; i++)
        {
            actionName = atspi_action_get_name(action, i, &err);
            ERROR("Action name = %s\n", actionName);
            GERROR_CHECK(err)

            if (actionName && !strcmp("click", actionName))
            {
                atspi_action_do_action(action, 0, &err);
                GERROR_CHECK(err)
            }
            g_free(actionName);
        }

}

static void _quickpanel_change_state(gboolean quickpanel_switch)
{
    DEBUG("START");
    Ecore_X_Window xwin = 0;

    ERROR(quickpanel_switch ? "QUICKPANEL STATE ON" : "QUICKPANEL STATE OFF");

    Ecore_X_Illume_Quickpanel_State state;



    ecore_x_window_prop_xid_get(ecore_x_window_root_first_get(),
                                     ECORE_X_ATOM_NET_ACTIVE_WINDOW,
                                     ECORE_X_ATOM_WINDOW,
                                     &xwin, 1);

    state = quickpanel_switch ? ECORE_X_ILLUME_QUICKPANEL_STATE_ON : ECORE_X_ILLUME_QUICKPANEL_STATE_OFF;

    ecore_x_e_illume_quickpanel_state_set(xwin, state);

    ecore_x_e_illume_quickpanel_state_send(ecore_x_e_illume_zone_get(xwin), state);

    ecore_main_loop_iterate();
}

/**
 * @brief Gets 'deepest' Scrollable accessible containing (x,y) point
 */
/*
static AtspiScrollable*
_find_scrollable_ancestor_at_xy(int x, int y)
{
   AtspiAccessible *ret = NULL;
   AtspiRect *rect;
   GError *err = NULL;

   if (!top_window || !ATSPI_IS_COMPONENT(top_window))
     {
        DEBUG("No active window detected or no AtspiComponent interface available");
        return NULL;
     }

   rect = atspi_component_get_extents(ATSPI_COMPONENT(top_window), ATSPI_COORD_TYPE_SCREEN, &err);
   GERROR_CHECK(err)
   if (!rect)
     {
        ERROR("Unable to fetch window screen coordinates");
        return NULL;
     }

   // Scroll must originate within window borders
   if ((x < rect->x) || (x > rect->x + rect->width) ||
       (y < rect->y) || (y > rect->y + rect->height))
     {
        DEBUG("Scroll don't start within active window borders");
        g_free(rect);
        return NULL;
     }

   ret = atspi_component_get_accessible_at_point(ATSPI_COMPONENT(top_window), x, y, ATSPI_COORD_TYPE_SCREEN, &err);
   GERROR_CHECK(err)
   if (!ret)
     {
        ERROR("Unable to get accessible objct at (%d, %d) screen coordinates.", x, y);
        return NULL;
     }
gchar *name;
gchar *role;
   // find accessible object with Scrollable interface
   while (ret && (ret != top_window))
     {
       name = atspi_accessible_get_name(ret, &err);
       GERROR_CHECK(err)
       role = atspi_accessible_get_role_name(ret, &err);
       GERROR_CHECK(err)
       DEBUG("Testing for scrollability: %s %s",
                   name, role);
        if (atspi_accessible_get_scrollable(ret))
          {
             DEBUG("Scrollable widget found at (%d, %d), name: %s, role: %s", x, y,
                   name ,role);
             g_free(name);
             g_free(role);
             return ATSPI_SCROLLABLE(ret);
          }
        g_free(name);
        g_free(role);
        ret = atspi_accessible_get_parent(ret, &err);
        if (err)
          {
             ERROR("Unable to fetch AT-SPI parent");
             GERROR_CHECK(err)
             return NULL;
          }
     }

   return NULL;
}

static void _widget_scroll_begin(Gesture_Info *gi)
{
   GError *err = NULL;

   if (scrolled_obj)
     {
        ERROR("Scrolling context active when initializing new scrolling context! This should never happen.");
        ERROR("Force reset of current scrolling context...");
        atspi_scrollable_scroll_after_pointer(scrolled_obj, ATSPI_SCROLL_POINTER_END, gi->x_begin, gi->y_begin, &err);
        if (err)
          {
             ERROR("Failed to reset scroll context.");
             GERROR_CHECK(err)
             scrolled_obj = NULL;
          }
     }

   scrolled_obj = _find_scrollable_ancestor_at_xy(gi->x_begin, gi->y_begin);

   if (!scrolled_obj)
     {
        DEBUG("No scrollable widget found at (%d, %d) coordinates", gi->x_begin, gi->y_begin);
        return;
     }

   atspi_scrollable_scroll_after_pointer(scrolled_obj, ATSPI_SCROLL_POINTER_START, gi->x_begin, gi->y_begin, &err);
   if (err)
     {
        ERROR("Failed to initialize scroll operation");
        GERROR_CHECK(err)
        scrolled_obj = NULL;
     }
}

static void _widget_scroll_continue(Gesture_Info *gi)
{
  GError *err = NULL;
   if (!scrolled_obj)
     {
        DEBUG("Scrolling context not initialized!");
        return;
     }
   atspi_scrollable_scroll_after_pointer(scrolled_obj, ATSPI_SCROLL_POINTER_CONTINUE, gi->x_begin, gi->y_begin, &err);
   GERROR_CHECK(err)
}

static void _widget_scroll_end(Gesture_Info *gi)
{
  GError *err = NULL;
   if (!scrolled_obj)
     {
        ERROR("Scrolling context not initialized!");
        return;
     }

   atspi_scrollable_scroll_after_pointer(scrolled_obj, ATSPI_SCROLL_POINTER_END, gi->x_begin, gi->y_begin, &err);
   scrolled_obj = NULL;
   GERROR_CHECK(err)
}
*/
#if 0
// part of structural navigation
static void _goto_children_widget(void)
{
   AtspiAccessible *obj;
   if (!current_obj)
     {
        DEBUG("No current object is set. Aborting diving into children structure");
        return;
     }
   obj = structural_navi_level_down(current_obj);
   if (obj)
     _current_highlight_object_set(obj);
   else
     DEBUG("Unable to find hihglightable children widget");
}

static void _escape_children_widget(void)
{
   AtspiAccessible *obj;
   if (!current_obj)
     {
        DEBUG("No current object is set. Aborting escaping from children structure");
        return;
     }
   obj = structural_navi_level_up(current_obj);
   if (obj)
     _current_highlight_object_set(obj);
   else
     DEBUG("Unable to find hihglightable parent widget");
}
#endif
/*
static void _widget_scroll(Gesture_Info *gi)
{
   switch (gi->state)
     {
      case 0:
         _widget_scroll_begin(gi);
         break;
      case 1:
         _widget_scroll_continue(gi);
         break;
      case 2:
         _widget_scroll_end(gi);
         break;
      default:
         ERROR("Unrecognized gesture state: %d", gi->state);
     }
}
*/
static void on_gesture_detected(void *data, Gesture_Info *info)
{
   switch(info->type)
   {
      case ONE_FINGER_HOVER:
          _focus_widget(info);
          break;
      case TWO_FINGERS_HOVER:
//          _widget_scroll(info);
          break;
      case ONE_FINGER_FLICK_LEFT:
          _focus_prev();
          break;
      case ONE_FINGER_FLICK_RIGHT:
          _focus_next();
          break;
      case ONE_FINGER_FLICK_UP:
          _value_inc_widget();
          break;
      case ONE_FINGER_FLICK_DOWN:
          _value_dec_widget();
          break;
      case ONE_FINGER_SINGLE_TAP:
          _focus_widget(info);
          break;
      case ONE_FINGER_DOUBLE_TAP:
          _activate_widget();
          break;
      case THREE_FINGERS_FLICK_DOWN:
          _quickpanel_change_state(QUICKPANEL_DOWN);
          break;
      case THREE_FINGERS_FLICK_UP:
          _quickpanel_change_state(QUICKPANEL_UP);
          break;
      default:
          DEBUG("Gesture type %d not handled in switch", info->type);
   }
}

static void
_on_cache_builded(void *data)
{
   DEBUG("Cache building");
   _window_cache_builded = EINA_TRUE;
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
   _window_cache_builded = EINA_FALSE;
   if (top_window)
      object_cache_build_async(top_window, 5, _on_cache_builded, NULL);
}

static void on_window_activate(void *data, AtspiAccessible *window)
{
  gchar *name;
      ERROR("... on window activate ...");

      app_tracker_callback_unregister(top_window, APP_TRACKER_EVENT_VIEW_CHANGED, _view_content_changed, NULL);

      if(window)
      {
         app_tracker_callback_register(window, APP_TRACKER_EVENT_VIEW_CHANGED, _view_content_changed, NULL);
         name = atspi_accessible_get_name(window, NULL);
         ERROR("Window name: %s", name);
         _window_cache_builded = EINA_FALSE;
         object_cache_build_async(window, 5, _on_cache_builded, NULL);
         g_free(name);
      }
      else
      {
          ERROR("No top window found!");
//          scrolled_obj = NULL;
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
   DEBUG("START");
   // register on gesture_getected
   gesture_tracker_register(on_gesture_detected, NULL);
   // register on active_window
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
  GError *err = NULL;
   if (current_obj)
     {
       AtspiComponent *comp = atspi_accessible_get_component(current_obj);
       if (comp)
         {
           GERROR_CHECK(err);
         }
     }
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
