#include "flat_navi.h"
#include "position_sort.h"
#include "object_cache.h"
#include "logger.h"
#include <glib.h>
#include <stdlib.h>

struct _FlatNaviContext {
     AtspiAccessible *root;
     GList *current;
     GList *current_line;
     GList *lines;
     GList *candidates;
};

static const AtspiStateType required_states[] = {
     ATSPI_STATE_VISIBLE,
     ATSPI_STATE_SHOWING,
     ATSPI_STATE_LAST_DEFINED
};

static const AtspiRole interesting_roles[] = {
     ATSPI_ROLE_CHECK_BOX,
     ATSPI_ROLE_COLOR_CHOOSER,
     ATSPI_ROLE_COMBO_BOX,
     ATSPI_ROLE_DATE_EDITOR,
     ATSPI_ROLE_FILE_CHOOSER,
     ATSPI_ROLE_FONT_CHOOSER,
     ATSPI_ROLE_HEADER,
     ATSPI_ROLE_HEADING,
     ATSPI_ROLE_ICON,
     ATSPI_ROLE_ENTRY,
     ATSPI_ROLE_LABEL,
     ATSPI_ROLE_LIST_ITEM,
     ATSPI_ROLE_MENU_ITEM,
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
 * @return GList of AtspiAccessible* type. List Should be
 * be free with _accessible_list_free after usage
 *
 * @note obj parameter will also be included (in list head)
 * @note every obj has increased ref. call g_object_unref on every
 * object after usage.
 */
static GList*
_descendants_list_get(AtspiAccessible *obj)
{
   GList *ret = NULL, *toprocess = NULL;
   int i;

   if (!obj) return NULL;

   // to keep all refcounts in ret list +1
   g_object_ref(obj);
   toprocess = g_list_append(toprocess, obj);

   while (toprocess)
     {
        AtspiAccessible *obj = toprocess->data;
        toprocess = g_list_delete_link(toprocess, toprocess);
        int n = atspi_accessible_get_child_count(obj, NULL);
        for (i = 0; i < n; i++)
          {
             AtspiAccessible *child = atspi_accessible_get_child_at_index(obj, i, NULL);
             if (child) toprocess = g_list_append(toprocess, child);
          }

        ret = g_list_append(ret, obj);
     }

   return ret;
}

static void
_accessible_list_free(GList *d)
{
   for (; d != NULL; d = d->next)
      g_object_unref(d->data);
}

typedef struct {
     GList *success;
     GList *failure;
} FilterResult;

typedef gboolean (*Filter_Each_Cb)(const void *container, void *data, void *fdata);

static FilterResult
_accessible_list_split_with_filter(GList *list, Filter_Each_Cb cb, void *user_data)
{
   FilterResult ret = { NULL, NULL};
   GList *l;
   AtspiAccessible *obj;

   for (l = list; l; l = l->next)
     {
        obj = l->data;
        gboolean res = cb(NULL, user_data, obj);
        if (res)
          ret.success = g_list_append(ret.success, obj);
        else
          ret.failure = g_list_append(ret.failure, obj);
     }
   g_list_free(list);

   return ret;
}

static gboolean
_filter_state_cb(const void *container, void *data, void *fdata)
{
   AtspiStateType *state = data;
   gboolean ret = TRUE;
   AtspiAccessible *obj = fdata;
   AtspiStateSet *ss = atspi_accessible_get_state_set(obj);

   while (*state != ATSPI_STATE_LAST_DEFINED)
     {
        if (!atspi_state_set_contains(ss, *state))
          {
             ret = FALSE;
             break;
          }
        state++;
     }

   g_object_unref(ss);
   return ret;
}

static gboolean
_filter_role_cb(const void *container, void *data, void *fdata)
{
   AtspiRole *role = data;
   gboolean ret = FALSE;
   AtspiAccessible *obj = fdata;

   while (*role != ATSPI_ROLE_LAST_DEFINED)
     {
        if (atspi_accessible_get_role(obj, NULL) == *role)
          {
             ret = TRUE;
             break;
          }
        role++;
     }

   return ret;
}

static inline gboolean
_rectangle_intersect(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
   return !(((y1 + h1) <= y2) || (y1 >= (y2 + h2)) || ((x1 + w1) <= x2) || (x1 >= (x2 + w2)));
}

static gboolean
_filter_viewport_cb(const void *container, void *data, void *fdata)
{
   const ObjectCache *oc, *ocr = data;
   AtspiAccessible *obj = fdata;

   oc = object_cache_get(obj);
   if (!oc || !oc->bounds || (oc->bounds->height < 0) || (oc->bounds->width < 0))
     return FALSE;

   // at least one pixel of child have to be in viewport
   return _rectangle_intersect(ocr->bounds->x, ocr->bounds->y, ocr->bounds->width, ocr->bounds->height,
                               oc->bounds->x, oc->bounds->y, oc->bounds->width, oc->bounds->height);
}

static GList*
_flat_review_candidates_get(AtspiAccessible *root)
{
   GList *desc, *ret = NULL;
   FilterResult fr0;
   
   desc = _descendants_list_get(root);

   DEBUG("Descendants: %d", g_list_length(desc));

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
   ret = g_list_concat(ret, fr2.success);
   _accessible_list_free(fr2.failure);

   DEBUG("Candidates: %d", g_list_length(ret));

   return ret;
}

static void
debug(FlatNaviContext *ctx)
{
   GList *l1, *l2, *line;
   AtspiAccessible *obj;
   int i, l = 0;

   DEBUG("===============================");
   for (l1 = ctx->lines; l1 != NULL; l1 = l1->next)
     {
        line = l1->data;
        i = 0;
        DEBUG("==== Line %d ====", l);
        for (l2 = line; l2 != NULL; l2 = l2->next)
          {
             obj = l2->data;
             char *name = atspi_accessible_get_name(obj, NULL);
             char *role = atspi_accessible_get_role_name(obj, NULL);
             const ObjectCache *oc = object_cache_get(obj);
             if (oc) {
             DEBUG("%d %s %s, (%d %d %d %d)", i++, name, role,
                   oc->bounds->x, oc->bounds->y, oc->bounds->width, oc->bounds->height);
             }
             if (name) g_free(name);
             if (role) g_free(role);
          }
        l++;
     }
   DEBUG("===============================");
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
   ret->current = ret->current_line ? ret->current_line->data : NULL;

   debug(ret);

   return ret;
}

void flat_navi_context_free(FlatNaviContext *ctx)
{
   GList *l;
   for (l = ctx->lines; l != NULL; l = l->next)
      g_list_free((GList*)l->data);
   _accessible_list_free(ctx->candidates);
   free(ctx);
}

AtspiAccessible *flat_navi_context_current_get(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;
   return ctx->current->data;
}

gboolean flat_navi_context_current_set(FlatNaviContext *ctx, AtspiAccessible *target)
{
   if(!ctx || !target) return FALSE;

   GList *l, *l2, *line;
   AtspiAccessible *obj;
   gboolean found = FALSE;

   for (l = ctx->lines; l; l = l->next)
     {
        line = l->data;
        for (l2 = line; l2; l2 = l2->next)
          {
             obj = l2->data;
             if (obj == target)
               {
                  found = TRUE;
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
   if (!ctx || !ctx->lines) return NULL;
   GList *ret = ctx->current->next;

   if (ret)
     {
        ctx->current = ret;
        return ctx->current->data;
     }
   return NULL;
}

AtspiAccessible *flat_navi_context_prev(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;
   GList *ret = ctx->current->prev;

   if (ret)
     {
        ctx->current = ret;
        return ctx->current->data;
     }
   return NULL;
}

AtspiAccessible *flat_navi_context_first(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;
   GList *ret = ctx->current_line->data;

   if (ret)
     {
        ctx->current = ret;
        return ctx->current->data;
     }
   return NULL;
}

AtspiAccessible *flat_navi_context_last(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;
   GList *ret = g_list_last(ctx->current);

   if (ret)
     {
        ctx->current = ret;
        return ctx->current->data;
     }
   return NULL;
}

AtspiAccessible *flat_navi_context_line_next(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;

   GList *ret = ctx->current_line->next;
   if (!ret) return NULL;

   ctx->current_line = ret;
   ctx->current = ctx->current_line->data;

   return ctx->current->data;
}

AtspiAccessible *flat_navi_context_line_prev(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;

   GList *ret = ctx->current_line->prev;
   if (!ret) return NULL;

   ctx->current_line = ret;
   ctx->current = ctx->current_line->data;

   return ctx->current->data;
}

AtspiAccessible *flat_navi_context_line_first(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;

   ctx->current_line = ctx->lines;
   ctx->current = ctx->current_line->data;

   return ctx->current->data;
}

AtspiAccessible *flat_navi_context_line_last(FlatNaviContext *ctx)
{
   if (!ctx || !ctx->lines) return NULL;

   ctx->current_line = g_list_last(ctx->current_line);
   ctx->current = ctx->current_line->data;

   return ctx->current->data;
}
