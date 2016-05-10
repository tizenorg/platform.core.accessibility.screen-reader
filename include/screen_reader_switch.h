#ifndef _SCREEN_READER_SWITCH
#define _SCREEN_READER_SWITCH

/**
 * @brief Set "ScreenReaderEnabled" status of AT-SPI framework
 *
 * Function used to iniform applications that screen reader has been enabled in
 * the system.
 *
 * ScreenReaderEnabled property refers to
 * org.a11y.Bus bus at /org/a11y/bus object path in org.a11y.Status interface
 *
 * @param value EINA_TRUE if screen reader should be enabled, EINA_FALSE otherwise.
 * @return EINA_TRUE if setting 'ScreenReaderEnabled' to value has successed,
 * EINA_FALSE otherwise
 */
Eina_Bool screen_reader_switch_enabled_set(Eina_Bool value);


/**
 * @brief Get "ScreenReaderEnabled" status of AT-SPI framework
 *
 * ScreenReaderEnabled property refers to
 * org.a11y.Bus bus at /org/a11y/bus object path in org.a11y.Status interface
 *
 * @param value EINA_TRUE if screen reader is enabled, EINA_FALSE otherwise.
 * @return EINA_TRUE if getting of value has successed,
 * EINA_FALSE otherwise
 */
Eina_Bool screen_reader_switch_enabled_get(Eina_Bool *value);

/**
 * @brief Set "ScreenReaderEnabled" status to window manager module
 *
 * Function used to inform window manager module that screen reader is enabled
 *
 * ScreenReaderEnabled property refers to
 * org.wm bus at org/wm object path in org.tizen.wm interface
 *
 * @param value EINA_TRUE if screen reader should be enabled and hence the gesture layer,
 * EINA_FALSE otherwise.
 * @return EINA_TRUE if setting 'ScreenReaderEnabled' to value has successed,
 * EINA_FALSE otherwise
 */
Eina_Bool screen_reader_switch_wm_enabled_set(Eina_Bool value);


#endif
