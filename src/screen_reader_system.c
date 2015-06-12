#define _GNU_SOURCE

#include <device/battery.h>
#include <device/display.h>
#include <device/callback.h>
#include <bluetooth.h>
#include <tapi_common.h>
#include <TelNetwork.h>
#include <vconf.h>
#include <wifi.h>
#include <notification.h>
#include <notification_list.h>

#include "screen_reader.h"
#include "screen_reader_tts.h"
#include "smart_notification.h"
#include "logger.h"

#define CHARGING "Battery charger connected"
#define NOT_CHARGING "Battery charger disconnected"
#define SCREEN_ON "Screen is on"
#define SCREEN_OFF "Screen is off"
#define BATTERY_EMPTY "Battery level is empty"
#define BATTERY_LOW "Battery level is low"
#define BATTERY_HIGH "Battery level is high"
#define BATTERY_FULL "Battery level is full"
#define BATTERY_CRITICAL "Battery level is critical"
#define MAX_SIM_COUNT 2
#define DATE_TIME_BUFFER_SIZE 26
#define BATTERY_INFO_CHARGING "Charging."
#define BATTERY_INFO_BATTERY_STR "Battery"
#define BATTERY_STR_FULL_CHARGED "Full charged"
#define SIGNAL_STR_SIGNAL_STRENGTH "Signal strength equals:"
#define SIGNAL_STR_SIMCARD "Simcard"
#define NETWORK_SERVICE_STR_LTE "LTE"
#define NETWORK_SERVICE_STR_HSDPA "HSDPA"
#define NETWORK_SERVICE_STR_3G "3G"
#define NETWORK_SERVICE_STR_EDGE "EDGE"
#define NETWORK_SERVICE_STR_25G "2.5G"
#define NETWORK_SERVICE_STR_2G "2G"
#define NETWORK_SERVICE_STR_SEARCHING "searching..."
#define NETWORK_SERVICE_STR_EMERGENCY "emergency"
#define NETWORK_SERVICE_STR_NO_SERVICE "no service"
#define NETWORK_SERVICE_STR_UNKNOWN "unknown"
#define NETWORK_TYPE_WIFI "Wi fi"
#define WIFI_SIGNAL_GOOD "Good"
#define WIFI_SIGNAL_MEDIUM "Medium"
#define WIFI_SIGNAL_WEAK "Weak"
#define WIFI_SIGNAL_POOR "Poor"
#define WIFI_NO_SIGNAL "No signal"
#define NOTI_NOTIFICATIONS_UNREAD_0 "No event"
#define NOTI_NOTIFICATIONS_UNREAD_1 "1 notification unread"
#define NOTI_NOTIFICATIONS_UNREAD_MANY "notifications unread"
#define BT_BLUETOOTH_OFF "Bluetooth off"
#define BT_NO_DEVICES_CONNECTED "Nothing connected"
#define BT_DEVICES_CONNECTED_COUNT "devices connected"

TapiHandle *tapi_handle[MAX_SIM_COUNT + 1] = {0, };

static void device_system_cb(device_callback_e type, void *value, void *user_data);


static void tapi_init(void)
{
   int i = 0;
   char **cp_list = tel_get_cp_name_list();

   DEBUG("TAPI INIT");
   for(i = 0; cp_list[i]; ++i)
      {
         tapi_handle[i] = tel_init(cp_list[i]);
         DEBUG("CP_LIST %d = %s", i, cp_list[i]);
      }

}

/**
 * @brief Initializer for smart notifications
 *
 */
void system_notifications_init(void)
{
   DEBUG("******************** START ********************");
   int ret = -1;

   // BATTERY LOW/FULL
   device_add_callback(DEVICE_CALLBACK_BATTERY_LEVEL, device_system_cb, NULL);
   // BATTERY CHARGING/NOT-CHARGING
   device_add_callback(DEVICE_CALLBACK_BATTERY_CHARGING, device_system_cb, NULL);
   // SCREEN OFF/ON
   device_add_callback(DEVICE_CALLBACK_DISPLAY_STATE, device_system_cb, NULL);

   ret = bt_initialize();
   if (ret != BT_ERROR_NONE)
      {
         ERROR("ret == %d", ret);
      }

   ret =  wifi_initialize();
   if (ret != WIFI_ERROR_NONE)
      {
         ERROR("ret == %d", ret);
      }


   tapi_init();

   DEBUG(" ********************* END ********************* ");
}

/**
 * @brief Initializer for smart notifications
 *
 */
void system_notifications_shutdown(void)
{
   int ret = -1;

   // BATTERY LOW/FULL
   device_remove_callback(DEVICE_CALLBACK_BATTERY_LEVEL, device_system_cb);
   // BATTERY CHARGING/NOT-CHARGING
   device_remove_callback(DEVICE_CALLBACK_BATTERY_CHARGING, device_system_cb);
   // SCREEN OFF/ON
   device_remove_callback(DEVICE_CALLBACK_DISPLAY_STATE, device_system_cb);

   ret = bt_deinitialize();
   if (ret != BT_ERROR_NONE)
      {
         ERROR("ret == %d", ret);
      }

   ret = wifi_deinitialize();
   if (ret != WIFI_ERROR_NONE)
      {
         ERROR("ret == %d", ret);
         return;
      }
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
               tts_speak(BATTERY_LOW, EINA_TRUE);
            }
         else if(status == DEVICE_BATTERY_LEVEL_CRITICAL)
            {
               tts_speak(BATTERY_CRITICAL, EINA_TRUE);
            }
         else if(status == DEVICE_BATTERY_LEVEL_FULL)
            {
               tts_speak(BATTERY_FULL, EINA_TRUE);
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
               tts_speak(CHARGING, EINA_FALSE);
            }
         else
            {
               tts_speak(NOT_CHARGING, EINA_FALSE);
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
               tts_speak(SCREEN_ON, EINA_FALSE);
            }
         else if(state == DISPLAY_STATE_SCREEN_OFF)
            {
               tts_speak(SCREEN_OFF, EINA_FALSE);
            }
      }
}

// ******************************** Indicator info ********************************** //

static int _read_text_get(char *key)
{
   int read_text = 0;
   int ret = -1;

   ret = vconf_get_bool(key, &read_text);
   if (ret != 0)
      {
         ERROR("ret == %d", ret);
         return true;
      }

   return read_text;
}


void device_time_get(void)
{
   char buffer[DATE_TIME_BUFFER_SIZE];
   int disp_12_24 = VCONFKEY_TIME_FORMAT_12;
   int ret = -1;
   time_t rawtime = 0;
   struct tm *timeinfo = NULL;

   if (!_read_text_get(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_INDICATOR_INFORMATION_TIME))
      {
         return;
      }

   time(&rawtime );
   timeinfo = localtime ( &rawtime );

   ret = vconf_get_int(VCONFKEY_REGIONFORMAT_TIME1224, &disp_12_24);
   if (ret != 0)
      {
         ERROR("ret == %d", ret);
      }

   if (disp_12_24 == VCONFKEY_TIME_FORMAT_24)
      {
         strftime(buffer, DATE_TIME_BUFFER_SIZE, "Current time: %H %M",
                  timeinfo);
      }
   else
      {
         strftime(buffer, DATE_TIME_BUFFER_SIZE, "Current time: %I %M %p",
                  timeinfo);
      }

   DEBUG("Text to say: %s", buffer);
   tts_speak(buffer, EINA_FALSE);
}


char *device_error_to_string(int e)
{
   switch(e)
      {
      case DEVICE_ERROR_NONE:
         return "DEVICE_ERROR_NONE";
         break;

      case DEVICE_ERROR_OPERATION_FAILED:
         return "DEVICE_ERROR_OPERATION_FAILED";
         break;

      case DEVICE_ERROR_PERMISSION_DENIED:
         return "DEVICE_ERROR_PERMISSION_DENIED";
         break;

      case DEVICE_ERROR_INVALID_PARAMETER:
         return "DEVICE_ERROR_INVALID_PARAMETER";
         break;

      case DEVICE_ERROR_ALREADY_IN_PROGRESS:
         return "DEVICE_ERROR_ALREADY_IN_PROGRESS";
         break;

      case DEVICE_ERROR_NOT_SUPPORTED:
         return "DEVICE_ERROR_NOT_SUPPORTED";
         break;

      case DEVICE_ERROR_RESOURCE_BUSY:
         return "DEVICE_ERROR_RESOURCE_BUSY";
         break;

      case DEVICE_ERROR_NOT_INITIALIZED:
         return "DEVICE_ERROR_NOT_INITIALIZED";
         break;

      default:
         return NETWORK_SERVICE_STR_UNKNOWN;
         break;
      }

   return NETWORK_SERVICE_STR_UNKNOWN;
}

void device_battery_get(void)
{
   char *buffer = NULL;
   char *charging_text = NULL;
   int percent;
   bool is_charging = false;
   int ret = -1;

   if (!_read_text_get(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_INDICATOR_INFORMATION_BATTERY))
      {
         return;
      }

   ret = device_battery_is_charging(&is_charging);
   if (ret != DEVICE_ERROR_NONE)
      {
         ERROR("ret == %s", device_error_to_string(ret));
      }

   if (is_charging)
      {
         charging_text = BATTERY_INFO_CHARGING;
      }
   else
      {
         charging_text = "";
      }

   ret = device_battery_get_percent(&percent);
   if (ret != DEVICE_ERROR_NONE)
      {
         ERROR("ret == %s", device_error_to_string(ret));
         return;
      }

   if(percent == 100)
      {
         if (!asprintf(&buffer, "%s %s", charging_text, BATTERY_STR_FULL_CHARGED))
            {
               ERROR("Buffer length == 0");
               return;
            }
      }
   else
      {
         if (!asprintf(&buffer, "%s %d %% %s", charging_text, percent, BATTERY_INFO_BATTERY_STR))
            {
               ERROR("Buffer length == 0");
               return;
            }

      }

   if (!buffer)
      {
         ERROR("buf == NULL");
         return;
      }

   DEBUG("Text to say: %s", buffer);
   tts_speak(buffer, EINA_FALSE);
   free(buffer);
}


static void _signal_strength_sim_get(void)
{
   int i = 0;
   int val = 0;
   int ret = -1;
   int sim_card_count = 0;
   Eina_Strbuf *str_buf = NULL;
   char *buffer = NULL;
   int service_type = TAPI_NETWORK_SERVICE_TYPE_UNKNOWN;
   char *service_type_text = NULL;

   str_buf = eina_strbuf_new();

   for (i = 0; tapi_handle[i]; ++i)
      {
         ++sim_card_count;
      }

   for (i = 0; tapi_handle[i]; ++i)
      {
         ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL, &val);
         if (ret != TAPI_API_SUCCESS)
            {
               ERROR("Can not get %s",TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL);
               val = 0;
            }

         if (sim_card_count > 1)
            eina_strbuf_append_printf(str_buf, "%s %d %s %d; ", SIGNAL_STR_SIMCARD, i + 1, SIGNAL_STR_SIGNAL_STRENGTH, val);
         else
            eina_strbuf_append_printf(str_buf, "%s %d; ", SIGNAL_STR_SIGNAL_STRENGTH, val);
         DEBUG("sim: %d TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL %d", i, val);


         ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_SERVICE_TYPE, &service_type);
         if (ret != TAPI_API_SUCCESS)
            {
               ERROR("Can not get %s",TAPI_PROP_NETWORK_SERVICE_TYPE);
            }

         switch(service_type)
            {
            case TAPI_NETWORK_SERVICE_TYPE_UNKNOWN:
               service_type_text = NETWORK_SERVICE_STR_UNKNOWN;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
               service_type_text = NETWORK_SERVICE_STR_NO_SERVICE;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
               service_type_text = NETWORK_SERVICE_STR_EMERGENCY;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
               service_type_text = NETWORK_SERVICE_STR_SEARCHING;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_2G:
               service_type_text = NETWORK_SERVICE_STR_2G;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_2_5G:
               service_type_text = NETWORK_SERVICE_STR_25G;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_2_5G_EDGE:
               service_type_text = NETWORK_SERVICE_STR_EDGE;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_3G:
               service_type_text = NETWORK_SERVICE_STR_3G;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_HSDPA:
               service_type_text = NETWORK_SERVICE_STR_HSDPA;
               break;

            case TAPI_NETWORK_SERVICE_TYPE_LTE:
               service_type_text = NETWORK_SERVICE_STR_LTE;
               break;
            }

         eina_strbuf_append_printf(str_buf, " Service type: %s.", service_type_text);
      }

   buffer = eina_strbuf_string_steal(str_buf);

   DEBUG("Text to say: %s", buffer);
   tts_speak(buffer, EINA_FALSE);

   eina_strbuf_string_free(str_buf);
   free(buffer);
}

static void _signal_strength_wifi_get(void)
{
   int val = 0;
   int ret = -1;
   char *buffer = NULL;
   char *wifi_text = NULL;
   bool wifi_activated = false;
   wifi_ap_h ap = NULL;

   ret = wifi_is_activated(&wifi_activated);
   if(ret != WIFI_ERROR_NONE)
      {
         ERROR("ret == %d", ret);
         return;
      }

   if(wifi_activated)
      {
         ret = wifi_get_connected_ap(&ap);
         if(ret != WIFI_ERROR_NONE)
            {
               ERROR("ret == %d", ret);
               return;
            }

         if(!ap)
            {
               DEBUG("Text to say: %s %s",NETWORK_TYPE_WIFI, "Not connected");

               if (!asprintf(&buffer, " %s, %s", NETWORK_TYPE_WIFI, "Not connected"))
                  {
                     ERROR("buffer length == 0");
                     return;
                  }

               tts_speak(buffer, EINA_FALSE);
               free(buffer);
               return;
            }

         ret = wifi_ap_get_rssi(ap, &val);
         if(ret != WIFI_ERROR_NONE)
            {
               ERROR("ret == %d", ret);
               wifi_ap_destroy(ap);
               return;
            }

         switch(val)
            {
            case 0:
               wifi_text = WIFI_NO_SIGNAL;
               break;

            case 1:
               wifi_text = WIFI_SIGNAL_POOR;
               break;

            case 2:
               wifi_text = WIFI_SIGNAL_WEAK;
               break;

            case 3:
               wifi_text = WIFI_SIGNAL_MEDIUM;
               break;

            case 4:
               wifi_text = WIFI_SIGNAL_GOOD;
               break;
            }

         if (!asprintf(&buffer, " %s, %s", NETWORK_TYPE_WIFI, wifi_text))
            {
               ERROR("buffer length == 0");
               wifi_ap_destroy(ap);
               return;
            }

         DEBUG("Text to say: %s", buffer);
         tts_speak(buffer, EINA_FALSE);
         free(buffer);
         wifi_ap_destroy(ap);
      }
}

void device_signal_strenght_get(void)
{
   if (!_read_text_get(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_INDICATOR_INFORMATION_SIGNAL_STRENGHT))
      {
         return;
      }
   _signal_strength_sim_get();
   _signal_strength_wifi_get();
}

void device_missed_events_get(void)
{
   notification_list_h list = NULL;
   notification_list_h elem = NULL;
   notification_h      noti = NULL;
   int ret = -1;
   char *noti_count_text = NULL;
   int noti_count = 0;
   int current_noti_count = 0;
   char *buffer = NULL;

   if (!_read_text_get(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_INDICATOR_INFORMATION_MISSED_EVENTS))
      {
         return;
      }

   ret = notification_get_list(NOTIFICATION_TYPE_NONE, -1, &list);
   if (ret != NOTIFICATION_ERROR_NONE)
      {
         ERROR("ret == %d", ret);
         return;
      }

   elem = notification_list_get_head(list);

   while(elem)
      {
         noti = notification_list_get_data(elem);
         notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &noti_count_text);


         if(noti_count_text)
            {
               current_noti_count = atoi(noti_count_text);
               if(current_noti_count > 0)
                  {
                     noti_count += current_noti_count;
                  }
               else
                  {
                     noti_count++;
                  }
            }
         else
            {
               noti_count++;
            }

         elem = notification_list_get_next(elem);
      }

   if(noti_count == 0)
      {
         DEBUG(NOTI_NOTIFICATIONS_UNREAD_0);
         tts_speak(NOTI_NOTIFICATIONS_UNREAD_0, EINA_FALSE);
      }
   else if(noti_count == 1)
      {
         DEBUG(NOTI_NOTIFICATIONS_UNREAD_1);
         tts_speak(NOTI_NOTIFICATIONS_UNREAD_1, EINA_FALSE);
      }
   else
      {
         DEBUG("%d %s", noti_count, NOTI_NOTIFICATIONS_UNREAD_MANY);

         if (asprintf(&buffer, "%d %s", noti_count, NOTI_NOTIFICATIONS_UNREAD_MANY))
            {
               ERROR("buffer length equals 0");
            }

         tts_speak(buffer, EINA_FALSE);
         free(buffer);
      }

   ret = notification_free_list(list);
   if(ret != NOTIFICATION_ERROR_NONE)
   {
	   ERROR("ret == %d", ret);
   }
}

void device_date_get(void)
{
   char buffer[DATE_TIME_BUFFER_SIZE];
   int date_format = SETTING_DATE_FORMAT_DD_MM_YYYY;
   int ret = -1;
   time_t rawtime = 0;
   struct tm *timeinfo = NULL;

   if (!_read_text_get(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_INDICATOR_INFORMATION_DATE))
      {
         return;
      }

   time(&rawtime );
   timeinfo = localtime ( &rawtime );
   strftime(buffer, DATE_TIME_BUFFER_SIZE, "%Y:%m:%d %H:%M:%S", timeinfo);

   ret = vconf_get_int(VCONFKEY_SETAPPL_DATE_FORMAT_INT, &date_format);
   if (ret != 0)
      {
         ERROR("ret == %d", ret);
      }

   switch(date_format)
      {
      case SETTING_DATE_FORMAT_DD_MM_YYYY:
         strftime(buffer, DATE_TIME_BUFFER_SIZE, "%d %B %Y", timeinfo);
         break;

      case SETTING_DATE_FORMAT_MM_DD_YYYY:
         strftime(buffer, DATE_TIME_BUFFER_SIZE, "%B %d %Y", timeinfo);
         break;

      case SETTING_DATE_FORMAT_YYYY_MM_DD:
         strftime(buffer, DATE_TIME_BUFFER_SIZE, "%Y %B %d", timeinfo);
         break;

      case SETTING_DATE_FORMAT_YYYY_DD_MM:
         strftime(buffer, DATE_TIME_BUFFER_SIZE, "%Y %d %B", timeinfo);
         break;
      }

   DEBUG("Text to say: %s", buffer);
   tts_speak(buffer, EINA_FALSE);
}

static bool bonded_device_count_cb(bt_device_info_s *device_info, void *user_data)
{
   int *device_count = (int *)user_data;

   (*device_count)++;

   return true;
}

static bool bonded_device_get_cb(bt_device_info_s *device_info, void *user_data)
{
   char **device_name = (char **)user_data;

   if(asprintf(device_name, "%s connected", device_info->remote_name))
      {
         ERROR("buffer length == 0");
      }

   return false;
}

void device_bluetooth_get(void)
{
   char *buffer = NULL;
   int device_count = 0;

   if (!_read_text_get(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_INDICATOR_INFORMATION_BLUETOOTH))
      {
         return;
      }

   bt_adapter_state_e adapter_state = BT_ADAPTER_DISABLED;
   int ret = bt_adapter_get_state(&adapter_state);
   if (ret != BT_ERROR_NONE)
      {
         ERROR("ret == %d", ret);
         return;
      }

   if (adapter_state == BT_ADAPTER_DISABLED)
      {
         DEBUG("Text to say: %s", BT_BLUETOOTH_OFF);
         tts_speak(BT_BLUETOOTH_OFF, EINA_FALSE);
         return;
      }
   else
      {
         bt_adapter_foreach_bonded_device(bonded_device_count_cb, (void *)&device_count);

         if(device_count == 0)
            {
               if (!asprintf(&buffer, BT_NO_DEVICES_CONNECTED))
                  {
                     ERROR("buffer length == 0");
                  }
            }
         else if(device_count == 1)
            {
               bt_adapter_foreach_bonded_device(bonded_device_get_cb, &buffer);
            }
         else
            {
               if (asprintf(&buffer, "%d %s", device_count,
                            BT_DEVICES_CONNECTED_COUNT))
                  {
                     ERROR("buffer length == 0");
                  }
            }

         DEBUG("Text to say: %s", buffer);
         tts_speak(buffer, EINA_FALSE);
         free(buffer);
      }
}
