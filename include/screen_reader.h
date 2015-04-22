#ifndef __screen_reader_H__
#define __screen_reader_H__

#include <atspi/atspi.h>
#include <tts.h>

#define LAN_NAME 6

#define MAX_REACHED ", maximum value reached"
#define MIN_REACHED ", minimum value reached"
#define MAX_POS_REACHED ", end of text reached"
#define MIN_POS_REACHED ", begin of text reached"

#define FOCUS_SIG "focused"

#define EDITING_STARTED "Editing"

#define FOCUS_CHANGED_SIG "object:state-changed:focused"
#define VALUE_CHANGED_SIG "object:property-change:accessible-value"
#define CARET_MOVED_SIG "object:text-caret-moved"

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
	int information_level;
	bool run_service;
	char language[LAN_NAME];
	int voice_type;
	int reading_speed;
	char *tracking_signal_name;

	//Set by tts
	tts_h tts;
	GList *available_languages;

	char *text_to_say_info;
	char *current_value;
	char *current_char;

	//Actions to do when tts state is 'ready'
	int _dbus_txt_readed;
	bool say_text;
	bool update_language_list;

	//Set by spi
	AtspiEventListener *spi_listener;

	AtspiAccessible  *currently_focused;
	AtspiAccessible  *mouse_down_widget;
	AtspiAccessible  *clicked_widget;

	//Set by dbus
	char **last_tokens;
	char *available_requests;
	char **available_apps;

	const char *text_from_dbus;
} Service_Data;

Service_Data *get_pointer_to_service_data_struct();

int screen_reader_create_service(void *data);

int screen_reader_terminate_service(void *data);

#endif
