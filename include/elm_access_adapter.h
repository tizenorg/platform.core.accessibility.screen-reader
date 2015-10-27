#ifndef ELM_ACCESS_KEYBOARD_ADAPTER_H_
#define ELM_ACCESS_KEYBOARD_ADAPTER_H_

#ifdef X11_ENABLED
#include <Ecore_X.h>
#else
#include <Ecore_Wayland.h>
#endif

/**
 * @brief Send ecore x message with elm access read action
 *
 * @param win keyboard window
 * @param x x coordinate of gesture relative to X root window
 * @param y y coordinate of gesture relative to X root window
 *
 */
#ifdef X11_ENABLED
void elm_access_adaptor_emit_read(Ecore_X_Window win, int x, int y);
#else
void elm_access_adaptor_emit_read(Ecore_Wl_Window *win, int x, int y);
#endif

/**
 * @brief Send ecore x message with elm access activate action
 *
 * @param win keyboard window
 * @param x x coordinate of gesture relative to X root window
 * @param y y coordinate of gesture relative to X root window
 *
 */
#ifdef X11_ENABLED
void elm_access_adaptor_emit_activate(Ecore_X_Window win, int x, int y);
#else
void elm_access_adaptor_emit_activate(Ecore_Wl_Window *win, int x, int y);
#endif

#endif
