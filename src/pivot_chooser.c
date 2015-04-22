#include <atspi/atspi.h>
#include "logger.h"

/**
 * @brief Finds first leaf in object hierarchy with given states,
 * starting from object given as parent.
 *
 * This heuristic assumes that focused element have focused
 * parent widgets.
 */
static AtspiAccessible*
_pivot_with_state_top_down_find(AtspiAccessible *parent, AtspiStateType type)
{
   AtspiAccessible *ret = NULL;
   AtspiStateSet *states;
   int i;

   states = atspi_accessible_get_state_set(parent);
   if (!states || atspi_state_set_contains(states, type))
     {
        int n = atspi_accessible_get_child_count(parent, NULL);
        if (n == 0) ret = parent;
        for (i = 0; i < n; i++)
          {
             AtspiAccessible *child = atspi_accessible_get_child_at_index(parent, i, NULL);
             if (!child) continue;

             ret = _pivot_with_state_top_down_find(child, type);

             g_object_unref(child);

             if (ret) break;
          }
     }
   g_object_unref(states);

   return ret;
}

/**
 * @brief Finds first leaf descendant of given object with state @p type
 */
static AtspiAccessible*
_pivot_with_state_flat_find(AtspiAccessible *parent, AtspiStateType type)
{
   GList *candidates = NULL, *queue = NULL;

   // ref object to keep same ref count
   g_object_ref(parent);
   queue = g_list_append(queue, parent);

   while (queue)
     {
        AtspiAccessible *obj = queue->data;
        queue = g_list_delete_link(queue, queue);

        int n = atspi_accessible_get_child_count(obj, NULL);
        if (n == 0)
          candidates = g_list_append(candidates, obj);
        else
          {
             int i;
             for (i = 0; i < n; i++)
               {
                  AtspiAccessible *child = atspi_accessible_get_child_at_index(obj, i, NULL);
                  if (child)
                     queue = g_list_append(queue, child);
               }
             g_object_unref(obj);
          }
     }

   // FIXME sort by (x,y) first ??
   while (candidates)
     {
        AtspiAccessible *obj = candidates->data;
        candidates = g_list_delete_link(candidates, candidates);

        AtspiStateSet *states = atspi_accessible_get_state_set(obj);
        if (states && atspi_state_set_contains(states, type))
          {
             g_object_unref(states);
             g_object_unref(obj);
             g_list_free(candidates);

             return obj;
          }

        g_object_unref(states);
        g_object_unref(obj);
     }

   return NULL;
}

/**
 * @brief Purpose of this methods is to find first visible object in
 * hierarchy
 */
AtspiAccessible *pivot_chooser_pivot_get(AtspiAccessible *win)
{
   AtspiAccessible *ret;

   if (atspi_accessible_get_role(win, NULL) != ATSPI_ROLE_WINDOW)
     {
        ERROR("Pivot search entry point must be a Window!");
        return NULL;
     }

   DEBUG("Finding SHOWING widget using top-down method.");
   ret = _pivot_with_state_top_down_find(win, ATSPI_STATE_SHOWING);
   if (ret) return ret;

   DEBUG("Finding SHOWING widget using top-down method.");
   ret = _pivot_with_state_flat_find(win, ATSPI_STATE_SHOWING);
   if (ret) return ret;

   DEBUG("Finding FOCUSED widget using top-down method.");
   ret = _pivot_with_state_top_down_find(win, ATSPI_STATE_FOCUSED);
   if (ret) return ret;

   DEBUG("Finding FOCUSED widget using flat search method.");
   ret = _pivot_with_state_flat_find(win, ATSPI_STATE_FOCUSED);
   if (ret) return ret;

   return NULL;
}
