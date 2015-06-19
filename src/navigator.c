#include <Ecore_X.h>
#include <Ecore.h>
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
#include "screen_reader_tts.h"
#include "screen_reader_gestures.h"

#define QUICKPANEL_DOWN TRUE
#define QUICKPANEL_UP FALSE

#define DISTANCE_NB 8
#define TTS_MAX_TEXT_SIZE  2000

#define TEXT_EDIT "Double tap to edit"
#define TEXT_EDIT_FOCUSED "Editing, flick up and down to adjust position."

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

static last_focus_t gesture_start_p = {-1,-1};
static last_focus_t last_focus = {-1,-1};
static AtspiAccessible *current_obj;
static AtspiAccessible *top_window;
//static AtspiScrollable *scrolled_obj;
static Eina_Bool _window_cache_builded;
static FlatNaviContext *context;

char *
state_to_char(AtspiStateType state)
{
   switch(state)
      {
      case ATSPI_STATE_INVALID:
         return strdup("ATSPI_STATE_INVALID");
      case ATSPI_STATE_ACTIVE:
         return strdup("ATSPI_STATE_ACTIVE");
      case ATSPI_STATE_ARMED:
         return strdup("ATSPI_STATE_ARMED");
      case ATSPI_STATE_BUSY:
         return strdup("ATSPI_STATE_BUSY");
      case ATSPI_STATE_CHECKED:
         return strdup("ATSPI_STATE_CHECKED");
      case ATSPI_STATE_COLLAPSED:
         return strdup("ATSPI_STATE_COLLAPSED");
      case ATSPI_STATE_DEFUNCT:
         return strdup("ATSPI_STATE_DEFUNCT");
      case ATSPI_STATE_EDITABLE:
         return strdup("ATSPI_STATE_EDITABLE");
      case ATSPI_STATE_ENABLED:
         return strdup("ATSPI_STATE_ENABLED");
      case ATSPI_STATE_EXPANDABLE:
         return strdup("ATSPI_STATE_EXPANDABLE");
      case ATSPI_STATE_EXPANDED:
         return strdup("ATSPI_STATE_EXPANDED");
      case ATSPI_STATE_FOCUSABLE:
         return strdup("ATSPI_STATE_FOCUSABLE");
      case ATSPI_STATE_FOCUSED:
         return strdup("ATSPI_STATE_FOCUSED");
      case ATSPI_STATE_HAS_TOOLTIP:
         return strdup("ATSPI_STATE_HAS_TOOLTIP");
      case ATSPI_STATE_HORIZONTAL:
         return strdup("ATSPI_STATE_HORIZONTAL");
      case ATSPI_STATE_ICONIFIED:
         return strdup("ATSPI_STATE_ICONIFIED");
      case ATSPI_STATE_MULTI_LINE:
         return strdup("ATSPI_STATE_MULTI_LINE");
      case ATSPI_STATE_MULTISELECTABLE:
         return strdup("ATSPI_STATE_MULTISELECTABLE");
      case ATSPI_STATE_OPAQUE:
         return strdup("ATSPI_STATE_OPAQUE");
      case ATSPI_STATE_PRESSED:
         return strdup("ATSPI_STATE_PRESSED");
      case ATSPI_STATE_RESIZABLE:
         return strdup("ATSPI_STATE_RESIZABLE");
      case ATSPI_STATE_SELECTABLE:
         return strdup("ATSPI_STATE_SELECTABLE");
      case ATSPI_STATE_SELECTED:
         return strdup("ATSPI_STATE_SELECTED");
      case ATSPI_STATE_SENSITIVE:
         return strdup("ATSPI_STATE_SENSITIVE");
      case ATSPI_STATE_SHOWING:
         return strdup("ATSPI_STATE_SHOWING");
      case ATSPI_STATE_SINGLE_LINE:
         return strdup("ATSPI_STATE_SINGLE_LINE");
      case ATSPI_STATE_STALE:
         return strdup("ATSPI_STATE_STALE");
      case ATSPI_STATE_TRANSIENT:
         return strdup("ATSPI_STATE_TRANSIENT");
      case ATSPI_STATE_VERTICAL:
         return strdup("ATSPI_STATE_VERTICAL");
      case ATSPI_STATE_VISIBLE:
         return strdup("ATSPI_STATE_VISIBLE");
      case ATSPI_STATE_MANAGES_DESCENDANTS:
         return strdup("ATSPI_STATE_MANAGES_DESCENDANTS");
      case ATSPI_STATE_INDETERMINATE:
         return strdup("ATSPI_STATE_INDETERMINATE");
      case ATSPI_STATE_REQUIRED:
         return strdup("ATSPI_STATE_REQUIRED");
      case ATSPI_STATE_TRUNCATED:
         return strdup("ATSPI_STATE_TRUNCATED");
      case ATSPI_STATE_ANIMATED:
         return strdup("ATSPI_STATE_ANIMATED");
      case ATSPI_STATE_INVALID_ENTRY:
         return strdup("ATSPI_STATE_INVALID_ENTRY");
      case ATSPI_STATE_SUPPORTS_AUTOCOMPLETION:
         return strdup("ATSPI_STATE_SUPPORTS_AUTOCOMPLETION");
      case ATSPI_STATE_SELECTABLE_TEXT:
         return strdup("ATSPI_STATE_SELECTABLE_TEXT");
      case ATSPI_STATE_IS_DEFAULT:
         return strdup("ATSPI_STATE_IS_DEFAULT");
      case ATSPI_STATE_VISITED:
         return strdup("ATSPI_STATE_VISITED");
      case ATSPI_STATE_CHECKABLE:
         return strdup("ATSPI_STATE_CHECKABLE");
      case ATSPI_STATE_HAS_POPUP:
         return strdup("ATSPI_STATE_HAS_POPUP");
      case ATSPI_STATE_READ_ONLY:
         return strdup("ATSPI_STATE_READ_ONLY");
      case ATSPI_STATE_LAST_DEFINED:
         return strdup("ATSPI_STATE_LAST_DEFINED");

      default:
         return strdup("\0");
      }

}

static void
display_info_about_object(AtspiAccessible *obj)
{
   DEBUG("START");
   DEBUG("------------------------");
   const char *name = atspi_accessible_get_name(obj, NULL);
   const char *role = atspi_accessible_get_role_name(obj, NULL);
   const char *description = atspi_accessible_get_description(obj, NULL);
   char *state_name = NULL;
   AtspiStateSet *st = atspi_accessible_get_state_set (obj);
   GArray *states = atspi_state_set_get_states (st);
   AtspiComponent *comp = atspi_accessible_get_component_iface(obj);

   DEBUG("NAME:%s", name);
   DEBUG("ROLE:%s", role)
   DEBUG("DESCRIPTION:%s", description);
   DEBUG("CHILDS:%d", atspi_accessible_get_child_count(obj, NULL));
   DEBUG("HIGHLIGHT_INDEX:%d", atspi_component_get_highlight_index(comp, NULL));
   DEBUG("STATES:");
   int a;
   AtspiStateType stat;
   for (a = 0; states && (a < states->len); ++a)
      {
         stat = g_array_index (states, AtspiStateType, a);
         state_name = state_to_char(stat);
         DEBUG("   %s", state_name);
         free(state_name);
      }

   DEBUG("------------------------");
   DEBUG("END");
}

char *
generate_description_for_subtrees(AtspiAccessible *obj)
{
   DEBUG("START");
   if (!obj)
      return strdup("");
   int child_count = atspi_accessible_get_child_count(obj, NULL);

   DEBUG("There is %d children inside this filler", child_count);
   if (!child_count)
      return strdup("");

   int i;
   char *name = NULL;
   char *below = NULL;
   char ret[TTS_MAX_TEXT_SIZE] = "\0";
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
         strncat(ret, " ", sizeof(ret) - strlen(ret) - 1);
         below = generate_description_for_subtrees(child);
         DEBUG("%s from below", below);
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

char *
generate_trait(AtspiAccessible *obj)
{
   if (!obj)
      return strdup("");

   char *role = atspi_accessible_get_role_name(obj, NULL);
   char ret[TTS_MAX_TEXT_SIZE] = "\0";
   if (!strncmp(role, "entry", strlen(role)))
      {
         AtspiStateSet* state_set = atspi_accessible_get_state_set (obj);
         strncat(ret, role, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
         if (atspi_state_set_contains(state_set, ATSPI_STATE_FOCUSED))
            strncat(ret, TEXT_EDIT_FOCUSED, sizeof(ret) - strlen(ret) - 1);
         else
            strncat(ret, TEXT_EDIT, sizeof(ret) - strlen(ret) - 1);
         if (state_set) g_object_unref(state_set);
      }
   else
      {
         strncat(ret, role, sizeof(ret) - strlen(ret) - 1);
      }

   free(role);
   return strdup(ret);
}

static char *
generate_what_to_read(AtspiAccessible *obj)
{
   char *name;
   char *names = NULL;
   char *description;
   char *role_name;
   char *other;
   char ret[TTS_MAX_TEXT_SIZE] = "\0";

   description = atspi_accessible_get_description(obj, NULL);
   name = atspi_accessible_get_name(obj, NULL);
   role_name = generate_trait(obj);
   other = generate_description_for_subtrees(obj);

   DEBUG("->->->->->-> WIDGET GAINED HIGHLIGHT: %s <-<-<-<-<-<-<-", name);
   DEBUG("->->->->->-> FROM SUBTREE HAS NAME:  %s <-<-<-<-<-<-<-", other);

   display_info_about_object(obj);

   if (name && strncmp(name, "\0", 1))
      names = strdup(name);
   else if (other && strncmp(other, "\0", 1))
      names = strdup(other);

   if (names)
      {
         strncat(ret, names, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
      }

   if (role_name)
      strncat(ret, role_name, sizeof(ret) - strlen(ret) - 1);

   if (description)
      {
         if (strncmp(description, "\0", 1))
            strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
         strncat(ret, description, sizeof(ret) - strlen(ret) - 1);
      }

   free(name);
   free(names);
   free(description);
   free(role_name);
   free(other);

   return strdup(ret);

}

static void
_current_highlight_object_set(AtspiAccessible *obj)
{
   DEBUG("START");
   GError *err = NULL;
   if (!obj)
      {
         DEBUG("Clearing highlight object");
         current_obj = NULL;
         return;
      }
   if (current_obj == obj)
      {
         DEBUG("Object already highlighted");
         DEBUG("Object name:%s", atspi_accessible_get_name(obj, NULL));
         return;
      }
   if (obj && ATSPI_IS_COMPONENT(obj))
      {
         DEBUG("OBJ WITH COMPONENT");
         AtspiComponent *comp = atspi_accessible_get_component_iface(obj);
         if (!comp)
            {
               GError *err = NULL;
               gchar *role = atspi_accessible_get_role_name(obj, &err);
               ERROR("AtspiComponent *comp NULL, [%s]", role);
               GERROR_CHECK(err);
               g_free(role);
               return;
            }
         atspi_component_grab_highlight(comp, &err);
         GERROR_CHECK(err)
         gchar *name;
         gchar *role;

         current_obj = obj;
         const ObjectCache *oc = object_cache_get(obj);

         if (oc)
            {
               name = atspi_accessible_get_name(obj, &err);
               GERROR_CHECK(err)
               role = atspi_accessible_get_role_name(obj, &err);
               GERROR_CHECK(err)
               DEBUG("New highlighted object: %s, role: %s, (%d %d %d %d)",
                     name,
                     role,
                     oc->bounds->x, oc->bounds->y, oc->bounds->width, oc->bounds->height);
               haptic_vibrate_start();
            }
         else
            {
               name = atspi_accessible_get_name(obj, &err);
               GERROR_CHECK(err)
               role = atspi_accessible_get_role_name(obj, &err);
               GERROR_CHECK(err)
               DEBUG("New highlighted object: %s, role: %s",
                     name,
                     role);
               haptic_vibrate_start();
            }
         char *text_to_speak = NULL;
         text_to_speak = generate_what_to_read(obj);
         DEBUG("SPEAK:%s", text_to_speak);
         tts_speak(text_to_speak, EINA_TRUE);
         g_free(role);
         g_free(name);
         g_free(text_to_speak);
      }
   else
      DEBUG("Unable to highlight on object");
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

         object_get_wh(app, &width, &height);
         object_get_x_y(app, &xpos1, &ypos1);

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

   if ((last_focus.x == info->x_beg) && (last_focus.y == info->y_beg))
      return;

   AtspiAccessible *obj;

   if (flat_navi_context_current_at_x_y_set(context, info->x_beg, info->y_beg, &obj))
      {
         if (obj == current_obj)
            {
               DEBUG("The same object");
               return;
            }
         last_focus.x = info->x_beg;
         last_focus.y = info->y_beg;
         _current_highlight_object_set(obj);
      }
   else
      DEBUG("NO widget under (%d, %d) found or the same widget under hover",
            info->x_beg, info->y_beg);

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

static void _focus_next_visible(void)
{
   DEBUG("START");
   AtspiAccessible *obj;
   AtspiStateSet *ss = NULL;
   Eina_Bool visible = EINA_FALSE;
   if (!context)
      {
         ERROR("No navigation context created");
         return;
      }

   do
      {
         obj = flat_navi_context_next(context);
         // try next line
         if (!obj)
            obj = flat_navi_context_line_next(context);
         // try 'cycle' objects in context
         ss = atspi_accessible_get_state_set(obj);
         visible = atspi_state_set_contains(ss, ATSPI_STATE_SHOWING);
         g_object_unref(ss);
      }
   while (obj && !visible);

   if (obj)
      _current_highlight_object_set(obj);
   else
      DEBUG("Next widget not found. Abort");
   DEBUG("END");
}

static void _focus_prev_visible(void)
{
   AtspiAccessible *obj;
   AtspiStateSet *ss = NULL;
   Eina_Bool visible = EINA_FALSE;
   if (!context)
      {
         ERROR("No navigation context created");
         return;
      }
   do
      {
         obj = flat_navi_context_prev(context);
         // try next line
         if (!obj)
            obj = flat_navi_context_line_prev(context);
         // try 'cycle' objects in context
         ss = atspi_accessible_get_state_set(obj);
         visible = atspi_state_set_contains(ss, ATSPI_STATE_SHOWING);
         g_object_unref(ss);
      }
   while (obj && !visible);

   if (obj)
      _current_highlight_object_set(obj);
   else
      DEBUG("Previous widget not found. Abort");
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

static void _caret_move_forward(void)
{
   AtspiAccessible* current_widget = NULL;
   AtspiText *text_interface;
   gint current_offset;
   gboolean ret;
   int offset_pos;
   gchar *text;
   GError *err = NULL;
   if(!current_obj)
      return;

   current_widget = current_obj;

   text_interface = atspi_accessible_get_text_iface(current_widget);
   if(text_interface)
      {
         current_offset = atspi_text_get_caret_offset(text_interface, &err);
         GERROR_CHECK(err)
         ret = atspi_text_set_caret_offset(text_interface, current_offset + 1, &err);
         GERROR_CHECK(err)
         if(ret)
            {
               offset_pos = atspi_text_get_caret_offset(text_interface, NULL);
               text = atspi_text_get_text(text_interface, offset_pos, offset_pos+1, NULL);
               DEBUG("Caret position increment done");
               DEBUG("Current caret position:%d", offset_pos);
               DEBUG("SPEAK:%s", text);
               tts_speak(text, EINA_TRUE);
               g_free(text);
            }
         else
            {
               ERROR("Caret position increment error");
            }
         g_object_unref(text_interface);
      }
   else
      ERROR("No text interface supported!");
   return;

}

static void _caret_move_backward(void)
{
   AtspiAccessible* current_widget = NULL;
   AtspiText *text_interface;
   gint current_offset;
   int offset_pos;
   gchar *text;
   GError *err = NULL;
   gboolean ret;

   if(!current_obj)
      return;

   current_widget = current_obj;

   GERROR_CHECK(err)

   text_interface = atspi_accessible_get_text_iface(current_widget);
   if(text_interface)
      {
         current_offset = atspi_text_get_caret_offset(text_interface, &err);
         GERROR_CHECK(err)
         ret = atspi_text_set_caret_offset(text_interface, current_offset - 1, &err);
         GERROR_CHECK(err)
         if(ret)
            {
               offset_pos = atspi_text_get_caret_offset(text_interface, NULL);
               text = atspi_text_get_text(text_interface, offset_pos, offset_pos+1, NULL);
               DEBUG("Caret position decrement done");
               DEBUG("Current caret position:%d", offset_pos);
               DEBUG("SPEAK:%s", text);
               tts_speak(text, EINA_TRUE);
               g_free(text);
            }
         else
            {
               ERROR("Caret position decrement error");
            }
         g_object_unref(text_interface);
      }
   else
      ERROR("No text interface supported!");
   return;
}

#if 0
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
   if (!role)
      {
         ERROR("The role is null");
         return;
      }
   GERROR_CHECK(err)

   if(!strcmp(role, "entry"))
      {
         text_interface = atspi_accessible_get_text_iface(current_widget);
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
   AtspiValue *value_interface = atspi_accessible_get_value_iface(current_widget);
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
   if (!role)
      {
         ERROR("The role is null");
         return;
      }
   GERROR_CHECK(err)

   if(!strcmp(role, "entry"))
      {
         text_interface = atspi_accessible_get_text_iface(current_widget);
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

   AtspiValue *value_interface = atspi_accessible_get_value_iface(current_widget);
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
#endif

static void _activate_widget(void)
{
   //activate the widget
   //only if activate mean click
   //special behavior for entry, caret should move from first/last last/first
   DEBUG("START");
   AtspiAccessible *current_widget = NULL;
   AtspiComponent *focus_component = NULL;
   AtspiAccessible *parent = NULL;
   AtspiStateSet *ss = NULL;
   AtspiSelection *selection = NULL;
   AtspiAction *action;
   AtspiEditableText *edit = NULL;

   GError *err = NULL;
   gchar *roleName = NULL;
   gchar *actionName = NULL;
   gint number = 0;
   gint i = 0;
   gint index = 0;
   Eina_Bool activate_found = EINA_FALSE;

   if(!current_obj)
      return;

   current_widget = current_obj;

   roleName = atspi_accessible_get_role_name(current_widget, &err);
   if (!roleName)
      {
         ERROR("The role name is null");
         return;
      }
   GERROR_CHECK(err)
   DEBUG("Widget role prev: %s\n", roleName);

   display_info_about_object(current_widget);

   edit = atspi_accessible_get_editable_text_iface (current_widget);
   if (edit)
      {
         DEBUG("Activated object has editable Interface");
         focus_component = atspi_accessible_get_component_iface(current_widget);
         if (focus_component)
            {
               if (atspi_component_grab_focus(focus_component, &err) == TRUE)
                  {
                     GERROR_CHECK(err)

                     DEBUG("Entry activated\n");

                     char *text_to_speak = NULL;
                     text_to_speak = generate_what_to_read(current_widget);

                     DEBUG("SPEAK:%s", text_to_speak);

                     tts_speak(text_to_speak, EINA_TRUE);
                     g_free(text_to_speak);
                     g_object_unref(focus_component);
                  }
            }
         g_object_unref(edit);
         return;
      }

   action = atspi_accessible_get_action_iface(current_widget);
   if (action)
      {
         number = atspi_action_get_n_actions(action, &err);
         DEBUG("Number of available action = %d\n", number);
         GERROR_CHECK(err)
         activate_found = EINA_FALSE;
         while (i < number && !activate_found)
            {
               actionName = atspi_action_get_name(action, i, &err);
               if (actionName && !strcmp("activate", actionName))
                  {
                     DEBUG("There is activate action");
                     activate_found = EINA_TRUE;
                  }
               else
                  {
                     i++;
                  }
               g_free(actionName);
            }
         if (activate_found)
            {
               DEBUG("PERFORMING ATSPI ACTION NO.%d", i);
               atspi_action_do_action(action, i, &err);
            }
         else if (number > 0)
            {
               DEBUG("PERFORMING ATSPI DEFAULT ACTION");
               atspi_action_do_action(action, 0, &err);
            }
         else
            ERROR("There is no actions inside Action interface");
         if (action) g_object_unref(action);
         GERROR_CHECK(err)
         return;
      }

   ss = atspi_accessible_get_state_set(current_widget);
   if (atspi_state_set_contains(ss, ATSPI_STATE_SELECTABLE) == EINA_TRUE)
      {
         DEBUG("OBJECT IS SELECTABLE");
         parent = atspi_accessible_get_parent(current_widget, NULL);
         if (parent)
            {
               index = atspi_accessible_get_index_in_parent(current_widget, NULL);
               selection = atspi_accessible_get_selection_iface (parent);
               if (selection)
                  {
                     DEBUG("SELECT CHILD NO:%d\n", index);
                     atspi_selection_select_child(selection, index, NULL);
                     g_object_unref(selection);
                     g_object_unref(parent);
                     g_object_unref(ss);
                     return;
                  }
               else
                  ERROR("no selection iterface in parent");
            }
      }
   g_object_unref(ss);

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

static void _widget_scroll(Gesture_Info *gi)
{
   DEBUG("Recognized gesture state: %d", gi->state);

   if (gi->state == 0)
      {
         DEBUG("save coordinates %d %d", gesture_start_p.x, gesture_start_p.y);
         gesture_start_p.x = gi->x_beg;
         gesture_start_p.y = gi->y_beg;
      }

   if (gi->state != 2)
      {
         DEBUG("Scroll not finished yet");
         return;
      }

   AtspiAccessible *obj = NULL;
   obj = flat_navi_context_current_get(context);
   if (!obj)
      {
         ERROR("No context");
         return;
      }

   AtspiStateSet *ss = atspi_accessible_get_state_set(obj);
   if (!ss)
      {
         ERROR("no stetes");
         return;
      }

   if (!atspi_state_set_contains(ss, ATSPI_STATE_SHOWING))
      {
         DEBUG("current context do not have visible state, swith to next/prev");
         if (gesture_start_p.y > gi->y_end ||
               gesture_start_p.x > gi->x_end)
            {
               DEBUG("NEXT");
               _focus_next_visible();
            }
         else if (gesture_start_p.y < gi->y_end ||
                  gesture_start_p.x < gi->x_end)
            {
               DEBUG("PREVIOUS");
               _focus_prev_visible();
            }
      }
   DEBUG("end");
   g_object_unref(ss);
   g_object_unref(obj);
}

static void _read_quickpanel(void )
{
   DEBUG("START");



   DEBUG("END");
}

static void
_direct_scroll_back(void)
{
   DEBUG("ONE_FINGER_FLICK_LEFT_RETURN");
   if (!context)
      {
         ERROR("No navigation context created");
         return;
      }

   int visi = 0;
   AtspiAccessible *obj = NULL;
   AtspiAccessible *current = NULL;
   AtspiAccessible *parent = NULL;
   gchar *role = NULL;

   current = flat_navi_context_current_get(context);
   parent = atspi_accessible_get_parent (current, NULL);
   role = atspi_accessible_get_role_name(parent, NULL);

   if (strcmp(role, "list"))
      {
         DEBUG("That operation can be done only on list, it is:%s", role);
         g_free(role);
         return;
      }

   g_free(role);
   visi = flat_navi_context_current_children_count_visible_get(context);
   DEBUG("There is %d elements visible", visi);

   int index = atspi_accessible_get_index_in_parent(current, NULL);
   int children_count = atspi_accessible_get_child_count(parent, NULL);

   if (visi <=0 || children_count <=0)
      {
         ERROR("NO visible element on list");
         return;
      }

   DEBUG("start from element with index:%d/%d", index, children_count);

   int target = index - visi;

   if (target <=0)
      {
         DEBUG("first element");
         obj = atspi_accessible_get_child_at_index (parent, 0, NULL);
         smart_notification(FOCUS_CHAIN_END_NOTIFICATION_EVENT, 0, 0);
      }

   else
      {
         DEBUG("go back to %d element", target);
         obj = atspi_accessible_get_child_at_index (parent, target, NULL);
      }


   if (obj)
      {
         DEBUG("Will set highlight and context");
         if (flat_navi_context_current_set(context, obj))
            {
               DEBUG("current obj set");
            }
         _current_highlight_object_set(obj);
      }
}

static void
_direct_scroll_forward(void)
{
   DEBUG("ONE_FINGER_FLICK_RIGHT_RETURN");

   if (!context)
      {
         ERROR("No navigation context created");
         return;
      }

   int visi = 0;
   AtspiAccessible *obj = NULL;
   AtspiAccessible *current = NULL;
   AtspiAccessible *parent = NULL;
   gchar *role = NULL;

   current = flat_navi_context_current_get(context);
   parent = atspi_accessible_get_parent (current, NULL);
   role = atspi_accessible_get_role_name(parent, NULL);

   if (strcmp(role, "list"))
      {
         DEBUG("That operation can be done only on list, it is:%s", role);
         g_free(role);
         return;
      }

   g_free(role);
   visi = flat_navi_context_current_children_count_visible_get(context);
   DEBUG("There is %d elements visible", visi);

   int index = atspi_accessible_get_index_in_parent(current, NULL);
   int children_count = atspi_accessible_get_child_count(parent, NULL);

   if (visi <=0 || children_count <=0)
      {
         ERROR("NO visible element on list");
         return;
      }

   DEBUG("start from element with index:%d/%d", index, children_count);

   int target = index + visi;

   if (target >= children_count)
      {
         DEBUG("last element");
         obj = atspi_accessible_get_child_at_index (parent, children_count-1, NULL);
         smart_notification(FOCUS_CHAIN_END_NOTIFICATION_EVENT, 0, 0);
      }

   else
      {
         DEBUG("go back to %d element", target);
         obj = atspi_accessible_get_child_at_index (parent, target, NULL);
      }


   if (obj)
      {
         DEBUG("Will set highlight and context");
         if (flat_navi_context_current_set(context, obj))
            {
               DEBUG("current obj set");
            }
         _current_highlight_object_set(obj);
      }
}

static void
_direct_scroll_to_first(void)
{
   AtspiAccessible *obj = NULL;
   DEBUG("ONE_FINGER_FLICK_UP_RETURN");
   flat_navi_context_line_first(context);
   obj = flat_navi_context_first(context);
   if (flat_navi_context_current_set(context, obj))
      _current_highlight_object_set(obj);
}

static void
_direct_scroll_to_last(void)
{
   DEBUG("ONE_FINGER_FLICK_DOWN_RETURN");
   AtspiAccessible *obj = NULL;
   flat_navi_context_line_last(context);
   obj = flat_navi_context_last(context);
   if (flat_navi_context_current_set(context, obj))
      _current_highlight_object_set(obj);
}

static Eina_Bool
_is_active_entry(void)
{
   DEBUG("START");

   if (!context)
      {
         ERROR("No navigation context created");
         return EINA_FALSE;
      }
   AtspiAccessible *obj = NULL;
   obj = flat_navi_context_current_get(context);

   if (!obj)
      return EINA_FALSE;

   char *role = atspi_accessible_get_role_name(obj, NULL);
   if (!strncmp(role, "entry", strlen(role)))
      {
         AtspiStateSet* state_set = atspi_accessible_get_state_set (obj);
         if (atspi_state_set_contains(state_set, ATSPI_STATE_FOCUSED))
            {
               g_object_unref(state_set);
               g_free(role);
               return EINA_TRUE;
            }
         g_object_unref(state_set);
         g_free(role);
         return EINA_FALSE;
      }
   g_free(role);
   return EINA_FALSE;

   DEBUG("END");
}

static void on_gesture_detected(void *data, Gesture_Info *info)
{
   switch(info->type)
      {
      case ONE_FINGER_HOVER:
         _focus_widget(info);
         break;
      case TWO_FINGERS_HOVER:
         _widget_scroll(info);
         break;
      case ONE_FINGER_FLICK_LEFT:
         _focus_prev();
         break;
      case ONE_FINGER_FLICK_RIGHT:
         _focus_next();
         break;
      case ONE_FINGER_FLICK_UP:
         if (_is_active_entry())
            _caret_move_backward();
         else
            _focus_prev();
         break;
      case ONE_FINGER_FLICK_DOWN:
         if (_is_active_entry())
            _caret_move_forward();
         else
            _focus_next();
         break;
      case ONE_FINGER_SINGLE_TAP:
         _focus_widget(info);
         break;
      case ONE_FINGER_DOUBLE_TAP:
         _activate_widget();
         break;
      case THREE_FINGERS_SINGLE_TAP:
         _read_quickpanel();
         break;
      case THREE_FINGERS_FLICK_DOWN:
         _quickpanel_change_state(QUICKPANEL_DOWN);
         break;
      case THREE_FINGERS_FLICK_UP:
         _quickpanel_change_state(QUICKPANEL_UP);
         break;
      case ONE_FINGER_FLICK_LEFT_RETURN:
         _direct_scroll_back();
         break;
      case ONE_FINGER_FLICK_RIGHT_RETURN:
         _direct_scroll_forward();
         break;
      case ONE_FINGER_FLICK_UP_RETURN:
         _direct_scroll_to_first();
         break;
      case ONE_FINGER_FLICK_DOWN_RETURN:
         _direct_scroll_to_last();
         break;
      default:
         DEBUG("Gesture type %d not handled in switch", info->type);
      }
}

static void
_on_cache_builded(void *data)
{
   DEBUG("START");
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
   DEBUG("END");
}

static void
_view_content_changed(AppTrackerEventType type, void *user_data)
{
   DEBUG("START");
   _window_cache_builded = EINA_FALSE;
   if (top_window)
      object_cache_build_async(top_window, 5, _on_cache_builded, NULL);
   DEBUG("END");
}

static void on_window_activate(void *data, AtspiAccessible *window)
{
   DEBUG("START");

   app_tracker_callback_unregister(top_window, APP_TRACKER_EVENT_VIEW_CHANGED, _view_content_changed, NULL);

   if(window)
      {
         DEBUG("Window name: %s", atspi_accessible_get_name(window, NULL));
         app_tracker_callback_register(window, APP_TRACKER_EVENT_VIEW_CHANGED, _view_content_changed, NULL);
         _window_cache_builded = EINA_FALSE;
         object_cache_build_async(window, 5, _on_cache_builded, NULL);
      }
   else
      {
         ERROR("No top window found!");
//     scrolled_obj = NULL;
      }
   top_window = window;
   DEBUG("END");
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
   screen_reader_gestures_tracker_register(on_gesture_detected, NULL);
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
         AtspiComponent *comp = atspi_accessible_get_component_iface(current_obj);
         if (comp)
            {
               atspi_component_clear_highlight(comp, &err);
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
