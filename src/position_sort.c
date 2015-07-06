#include "logger.h"
#include "object_cache.h"
#include "position_sort.h"


static int
_sort_vertically(const void *a, const void *b)
{
   AtspiAccessible *objA, *objB;
   const ObjectCache *cA, *cB;

   objA = (AtspiAccessible*)a;
   objB = (AtspiAccessible*)b;

   cA = object_cache_get(objA);
   cB = object_cache_get(objB);

   if (!cA || !cB)
      {
         ERROR("Cache is not ready yet");
         return 0;
      }

   if (cA->bounds->y == cB->bounds->y)
      return 0;
   else if (cA->bounds->y > cB->bounds->y)
      return 1;
   else
      return -1;
}

static int
_sort_horizontally(const void *a, const void *b)
{
   AtspiAccessible *objA, *objB;
   const ObjectCache *cA, *cB;

   objA = (AtspiAccessible*)a;
   objB = (AtspiAccessible*)b;

   cA = object_cache_get(objA);
   cB = object_cache_get(objB);

   if (!cA || !cB)
      {
         ERROR("Cache is not ready yet");
         return 0;
      }

   if (cA->bounds->x == cB->bounds->x)
      {
         if (cA->bounds->y > cB->bounds->y)
            return 1;
         if (cA->bounds->y < cB->bounds->y)
            return -1;
         return 0;
      }
   else if (cA->bounds->x > cB->bounds->x)
      return 1;
   else
      return -1;
}

static int
_sort_index(const void *a, const void *b)
{
   int ia, ib;

   AtspiComponent *comp1 = atspi_accessible_get_component_iface((AtspiAccessible*)a);
   AtspiComponent *comp2 = atspi_accessible_get_component_iface((AtspiAccessible*)b);

   ia = atspi_component_get_highlight_index(comp1, NULL);
   ib = atspi_component_get_highlight_index(comp2, NULL);
   g_object_unref(comp1);
   g_object_unref(comp2);

   return ia == ib ? 0 : ia > ib ? 1 : -1;
}

static void
_get_priorities(const Eina_List *objs, Eina_List **priority, Eina_List **candidates, Eina_List **priority_after)
{
   const Eina_List *l;
   AtspiAccessible *obj;
   AtspiComponent *comp;

   EINA_LIST_FOREACH(objs, l, obj)
   {
      if ((comp = atspi_accessible_get_component_iface(obj)) != NULL)
         {
            int index = atspi_component_get_highlight_index(comp, NULL);
            g_clear_object(&comp);
            if (index == 0)
               *candidates = eina_list_append(*candidates, obj);
            else if (index > 0)
              {
                 if (index < INT_MAX/2)
                    *priority = eina_list_append(*priority, obj);
                 else
                    *priority_after = eina_list_append(*priority_after, obj);
              }
         }
      else
         DEBUG("No component interface: skipping %s %s",
               atspi_accessible_get_name(obj, NULL),
               atspi_accessible_get_role_name(obj, NULL));
   }

   *priority = eina_list_sort(*priority, 0, _sort_index);
   *priority_after = eina_list_sort(*priority_after, 0, _sort_index);
}

static Eina_List*
_get_zones(const Eina_List *objs)
{
   Eina_List *candidates = NULL;
   const Eina_List *l;
   AtspiAccessible *obj;
   const ObjectCache *oc;

   EINA_LIST_FOREACH(objs, l, obj)
   {

        oc = object_cache_get(obj);

        if (!oc)
           {
              ERROR("Cache is not ready yet");
              continue;
           }
        // some objects may implement AtspiCompoment interface, however
        // they do not have valid sizes.
        if (!oc->bounds || (oc->bounds->width < 0) || oc->bounds->height < 0)
           {
              DEBUG("Invalid bounds. skipping from zone list: %s %s",
                    atspi_accessible_get_name(obj, NULL),
                    atspi_accessible_get_role_name(obj, NULL));
              continue;
           }
        candidates = eina_list_append(candidates, obj);

   }

   // Sort object by y - coordinate
   return eina_list_sort(candidates, 0, _sort_vertically);
}

static Eina_List*
_get_lines(const Eina_List *objs)
{
   Eina_List *line = NULL, *lines = NULL;
   const Eina_List *l;
   AtspiAccessible *obj;
   const ObjectCache *line_beg;

   EINA_LIST_FOREACH(objs, l, obj)
   {
      if (!line)
         {
            // set first object in line
            line = eina_list_append(line, obj);
            line_beg = object_cache_get(obj);
            continue;
         }

      const ObjectCache *oc = object_cache_get(obj);
      // Object are considered as present in same line, if
      // its y coordinate begins maximum 25% below
      // y coordinate% of first object in line.
      if ((line_beg->bounds->y + (int)(0.25 * (double)line_beg->bounds->height)) >
            oc->bounds->y)
         {
            line = eina_list_append(line, obj);
            continue;
         }
      else
         {
            //finish line & set new line leader
            lines = eina_list_append(lines, line);
            line = NULL;
            line = eina_list_append(line, obj);
            line_beg = object_cache_get(obj);
         }
   }

   // finish last line
   if (line) lines = eina_list_append(lines, line);

   return lines;
}

Eina_List *position_sort(const Eina_List *objs)
{
   Eina_List *l = NULL, *line = NULL, *zones = NULL, *priority = NULL, *priority_after = NULL, *lines = NULL;
   Eina_List *candidates = NULL;
   int i = 0;

   _get_priorities(objs, &priority, &candidates, &priority_after);
   DEBUG("With positive index it is: %d", eina_list_count(priority));
   // Get list of objects occupying place on the screen
   DEBUG("PositionSort: Candidates; %d", eina_list_count(candidates));
   zones = _get_zones(candidates);

   // Cluster all zones into lines - verticaly
   DEBUG("PositionSort: Zones; %d", eina_list_count(zones));
   lines = _get_lines(zones);

   // sort all zones in line - horizontaly
   DEBUG("PositionSort: Lines; %d", eina_list_count(lines));
   EINA_LIST_FOREACH(lines, l, line)
   {
      DEBUG("PositionSort: Line %d: %d items", i++, eina_list_count(line));
      line = eina_list_sort(line, 0, _sort_horizontally);
      eina_list_data_set(l, line);
   }

   if (eina_list_count(priority) > 0)
      lines = eina_list_prepend(lines, priority);

   if (eina_list_count(priority_after) > 0)
      lines = eina_list_append(lines, priority_after);

   if (zones) eina_list_free(zones);

   return lines;
}
