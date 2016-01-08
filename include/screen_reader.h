#ifndef __screen_reader_H__
#define __screen_reader_H__

#include <atspi/atspi.h>
#include <Eldbus.h>
#include <tts.h>

#define LANGUAGE_NAME_SIZE 6

#define FOCUS_CHANGED_SIG "object:state-changed:focused"
#define HIGHLIGHT_CHANGED_SIG "object:state-change:highlighted"
#define VALUE_CHANGED_SIG "object:property-change:accessible-value"
#define CARET_MOVED_SIG "object:text-caret-moved"
#define STATE_FOCUSED_SIG "object:state-changed:focused"
#define TEXT_INSERT_SIG "object:text-changed:insert"
#define TEXT_DELETE_SIG "object:text-changed:delete"

typedef struct
{
   char *language;
   int voice_type;
} Voice_Info;


typedef enum
{
   read_as_xml,
   read_as_plain,
   dont_read
} Wrong_Validation_Reacction;

typedef struct _Service_Data
{
   //Set by vconf
   bool run_service;
   char display_language[LANGUAGE_NAME_SIZE];
   char *tracking_signal_name;

   //Set by tts
   tts_h tts;
   Eina_List *available_languages;

   char *text_to_say_info;
   char *current_value;
   char *current_char;

   //Actions to do when tts state is 'ready'
   int _dbus_txt_readed;
   bool say_text;
   bool update_language_list;

   //Set by spi
   AtspiEventListener *state_changed_listener;
   AtspiEventListener *value_changed_listener;
   AtspiEventListener *caret_moved_listener;
   AtspiEventListener *spi_listener;

   AtspiAccessible  *currently_focused;
   AtspiAccessible  *mouse_down_widget;
   AtspiAccessible  *clicked_widget;

   //Set by dbus
   Eldbus_Proxy *proxy;
   char **last_tokens;
   char *available_requests;
   char **available_apps;

   const char *text_from_dbus;
   const char *lua_script_path;
} Service_Data;

Service_Data *get_pointer_to_service_data_struct();

int screen_reader_create_service(void *data);

int screen_reader_terminate_service(void *data);

#endif
