#include <eldbus-1/Eldbus.h>
#include <atspi/atspi.h>

/**
 * @brief Accessibility gestures
 */
enum _Gesture {
     ONE_FINGER_HOVER,
     TWO_FINGERS_HOVER,
     ONE_FINGER_FLICK_LEFT,
     ONE_FINGER_FLICK_RIGHT,
     ONE_FINGER_FLICK_UP,
     ONE_FINGER_FLICK_DOWN,
     THREE_FINGERS_FLICK_LEFT,
     THREE_FINGERS_FLICK_RIGHT,
     THREE_FINGERS_FLICK_UP,
     THREE_FINGERS_FLICK_DOWN,
     ONE_FINGER_SINGLE_TAP,
     ONE_FINGER_DOUBLE_TAP,
     TWO_FINGERS_FLICK_UP,
     TWO_FINGERS_FLICK_LEFT,
     TWO_FINGERS_FLICK_RIGHT,
     TWO_FINGERS_FLICK_DOWN,
     GESTURES_COUNT,
};
typedef enum _Gesture Gesture;

typedef struct {
     Gesture type;         // Type of recognized gesture
     int x_begin, x_end;     // (x,y) coordinates when gesture begin
     int y_begin, y_end;     // (x,y) coordinates when gesture ends
     int state;              // 0 - gesture begins, 1 - continues, 2 - ended
} Gesture_Info;


typedef void (*Gesture_Tracker_Cb) (void *data, Gesture_Info *g);
void gesture_tracker_init(Eldbus_Connection *conn);
void gesture_tracker_register(Gesture_Tracker_Cb cb, void *data);
void gesture_tracker_shutdown(void);
AtspiAccessible* _get_active_win(void);
