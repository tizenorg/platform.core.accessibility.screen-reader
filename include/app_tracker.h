#ifndef APP_TRACKER_H_
#define APP_TRACKER_H_

#include <atspi/atspi.h>

/**
 * @brief Application events
 *
 * @APP_TRACKER_EVENT_VIEW_CHANGE_STARTED - occuring when any of application's
 * UI parts has change its visuals, including moving, resizing widgets, scrolling etc.
 *
 * @APP_TRACKER_EVENT_VIEW_CHANGED occuring when any of application's
 * UI parts has changed its visuals and a prefined time period elapsed
 *
 */
typedef enum
{
   APP_TRACKER_EVENT_VIEW_CHANGE_STARTED,
   APP_TRACKER_EVENT_VIEW_CHANGED,
   APP_TRACKER_EVENT_COUNT
} AppTrackerEventType;

/**
 * @brief Callback
 */
typedef void (*AppTrackerEventCB)(AppTrackerEventType type, void *user_data);

/**
 * @brief Register listener on given event type.
 *
 * @param obj AtspiAccessible application object type
 * @param event type of event to register on
 * @param cb callback type
 * @param user_data pointer passed to cb function.
 */
void app_tracker_callback_register(AtspiAccessible *app, AppTrackerEventType event, AppTrackerEventCB cb, void *user_data);

/**
 * @brief Unregister listener on given event type.
 *
 * @param obj AtspiAccessible application object type
 * @param event type of event to register on
 * @param cb callback type
 * @param user_data pointer passed to cb function.
 */
void app_tracker_callback_unregister(AtspiAccessible *app, AppTrackerEventType event, AppTrackerEventCB cb, void *user_data);

/**
 * @brief Initialize app tracker module.
 *
 * @return initi count.
 *
 * @nore function atspi_init will be called.
 */
int app_tracker_init(void);

/**
 * @brief Shutdown app tracker module
 */
void app_tracker_shutdown(void);

#endif /* end of include guard: APP_TRACKER_H_ */
