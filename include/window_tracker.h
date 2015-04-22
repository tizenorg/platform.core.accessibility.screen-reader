#include <atspi/atspi.h>

void window_tracker_init(void);
void window_tracker_shutdown(void);

typedef void (*Window_Tracker_Cb) (void *data, AtspiAccessible *window);
void window_tracker_register(Window_Tracker_Cb cb, void *data);
void window_tracker_active_window_request(void);
