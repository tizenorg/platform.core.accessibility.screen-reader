#ifndef SMART_NAVI_OBJECT_CACHE_H_H
#define SMART_NAVI_OBJECT_CACHE_H_H

#include <atspi/atspi.h>

typedef struct
{
   // Boundaries of object taken from atspi_component_get_extents function.
   // NULL if object do not implemetn AtspiComponent interface
   AtspiRect *bounds;
} ObjectCache;

typedef void (*ObjectCacheReadyCb)(void *data);

/**
 * @brief Recursivly build ObjectCache structures for root Accessible
 * object from and its descendants. (Children's children also, etc.)
 *
 * @param root starting object.
 *
 * @remarks This function may block main-loop for significant ammount of time.
 *          Flushes all previously cached items.
 */
void object_cache_build(AtspiAccessible *root);

/**
 * @brief Recursivly build ObjectCache structures for ever Accessible
 * object from root and its all descendants.
 *
 * @param root starting object.
 * @param bulk_size Ammount of AtspiAccessible that will be cached on every
 * ecore_main_loop entering into 'idle' state.
 * @param cb function to be called after creating cache is done.
 * @param user_data passed to cb
 *
 * @remarks Flushes all previously cached items.
 */
void object_cache_build_async(AtspiAccessible *root, int bulk_size, ObjectCacheReadyCb cb, void *user_data);

/**
 * @brief Gets and ObjectCache item from accessible object
 *
 * @param obj AtspiAccessible
 *
 * @remarks If obj is not in cache function will block main loop and build ObjectCache
 * struct.
 */
const ObjectCache *object_cache_get(AtspiAccessible *obj);

/**
 * @brief Shoutdown cache.
 *
 * Function will free all gathered caches.
 *
 * @remarks Use it after You have used any of above object_cache_* functions.
 */
void object_cache_shutdown(void);

#endif /* end of include guard: OBJECT_CACHE_H_H */
