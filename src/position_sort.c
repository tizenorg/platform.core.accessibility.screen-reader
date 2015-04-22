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

static GList*
_get_zones(const GList *objs)
{
   GList *candidates = NULL;
   const GList *l;
   AtspiAccessible *obj;
   AtspiComponent *comp;
   const ObjectCache *oc;

   for (l = objs; l; l = l->next)
     {
        obj = l->data;
        if ((comp = atspi_accessible_get_component(obj)) != NULL)
          {
             oc = object_cache_get(obj);

             // some objects may implement AtspiCompoment interface, however
             // they do not have valid sizes.
             if (!oc->bounds || (oc->bounds->width < 0) || oc->bounds->height < 0)
               {
                  DEBUG("Invalid bounds. skipping from zone list: %s %s",
                   atspi_accessible_get_name(obj, NULL),
                   atspi_accessible_get_role_name(obj, NULL));
                  continue;
               }
             candidates = g_list_append(candidates, obj);
          }
        else
          DEBUG("No component interface: skipping %s %s",
                atspi_accessible_get_name(obj, NULL),
                atspi_accessible_get_role_name(obj, NULL));
     }

   // Sort object by y - coordinate
   return g_list_sort(candidates, _sort_vertically);
}

static GList*
_get_lines(const GList *objs)
{
   GList *line = NULL, *lines = NULL;
   const GList *l;
   AtspiAccessible *obj;
   const ObjectCache *line_beg;

   for (l = objs; l; l = l->next)
     {
        obj = l->data;
        if (!line) {
          // set first object in line
          line = g_list_append(line, obj);
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
             line = g_list_append(line, obj);
             continue;
          }
        else
          {
             //finish line & set new line leader
             lines = g_list_append(lines, line);
             line = NULL;
             line = g_list_append(line, obj);
             line_beg = object_cache_get(obj);
          }
     }

   // finish last line
   if (line) lines = g_list_append(lines, line);

   return lines;
}

GList *position_sort(const GList *objs)
{
   GList *l, *line, *zones, *lines = NULL;
   int i = 0;

   // Get list of objects occupying place on the screen
   DEBUG("PositionSort: Candidates; %d", g_list_length((GList*)objs));
   zones = _get_zones(objs);

   // Cluster all zones into lines - verticaly
   DEBUG("PositionSort: Zones; %d", g_list_length(zones));
   lines = _get_lines(zones);

   // sort all zones in line - horizontaly
   DEBUG("PositionSort: Lines; %d", g_list_length(lines));
   for (l = lines; l; l = l->next)
     {
        line = l->data;
        DEBUG("PositionSort: Line %d: %d items", i++, g_list_length(line));
        line = g_list_sort(line, _sort_horizontally);
        l->data = line;
     }

   if (zones) g_list_free(zones);

   return lines;
}
