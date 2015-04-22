#include <device/battery.h>
#include <device/display.h>
#include <device/callback.h>
#include "screen_reader.h"
#include "screen_reader_tts.h"
#include "smart_notification.h"
#include "logger.h"

#define CHARGING "Battery charger connected"
#define NOT_CHARGING "Battery charger disconnected"
#define SCREEN_ON "Screen is on"
#define SCREEN_OFF "Screen is off"
#define BATTERY_LOW "Battery level is low"
#define BATTERY_FULL "Battery level is full"
#define BATTERY_CRITICAL "Battery level is critical"

static void device_system_cb(device_callback_e type, void *value, void *user_data);

/**
 * @brief Initializer for smart notifications
 *
 */
void system_notifications_init(void)
{
    // BATTERY LOW/FULL
    device_add_callback(DEVICE_CALLBACK_BATTERY_LEVEL, device_system_cb, NULL);
    // BATTERY CHARGING/NOT-CHARGING
    device_add_callback(DEVICE_CALLBACK_BATTERY_CHARGING, device_system_cb, NULL);
    // SCREEN OFF/ON
    device_add_callback(DEVICE_CALLBACK_DISPLAY_STATE, device_system_cb, NULL);
}

/**
 * @brief Initializer for smart notifications
 *
 */
void system_notifications_shutdown(void)
{
    // BATTERY LOW/FULL
    device_remove_callback(DEVICE_CALLBACK_BATTERY_LEVEL, device_system_cb);
    // BATTERY CHARGING/NOT-CHARGING
    device_remove_callback(DEVICE_CALLBACK_BATTERY_CHARGING, device_system_cb);
    // SCREEN OFF/ON
    device_remove_callback(DEVICE_CALLBACK_DISPLAY_STATE, device_system_cb);
}

/**
 * @brief Device system callback handler
 *
 * @param type Device callback type
 * @param value UNUSED
 * @param user_data UNUSED
 */
static void device_system_cb(device_callback_e type, void *value, void *user_data)
{
    if(type == DEVICE_CALLBACK_BATTERY_LEVEL)
      {
         device_battery_level_e status;
         if(device_battery_get_level_status(&status))
           {
              ERROR("Cannot get battery level status");
              return;
           }

         if(status == DEVICE_BATTERY_LEVEL_LOW)
           {
              tts_speak(BATTERY_LOW, TRUE);
           }
         else if(status == DEVICE_BATTERY_LEVEL_CRITICAL)
           {
              tts_speak(BATTERY_CRITICAL, TRUE);
           }
         else if(status == DEVICE_BATTERY_LEVEL_FULL)
           {
              tts_speak(BATTERY_FULL, TRUE);
           }
      }
    else if(type == DEVICE_CALLBACK_BATTERY_CHARGING)
      {
         bool charging;
         if(device_battery_is_charging(&charging))
           {
              ERROR("Cannot check if battery is charging");
              return;
           }

         if(charging)
           {
              tts_speak(CHARGING, FALSE);
           }
         else
           {
              tts_speak(NOT_CHARGING, FALSE);
           }
      }
    else if(type == DEVICE_CALLBACK_DISPLAY_STATE)
      {
         display_state_e state;
         if(device_display_get_state(&state))
           {
              ERROR("Cannot check if battery is charging");
              return;
           }

         if(state == DISPLAY_STATE_NORMAL)
           {
              tts_speak(SCREEN_ON, FALSE);
           }
         else if(state == DISPLAY_STATE_SCREEN_OFF)
           {
              tts_speak(SCREEN_OFF, FALSE);
           }
      }
}
