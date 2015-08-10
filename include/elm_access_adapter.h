#ifndef ELM_ACCESS_KEYBOARD_ADAPTER_H_
#define ELM_ACCESS_KEYBOARD_ADAPTER_H_

#include <Ecore.h>
#include <Ecore_X.h>

/**
 * @brief Send ecore x message with elm access read action
 *
 * @param win keyboard window
 * @param x x coordinate of gesture relative to X root window
 * @param y y coordinate of gesture relative to X root window
 *
 */
void elm_access_adaptor_emit_read(Ecore_X_Window win, int x, int y);

/**
 * @brief Send ecore x message with elm access activate action
 *
 * @param win keyboard window
 * @param x x coordinate of gesture relative to X root window
 * @param y y coordinate of gesture relative to X root window
 *
 */
void elm_access_adaptor_emit_activate(Ecore_X_Window win, int x, int y);

#endif
