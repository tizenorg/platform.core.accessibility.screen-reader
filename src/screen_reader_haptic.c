#include <device/haptic.h>
#include "logger.h"
#include "smart_notification.h"

static haptic_device_h handle;
static haptic_effect_h effect_handle;

#define RED  "\x1B[31m"
#define RESET "\033[0m"

/**
 * @brief Initializer for haptic module
 *
 */
void haptic_module_init(void)
{
    int num;

    if(!device_haptic_get_count(&num))
      {
         DEBUG(RED"Haptic device received!"RESET);
      }
    else
      {
         ERROR("Cannot receive haptic device count");
         return;
      }

    if(!device_haptic_open(0, &handle))
      {
         DEBUG(RED"Device connected!"RESET);
      }
    else
      {
         ERROR("Cannot open haptic device");
      }
}

/**
 * @brief Disconnect haptic handle
 *
 */
void haptic_module_disconnect(void)
{
    if(!handle)
      {
         ERROR("Haptic handle lost");
         return;
      }
    if(!device_haptic_close(handle))
      {
         DEBUG("Haptic disconnected");
      }
    else
      {
         ERROR("Haptic close error");
      }
}

/**
 * @brief Start vibrations
 *
 */
void haptic_vibrate_start(void)
{
    if(!handle)
      {
         ERROR("Haptic handle lost");
         return;
      }
    if(!device_haptic_vibrate(handle, 1000, 100, &effect_handle))
      {
         DEBUG(RED"Vibrations started!"RESET);
      }
    else
      {
         ERROR("Cannot start vibration");
      }
}

/**
 * @brief Stop vibrations
 *
 */
void haptic_vibrate_stop(void)
{
    if(!handle)
      {
         ERROR("Haptic handle lost");
         return;
      }
    if(!device_haptic_stop(handle, &effect_handle))
      {
         ERROR("Vibrations stopped!");
      }
    else
      {
         DEBUG(RED"Cannot stop vibration"RESET);
      }
}
