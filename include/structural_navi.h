#ifndef SMART_NAVI_STRUCTURAL_NAVI_H
#define SMART_NAVI_STRUCTURAL_NAVI_H_

#include <atspi/atspi.h>

/**
 * @brief Structural navigation submodule.
 *
 * @detail
 * Structural navigation submodule can be used to traverse at-spi object tree
 * hierarchy in unified, position sorted manner.
 */

/**
 * @brief Gets next Accessible object in "structural navigation" hierarchy.
 *
 * @detail
 * Gest next sibling object in accessibile objects tree which is considered as
 * next by taking into account its position on screen. (see @position_sort for
 * more details about visuall sorting)
 *
 * @param current AtspiAccessible instance from which sibling object will be
 * searched.
 *
 * @retun 'Next' AtspiAccessible object pointer or NULL if not found.
 */
AtspiAccessible *structural_navi_same_level_next(AtspiAccessible *current);

/**
 * @brief Gets previous Accessible object in "structural navigation" hierarchy.
 *
 * @detail
 * Gest previous sibling object in accessibile objects tree which is considered as
 * previous by taking into account its position on screen. (see @position_sort for
 * more details about visuall sorting)
 *
 * @param current AtspiAccessible instance from which sibling object will be
 * searched.
 *
 * @retun 'Previous' AtspiAccessible object pointer or NULL if not found.
 */
AtspiAccessible *structural_navi_same_level_prev(AtspiAccessible *current);

/**
 * @brief Gests parent's object.
 *
 * @detail
 * Gets parent object contained within same application. If there is no parent
 * object contained in same application function should return NULL.
 *
 * @param object AtspiAccessible which parent will be searched.
 * @return Parent AtspiAccessible object or NULL if not found or @current
 * is an application object.
 */
AtspiAccessible *structural_navi_level_up(AtspiAccessible *current);

/**
 * @brief Gets child object.
 *
 * @detail
 * Gets AtspiAccessible object considered as first child in structural
 * navigation hierachy. Child will be sorted by its position on the screen.
 * (see @position_sort for more details)
 *
 * @param object AtspiAccessible which child will be searched.
 * @return child AtspiAccessible object or NULL if not found.
 */
AtspiAccessible *structural_navi_level_down(AtspiAccessible *current);

/**
 * @brief Gets next object.
 *
 * @detail
 * Get next AtspiAccessible object from Atspi realation FLOWS_TO.
 *
 * @param object AtspiAccessible
 * @return next AtspiAccessible object or NULL if not found.
 */
AtspiAccessible *structural_navi_app_chain_next(AtspiAccessible *current);

/**
 * @brief Gets previous object.
 *
 * @detail
 * Get previous AtspiAccessible object from Atspi realation FLOWS_FROM.
 *
 * @param object AtspiAccessible
 * @return next AtspiAccessible object or NULL if not found.
 */
AtspiAccessible *structural_navi_app_chain_prev(AtspiAccessible *current);

#endif /* end of include guard: STRUCTURAL_NAVI_H_ */
