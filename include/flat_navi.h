#ifndef FLAT_NAVI_H_
#define FLAT_NAVI_H_

#include <atspi/atspi.h>
#include <Eina.h>

typedef struct _FlatNaviContext FlatNaviContext;

/**
 * @brief Creates new FlatNaviContext.
 *
 * FlatNaviContext organizes UI elements in a way that is similar to
 * reading orded (from left to right) and allow it traversing.
 *
 * @param root AtspiAccessible root object on which descendants FlatNaviContext
 * will be builded.
 *
 * @return New FlatNaviContext. Should be free with flat_navi_context_free.
 *
 * @note Context will use positions obtain from object_cache. It is up
 * to developer to guarantee that cache is up-to-date.
 * @note Context will use position_sort to sort elements into lines.
 * @note Context will use heuristics to those choose elements of UI
 * that will be segmented into lines. Plese refer to source code for further details.
 * @note default current element is first object in first line.
 *
 */
FlatNaviContext *flat_navi_context_create(AtspiAccessible *root);

/**
 * @brief Frees FlatNaviContext
 * @param ctx FlatNaviContext
 */
void flat_navi_context_free(FlatNaviContext *ctx);

/**
 * Advances to next element in natural reading order and returns
 * new current element.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 *
 * @note If current element is last in line function returns NULL
 */
AtspiAccessible *flat_navi_context_next(FlatNaviContext *ctx);

/**
 * Advances to previous element in natural reading order and returns
 * new current element.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 *
 * @note If current element is first in line function returns NULL
 */
AtspiAccessible *flat_navi_context_prev(FlatNaviContext *ctx);

/**
 * Advances to last element in current line.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 */
AtspiAccessible *flat_navi_context_last(FlatNaviContext *ctx);

/**
 * Advances to first element in current line.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 */
AtspiAccessible *flat_navi_context_first(FlatNaviContext *ctx);

/**
 * Get current element
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 */
AtspiAccessible *flat_navi_context_current_get(FlatNaviContext *ctx);

/**
 * Set current element.
 *
 * @note There might be situation when target object is not in FlatNaviContext.
 * (eg. was filtered by internal heuristic) in this situations function returns
 * EINA_FALSE.
 *
 * @param ctx FlatNaviContext
 * @param target new current FlatNaviContext object
 *
 * @return EINA_TRUE is operation successed, EINA_FALSE otherwise
 */
Eina_Bool flat_navi_context_current_set(FlatNaviContext *ctx, AtspiAccessible *target);

/**
 * Advances to previous line in natural reading order and returns
 * new current element.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 *
 * @note If current line is first one, function returns NULL
 * @note current element will be first of line.
 */
AtspiAccessible *flat_navi_context_line_prev(FlatNaviContext *ctx);

/**
 * Advances to next line in natural reading order and returns
 * new current element.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 *
 * @note If current line is last one, function returns NULL
 * @note current element will be first of line.
 */
AtspiAccessible *flat_navi_context_line_next(FlatNaviContext *ctx);

/**
 * Advances to first line.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 *
 * @note current element will be first of line.
 */
AtspiAccessible *flat_navi_context_line_first(FlatNaviContext *ctx);

/**
 * Advances to last line.
 *
 * @param ctx FlatNaviContext
 *
 * @return AtspiAccessible* pointer to current object
 *
 * @note current element will be first of line.
 */
AtspiAccessible *flat_navi_context_line_last(FlatNaviContext *ctx);

/**
 * Set current context at given position.
 *
 * @param ctx FlatNaviContext
 *
 * @param x_cord gint X coordinate
 *
 * @param y_cord gint Y coordinate
 *
 * @param obj AtspiAccessible Reference to object on point
 *
 * @return Eina_Bool true on success
 *
 * @note current element will be first of line.
 */
Eina_Bool flat_navi_context_current_at_x_y_set( FlatNaviContext *ctx, gint x_cord, gint y_cord , AtspiAccessible **obj);

#endif /* end of include guard: FLAT_NAVI_H_ */
