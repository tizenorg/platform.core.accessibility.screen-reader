#include <string.h>
#include "gesture_tracker.h"
#include "logger.h"

#define BUS "com.samsung.EModule"
#define INTERFACE "com.samsung.GestureNavigation"
#define PATH "/com/samsung/GestureNavigation"
#define SIGNAL "GestureDetected"

static Eldbus_Connection *a11y_conn;
static Eldbus_Object *object;
static Eldbus_Proxy *man;
static Gesture_Tracker_Cb user_cb;
static void *user_data;

static Gesture gesture_name_to_enum (const char *gesture_name)
{
  if(!gesture_name)
     return GESTURES_COUNT;
 
  DEBUG("Dbus incoming gesture: %s", gesture_name);
 
  if(!strcmp("OneFingerHover", gesture_name))
     return ONE_FINGER_HOVER;

  if(!strcmp("TwoFingersHover", gesture_name))
     return TWO_FINGERS_HOVER;

  if(!strcmp("OneFingerFlickLeft", gesture_name))
     return ONE_FINGER_FLICK_LEFT;

  if(!strcmp("OneFingerFlickRight", gesture_name))
     return ONE_FINGER_FLICK_RIGHT;

  if(!strcmp("OneFingerFlickUp", gesture_name))
     return ONE_FINGER_FLICK_UP;

  if(!strcmp("OneFingerFlickDown", gesture_name))
     return ONE_FINGER_FLICK_DOWN;

 if(!strcmp("ThreeFingersFlickLeft", gesture_name))
     return THREE_FINGERS_FLICK_LEFT;

 if(!strcmp("ThreeFingersFlickRight", gesture_name))
     return THREE_FINGERS_FLICK_RIGHT;

 if(!strcmp("ThreeFingersFlickUp", gesture_name))
     return THREE_FINGERS_FLICK_UP;

 if(!strcmp("ThreeFingersFlickDown", gesture_name))
     return THREE_FINGERS_FLICK_DOWN;

 if(!strcmp("OneFingerSingleTap", gesture_name))
     return ONE_FINGER_SINGLE_TAP;

 if(!strcmp("OneFingerDoubleTap", gesture_name))
     return ONE_FINGER_DOUBLE_TAP;

 if(!strcmp("TwoFingersFlickLeft", gesture_name))
    return TWO_FINGERS_FLICK_LEFT;

 if(!strcmp("TwoFingersFlickRight", gesture_name))
    return TWO_FINGERS_FLICK_RIGHT;

 if(!strcmp("TwoFingersFlickUp", gesture_name))
    return TWO_FINGERS_FLICK_UP;

 if(!strcmp("TwoFingersFlickDown", gesture_name))
     return TWO_FINGERS_FLICK_DOWN;

 return GESTURES_COUNT;
}

static void on_gesture_detected(void *context EINA_UNUSED, const Eldbus_Message *msg)
{
    DEBUG("START");
    const char *gesture_name;
    int x_s, y_s, x_e, y_e, state;

    if(!eldbus_message_arguments_get(msg, "siiiiu", &gesture_name, &x_s, &y_s, &x_e, &y_e, &state))
        ERROR("error geting arguments on_gesture_detected");

    Gesture_Info g;
    g.type = gesture_name_to_enum(gesture_name);
    g.x_begin = x_s;
    g.y_begin = y_s;
    g.x_end = x_e;
    g.y_end = y_e;
    g.state = state;

    if(user_cb)
        user_cb(user_data, &g);
}

void gesture_tracker_init(Eldbus_Connection *conn)
{
    DEBUG("START");
    a11y_conn = conn;
    eldbus_connection_ref(conn);
    object = eldbus_object_get(conn, BUS, PATH);
    man = eldbus_proxy_get(object, INTERFACE);
    eldbus_proxy_signal_handler_add(man, SIGNAL, on_gesture_detected, NULL);
}

void gesture_tracker_register(Gesture_Tracker_Cb cb, void *data)
{
   user_cb = cb;
   user_data = data;
}

void gesture_tracker_shutdown(void)
{
    eldbus_proxy_unref(man);
    eldbus_object_unref(object);
    eldbus_connection_unref(a11y_conn);
}
