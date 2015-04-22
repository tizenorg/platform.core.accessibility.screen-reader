#include "structural_navi.h"
#include "position_sort.h"
#include "logger.h"

static Eina_List*
_atspi_children_list_get(AtspiAccessible *parent)
{
   Eina_List *ret = NULL;
   int count = atspi_accessible_get_child_count(parent, NULL);
   int i;

   for (i = 0; i < count; i++)
     {
        AtspiAccessible *child = atspi_accessible_get_child_at_index(parent, i, NULL);
        if (child) ret = eina_list_append(ret, child);
     }

   return ret;
}

static void
_atspi_children_list_free(Eina_List *children)
{
   AtspiAccessible *obj;

   EINA_LIST_FREE(children, obj)
      g_object_unref(obj);
}

static Eina_List*
_flat_review_get(Eina_List *tosort)
{
   Eina_List *ret = NULL, *lines, *l, *line;

   lines = position_sort(tosort);

   EINA_LIST_FOREACH(lines, l, line)
     {
        ret = eina_list_merge(ret, line);
     }

   eina_list_free(lines);

   return ret;
}

AtspiAccessible *structural_navi_same_level_next(AtspiAccessible *current)
{
    AtspiAccessible *parent;
    AtspiRole role;

    parent = atspi_accessible_get_parent(current, NULL);
    if (!parent) return NULL;

    role = atspi_accessible_get_role(parent, NULL);
    if (role != ATSPI_ROLE_DESKTOP_FRAME)
      {
         Eina_List *children = _atspi_children_list_get(parent);
         Eina_List *me = eina_list_data_find_list(children, current);
         Eina_List *sorted = _flat_review_get(children);
         me = eina_list_data_find_list(sorted, current);
         AtspiAccessible *ret = eina_list_data_get(eina_list_next(me));

         eina_list_free(sorted);
         _atspi_children_list_free(children);

         return ret;
      }
    return NULL;
}

AtspiAccessible *structural_navi_same_level_prev(AtspiAccessible *current)
{
    AtspiAccessible *parent;
    AtspiRole role;

    parent = atspi_accessible_get_parent(current, NULL);
    if (!parent) return NULL;

    role = atspi_accessible_get_role(parent, NULL);
    if (role != ATSPI_ROLE_DESKTOP_FRAME)
      {
         Eina_List *children = _atspi_children_list_get(parent);
         Eina_List *sorted = _flat_review_get(children);
         Eina_List *me = eina_list_data_find_list(sorted, current);
         AtspiAccessible *ret = eina_list_data_get(eina_list_prev(me));

         eina_list_free(sorted);
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

   Eina_List *children = _atspi_children_list_get(current);
   Eina_List *sorted = _flat_review_get(children);

   ret = eina_list_data_get(sorted);

   eina_list_free(sorted);
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
