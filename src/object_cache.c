#include "object_cache.h"
#include "logger.h"

#include <glib.h>

static GHashTable *cache;
static int idler = -1;
static GList *toprocess;
static void *user_data;
static ObjectCacheReadyCb callback;
static int bulk;

static void
_cache_item_free_cb(gpointer data)
{
   if (!data) return;
   ObjectCache *co = data;
   g_free(co->bounds);
   g_free(co);
}

static void
_list_obj_unref_and_free(GList *toprocess)
{
   for (; toprocess; toprocess = toprocess->next)
      g_object_unref(toprocess->data);
}

static void
_object_cache_free_internal(void)
{
   if (idler > 0)
     {
        g_source_remove(idler);
        idler = -1;
     }
   if (toprocess)
     {
        _list_obj_unref_and_free(toprocess);
        toprocess = NULL;
     }
   if (cache)
     {
        g_hash_table_destroy(cache);
        cache = NULL;
     }
}

/**
 * Returnes a list of all accessible object implementing AtkCompoment interface
 * GList should be free with g_list_Free function.
 * Every AtspiAccessible in list should be unrefed with g_object_unref.
 */
static GList*
_cache_candidates_list_prepare(AtspiAccessible *root)
{
   GList *toprocess = NULL, *ret = NULL;
   int n, i;

   if (!root) return NULL;

   // Keep ref counter +1 on every object in returned list
   g_object_ref(root);
   toprocess = g_list_append(toprocess, root);

   while (toprocess)
     {
        AtspiAccessible *obj = toprocess->data;
        toprocess = g_list_delete_link(toprocess, toprocess);
        if (!obj)
          continue;
        n = atspi_accessible_get_child_count(obj, NULL);
        for (i = 0; i < n; i++)
          {
             AtspiAccessible *cand = atspi_accessible_get_child_at_index(obj, i, NULL);
             if(cand)
               toprocess = g_list_append(toprocess, cand);
          }
        ret = g_list_append(ret, obj);
     }

   return ret;
}

static ObjectCache*
_cache_item_construct(AtspiAccessible *obj)
{
   ObjectCache *ret;
   AtspiComponent *comp;

   if (!obj)
     return NULL;

   ret = g_new0(ObjectCache, 1);
   if (!ret)
     {
        ERROR("out-of memory");
        return NULL;
     }

   comp = atspi_accessible_get_component(obj);
   if (!comp) {
     ERROR("Cached Object do not implement Component interface");
   }
   else {
      ret->bounds = atspi_component_get_extents(comp, ATSPI_COORD_TYPE_SCREEN, NULL);
      g_object_unref(comp);
   }

   return ret;
}

static GList*
_cache_item_n_cache(GList *objs, int count)
{
   int i = 0;
   GList *ret = objs;
   ObjectCache *oc;
   AtspiAccessible *obj;

   for (i = 0; (i < count) && ret; i++)
     {
        obj = ret->data;
        ret = g_list_delete_link(ret, ret);

        oc = _cache_item_construct(obj);
        if (!oc) break;

        g_hash_table_insert (cache, obj, oc);

        g_object_unref(obj);
     }

   return ret;
}

static GHashTable*
_object_cache_new(void)
{
   return g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, _cache_item_free_cb);
}

void
object_cache_build(AtspiAccessible *root)
{
   GList *objs;

   _object_cache_free_internal();
   cache = _object_cache_new();
   if (!cache)
     {
        ERROR("ObjectCache: hash table creation failed");
        return;
     }

   objs = _cache_candidates_list_prepare(root);
   _cache_item_n_cache(objs, g_list_length(objs));

   return;
}

static gboolean
_do_cache(void *data)
{
   toprocess = _cache_item_n_cache(toprocess, bulk);
   idler = -1;

   if (toprocess)
     idler = g_idle_add(_do_cache, NULL);
   else if (callback) callback(user_data);

   return FALSE;
}

void
object_cache_build_async(AtspiAccessible *root, int bulk_size, ObjectCacheReadyCb cb, void *ud)
{
   _object_cache_free_internal();

   callback = cb;
   user_data = ud;
   bulk = bulk_size;

   cache = _object_cache_new();
   if (!cache)
     {
        ERROR("ObjectCache: hash table creation failed");
        return;
     }

   toprocess = _cache_candidates_list_prepare(root);
   idler = g_idle_add(_do_cache, NULL);

   return;
}

void
object_cache_shutdown(void)
{
   _object_cache_free_internal();
}

const ObjectCache*
object_cache_get(AtspiAccessible *obj)
{
   ObjectCache *ret = NULL;
   if (idler > 0)
     {
        ERROR("Invalid usage. Async cache build is ongoing...");
        return NULL;
     }

   if (!cache)
     {
        cache = _object_cache_new();
        if (!cache)
          {
             ERROR("ObjectCache: hash table creation failed");
             return NULL;
          }
     }

   ret = g_hash_table_lookup(cache, obj);
   if (!ret)
     {
        // fallback to blocking d-bus call
        ret = _cache_item_construct(obj);
        g_hash_table_insert(cache, obj, ret);
     }

   return ret;
}
