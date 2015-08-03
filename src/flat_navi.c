/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This file is a modified version of BSD licensed file and
 * licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please, see the LICENSE file for the original copyright owner and
 * license.
 */

#include "flat_navi.h"
#include "logger.h"

struct _FlatNaviContext
{
   AtspiAccessible *root;
   AtspiAccessible *current;
   AtspiAccessible *first;
   AtspiAccessible *last;
   AtspiAccessible *prev;
};

static const AtspiStateType required_states[] =
{
   ATSPI_STATE_SHOWING,
   ATSPI_STATE_VISIBLE,
   ATSPI_STATE_FOCUSABLE,
   ATSPI_STATE_LAST_DEFINED
};

static const AtspiRole interesting_roles[] =
{
   ATSPI_ROLE_CALENDAR,
   ATSPI_ROLE_CHECK_BOX,
   ATSPI_ROLE_COLOR_CHOOSER,
   ATSPI_ROLE_COMBO_BOX,
   ATSPI_ROLE_DATE_EDITOR,
   ATSPI_ROLE_DIALOG,
   ATSPI_ROLE_FILE_CHOOSER,
   ATSPI_ROLE_FILLER,
   ATSPI_ROLE_FONT_CHOOSER,
   ATSPI_ROLE_GLASS_PANE,
   ATSPI_ROLE_HEADER,
   ATSPI_ROLE_HEADING,
   ATSPI_ROLE_ICON,
   ATSPI_ROLE_ENTRY,
   ATSPI_ROLE_LABEL,
   ATSPI_ROLE_LINK,
   ATSPI_ROLE_LIST_ITEM,
   ATSPI_ROLE_MENU_ITEM,
   ATSPI_ROLE_PANEL,
   ATSPI_ROLE_PARAGRAPH,
   ATSPI_ROLE_PASSWORD_TEXT,
   ATSPI_ROLE_POPUP_MENU,
   ATSPI_ROLE_PUSH_BUTTON,
   ATSPI_ROLE_PROGRESS_BAR,
   ATSPI_ROLE_RADIO_BUTTON,
   ATSPI_ROLE_RADIO_MENU_ITEM,
   ATSPI_ROLE_SLIDER,
   ATSPI_ROLE_SPIN_BUTTON,
   ATSPI_ROLE_TABLE_CELL,
   ATSPI_ROLE_TEXT,
   ATSPI_ROLE_TOGGLE_BUTTON,
   ATSPI_ROLE_TOOL_TIP,
   ATSPI_ROLE_TREE_ITEM,
   ATSPI_ROLE_LAST_DEFINED
};

static Eina_Bool
_has_escape_action(AtspiAccessible *obj)
{
   Eina_Bool ret = EINA_FALSE;

   AtspiAction *action = NULL;

   action = atspi_accessible_get_action_iface(obj);
   if(action)
      {
         int i = 0;
         for (; i < atspi_action_get_n_actions(action, NULL); i++)
            {
               gchar *action_name = atspi_action_get_action_name(action, i, NULL);
               Eina_Bool equal = !strcmp(action_name, "escape");
               g_free(action_name);
               if (equal)
                  {
                     ret = EINA_TRUE;
                     break;
                  }
            }
         g_object_unref(action);
      }
   DEBUG("Obj %s %s escape action",atspi_accessible_get_role_name(obj, NULL), ret ? "has" : "doesn't have");
   return ret;
}

static Eina_Bool
_is_collapsed(AtspiStateSet *ss)
{
   if (!ss)
      return EINA_FALSE;

   Eina_Bool ret = EINA_FALSE;
   if (atspi_state_set_contains(ss, ATSPI_STATE_EXPANDABLE) && !atspi_state_set_contains(ss, ATSPI_STATE_EXPANDED))
      ret = EINA_TRUE;

   return ret;
}

static Eina_Bool
_object_is_item(AtspiAccessible *obj)
{
   if (!obj)
      return EINA_FALSE;

   Eina_Bool ret = EINA_FALSE;
   AtspiRole role = atspi_accessible_get_role(obj, NULL);
   if (role == ATSPI_ROLE_LIST_ITEM || role == ATSPI_ROLE_MENU_ITEM)
      ret = EINA_TRUE;
   DEBUG("IS ITEM %d", ret);
   return ret;
}


static AtspiAccessible* _get_object_in_relation(AtspiAccessible *source, AtspiRelationType search_type)
{
   GArray *relations;
   AtspiAccessible *ret = NULL;
   AtspiRelation *relation;
   AtspiRelationType type;
   int i;
   if (source)
      {
         DEBUG("CHECKING RELATIONS");
         relations = atspi_accessible_get_relation_set(source, NULL);
         for (i = 0; i < relations->len; i++)
            {
               DEBUG("ALL RELATIONS FOUND: %d",relations->len);
               relation = g_array_index (relations, AtspiRelation*, i);
               type = atspi_relation_get_relation_type(relation);
               DEBUG("RELATION:  %d",type);

               if (type == search_type)
                  {
                     ret = atspi_relation_get_target(relation, 0);
                     DEBUG("SEARCHED RELATION FOUND");
                     break;
                  }
            }
         g_array_free(relations, TRUE);
      }
   return ret;
}

static Eina_Bool
_accept_object(AtspiAccessible *obj)
{
   DEBUG("START");
   if (!obj) return EINA_FALSE;

   Eina_Bool ret = EINA_FALSE;
   gchar *name = NULL;
   gchar *desc = NULL;
   AtspiAction *action = NULL;
   AtspiEditableText *etext = NULL;
   AtspiText *text = NULL;
   AtspiValue *value = NULL;
   AtspiStateSet *ss = NULL;

   AtspiRole r;

   ss = atspi_accessible_get_state_set(obj);
   if (ss)
      {
         if (_object_is_item(obj))
            {
               AtspiAccessible *parent = atspi_accessible_get_parent(obj, NULL);
               if (parent)
                  {
                     AtspiStateSet *pss = atspi_accessible_get_state_set(parent);
                     g_object_unref(parent);
                     if (pss)
                        {
                           ret = atspi_state_set_contains(pss, ATSPI_STATE_SHOWING) && atspi_state_set_contains(pss, ATSPI_STATE_VISIBLE) && !_is_collapsed(pss);
                           DEBUG("ITEM HAS SHOWING && VISIBLE && NOT COLLAPSED PARENT %d",ret);
                           g_object_unref(pss);
                           return ret;
                        }
                  }
            }
         else
            {
               ret = atspi_state_set_contains(ss, ATSPI_STATE_SHOWING) && atspi_state_set_contains(ss, ATSPI_STATE_VISIBLE);
            }
         g_object_unref(ss);
      }
   if (!ret)
      {
         return EINA_FALSE;
      }

   r = atspi_accessible_get_role(obj, NULL);

   switch(r)
      {
      case ATSPI_ROLE_APPLICATION:
      case ATSPI_ROLE_FILLER:
      case ATSPI_ROLE_SCROLL_PANE:
      case ATSPI_ROLE_SPLIT_PANE:
      case ATSPI_ROLE_WINDOW:
      case ATSPI_ROLE_IMAGE:
      case ATSPI_ROLE_LIST:
      case ATSPI_ROLE_PAGE_TAB_LIST:
      case ATSPI_ROLE_TOOL_BAR:
         return EINA_FALSE;
      case ATSPI_ROLE_DIALOG:
         if (!_has_escape_action(obj))
            return EINA_FALSE;
         break;
      default:
         break;
      }

   name = atspi_accessible_get_name(obj, NULL);

   ret = EINA_FALSE;
   if (strncmp(name, "\0", 1))
      {
         DEBUG("Has name:[%s]", name);
         ret = EINA_TRUE;
      }
   g_free(name);
   desc = atspi_accessible_get_description(obj, NULL);
   if (!ret && strncmp(desc, "\0", 1))
      {
         DEBUG("Has description:[%s]", desc);
         ret = EINA_TRUE;
      }
   g_free(desc);
   if (!ret)
      {
         action = atspi_accessible_get_action_iface(obj);
         if (action)
            {
               DEBUG("Has action interface");
               ret = EINA_TRUE;
               g_object_unref(action);
            }
      }
   if (!ret)
      {
         value = atspi_accessible_get_value_iface(obj);
         if (value)
            {
               DEBUG("Has value interface");
               ret = EINA_TRUE;
               g_object_unref(value);
            }
      }
   if (!ret)
      {
         etext = atspi_accessible_get_editable_text_iface(obj);
         if (etext)
            {
               DEBUG("Has editable text interface");
               ret = EINA_TRUE;
               g_object_unref(etext);
            }
      }
   if (!ret)
      {
         text = atspi_accessible_get_text_iface(obj);
         if (text)
            {
               DEBUG("Has text interface");
               ret = EINA_TRUE;
               g_object_unref(text);
            }
      }

   DEBUG("END:%d", ret);
   return ret;
}

#ifdef SCREEN_READER_FLAT_NAVI_TEST_DUMMY_IMPLEMENTATION
Eina_Bool flat_navi_context_current_at_x_y_set( FlatNaviContext *ctx, gint x_cord, gint y_cord , AtspiAccessible **target)
{
   return EINA_FALSE;
}
#else

int _object_has_modal_state(AtspiAccessible *obj)
{
   if (!obj)
      return EINA_FALSE;

   Eina_Bool ret = EINA_FALSE;

   AtspiStateSet *ss = atspi_accessible_get_state_set(obj);

   if (atspi_state_set_contains(ss, ATSPI_STATE_MODAL))
      ret = EINA_TRUE;
   g_object_unref(ss);
   return ret;
}

static AtspiAccessible *_first_modal_object_in_object_chain(AtspiAccessible *obj)
{
   AtspiAccessible *ret = g_object_ref(obj);

   while(ret && !_object_has_modal_state(ret))
      {
         g_object_unref(ret);
         ret = atspi_accessible_get_parent(ret, NULL);
      }
   return ret;
}

Eina_Bool flat_navi_context_current_at_x_y_set( FlatNaviContext *ctx, gint x_cord, gint y_cord , AtspiAccessible **target)
{
   if(!ctx || !target) return EINA_FALSE;

   if (!ctx->root)
      {
         DEBUG("NO top window");
         return EINA_FALSE;
      }

   AtspiAccessible *current_obj = flat_navi_context_current_get(ctx);

   Eina_Bool ret = EINA_FALSE;
   GError * error = NULL;

   AtspiAccessible *obj = g_object_ref(ctx->root);
   AtspiAccessible *youngest_ancestor_in_context = (_accept_object(obj) ? g_object_ref(obj) : NULL);
   AtspiComponent *component;
   Eina_Bool look_for_next_descendant = EINA_TRUE;

   while(look_for_next_descendant)
      {
         component = atspi_accessible_get_component_iface(obj);

         g_object_unref(obj);
         obj = component ? atspi_component_get_accessible_at_point(component,
               x_cord,
               y_cord,
               ATSPI_COORD_TYPE_WINDOW,
               &error
                                                                  ) : NULL;
         g_clear_object(&component);

         if (error)
            {
               DEBUG("Got error from atspi_component_get_accessible_at_point, domain: %i, code: %i, message: %s",
                     error->domain, error->code, error->message);
               g_clear_error(&error);
               g_clear_object(&obj);
               if (youngest_ancestor_in_context)
                  g_clear_object(&youngest_ancestor_in_context);
               look_for_next_descendant = EINA_FALSE;
            }
         else if (obj)
            {
               DEBUG("Found object %s, role %s", atspi_accessible_get_name(obj, NULL), atspi_accessible_get_role_name(obj, NULL));
               if (_accept_object(obj))
                  {
                     DEBUG("Object  %s with role %s fulfills highlight conditions", atspi_accessible_get_name(obj, NULL), atspi_accessible_get_role_name(obj, NULL));
                     if (youngest_ancestor_in_context)
                        g_object_unref(youngest_ancestor_in_context);
                     youngest_ancestor_in_context = g_object_ref(obj);
                  }
            }
         else
            {
               g_clear_object(&obj);
               look_for_next_descendant = EINA_FALSE;
            }
      }

   if (youngest_ancestor_in_context && !_object_has_modal_state(youngest_ancestor_in_context))
      {
         if (youngest_ancestor_in_context == current_obj ||
               flat_navi_context_current_set(ctx, youngest_ancestor_in_context))
            {
               DEBUG("Setting highlight to object %s with role %s",
                     atspi_accessible_get_name(youngest_ancestor_in_context, NULL),
                     atspi_accessible_get_role_name(youngest_ancestor_in_context, NULL));
               *target = youngest_ancestor_in_context;
               ret = EINA_TRUE;
            }
      }
   else
      DEBUG("NO widget under (%d, %d) found or the same widget under hover",
            x_cord, y_cord);
   DEBUG("END");
   return ret;
}
#endif


AtspiAccessible *_get_child(AtspiAccessible *obj, int i)
{
   DEBUG("START:%d", i);
   if (i<0)
      {
         DEBUG("END");
         return NULL;
      }
   if (!obj)
      {
         DEBUG("END");
         return NULL;
      }
   int cc = atspi_accessible_get_child_count(obj, NULL);
   if (cc == 0 || i>=cc)
      {
         DEBUG("END");
         return NULL;
      }
   return atspi_accessible_get_child_at_index (obj, i, NULL);
}

static Eina_Bool _has_next_sibling(AtspiAccessible *obj, int next_sibling_idx_modifier)
{
   Eina_Bool ret = EINA_FALSE;
   int idx = atspi_accessible_get_index_in_parent(obj, NULL);
   if (idx >= 0)
      {
         AtspiAccessible *parent = atspi_accessible_get_parent(obj, NULL);
         int cc = atspi_accessible_get_child_count(parent, NULL);
         g_object_unref(parent);
         if ((next_sibling_idx_modifier > 0 && idx < cc-1) || (next_sibling_idx_modifier < 0 && idx > 0))
            {
               ret = EINA_TRUE;
            }
      }
   return ret;
}

AtspiAccessible * _directional_depth_first_search(AtspiAccessible *root, AtspiAccessible *start, int next_sibling_idx_modifier, Eina_Bool (*stop_condition)(AtspiAccessible*))
{
   Eina_Bool start_is_not_defunct = EINA_FALSE;
   if (start)
      {
         AtspiStateSet *ss = atspi_accessible_get_state_set(start);
         start_is_not_defunct = !atspi_state_set_contains(ss, ATSPI_STATE_DEFUNCT);
         g_object_unref(ss);
         if (!start_is_not_defunct)
            DEBUG("Start is defunct!");
      }

   AtspiAccessible *node = (start && start_is_not_defunct)
                           ? g_object_ref(start)
                           : (root ? g_object_ref(root) : NULL);

   if (!node) return NULL;

   AtspiAccessible *next_related_in_direction = (next_sibling_idx_modifier > 0)
         ? _get_object_in_relation(node, ATSPI_RELATION_FLOWS_TO)
         : _get_object_in_relation(node, ATSPI_RELATION_FLOWS_FROM);

   Eina_Bool relation_mode = EINA_FALSE;
   if (next_related_in_direction)
      {
         relation_mode = EINA_TRUE;
         g_object_unref(next_related_in_direction);
      }

   while (1)
      {
         AtspiAccessible *prev_related_in_direction = (next_sibling_idx_modifier > 0)
               ? _get_object_in_relation(node, ATSPI_RELATION_FLOWS_FROM)
               : _get_object_in_relation(node, ATSPI_RELATION_FLOWS_TO);

         if (node != start && (relation_mode || !prev_related_in_direction) && stop_condition(node))
            {
               g_object_unref(prev_related_in_direction);
               return node;
            }

         AtspiAccessible *next_related_in_direction = (next_sibling_idx_modifier > 0)
               ? _get_object_in_relation(node, ATSPI_RELATION_FLOWS_TO)
               : _get_object_in_relation(node, ATSPI_RELATION_FLOWS_FROM);

         DEBUG("RELATION MODE: %d",relation_mode);
         if (!prev_related_in_direction)
            DEBUG("PREV IN RELATION NULL");
         if (!next_related_in_direction)
            DEBUG("NEXT IN RELATION NULL");


         if ((!relation_mode && !prev_related_in_direction && next_related_in_direction) || (relation_mode && next_related_in_direction))
            {
               DEBUG("APPLICABLE FOR RELATION NAVIG");
               g_object_unref(prev_related_in_direction);
               relation_mode = EINA_TRUE;
               g_object_unref(node);
               node = next_related_in_direction;
            }
         else
            {
               g_object_unref(prev_related_in_direction);
               g_object_unref(next_related_in_direction);
               relation_mode = EINA_FALSE;
               int cc = atspi_accessible_get_child_count(node, NULL);
               if (cc > 0) // walk down
                  {
                     int idx = next_sibling_idx_modifier > 0 ? 0 : cc-1;
                     g_object_unref(node);
                     node = atspi_accessible_get_child_at_index (node,idx, NULL);
                     DEBUG("DFS DOWN");
                  }
               else
                  {
                     while (!_has_next_sibling(node, next_sibling_idx_modifier) || node==root) // no next sibling
                        {
                           DEBUG("DFS NO SIBLING");
                           if (node == root)
                              {
                                 DEBUG("DFS ROOT")
                                 g_object_unref(node);
                                 return NULL;
                              }
                           g_object_unref(node);
                           node = atspi_accessible_get_parent(node, NULL);       // walk up...
                           DEBUG("DFS UP");
                        }
                     int idx = atspi_accessible_get_index_in_parent(node, NULL);
                     g_object_unref(node);
                     node = atspi_accessible_get_child_at_index(atspi_accessible_get_parent(node, NULL), idx + next_sibling_idx_modifier, NULL);     //... and next
                     DEBUG("DFS NEXT %d",idx + next_sibling_idx_modifier);
                  }
            }
      }
}

AtspiAccessible *_first(FlatNaviContext *ctx)
{
   DEBUG("START");
   return _directional_depth_first_search(ctx->root, NULL, 1, &_accept_object);
}

AtspiAccessible *_last(FlatNaviContext *ctx)
{
   DEBUG("START");
   return _directional_depth_first_search(ctx->root, NULL, -1, &_accept_object);
}

AtspiAccessible *_next(FlatNaviContext *ctx)
{
   DEBUG("START");
   AtspiAccessible *root = ctx->root;
   AtspiAccessible *current = ctx->current;
   AtspiAccessible *ret = NULL;

   ret = _directional_depth_first_search(root, current, 1, &_accept_object);

   if (current && !ret)
      {
         DEBUG("DFS SECOND PASS");
         ret = _directional_depth_first_search(root, NULL, 1, &_accept_object);
      }
   return ret;
}

AtspiAccessible *_prev(FlatNaviContext *ctx)
{
   DEBUG("START\n");
   AtspiAccessible *root = ctx->root;
   AtspiAccessible *current = ctx->current;
   AtspiAccessible *ret = NULL;

   ret = _directional_depth_first_search(root, current, -1, &_accept_object);
   if (current && !ret)
      {
         DEBUG("DFS SECOND PASS");
         ret = _directional_depth_first_search(root, NULL, -1, &_accept_object);
      }
   return ret;
}


int flat_navi_context_current_children_count_visible_get( FlatNaviContext *ctx)
{
   if(!ctx) return -1;
   int count = 0;
   /*
      AtspiAccessible *obj = NULL;
      AtspiStateSet *ss = NULL;

      Eina_List *l, *l2, *line;
      AtspiAccessible *current = flat_navi_context_current_get(ctx);
      AtspiAccessible *parent = atspi_accessible_get_parent (current, NULL);

      EINA_LIST_FOREACH(ctx->lines, l, line)
      {
         EINA_LIST_FOREACH(line, l2, obj)
         {
            ss = atspi_accessible_get_state_set(obj);
            if (atspi_state_set_contains(ss, ATSPI_STATE_SHOWING) && parent == atspi_accessible_get_parent(obj, NULL))
               count++;
            g_object_unref(ss);
         }
      }
   */
   return count;
}

FlatNaviContext *flat_navi_context_create(AtspiAccessible *root)
{
   DEBUG("START");
   if (!root) return NULL;
   FlatNaviContext *ret;
   ret = calloc(1, sizeof(FlatNaviContext));
   if (!ret) return NULL;

   ret->root = root;
   ret->prev = NULL;
   ret->current = _first(ret);
   return ret;
}

void flat_navi_context_free(FlatNaviContext *ctx)
{
   if (!ctx) return;
   free(ctx);
}

AtspiAccessible *flat_navi_context_root_get(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   return ctx->root;
}

const AtspiAccessible *flat_navi_context_first_get(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   return _first(ctx);
}

const AtspiAccessible *flat_navi_context_last_get(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   return _last(ctx);
}

AtspiAccessible *flat_navi_context_current_get(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   return ctx->current;
}

Eina_Bool flat_navi_context_current_set(FlatNaviContext *ctx, AtspiAccessible *target)
{
   if(!ctx || !target) return EINA_FALSE;

   ctx->current = target;

   return EINA_TRUE;
}

AtspiAccessible *flat_navi_context_next(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   AtspiAccessible *ret = _next(ctx);
   ctx->current = ret;

   return ret;

}

AtspiAccessible *flat_navi_context_prev(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   AtspiAccessible *ret = _prev(ctx);
   ctx->current = ret;

   return ret;
}

AtspiAccessible *flat_navi_context_first(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   AtspiAccessible *ret = _first(ctx);
   ctx->current = ret;

   return ret;
}

AtspiAccessible *flat_navi_context_last(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   AtspiAccessible *ret = _last(ctx);
   ctx->current = ret;

   return ret;
}

Eina_Bool flat_navi_is_valid(FlatNaviContext *context, AtspiAccessible *new_root)
{
   Eina_Bool ret = EINA_FALSE;
   if (!context || !context->current || context->root != new_root)
      return ret;
   AtspiStateSet *ss = atspi_accessible_get_state_set(context->current);

   ret = atspi_state_set_contains(ss, ATSPI_STATE_SHOWING) &&  !atspi_state_set_contains(ss, ATSPI_STATE_DEFUNCT);
   g_object_unref(ss);
   return ret;
}
