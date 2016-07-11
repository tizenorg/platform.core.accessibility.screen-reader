#ifndef SMART_NOTIFICATION_H_
#define SMART_NOTIFICATION_H_

/**
 * @brief Type of notification events.
 *
 * @FOCUS_CHAIN_END_NOTIFICATION_EVENT emitted when
 * currnetly focued or highlighted widget is the last one
 * in focus chain for application current view.
 *
 * @REALIZED_ITEMS_NOTIFICATION_EVENT
 */
enum _Notification_Type
{
   FOCUS_CHAIN_END_NOTIFICATION_EVENT,
   REALIZED_ITEMS_NOTIFICATION_EVENT,
   HIGHLIGHT_NOTIFICATION_EVENT
};

typedef enum _Notification_Type Notification_Type;

/**
 * @brief Initializes notification submodule.
 *
 * @description
 * Notification submodule is resposnisle for providing
 * voice output to end user when on of following events
 * occur:
 *
 * 1. User starts scrolling active application view.
 * 2. User finished scrolling (and all scrolling related
 *    animations has stopped) active application view.
 * 3. When user has navigated to the last widget in focus
 *    chain.
 * 4. After scrolling lists, some of item becomes visible
 *
 * @nore
 * 1 and 2 are handled internally by smart navigation submodule.
 * about events 3 and 4 submodule has to be informed by the developer
 * by using smart_notification API.
 */
void smart_notification_init(void);

/**
 * @brief Notifies smart_notification about UI event.
 *
 * @param nt Type of the occured event.
 * @param start_index Item of the first item that becomes visible
 * after scrolling list widget.
 * @param end_index index of the last item that becomes visible
 * after scrolling list widget.
 *
 * @note start_index and end_index are interpreted only
 * when nt parameter is REALIZED_ITEMS_NOTIFICATION_EVENT
 * @note initializes haptic module.
 */
void smart_notification(Notification_Type nt, int start_index, int end_index);

/**
 * @brief Shutdowns notification subsystem.
 */
void smart_notification_shutdown(void);

#endif /* end of include guard: SMART_NOTIFICATION_H_ */
