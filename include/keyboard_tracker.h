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
void keyboard_tracker_register(Keyboard_Tracker_Cb cb, void *data);
void keyboard_tracker_shutdown(void);
