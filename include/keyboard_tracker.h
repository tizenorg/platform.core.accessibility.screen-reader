/**
 * @brief Supported Keys
 */
enum _Key
{
   KEY_LEFT,
   KEY_RIGHT,
   KEY_UP,
   KEY_DOWN,
   KEY_COUNT
};

typedef enum _Key Key;

typedef void (*Keyboard_Tracker_Cb) (void *data, Key k);
void keyboard_tracker_init(void);
void keyboard_tracker_shutdown(void);
#ifndef X11_ENABLED
void keyboard_geometry_set(int x, int y, int width, int height);
void keyboard_geometry_get(int *x, int *y, int *width, int *height);
Eina_Bool keyboard_event_status(int x, int y);
void keyboard_signal_emit(int type, int x, int y);
#endif
