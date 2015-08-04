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

#include "atspi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AtspiValue *value = NULL;
static AtspiText *text = NULL;
static AtspiEditableText *editable_text = NULL;
static AtspiAction *action = NULL;
//static AtspiComponent *component = NULL;

G_DEFINE_TYPE(AtspiAccessible, atspi_accessible, G_TYPE_OBJECT);
G_DEFINE_TYPE(AtspiAction, atspi_action, G_TYPE_OBJECT);
G_DEFINE_TYPE(AtspiComponent, atspi_component, G_TYPE_OBJECT);
G_DEFINE_TYPE(AtspiStateSet, atspi_state_set, G_TYPE_OBJECT);

void atspi_rect_free (AtspiRect *rect)
{
   g_free (rect);
}

AtspiRect *atspi_rect_copy (AtspiRect *src)
{
   AtspiRect *dst = g_new (AtspiRect, 1);
   dst->x = src->x;
   dst->y = src->y;
   dst->height = src->height;
   dst->width = src->width;
   return dst;
}

G_DEFINE_BOXED_TYPE (AtspiRect, atspi_rect, atspi_rect_copy, atspi_rect_free)

void atspi_alloc_memory()
{
   value = (AtspiValue*)malloc(sizeof(AtspiValue));
   text = (AtspiText*)malloc(sizeof(AtspiText));
   editable_text = (AtspiEditableText*)malloc(sizeof(AtspiEditableText));
}

void atspi_free_memory(void)
{
   free(editable_text);
   free(value);
   free(text);
}

gchar * atspi_accessible_get_name (AtspiAccessible *obj, GError **error)
{
   if(!obj || !obj->name) return strdup("\0");

   return strdup(obj->name);
}

gchar * atspi_accessible_get_role_name (AtspiAccessible *obj, GError **error)
{
   if(!obj) return strdup("\0");
   AtspiRole role = obj->role;
   switch(role)
      {
      case ATSPI_ROLE_APPLICATION:
         return strdup("Application");
      case ATSPI_ROLE_PUSH_BUTTON:
         return strdup("Push button");
      case ATSPI_ROLE_ICON:
         return strdup("Icon");
      case ATSPI_ROLE_CHECK_BOX:
         return strdup("Check box");
      case ATSPI_ROLE_ENTRY:
         return strdup("Entry");
      case ATSPI_ROLE_FILLER:
         return strdup("filler");
      case ATSPI_ROLE_SCROLL_PANE:
         return strdup("scroll pane");
      case ATSPI_ROLE_IMAGE:
         return strdup("image");
      case ATSPI_ROLE_SPLIT_PANE:
         return strdup("split pane");
      case ATSPI_ROLE_UNKNOWN:
         return strdup("unknown");
      case ATSPI_ROLE_RULER:
         return strdup("ruler");
      case ATSPI_ROLE_FOOTER:
         return strdup("footer");
      case ATSPI_ROLE_INFO_BAR:
         return strdup("infobar");
      case ATSPI_ROLE_LINK:
         return strdup("link");
      default:
         return strdup("\0");
      }
}

gchar * atspi_accessible_get_localized_role_name (AtspiAccessible *obj, GError **error)
{
   if(!obj) return strdup("\0");
   AtspiRole role = obj->role;
   switch(role)
      {
      case ATSPI_ROLE_APPLICATION:
         return strdup("Application");
      case ATSPI_ROLE_PUSH_BUTTON:
         return strdup("Push button");
      case ATSPI_ROLE_ICON:
         return strdup("Icon");
      case ATSPI_ROLE_CHECK_BOX:
         return strdup("Check box");
      case ATSPI_ROLE_ENTRY:
         return strdup("Entry");
      case ATSPI_ROLE_FILLER:
         return strdup("filler");
      default:
         return strdup("\0");
      }
}

gchar * atspi_accessible_get_toolkit_name (AtspiAccessible *obj, GError **error)
{
   return "fake atspi";
}

gchar * atspi_accessible_get_description (AtspiAccessible *obj, GError **error)
{
   if(!obj || !obj->description) return strdup("\0");
   return strdup(obj->description);
}

AtspiAction * atspi_accessible_get_action_iface (AtspiAccessible *obj)
{
   if(!obj) return NULL;
   return action;
}

AtspiText * atspi_accessible_get_text_iface (AtspiAccessible *obj)
{
   if(!obj) return NULL;
   return text;
}

gint atspi_text_get_character_count (AtspiText *obj, GError **error)
{
   if(!obj) return -1;
   return 6;
}

gint atspi_text_get_caret_offset (AtspiText *obj, GError **error)
{
   if(!obj) return -1;
   return 5;
}

gchar * atspi_text_get_text (AtspiText *obj, gint start_offset, gint end_offset, GError **error)
{
   if(!obj) return NULL;
   return "AtspiText text";
}

AtspiValue * atspi_accessible_get_value_iface (AtspiAccessible *obj)
{
   if(!obj) return NULL;
   return value;
}

gdouble atspi_value_get_current_value (AtspiValue *obj, GError **error)
{
   return 1.0;
}

gdouble atspi_value_get_maximum_value (AtspiValue *obj, GError **error)
{
   return 2.0;
}

gdouble atspi_value_get_minimum_value (AtspiValue *obj, GError **error)
{
   return 0.0;
}

AtspiEventListener *atspi_event_listener_new (AtspiEventListenerCB callback,
      gpointer user_data,
      GDestroyNotify callback_destroyed)
{
   return NULL;
}

gboolean atspi_event_listener_register (AtspiEventListener *listener,
                                        const gchar *event_type,
                                        GError **error)
{
   return FALSE;
}

gboolean atspi_event_listener_deregister (AtspiEventListener *listener,
      const gchar *event_type,
      GError **error)
{
   return FALSE;
}

AtspiStateSet * atspi_accessible_get_state_set (AtspiAccessible *obj)
{
   if (!obj || !obj->states) return NULL;
   return obj->states;
}

gboolean atspi_state_set_contains (AtspiStateSet *set, AtspiStateType state)
{
   if (!set)
      return FALSE;
   return (set->states & ((gint64)1 << state)) ? TRUE : FALSE;
}

void atspi_state_set_add (AtspiStateSet *set, AtspiStateType state)
{
   if (!set) return;
   set->states |= (((gint64)1) << state);
}

gboolean atspi_component_grab_highlight (AtspiComponent *obj, GError **error)
{
   return FALSE;
}

AtspiScrollable *atspi_accessible_get_scrollable (AtspiAccessible *accessible)
{
   return NULL;
}

gboolean atspi_component_clear_highlight (AtspiComponent *obj, GError **error)
{
   return FALSE;
}

gboolean atspi_component_contains (AtspiComponent *obj, gint x, gint y, AtspiCoordType ctype, GError **error)
{
   return TRUE;
}

GArray *atspi_state_set_get_states (AtspiStateSet *set)
{
   gint i = 0;
   guint64 val = 1;
   GArray *ret;

   g_return_val_if_fail (set != NULL, NULL);
   ret = g_array_new (TRUE, TRUE, sizeof (AtspiStateType));
   if (!ret)
      return NULL;
   for (i = 0; i < 64; i++)
      {
         if (set->states & val)
            ret = g_array_append_val (ret, i);
         val <<= 1;
      }
   return ret;

}

AtspiRole atspi_accessible_get_role (AtspiAccessible *obj, GError **error)
{
   if(!obj) return ATSPI_ROLE_INVALID;
   return obj->role;
}

gint atspi_accessible_get_child_count (AtspiAccessible *obj, GError **error)
{
   if(!obj || !obj->children) return 0;
   return g_list_length(obj->children);
}

AtspiAccessible * atspi_accessible_get_child_at_index (AtspiAccessible *obj, gint child_index, GError **error)
{
   if(!obj || child_index >= g_list_length(obj->children)) return NULL;
   return g_object_ref(g_list_nth_data(obj->children, child_index));
}

AtspiComponent * atspi_accessible_get_component_iface (AtspiAccessible *obj)
{
   if(!obj) return NULL;
   AtspiComponent *component = g_object_new(ATSPI_COMPONENT_OBJECT_TYPE, 0);
   *(component->role) = obj->role;
   return component;
}

AtspiStateSet * atspi_state_set_new (GArray *states)
{
   AtspiStateSet *set = g_object_new (ATSPI_STATE_OBJECT_TYPE, NULL);
   if (!set) return NULL;
   int i;
   for (i = 0; i < states->len; i++)
      {
         atspi_state_set_add (set, g_array_index (states, AtspiStateType, i));
      }
   return set;
}

AtspiRect *atspi_component_get_extents (AtspiComponent *component, AtspiCoordType ctype, GError **error)
{
   if(!component) return NULL;

   AtspiRect rect;
   if(*(component->role) == ATSPI_ROLE_APPLICATION)
      {
         rect.x = 0;
         rect.y = 0;
         rect.width = 100;
         rect.height = 100;
      }
   else if(*(component->role) == ATSPI_ROLE_PUSH_BUTTON)
      {
         rect.x = 1;
         rect.y = 1;
         rect.width = 50;
         rect.height = 50;
      }
   else if(*(component->role) == ATSPI_ROLE_ICON)
      {
         rect.x = 50;
         rect.y = 0;
         rect.width = 50;
         rect.height = 50;
      }
   else if(*(component->role) == ATSPI_ROLE_CHECK_BOX)
      {
         rect.x = 0;
         rect.y = 50;
         rect.width = 50;
         rect.height = 50;
      }
   else if(*(component->role) == ATSPI_ROLE_ENTRY)
      {
         rect.x = 50;
         rect.y = 50;
         rect.width = 50;
         rect.height = 50;
      }
   else
      {
         rect.x = 0;
         rect.y = 0;
         rect.width = 0;
         rect.height = 0;
      }
   return atspi_rect_copy(&rect);
}

AtspiAccessible *atspi_create_accessible()
{
   AtspiAccessible *obj = g_object_new(ATSPI_ACCESSIBLE_OBJECT_TYPE, 0);
   obj->children = NULL;
   obj->index_in_parent = 0;
   obj->child_count = 0;

   GArray *states = g_array_new (TRUE, TRUE, sizeof (AtspiStateType));
   AtspiStateType s[] =
   {
      ATSPI_STATE_VISIBLE,
      ATSPI_STATE_SHOWING,
      ATSPI_STATE_FOCUSABLE,
      ATSPI_STATE_LAST_DEFINED
   };
   int i;
   for (i=0; i<(int)(sizeof(s)/sizeof(s[0])); i++)
      g_array_append_val (states, s[i]);
   obj->states = atspi_state_set_new(states);

   return obj;
}

void atspi_delete_accessible(AtspiAccessible *obj)
{
   if(!obj) return;
   if(obj->children)
      {
         atpis_accessible_remove_children(obj);
      }
   g_object_unref(obj);
}

void atspi_accessible_add_child(AtspiAccessible *obj, AtspiAccessible *child)
{
   child->index_in_parent = obj->child_count;
   child->accessible_parent = obj;
   obj->children = g_list_append(obj->children, child);
   obj->child_count++;
}

void atpis_accessible_remove_children(AtspiAccessible *obj)
{
   GList *l = obj->children;
   while (l != NULL)
      {
         GList *next = l->next;
         if(l->data)
            {
               atspi_delete_accessible(l->data);
            }
         l = next;
      }
   g_list_free(obj->children);
}

static void atspi_state_set_init(AtspiStateSet* set)
{
}

static void atspi_state_set_class_init(AtspiStateSetClass *_class)
{
}

static void atspi_action_class_init(AtspiActionClass *_class)
{
}

static void atspi_action_init(AtspiAction* obj)
{
}

static void atspi_accessible_class_init(AtspiAccessibleClass *_class)
{
}

static void atspi_accessible_init(AtspiAccessible* obj)
{
}

static void atspi_component_finalize(GObject *obj)
{
   AtspiComponent *component = (AtspiComponent*)obj;
   free(component->role);
}

static void atspi_component_class_init(AtspiComponentClass *class)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (class);

   gobject_class->finalize = atspi_component_finalize;
}

static void atspi_component_init(AtspiComponent* obj)
{
   obj->role = (AtspiRole*)malloc(sizeof(AtspiRole));
}

AtspiEditableText * atspi_accessible_get_editable_text_iface (AtspiAccessible *obj)
{
   return editable_text;
}

GArray * atspi_accessible_get_relation_set (AtspiAccessible *obj, GError **error)
{
   return NULL;
}

AtspiRelationType atspi_relation_get_relation_type (AtspiRelation *obj)
{
   return ATSPI_RELATION_NULL;
}

gint atspi_relation_get_n_targets (AtspiRelation *obj)
{
   return 0;
}

AtspiAccessible * atspi_relation_get_target (AtspiRelation *obj, gint i)
{
   return NULL;
}

AtspiAccessible * atspi_accessible_get_parent (AtspiAccessible *obj, GError **error)
{
   return obj->accessible_parent;
}

int atspi_component_get_highlight_index(AtspiComponent *obj, GError **error)
{
   return 0;
}

gint atspi_action_get_n_actions(AtspiAction *obj, GError **error)
{
   return 0;
}

gchar * atspi_action_get_action_name (AtspiAction *obj, gint i, GError **error)
{
    return strdup("");
}

gint atspi_accessible_get_index_in_parent (AtspiAccessible *obj, GError **error)
{
   return obj->index_in_parent;
}

int atspi_exit(void)
{
   return 1;
}
