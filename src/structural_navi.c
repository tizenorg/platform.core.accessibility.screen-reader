#include "structural_navi.h"
#include "position_sort.h"
#include "logger.h"

static GList*
_atspi_children_list_get(AtspiAccessible *parent)
{
   GList *ret = NULL;
   int count = atspi_accessible_get_child_count(parent, NULL);
   int i;

   for (i = 0; i < count; i++)
     {
        AtspiAccessible *child = atspi_accessible_get_child_at_index(parent, i, NULL);
        if (child) ret = g_list_append(ret, child);
     }

   return ret;
}

static void
_atspi_children_list_free(GList *children)
{
   for (; children; children = children->next)
      g_object_unref(children->data);
}

static GList*
_flat_review_get(GList *tosort)
{
   GList *ret = NULL, *lines, *l, *line;

   lines = position_sort(tosort);

   for (l = lines; l; l = l->next)
     {
        line = l->data;
        ret = g_list_concat(ret, line);
     }

   g_list_free(lines);

   return ret;
}

AtspiAccessible *structural_navi_same_level_next(AtspiAccessible *current)
{
    AtspiAccessible *parent, *ret = NULL;
    AtspiRole role;

    parent = atspi_accessible_get_parent(current, NULL);
    if (!parent) return NULL;

    role = atspi_accessible_get_role(parent, NULL);
    if (role != ATSPI_ROLE_DESKTOP_FRAME)
      {
         GList *children = _atspi_children_list_get(parent);
         GList *sorted = _flat_review_get(children);
         GList *me = g_list_find(sorted, current);
         if (me)
           ret = me->next ? me->next->data : NULL;

         if (sorted) g_list_free(sorted);
         _atspi_children_list_free(children);

         return ret;
      }
    return NULL;
}

AtspiAccessible *structural_navi_same_level_prev(AtspiAccessible *current)
{
    AtspiAccessible *parent, *ret = NULL;
    AtspiRole role;

    parent = atspi_accessible_get_parent(current, NULL);
    if (!parent) return NULL;

    role = atspi_accessible_get_role(parent, NULL);
    if (role != ATSPI_ROLE_DESKTOP_FRAME)
      {
         GList *children = _atspi_children_list_get(parent);
         GList *sorted = _flat_review_get(children);
         GList *me = g_list_find(sorted, current);
         if (me)
           ret = me->prev ? me->prev->data : NULL;

         if (sorted) g_list_free(sorted);
         _atspi_children_list_free(children);

         return ret;
      }
    return NULL;
}

AtspiAccessible *structural_navi_level_up(AtspiAccessible *current)
{
    AtspiAccessible *parent;
    AtspiRole role;

    parent = atspi_accessible_get_parent(current, NULL);
    if (!parent) return NULL;

    role = atspi_accessible_get_role(parent, NULL);
    if (role != ATSPI_ROLE_DESKTOP_FRAME)
      {
         return parent;
      }
    return NULL;
}

AtspiAccessible *structural_navi_level_down(AtspiAccessible *current)
{
   AtspiAccessible *ret;

   GList *children = _atspi_children_list_get(current);
   GList *sorted = _flat_review_get(children);

   ret = sorted ? sorted->data : NULL;

   if (sorted) g_list_free(sorted);
   _atspi_children_list_free(children);

   return ret;
}

static AtspiAccessible*
_navi_app_chain_next(AtspiAccessible *current, AtspiRelationType search_type)
{
   GArray *relations;
   AtspiAccessible *ret = NULL;
   AtspiRelation *relation;
   AtspiRelationType type;
   int i;

   relations = atspi_accessible_get_relation_set(current, NULL);

    for (i = 0; i < relations->len; i++)
      {
         relation = g_array_index (relations, AtspiRelation*, i);
         type = atspi_relation_get_relation_type(relation);

         if (type == search_type)
           {
              ret = atspi_relation_get_target(relation, 0);
              break;
           }
      }

   g_array_free(relations, TRUE);
   return ret;
}

AtspiAccessible *structural_navi_app_chain_next(AtspiAccessible *current)
{
   return _navi_app_chain_next(current, ATSPI_RELATION_FLOWS_TO);
}

AtspiAccessible *structural_navi_app_chain_prev(AtspiAccessible *current)
{
   return _navi_app_chain_next(current, ATSPI_RELATION_FLOWS_FROM);
}
