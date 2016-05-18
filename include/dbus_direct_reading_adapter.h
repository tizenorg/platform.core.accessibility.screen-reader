#ifndef DBUS_DIRECT_READING_ADAPTER_H_
#define DBUS_DIRECT_READING_ADAPTER_H_

#include "screen_reader_tts.h"

enum _Signal {
	READING_STARTED = 0,
	READING_STOPPED = 1,
	READING_CANCELLED = 2
};

typedef enum _Signal Signal;

void dbus_direct_reading_adapter_init(void);

void dbus_direct_reading_adapter_shutdown(void);

void dbus_direct_reading_adapter_emit(const Signal signal, const char *bus_name, const char *object_path);

#endif
