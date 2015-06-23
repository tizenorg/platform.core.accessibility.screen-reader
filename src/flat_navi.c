#include "flat_navi.h"
#include "position_sort.h"
#include "object_cache.h"
#include "logger.h"

struct _FlatNaviContext
{
   AtspiAccessible *root;
   Eina_List *current;
   Eina_List *current_line;
   Eina_List *lines;
   Eina_List *candidates;
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
   ATSPI_ROLE_CHECK_BOX,
   ATSPI_ROLE_COLOR_CHOOSER,
   ATSPI_ROLE_COMBO_BOX,
   ATSPI_ROLE_DATE_EDITOR,
   ATSPI_ROLE_FILE_CHOOSER,
   ATSPI_ROLE_FILLER,
   ATSPI_ROLE_FONT_CHOOSER,
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

/**
 * @brief Returns a list of all descendants objects
 *
 * @return Eina_List of AtspiAccessible* type. List Should be
 * be free with _accessible_list_free after usage
 *
 * @note obj parameter will also be included (in list head)
 * @note every obj has increased ref. call g_object_unref on every
 * object after usage.
 */
static Eina_List*
_descendants_list_get(AtspiAccessible *obj)
{
   Eina_List *ret = NULL, *toprocess = NULL;
   int i;

   if (!obj) return NULL;

   // to keep all refcounts in ret list +1
   g_object_ref(obj);
   toprocess = eina_list_append(toprocess, obj);

   while (toprocess)
      {
         AtspiAccessible *obj = eina_list_data_get(toprocess);
         toprocess = eina_list_remove_list(toprocess, toprocess);
         int n = atspi_accessible_get_child_count(obj, NULL);
         for (i = 0; i < n; i++)
            {
               AtspiAccessible *child = atspi_accessible_get_child_at_index(obj, i, NULL);
               if (child) toprocess = eina_list_append(toprocess, child);
            }

         ret = eina_list_append(ret, obj);
      }

   return ret;
}

static void
_accessible_list_free(Eina_List *d)
{
   AtspiAccessible *obj;

   EINA_LIST_FREE(d, obj)
   {
      g_object_unref(obj);
   }
}

typedef struct
{
   Eina_List *success;
   Eina_List *failure;
} FilterResult;

static FilterResult
_accessible_list_split_with_filter(Eina_List *list, Eina_Each_Cb cb, void *user_data)
{
   FilterResult ret = { NULL, NULL};
   Eina_List *l, *ln;
   AtspiAccessible *obj;

   EINA_LIST_FOREACH_SAFE(list, l, ln, obj)
   {
      Eina_Bool res = cb(NULL, user_data, obj);
      if (res)
         eina_list_move_list(&ret.success, &list, l);
      else
         eina_list_move_list(&ret.failure, &list, l);
   }

   return ret;
}

static Eina_Bool
_filter_state_cb(const void *container, void *data, void *fdata)
{
   AtspiStateType *state = data;

   Eina_Bool ret = EINA_TRUE;
   AtspiAccessible *obj = fdata;
   AtspiAccessible *parent = atspi_accessible_get_parent(obj, NULL);
   AtspiStateSet *ss_parent = NULL;
   AtspiStateSet *ss = NULL;

   if (parent)
      {
         if (atspi_accessible_get_role(obj, NULL) == ATSPI_ROLE_LIST_ITEM)
            {
               ss_parent = atspi_accessible_get_state_set(parent);
               if (ss_parent)
                  {
                     if (atspi_state_set_contains(ss_parent, ATSPI_STATE_SHOWING)
                           && atspi_state_set_contains(ss_parent, ATSPI_STATE_VISIBLE))
                        {
                           g_object_unref(parent);
                           g_object_unref(ss_parent);
                           return EINA_TRUE;
                        }
                     g_object_unref(ss_parent);
                  }
            }
         g_object_unref(parent);
      }

   ss = atspi_accessible_get_state_set(obj);
   while (*state != ATSPI_STATE_LAST_DEFINED)
      {
         if (!atspi_state_set_contains(ss, *state))
            {
               ret = EINA_FALSE;
               break;
            }
         state++;
      }
   if (ss)
      g_object_unref(ss);
   return ret;
}

static Eina_Bool
_filter_role_cb(const void *container, void *data, void *fdata)
{
   AtspiRole *role = data;
   Eina_Bool ret = EINA_FALSE;
   AtspiAccessible *obj = fdata;

   while (*role != ATSPI_ROLE_LAST_DEFINED)
      {
         if (atspi_accessible_get_role(obj, NULL) == *role)
            {
               ret = EINA_TRUE;
               break;
            }
         role++;
      }
   return ret;
}

static Eina_Bool
_filter_viewport_cb(const void *container, void *data, void *fdata)
{
   const ObjectCache *oc = data;
   AtspiAccessible *obj = fdata;

   if (atspi_accessible_get_role(obj, NULL) == ATSPI_ROLE_LIST_ITEM)
      return EINA_TRUE;

   oc = object_cache_get(obj);
   if (!oc || !oc->bounds || (oc->bounds->height < 0) || (oc->bounds->width < 0))
      return EINA_FALSE;

   if (((oc->bounds->x + oc->bounds->width) < 0) || ((oc->bounds->y + oc->bounds->height) < 0))
      return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_List*
_flat_review_candidates_get(AtspiAccessible *root)
{
   Eina_List *desc, *ret = NULL;
   FilterResult fr0;

   desc = _descendants_list_get(root);

   DEBUG("All descendants: %d", eina_list_count(desc));
   Eina_List *l, *ln;
   AtspiAccessible *obj;

   // remove object that are not in root's viewport

   const ObjectCache *oc = object_cache_get(root);
   if (!oc || !oc->bounds || (oc->bounds->height < 0) || (oc->bounds->width < 0))
      {
         ERROR("Invalid root object bounds. Unable to filter descendants by position");
         fr0.success = desc;
      }
   else
      {
         fr0 = _accessible_list_split_with_filter(desc, _filter_viewport_cb, (void*)oc);
         _accessible_list_free(fr0.failure);
      }

   // remove objects without required states
   FilterResult fr1 = _accessible_list_split_with_filter(fr0.success, _filter_state_cb, (void*)required_states);
   _accessible_list_free(fr1.failure);

   // get 'interesting' objects
   FilterResult fr2 = _accessible_list_split_with_filter(fr1.success, _filter_role_cb, (void*)interesting_roles);
   ret = eina_list_merge(ret, fr2.success);
   _accessible_list_free(fr2.failure);

   DEBUG("Candidates after filters count: %d", eina_list_count(ret));
   EINA_LIST_FOREACH_SAFE(ret, l, ln, obj)
   {
      DEBUG("Name:[%s] Role:[%s]", atspi_accessible_get_name(obj, NULL), atspi_accessible_get_role_name(obj, NULL));
   }

   return ret;
}

static void
debug(FlatNaviContext *ctx)
{
   Eina_List *l1, *l2, *line;
   AtspiAccessible *obj;
   int i, l = 0;

   DEBUG("===============================");
   EINA_LIST_FOREACH(ctx->lines, l1, line)
   {
      i = 0;
      DEBUG("==== Line %d ====", l);
      EINA_LIST_FOREACH(line, l2, obj)
      {
         char *name = atspi_accessible_get_name(obj, NULL);
         char *role = atspi_accessible_get_role_name(obj, NULL);
         const ObjectCache *oc = object_cache_get(obj);
         DEBUG("%d %s %s, (%d %d %d %d)", i++, name, role,
               oc->bounds->x, oc->bounds->y, oc->bounds->width, oc->bounds->height);
         if (name) g_free(name);
         if (role) g_free(role);
      }
      l++;
   }
   DEBUG("===============================");
}

static Eina_Bool
_contains(AtspiAccessible *obj, gint x, gint y)
{
   const ObjectCache *oc = object_cache_get(obj);

   if (oc && x >= oc->bounds->x && x <= oc->bounds->x + oc->bounds->width
          && y >= oc->bounds->y && y <= oc->bounds->y + oc->bounds->height)
      {
         DEBUG("INSIDE");
         return EINA_TRUE;
      }

   return EINA_FALSE;

}

Eina_Bool flat_navi_context_current_at_x_y_set( FlatNaviContext *ctx, gint x_cord, gint y_cord , AtspiAccessible **target)
{
   if(!ctx || !target) return EINA_FALSE;

   AtspiAccessible *current = flat_navi_context_current_get(ctx);
   if (current && _contains(current, x_cord, y_cord))
      {
         *target = current;
         return EINA_TRUE;
      }

   Eina_List *l, *l2, *line;
   Eina_Bool found = EINA_FALSE;
   AtspiAccessible *obj;
   EINA_LIST_FOREACH(ctx->lines, l, line)
   {
      EINA_LIST_FOREACH(line, l2, obj)
      if (_contains(obj, x_cord, y_cord))
         {
            found = EINA_TRUE;
            break;
         }
      if (found)
         {
            *target = obj;
            ctx->current_line = l;
            ctx->current = l2;
            break;
         }
   }

   return found;
}

int flat_navi_context_current_children_count_visible_get( FlatNaviContext *ctx)
{
   if(!ctx) return -1;

   int count = 0;
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
   return count;

}

FlatNaviContext *flat_navi_context_create(AtspiAccessible *root)
{
   FlatNaviContext *ret;
   ret = calloc(1, sizeof(FlatNaviContext));
   if (!ret) return NULL;

   ret->root = root;
   ret->candidates = _flat_review_candidates_get(root);
   ret->lines = position_sort(ret->candidates);
   ret->current_line = ret->lines;
   ret->current = eina_list_data_get(ret->current_line);

   debug(ret);

   return ret;
}

void flat_navi_context_free(FlatNaviContext *ctx)
{
   if (!ctx) return;
   Eina_List *l;
   EINA_LIST_FREE(ctx->lines, l)
   {
      eina_list_free(l);
   }
   _accessible_list_free(ctx->candidates);
   free(ctx);
}

AtspiAccessible *flat_navi_context_current_get(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   return eina_list_data_get(ctx->current);
}

Eina_Bool flat_navi_context_current_set(FlatNaviContext *ctx, AtspiAccessible *target)
{
   if(!ctx || !target) return EINA_FALSE;

   Eina_List *l, *l2, *line;
   AtspiAccessible *obj;
   Eina_Bool found = EINA_FALSE;

   EINA_LIST_FOREACH(ctx->lines, l, line)
   {
      EINA_LIST_FOREACH(line, l2, obj)
      {
         if (obj == target)
            {
               found = EINA_TRUE;
               break;
            }
      }
      if (found)
         {
            ctx->current_line = l;
            ctx->current = l2;
            break;
         }
   }

   return found;
}

AtspiAccessible *flat_navi_context_next(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = eina_list_next(ctx->current);

   if (ret)
      {
         ctx->current = ret;
         return eina_list_data_get(ctx->current);
      }
   return NULL;
}

AtspiAccessible *flat_navi_context_prev(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = eina_list_prev(ctx->current);

   if (ret)
      {
         ctx->current = ret;
         return eina_list_data_get(ctx->current);
      }
   return NULL;
}

AtspiAccessible *flat_navi_context_first(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = eina_list_data_get(ctx->current_line);

   if (ret)
      {
         ctx->current = ret;
         return eina_list_data_get(ctx->current);
      }
   return NULL;
}

AtspiAccessible *flat_navi_context_last(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = eina_list_last(ctx->current);

   if (ret)
      {
         ctx->current = ret;
         return eina_list_data_get(ctx->current);
      }
   return NULL;
}

AtspiAccessible *flat_navi_context_line_next(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = eina_list_next(ctx->current_line);
   if (!ret) return NULL;

   ctx->current_line = ret;
   ctx->current = eina_list_data_get(ctx->current_line);

   return eina_list_data_get(ctx->current);
}

AtspiAccessible *flat_navi_context_line_prev(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = eina_list_prev(ctx->current_line);
   if (!ret) return NULL;

   ctx->current_line = ret;
   ctx->current = eina_list_data_get(ctx->current_line);

   return eina_list_data_get(ctx->current);
}

AtspiAccessible *flat_navi_context_line_first(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = ctx->lines;

   ctx->current_line = ret;
   ctx->current = eina_list_data_get(ctx->current_line);

   return eina_list_data_get(ctx->current);
}

AtspiAccessible *flat_navi_context_line_last(FlatNaviContext *ctx)
{
   if(!ctx) return NULL;

   Eina_List *ret = eina_list_last(ctx->current_line);

   ctx->current_line = ret;
   ctx->current = eina_list_data_get(ctx->current_line);

   return eina_list_data_get(ctx->current);
}
