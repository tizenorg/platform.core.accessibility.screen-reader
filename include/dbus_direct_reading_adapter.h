#ifndef DBUS_DIRECT_READING_ADAPTER_H_
#define DBUS_DIRECT_READING_ADAPTER_H_

enum _Signal {
    READING_STARTED = 0,
    READING_STOPPED = 1,
    READING_CANCELLED = 2
};

typedef enum _Signal Signal;

const char *get_signal_name(const Signal signal);

int dbus_direct_reading_init(void);

void dbus_direct_reading_shutdown(void);

int dbus_direct_reading_adapter_emit(const Signal signal, const void *data);

#endif
