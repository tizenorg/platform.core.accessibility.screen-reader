#include <Ecore_X.h>
#include <Ecore.h>
#include <math.h>
#include <atspi/atspi.h>
#include "logger.h"
#include "navigator.h"
#include "window_tracker.h"
#include "keyboard_tracker.h"
#include "pivot_chooser.h"
#include "flat_navi.h"
#include "app_tracker.h"
#include "smart_notification.h"
#include "screen_reader_system.h"
#include "screen_reader_haptic.h"
#include "screen_reader_tts.h"
#include "screen_reader_gestures.h"
#include "dbus_gesture_adapter.h"

#define QUICKPANEL_DOWN TRUE
#define QUICKPANEL_UP FALSE

#define DISTANCE_NB 8
#define MENU_ITEM_TAB_INDEX_SIZE 16
#define HOVERSEL_TRAIT_SIZE 200
#define TTS_MAX_TEXT_SIZE  2000
#define GESTURE_LIMIT 10


//Timeout in ms which will be used as interval for handling ongoing
//hoved gesture updates. It is introduced to improve performance.
//Even if user makes many mouse move events within hover gesture
//only 5 highlight updates a second will be performed. Else we will
//highly pollute dbus bus and and decrease highlight performance.
#define ONGOING_HOVER_GESTURE_INTERPRETATION_INTERVAL 200

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
static AtspiComponent *current_comp = NULL;
static AtspiAccessible *top_window;
static FlatNaviContext *context;
static bool prepared = false;
static int counter=0;
int _last_hover_event_time = -1;

static struct
{
   AtspiAccessible *focused_object;
   bool auto_review_on;
} s_auto_review =
{
   .focused_object = NULL,
   .auto_review_on = false
};

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
      case ATSPI_STATE_MODAL:
         return strdup("ATSPI_STATE_MODAL");
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
   const char *role = atspi_accessible_get_localized_role_name(obj, NULL);
   const char *description = atspi_accessible_get_description(obj, NULL);
   char *state_name = NULL;
   AtspiStateSet *st = atspi_accessible_get_state_set (obj);
   GArray *states = atspi_state_set_get_states (st);
   AtspiComponent *comp = atspi_accessible_get_component_iface(obj);
   AtspiValue *value = atspi_accessible_get_value_iface(obj);
   AtspiRect *rect_screen = atspi_component_get_extents(comp, ATSPI_COORD_TYPE_SCREEN, NULL);
   AtspiRect *rect_win = atspi_component_get_extents(comp, ATSPI_COORD_TYPE_WINDOW, NULL);

   DEBUG("NAME:%s", name);
   DEBUG("ROLE:%s", role)
   DEBUG("DESCRIPTION:%s", description);
   DEBUG("CHILDS:%d", atspi_accessible_get_child_count(obj, NULL));
   DEBUG("HIGHLIGHT_INDEX:%d", atspi_component_get_highlight_index(comp, NULL));
   DEBUG("INDEX IN PARENT:%d", atspi_accessible_get_index_in_parent(obj, NULL));
   if (value)
      {
         DEBUG("VALUE:%f", atspi_value_get_current_value (value, NULL));
         DEBUG("VALUE MAX:%f", atspi_value_get_maximum_value (value, NULL));
         DEBUG("VALUE MIN:%f", atspi_value_get_minimum_value (value, NULL));
      }
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
   g_array_free(states, 0);
   DEBUG("LOCALE:%s", atspi_accessible_get_object_locale(obj, NULL));
   DEBUG("SIZE ON SCREEN, width:%d, height:%d",rect_screen->width, rect_screen->height);
   DEBUG("POSITION ON SCREEN: x:%d y:%d", rect_screen->x, rect_screen->y);
   DEBUG("SIZE ON WIN, width:%d, height:%d",rect_win->width, rect_win->height);
   DEBUG("POSITION ON WIN: x:%d y:%d", rect_win->x, rect_win->y);
   DEBUG("------------------------");
   DEBUG("END");
}

char *
generate_description_for_subtrees(AtspiAccessible *obj)
{
   DEBUG("START");

   if (!obj)
      return strdup("");
   return strdup("");
   /*
   AtspiRole role;
   int child_count;
   int i;
   char *name = NULL;
   char *below = NULL;
   char ret[TTS_MAX_TEXT_SIZE] = "\0";
   AtspiAccessible *child = NULL;

   int child_count = atspi_accessible_get_child_count(obj, NULL);

   role = atspi_accessible_get_role(obj, NULL);

   // Do not generate that for popups
   if (role == ATSPI_ROLE_POPUP_MENU || role == ATSPI_ROLE_DIALOG)
      return strdup("");

   child_count = atspi_accessible_get_child_count(obj, NULL);

   DEBUG("There is %d children inside this object", child_count);
   if (!child_count)
      return strdup("");

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
   */
}

static int
_check_list_children_count(AtspiAccessible *obj)
{
   int list_count = 0;
   int i;
   AtspiAccessible *child = NULL;

   if (!obj)
      return 0;

   if (atspi_accessible_get_role(obj, NULL) == ATSPI_ROLE_LIST)
      {
         int children_count = atspi_accessible_get_child_count(obj, NULL);

         for (i=0; i<children_count; i++)
            {
               child = atspi_accessible_get_child_at_index(obj, i, NULL);
               if (atspi_accessible_get_role(child, NULL) == ATSPI_ROLE_LIST_ITEM)
                  list_count++;
               g_object_unref(child);
            }
      }

   return list_count;
}

static int
_find_popup_list_children_count(AtspiAccessible *obj)
{
   int list_items_count = 0;
   int children_count = atspi_accessible_get_child_count(obj, NULL);
   int i;
   AtspiAccessible *child = NULL;

   list_items_count = _check_list_children_count(obj);
   if (list_items_count > 0)
      return list_items_count;

   for (i=0; i<children_count; i++)
      {
         child = atspi_accessible_get_child_at_index(obj, i, NULL);
         list_items_count = _find_popup_list_children_count(child);
         if (list_items_count > 0)
            return list_items_count;
         g_object_unref(child);
      }

   return 0;
}

char *
generate_trait(AtspiAccessible *obj)
{
   if (!obj)
      return strdup("");

   AtspiRole role = atspi_accessible_get_role(obj, NULL);
   AtspiStateSet* state_set = atspi_accessible_get_state_set(obj);
   char ret[TTS_MAX_TEXT_SIZE] = "\0";
   if (role == ATSPI_ROLE_ENTRY)
      {
         char *role_name = atspi_accessible_get_localized_role_name(obj, NULL);
         strncat(ret, role_name, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
         if (atspi_state_set_contains(state_set, ATSPI_STATE_FOCUSED))
            strncat(ret, _("IDS_TRAIT_TEXT_EDIT_FOCUSED"), sizeof(ret) - strlen(ret) - 1);
         else
            strncat(ret, _("IDS_TRAIT_TEXT_EDIT"), sizeof(ret) - strlen(ret) - 1);
         free(role_name);
      }
   else if (role == ATSPI_ROLE_MENU_ITEM)
      {
         AtspiAccessible *parent = atspi_accessible_get_parent(obj, NULL);
         int children_count = atspi_accessible_get_child_count(parent, NULL);
         int index = atspi_accessible_get_index_in_parent(obj, NULL);
         char tab_index[MENU_ITEM_TAB_INDEX_SIZE];
         snprintf(tab_index, MENU_ITEM_TAB_INDEX_SIZE, _("IDS_TRAIT_MENU_ITEM_TAB_INDEX"), index+1, children_count);
         strncat(ret, tab_index, sizeof(ret) - strlen(ret) - 1);
      }
   else if (role == ATSPI_ROLE_POPUP_MENU)
      {
         int children_count = atspi_accessible_get_child_count(obj, NULL);
         char trait[HOVERSEL_TRAIT_SIZE];

         snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_CTX_POPUP"));
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);

         snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_SHOWING"));
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, " ", sizeof(ret) - strlen(ret) - 1);

         snprintf(trait, HOVERSEL_TRAIT_SIZE, "%d", children_count);
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, " ", sizeof(ret) - strlen(ret) - 1);

         snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_ITEMS"));
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);

         snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_POPUP_CLOSE"));
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
      }
   else if (role == ATSPI_ROLE_DIALOG)
      {
         int children_count = _find_popup_list_children_count(obj);
         char trait[HOVERSEL_TRAIT_SIZE];

         snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_POPUP"));
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);

         if(children_count > 0)
            {
               snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_SHOWING"));
               strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
               strncat(ret, " ", sizeof(ret) - strlen(ret) - 1);

               snprintf(trait, HOVERSEL_TRAIT_SIZE, "%d", children_count);
               strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
               strncat(ret, " ", sizeof(ret) - strlen(ret) - 1);

               snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_ITEMS"));
               strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
               strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
            }

         snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_POPUP_CLOSE"));
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
      }
   else if (role == ATSPI_ROLE_GLASS_PANE)
      {
         AtspiAccessible *parent = atspi_accessible_get_parent(obj, NULL);
         int children_count = atspi_accessible_get_child_count(parent, NULL);
         char trait[HOVERSEL_TRAIT_SIZE];
         snprintf(trait, HOVERSEL_TRAIT_SIZE, _("IDS_TRAIT_PD_HOVERSEL"), children_count);
         strncat(ret, trait, sizeof(ret) - strlen(ret) - 1);
      }
   else if (role == ATSPI_ROLE_LIST_ITEM && atspi_state_set_contains(state_set, ATSPI_STATE_EXPANDABLE))
      {
         strncat(ret, _("IDS_TRAIT_GROUP_INDEX"), sizeof(ret) - strlen(ret) - 1);
         strncat(ret, ", ", sizeof(ret) - strlen(ret) - 1);
         if (atspi_state_set_contains(state_set, ATSPI_STATE_EXPANDED))
            strncat(ret, _("IDS_TRAIT_GROUP_INDEX_EXPANDED"), sizeof(ret) - strlen(ret) - 1);
         else
            strncat(ret, _("IDS_TRAIT_GROUP_INDEX_COLLAPSED"), sizeof(ret) - strlen(ret) - 1);
      }
   else
      {
         char *role_name = atspi_accessible_get_localized_role_name(obj, NULL);
         strncat(ret, role_name, sizeof(ret) - strlen(ret) - 1);
         free(role_name);
      }

   if (state_set) g_object_unref(state_set);

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
   gchar *role = NULL;

   if (!obj)
      {
         DEBUG("Clearing highlight object");
         current_obj = NULL;
         if (current_comp)
            {
               atspi_component_clear_highlight(current_comp, &err);
               g_object_ref(current_comp);
               current_comp = NULL;
            }

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
               role = atspi_accessible_get_role_name(obj, &err);
               ERROR("AtspiComponent *comp NULL, [%s]", role);
               GERROR_CHECK(err);
               g_free(role);
               return;
            }
         if (current_comp)
            {
               atspi_component_clear_highlight(current_comp, &err);
            }
         atspi_component_grab_highlight(comp, &err);
         current_comp = comp;
         GERROR_CHECK(err)

         Eina_Bool is_paused = tts_pause_get();
         if(is_paused)
            {
               tts_stop_set();
               tts_pause_set(EINA_FALSE);
            }
         current_obj = obj;
         char *text_to_speak = NULL;
         text_to_speak = generate_what_to_read(obj);
         DEBUG("SPEAK:%s", text_to_speak);

         tts_speak(text_to_speak, EINA_TRUE);
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

static void _focus_widget(Gesture_Info *info)
{
   DEBUG("START");
   if (info->type == ONE_FINGER_HOVER)
      {
         if (_last_hover_event_time < 0)
            _last_hover_event_time = info->event_time;
         //info->event_time and _last_hover_event_time contain timestamp in ms.
         if (info->event_time - _last_hover_event_time < ONGOING_HOVER_GESTURE_INTERPRETATION_INTERVAL && info->state == 1)
            return;

         _last_hover_event_time = info->state != 1 ? -1 : info->event_time;
      }

   if ((last_focus.x == info->x_beg) && (last_focus.y == info->y_beg))
      return;

   AtspiAccessible *obj = NULL;
   if (flat_navi_context_current_at_x_y_set(context, info->x_beg, info->y_beg, &obj))
      {
         last_focus.x = info->x_beg;
         last_focus.y = info->y_beg;
         _current_highlight_object_set(obj);
      }

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
         // try 'cycle' objects in context
         if (obj)
            {
               ss = atspi_accessible_get_state_set(obj);
               visible = atspi_state_set_contains(ss, ATSPI_STATE_SHOWING);
               g_object_unref(ss);
            }
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
         // try 'cycle' objects in context
         if (obj)
            {
               ss = atspi_accessible_get_state_set(obj);
               visible = atspi_state_set_contains(ss, ATSPI_STATE_SHOWING);
               g_object_unref(ss);
            }
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
   if (obj)
      _current_highlight_object_set(obj);
   else
      DEBUG("Previous widget not found. Abort");
}

static void _caret_move_beg(void)
{
   AtspiAccessible* current_widget = NULL;
   AtspiText *text_interface;
   gboolean ret;
   GError *err = NULL;

   if(!current_obj)
      return;

   current_widget = current_obj;

   text_interface = atspi_accessible_get_text_iface(current_widget);
   if(text_interface)
      {
         ret = atspi_text_set_caret_offset(text_interface, 0, &err);
         GERROR_CHECK(err)
         if(ret)
            {
               DEBUG("Caret position increment done");
               gchar *text = atspi_text_get_text(text_interface, 0, 1, NULL);
               DEBUG("SPEAK:%s", text);
               tts_speak(text, EINA_TRUE);
               tts_speak(_("IDS_TEXT_BEGIN"), EINA_FALSE);
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
}

static void _caret_move_end(void)
{
   AtspiAccessible* current_widget = NULL;
   AtspiText *text_interface;
   gboolean ret;
   GError *err = NULL;

   if(!current_obj)
      return;

   current_widget = current_obj;

   text_interface = atspi_accessible_get_text_iface(current_widget);
   if(text_interface)
      {
         int len = atspi_text_get_character_count (text_interface, NULL);
         ret = atspi_text_set_caret_offset(text_interface, len, &err);
         if (ret)
            {
               DEBUG("Caret position increment done");
               DEBUG("SPEAK:%s", _("IDS_TEXT_END"));
               tts_speak(_("IDS_TEXT_END"), EINA_TRUE);
            }
         else
            ERROR("Caret position to end error");
         g_object_unref(text_interface);
      }
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
               DEBUG("Current caret offset:%d", current_offset);
               if (offset_pos == atspi_text_get_character_count (text_interface, NULL))
                  {
                     DEBUG("SPEAK:%s", _("IDS_TEXT_END"));
                     tts_speak(_("IDS_TEXT_END"), EINA_FALSE);
                  }
               else
                  {
                     DEBUG("SPEAK:%s", text);
                     tts_speak(text, EINA_TRUE);
                  }
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
               if (offset_pos == 0)
                  {
                     DEBUG("SPEAK:%s", _("IDS_TEXT_BEGIN"));
                     tts_speak(_("IDS_TEXT_BEGIN"), EINA_FALSE);
                  }
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

static void _read_value( AtspiValue *value)
{
   if (!value)
      return;

   gdouble current_val = atspi_value_get_current_value(value, NULL);
   gdouble max_val = atspi_value_get_maximum_value (value, NULL);
   gdouble min_val = atspi_value_get_minimum_value (value, NULL);

   int proc = (current_val/fabs(max_val-min_val)) * 100;

   char buf[256] = "\0";
   snprintf(buf, sizeof(buf), "%d percent", proc);
   DEBUG("has value %s", buf);
   tts_speak(strdup(buf), EINA_TRUE);
}

static void _value_inc(void)
{
   AtspiAccessible* current_widget = NULL;
   GError *err = NULL;

   if(!current_obj)
      return;

   current_widget = current_obj;

   AtspiValue *value_interface = atspi_accessible_get_value_iface(current_widget);
   if(value_interface)
      {
         DEBUG("Value interface supported!\n");
         gdouble current_val = atspi_value_get_current_value(value_interface, &err);
         GERROR_CHECK(err)
         DEBUG("Current value: %f\n ", (double)current_val);
         gdouble minimum_inc = atspi_value_get_minimum_increment(value_interface, &err);
         DEBUG("Minimum increment: %f\n ", (double)minimum_inc);
         GERROR_CHECK(err)
         atspi_value_set_current_value(value_interface, current_val + minimum_inc, &err);
         GERROR_CHECK(err)
         _read_value(value_interface);
         g_object_unref(value_interface);
         return;
      }
   ERROR("No value interface supported!\n");
}

static void _value_dec(void)
{
   AtspiAccessible* current_widget = NULL;
   GError *err = NULL;

   if(!current_obj)
      return;
   current_widget = current_obj;

   AtspiValue *value_interface = atspi_accessible_get_value_iface(current_widget);
   if(value_interface)
      {
         DEBUG("Value interface supported!\n");
         gdouble current_val = atspi_value_get_current_value(value_interface, &err);
         GERROR_CHECK(err)
         DEBUG("Current value: %f\n ", (double)current_val);
         gdouble minimum_inc = atspi_value_get_minimum_increment(value_interface, &err);
         GERROR_CHECK(err)
         DEBUG("Minimum increment: %f\n ", (double)minimum_inc);
         atspi_value_set_current_value(value_interface, current_val - minimum_inc, &err);
         GERROR_CHECK(err)
         _read_value(value_interface);
         g_object_unref(value_interface);
         return;
      }
   ERROR("No value interface supported!\n");
}

static bool
_check_if_widget_is_enabled(AtspiAccessible *obj)
{
   Eina_Bool ret = EINA_FALSE;
   AtspiStateSet *st = atspi_accessible_get_state_set (obj);

   if (atspi_state_set_contains(st, ATSPI_STATE_ENABLED))
      ret = EINA_TRUE;

   g_object_unref(st);
   return ret;
}

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
   gchar *actionName = NULL;
   gint number = 0;
   gint i = 0;
   gint index = 0;
   Eina_Bool activate_found = EINA_FALSE;
   AtspiRole role = ATSPI_ROLE_INVALID;

   if(!current_obj)
      return;

   if (!_check_if_widget_is_enabled(current_obj))
      {
         DEBUG("Widget disable so cannot be activated");
         return;
      }

   current_widget = current_obj;

   role = atspi_accessible_get_role (current_widget, NULL);
   if(role == ATSPI_ROLE_SLIDER)
      {
         return;
      }

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

   device_time_get();
   device_battery_get();
   device_bluetooth_get();
   device_signal_strenght_get();

   device_date_get();
   device_missed_events_get();
   DEBUG("END");
}

static void _set_pause(void )
{
   DEBUG("START");

   Eina_Bool res = EINA_FALSE;
   bool pause = tts_pause_get();
   res = tts_pause_set(!pause);
   if(!res)
      {
         ERROR("Failed to set pause state");
      }

   DEBUG("END");
}

void auto_review_highlight_set(void)
{
   AtspiAccessible *obj = flat_navi_context_next(context);

   DEBUG("START");

   if(!obj)
      {
         s_auto_review.auto_review_on = false;
         return;
      }

   _current_highlight_object_set(obj);

   DEBUG("END");
}

void auto_review_highlight_top(void)
{
   DEBUG("START");
   char *text_to_speak = NULL;
   AtspiAccessible *first = flat_navi_context_first(context);
   AtspiAccessible *obj = flat_navi_context_current_get(context);

   if(first != obj)
      {
         _current_highlight_object_set(first);
      }
   else
      {
         text_to_speak = generate_what_to_read(obj);
         DEBUG("Text to speak: %s", text_to_speak);
         tts_speak(text_to_speak, EINA_TRUE);
         free(text_to_speak);
      }

   DEBUG("END");
}


static void _on_auto_review_stop(void)
{
   DEBUG("START");
   s_auto_review.auto_review_on = false;
   DEBUG("END");
}

static void _on_utterance(void)
{
   DEBUG("START");
   DEBUG("s_auto_review.auto_review_on == %d", s_auto_review.auto_review_on);

   if(s_auto_review.auto_review_on)
      {
         auto_review_highlight_set();
      }
   DEBUG("END");
}


static void _review_from_current(void)
{
   DEBUG("START");

   s_auto_review.focused_object = flat_navi_context_current_get(context);
   s_auto_review.auto_review_on = true;
   auto_review_highlight_set();

   DEBUG("END");
}

static void _review_from_top()
{
   DEBUG("START");

   s_auto_review.focused_object = flat_navi_context_current_get(context);
   s_auto_review.auto_review_on = true;
   auto_review_highlight_top();

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

   AtspiAccessible *obj = NULL;
   AtspiAccessible *current = NULL;
   AtspiAccessible *parent = NULL;
   AtspiRole role;

   current = flat_navi_context_current_get(context);
   parent = atspi_accessible_get_parent (current, NULL);
   role = atspi_accessible_get_role(parent, NULL);

   if (role != ATSPI_ROLE_LIST)
      {
         DEBUG("That operation can be done only on list, it is:%s", atspi_accessible_get_role_name(parent, NULL));
         return;
      }

   int index = atspi_accessible_get_index_in_parent(current, NULL);
   int children_count = atspi_accessible_get_child_count(parent, NULL);

   if (children_count <=0)
      {
         ERROR("NO visible element on list");
         return;
      }

   DEBUG("start from element with index:%d/%d", index, children_count);

   if (index <=0)
      {
         DEBUG("first element");
         obj = atspi_accessible_get_child_at_index (parent, 0, NULL);
         smart_notification(FOCUS_CHAIN_END_NOTIFICATION_EVENT, 0, 0);
      }

   else
      {
         DEBUG("go back to %d element", index);
         obj = atspi_accessible_get_child_at_index (parent, index, NULL);
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

   AtspiAccessible *obj = NULL;
   AtspiAccessible *current = NULL;
   AtspiAccessible *parent = NULL;
   AtspiRole role;

   current = flat_navi_context_current_get(context);
   parent = atspi_accessible_get_parent (current, NULL);
   role = atspi_accessible_get_role(parent, NULL);

   if (role != ATSPI_ROLE_LIST)
      {
         DEBUG("That operation can be done only on list, it is:%s", atspi_accessible_get_role_name(parent, NULL));
         return;
      }

   int index = atspi_accessible_get_index_in_parent(current, NULL);
   int children_count = atspi_accessible_get_child_count(parent, NULL);

   if (children_count <=0)
      {
         ERROR("NO visible element on list");
         return;
      }

   DEBUG("start from element with index:%d/%d", index, children_count);

   if (index >= children_count)
      {
         DEBUG("last element");
         obj = atspi_accessible_get_child_at_index (parent, children_count-1, NULL);
         smart_notification(FOCUS_CHAIN_END_NOTIFICATION_EVENT, 0, 0);
      }

   else
      {
         DEBUG("go back to %d element", index);
         obj = atspi_accessible_get_child_at_index (parent, index, NULL);
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
   DEBUG("ONE_FINGER_FLICK_UP_RETURN");
   if (!context)
      {
         ERROR("No navigation context created");
         return;
      }
   AtspiAccessible *obj = flat_navi_context_first(context);
   if (obj)
      _current_highlight_object_set(obj);
   else
      DEBUG("First widget not found. Abort");
   DEBUG("END");
}

static void
_direct_scroll_to_last(void)
{
   DEBUG("ONE_FINGER_FLICK_DOWN_RETURN");
   if (!context)
      {
         ERROR("No navigation context created");
         return;
      }
   AtspiAccessible *obj = flat_navi_context_last(context);
   if (obj)
      _current_highlight_object_set(obj);
   else
      DEBUG("Last widget not found. Abort");
   DEBUG("END");
}

static Eina_Bool
_has_value(void)
{
   DEBUG("START");
   AtspiAccessible *obj = NULL;

   if(!current_obj)
      return EINA_FALSE;

   obj = current_obj;

   if (!obj)
      return EINA_FALSE;

   AtspiValue *value = atspi_accessible_get_value_iface(obj);

   if (value)
      {
         g_object_unref(value);
         return EINA_TRUE;
      }

   return EINA_FALSE;
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
   AtspiRole role;
   obj = flat_navi_context_current_get(context);

   if (!obj)
      return EINA_FALSE;

   role = atspi_accessible_get_role(obj, NULL);
   if (role == ATSPI_ROLE_ENTRY)
      {
         AtspiStateSet* state_set = atspi_accessible_get_state_set (obj);
         if (atspi_state_set_contains(state_set, ATSPI_STATE_FOCUSED))
            {
               g_object_unref(state_set);
               return EINA_TRUE;
            }
         g_object_unref(state_set);
         return EINA_FALSE;
      }
   return EINA_FALSE;

   DEBUG("END");
}

static Eina_Bool
_is_slider(AtspiAccessible *obj)
{
   DEBUG("START");

   if (!obj)
      return EINA_FALSE;

   AtspiRole role;

   role = atspi_accessible_get_role(obj, NULL);
   if (role == ATSPI_ROLE_SLIDER)
      {
         return EINA_TRUE;
      }
   return EINA_FALSE;
}

static void
_move_slider(Gesture_Info *gi)
{
   DEBUG("ONE FINGER DOUBLE TAP AND HOLD");

   if (!context)
      {
         ERROR("No navigation context created");
         return;
      }

   AtspiAccessible *obj = NULL;
   AtspiValue *value = NULL;
   AtspiComponent *comp = NULL;
   AtspiRect *rect = NULL;
   int click_point_x = 0;
   int click_point_y = 0;

   obj = current_obj;

   if (!obj)
      {
         DEBUG("no object");
         return;
      }

   if (!_is_slider(obj))
      {
         ERROR("Object is not a slider");
         return;
      }

   if (gi->state == 0)
      {
         comp = atspi_accessible_get_component_iface(obj);
         if (!comp)
            {
               ERROR("that slider do not have component interface");
               return;
            }

         rect = atspi_component_get_extents(comp, ATSPI_COORD_TYPE_SCREEN, NULL);

         DEBUG("Current object is in:%d %d", rect->x, rect->y);
         DEBUG("Current object has size:%d %d", rect->width, rect->height);

         click_point_x = rect->x+rect->width/2;
         click_point_y = rect->y+rect->height/2;
         DEBUG("Click on point %d %d", click_point_x, click_point_y);
         start_scroll(click_point_x, click_point_y);
      }

   if (gi->state == 1)
      {
         counter ++;
         DEBUG("SCROLLING but not meet counter:%d", counter)
         if (counter >= GESTURE_LIMIT)
            {
               counter = 0;
               DEBUG("Scroll on point %d %d", gi->x_end, gi->y_end);
               continue_scroll(gi->x_end, gi->y_end);
            }
      }

   if (gi->state == 2)
      {
         DEBUG("state == 2");
         end_scroll(gi->x_end, gi->y_end);
         prepared = false;
         value = atspi_accessible_get_value_iface(obj);
         if (value)
            {
               _read_value(value);
               g_object_unref(value);
            }
         else
            {
               ERROR("There is not value interface in slider");
            }
      }
   DEBUG("END");
}

static void on_gesture_detected(void *data, Gesture_Info *info)
{
   dbus_gesture_adapter_emit(info);
   _on_auto_review_stop();

   if (info->type == ONE_FINGER_SINGLE_TAP && info->state == 3)
      {
         DEBUG("One finger single tap aborted");
         prepared = true;
      }

   switch(info->type)
      {
      case ONE_FINGER_HOVER:
         if (prepared)
            {
               DEBUG("Prepare to move slider");
               _move_slider(info);
            }
         else
            {
               DEBUG("Will focus on object, slider is not ready");
               _focus_widget(info);
            }
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
         else if (_has_value())
            _value_inc();
         else
            _focus_prev();
         break;
      case ONE_FINGER_FLICK_DOWN:
         if (_is_active_entry())
            _caret_move_forward();
         else if(_has_value())
            _value_dec();
         else
            _focus_next();
         break;
      case ONE_FINGER_SINGLE_TAP:
         if (!prepared)
            _focus_widget(info);
         break;
      case ONE_FINGER_DOUBLE_TAP:
         _activate_widget();
         break;
      case TWO_FINGERS_SINGLE_TAP:
         _set_pause();
         break;
      case TWO_FINGERS_TRIPLE_TAP:
         _read_quickpanel();
         break;
      case THREE_FINGERS_SINGLE_TAP:
         _review_from_top();
         break;
      case THREE_FINGERS_DOUBLE_TAP:
         _review_from_current();
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
         if (_is_active_entry())
            _caret_move_beg();
         else
            _direct_scroll_to_first();
         break;
      case ONE_FINGER_FLICK_DOWN_RETURN:
         if (_is_active_entry())
            _caret_move_end();
         else
            _direct_scroll_to_last();
         break;
      default:
         DEBUG("Gesture type %d not handled in switch", info->type);
      }
}

static void
_view_content_changed(AtspiAccessible *root, void *user_data)
{
   DEBUG("START");
   if (flat_navi_is_valid(context, root))
      return;
   flat_navi_context_free(context);
   context = flat_navi_context_create(root);
   _current_highlight_object_set(flat_navi_context_current_get(context));
   DEBUG("END");
}

void clear(gpointer d)
{
   AtspiAccessible ** data = d;
   AtspiAccessible *obj = *data;
   g_object_unref(obj);
}

static AtspiAccessible* _get_modal_descendant(AtspiAccessible *root)
{
   GError *err= NULL;
   AtspiStateSet *states = atspi_state_set_new (NULL);
   atspi_state_set_add (states, ATSPI_STATE_MODAL);
   DEBUG("GET MODAL: STATE SET PREPARED");
   AtspiMatchRule *rule = atspi_match_rule_new (states,
                                                ATSPI_Collection_MATCH_ANY,
                                                NULL,
                                                ATSPI_Collection_MATCH_INVALID,
                                                NULL,
                                                ATSPI_Collection_MATCH_INVALID,
                                                NULL,
                                                ATSPI_Collection_MATCH_INVALID,
                                                0);
   DEBUG("GET MODAL: MATCHING RULE PREPARED");
   AtspiAccessible *ret = NULL;
   AtspiCollection *col_iface = atspi_accessible_get_collection_iface(root);
   GArray *result = atspi_collection_get_matches (col_iface,
                                                  rule,
                                                  ATSPI_Collection_SORT_ORDER_INVALID,
                                                  1,
                                                  1,
                                                  &err);
   GERROR_CHECK(err);
   DEBUG("GET MODAL: QUERY PERFORMED");
   g_object_unref(states);
   g_object_unref(rule);
   g_object_unref(col_iface);
   if (result && result->len > 0)
     {
        DEBUG("GET MODAL: MODAL FOUND");
        g_array_set_clear_func(result, clear);
        ret = g_object_ref(g_array_index(result, AtspiAccessible*, 0));
        g_array_free(result, TRUE);
     }
   return ret;
}

static void on_window_activate(void *data, AtspiAccessible *window)
{
   DEBUG("START");

   app_tracker_callback_unregister(top_window, _view_content_changed, NULL);

   if (window)
     {
        DEBUG("Window name: %s", atspi_accessible_get_name(window, NULL));
        app_tracker_callback_register(window, _view_content_changed, NULL);
        // TODO: modal descendant of window should be used (if exists) otherwise window
        AtspiAccessible *modal_descendant = _get_modal_descendant(window);
        _view_content_changed(modal_descendant ? modal_descendant : window, NULL);
        g_object_unref(modal_descendant);
     }
   else
     {
        flat_navi_context_free(context);
        ERROR("No top window found!");
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

   set_utterance_cb(_on_utterance);

   screen_reader_gestures_tracker_register(on_gesture_detected, NULL);
   // register on active_window
   dbus_gesture_adapter_init();
   app_tracker_init();
   window_tracker_init();
   window_tracker_register(on_window_activate, NULL);
   window_tracker_active_window_request();
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
   dbus_gesture_adapter_shutdown();
   app_tracker_shutdown();
   window_tracker_shutdown();
   smart_notification_shutdown();
   system_notifications_shutdown();
   keyboard_tracker_shutdown();
}
