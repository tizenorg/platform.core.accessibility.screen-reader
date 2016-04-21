#ifndef DBUS_GESTURE_ADAPTER_H_
#define DBUS_GESTURE_ADAPTER_H_

#include "screen_reader_gestures.h"

void dbus_gesture_adapter_init(void);

void dbus_gesture_adapter_shutdown(void);

void dbus_gesture_adapter_emit(const Gesture_Info *info);

const char *_gesture_enum_to_string(Gesture g);

#endif
