#ifndef APP_TRACKER_H_
#define APP_TRACKER_H_

#include <string.h>
#include <atspi/atspi.h>

/**
 * @brief Callback
 */
typedef void (*AppTrackerEventCB)(void *user_data);

/**
 * @brief Register listener on given event type.
 *
 * @param obj AtspiAccessible application object type
 * @param cb callback type
 * @param user_data pointer passed to cb function.
 */
void app_tracker_callback_register(AtspiAccessible *app, AppTrackerEventCB cb, void *user_data);

/**
 * @brief Unregister listener on given event type.
 *
 * @param obj AtspiAccessible application object type
 * @param cb callback type
 * @param user_data pointer passed to cb function.
 */
void app_tracker_callback_unregister(AtspiAccessible *app, AppTrackerEventCB cb, void *user_data);

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
