#include <Eina.h>
#include <Ecore.h>

#include "object_cache.h"
#include "logger.h"

static Eina_Hash *cache;
static Ecore_Idler *idler;
static Eina_List *toprocess;
static void *user_data;
static ObjectCacheReadyCb callback;
static int bulk;

static void
_cache_item_free_cb(void *data)
{
   ObjectCache *co = data;
   g_free(co->bounds);
   free(co);
}

static void
_list_obj_unref_and_free(Eina_List *toprocess)
{
   AtspiAccessible *obj;
   EINA_LIST_FREE(toprocess, obj)
   {
      if (obj) g_object_unref(obj);
   }
}

static void
_object_cache_free_internal(void)
{
   if (idler)
      {
         ecore_idler_del(idler);
         idler = NULL;
      }
   if (toprocess)
      {
         _list_obj_unref_and_free(toprocess);
         toprocess = NULL;
      }
   if (cache)
      {
         eina_hash_free(cache);
         cache = NULL;
      }
}

/**
 * Returnes a list of all accessible object implementing AtkCompoment interface
 * Eina_List should be free with eina_list_free function.
 * Every AtspiAccessible in list should be unrefed with g_object_unref.
 */
static Eina_List*
_cache_candidates_list_prepare(AtspiAccessible *root)
{
   DEBUG("START");

   if (!root) return NULL;
   AtspiAccessible *r = atspi_accessible_get_parent(root, NULL);
   if (r)
      DEBUG("From application:%s [%s]", atspi_accessible_get_name(r, NULL), atspi_accessible_get_role_name(r, NULL));
   DEBUG("Preparing candidates from root window: %s [%s]", atspi_accessible_get_name(root, NULL), atspi_accessible_get_role_name(root, NULL));

   Eina_List *toprocess = NULL, *ret = NULL;
   int n, i;

   // Keep ref counter +1 on every object in returned list
   g_object_ref(root);
   toprocess = eina_list_append(toprocess, root);

   while (toprocess)
      {
         AtspiAccessible *obj = eina_list_data_get(toprocess);
         toprocess = eina_list_remove_list(toprocess, toprocess);
         if (!obj)
            {
               DEBUG("NO OBJ");
               continue;
            }
         n = atspi_accessible_get_child_count(obj, NULL);
         for (i = 0; i < n; i++)
            {
               AtspiAccessible *cand = atspi_accessible_get_child_at_index(obj, i, NULL);
               if(cand)
                  toprocess = eina_list_append(toprocess, cand);
            }
         ret = eina_list_append(ret, obj);
      }
   DEBUG("END");

   return ret;
}

static ObjectCache*
_cache_item_construct(AtspiAccessible *obj)
{
   ObjectCache *ret;
   AtspiComponent *comp;

   if (!obj)
      {
         ERROR("object is NULL");
         return NULL;
      }

   ret = calloc(1, sizeof(ObjectCache));
   if (!ret)
      {
         ERROR("out-of memory");
         return NULL;
      }

   comp = atspi_accessible_get_component_iface(obj);
   if (!comp)
      {
         ERROR("Cached Object do not implement Component interface");
      }
   else
      {
         ret->bounds = atspi_component_get_extents(comp, ATSPI_COORD_TYPE_SCREEN, NULL);
         g_object_unref(comp);
      }

   return ret;
}

static Eina_List*
_cache_item_n_cache(Eina_List *objs, int count)
{
   int i = 0;
   Eina_List *ret = objs;
   ObjectCache *oc;
   AtspiAccessible *obj;

   for (i = 0; (i < count) && ret; i++)
      {
         obj = eina_list_data_get(ret);
         ret = eina_list_remove_list(ret, ret);

         oc = _cache_item_construct(obj);
         if (!oc) break;

         eina_hash_add(cache, &obj, oc);
         g_object_unref(obj);
      }

   return ret;
}

static Eina_Hash*
_object_cache_new(void)
{
   return eina_hash_pointer_new(_cache_item_free_cb);
}

void
object_cache_build(AtspiAccessible *root)
{
   DEBUG("START");
   Eina_List *objs;

   _object_cache_free_internal();
   cache = _object_cache_new();
   if (!cache)
      {
         ERROR("ObjectCache: hash table creation failed");
         return;
      }

   objs = _cache_candidates_list_prepare(root);
   _cache_item_n_cache(objs, eina_list_count(objs));

   return;
}

static Eina_Bool
_do_cache(void *data)
{
   toprocess = _cache_item_n_cache(toprocess, bulk);
   idler = NULL;

   if (toprocess)
      idler = ecore_idler_add(_do_cache, NULL);
   else if (callback) callback(user_data);

   return EINA_FALSE;
}

void
object_cache_build_async(AtspiAccessible *root, int bulk_size, ObjectCacheReadyCb cb, void *ud)
{
   DEBUG("START");
   if (idler)
      {
         ERROR("Invalid usage. Async cache build is ongoing...");
         return;
      }
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

   DEBUG("will be preparing async candidates from:[%s] with role:[%s]\n",
         atspi_accessible_get_name(root, NULL),
         atspi_accessible_get_role_name(root, NULL));
   toprocess = _cache_candidates_list_prepare(root);
   idler = ecore_idler_add(_do_cache, NULL);
   DEBUG("END");
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
   if (idler)
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

   ret = eina_hash_find(cache, &obj);
   if (!ret)
      {
         DEBUG("EMPTY");
         // fallback to blocking d-bus call
         ret = _cache_item_construct(obj);
         eina_hash_add(cache, &obj, ret);
      }
   return ret;
}
