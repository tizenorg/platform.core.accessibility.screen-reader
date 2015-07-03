#ifndef SCREEN_READER_GESTURES_H_
#define SCREEN_READER_GESTURES_H_

#include <Ecore.h>
#include <Ecore_X.h>

/**
 * @brief Accessibility gestures
 */
enum _Gesture
{
   ONE_FINGER_HOVER,
   TWO_FINGERS_HOVER,
   THREE_FINGERS_HOVER,
   ONE_FINGER_FLICK_LEFT,
   ONE_FINGER_FLICK_RIGHT,
   ONE_FINGER_FLICK_UP,
   ONE_FINGER_FLICK_DOWN,
   TWO_FINGERS_FLICK_LEFT,
   TWO_FINGERS_FLICK_RIGHT,
   TWO_FINGERS_FLICK_UP,
   TWO_FINGERS_FLICK_DOWN,
   THREE_FINGERS_FLICK_LEFT,
   THREE_FINGERS_FLICK_RIGHT,
   THREE_FINGERS_FLICK_UP,
   THREE_FINGERS_FLICK_DOWN,
   ONE_FINGER_SINGLE_TAP,
   ONE_FINGER_DOUBLE_TAP,
   ONE_FINGER_TRIPLE_TAP,
   TWO_FINGERS_SINGLE_TAP,
   TWO_FINGERS_DOUBLE_TAP,
   TWO_FINGERS_TRIPLE_TAP,
   THREE_FINGERS_SINGLE_TAP,
   THREE_FINGERS_DOUBLE_TAP,
   THREE_FINGERS_TRIPLE_TAP,
   ONE_FINGER_FLICK_LEFT_RETURN,
   ONE_FINGER_FLICK_RIGHT_RETURN,
   ONE_FINGER_FLICK_UP_RETURN,
   ONE_FINGER_FLICK_DOWN_RETURN,
   TWO_FINGERS_FLICK_LEFT_RETURN,
   TWO_FINGERS_FLICK_RIGHT_RETURN,
   TWO_FINGERS_FLICK_UP_RETURN,
   TWO_FINGERS_FLICK_DOWN_RETURN,
   THREE_FINGERS_FLICK_LEFT_RETURN,
   THREE_FINGERS_FLICK_RIGHT_RETURN,
   THREE_FINGERS_FLICK_UP_RETURN,
   THREE_FINGERS_FLICK_DOWN_RETURN,
   GESTURES_COUNT,
};
typedef enum _Gesture Gesture;

typedef struct
{
   Gesture type;         // Type of recognized gesture
   int x_beg, x_end;     // (x,y) coordinates when gesture begin (screen coords)
   int y_beg, y_end;     // (x,y) coordinates when gesture ends (screen coords)
   pid_t pid;            // pid of process on which gesture took place.
   int state;            // 0 - begin, 1 - ongoing, 2 - ended, 3 - aborted
   int event_time;
} Gesture_Info;

/**
 * @brief Initialize gesture navigation profile.
 *
 * @return EINA_TRUE is initialization was successfull, EINA_FALSE otherwise
 */
Eina_Bool screen_reader_gestures_init(void);

/**
 * @brief Shutdown gesture navigation profile.
 */
void screen_reader_gestures_shutdown(void);

Eina_Bool screen_reader_gesture_x_grab_touch_devices(Ecore_X_Window win);

typedef void (*GestureCB)(void *data, Gesture_Info *info);

/**
 * @brief Registers callback on gestures
 */
void screen_reader_gestures_tracker_register(GestureCB cb, void *data);

/**
 * @brief Start event emission
 */
void start_scroll(int x, int y);

/**
 * @brief Continue event emission
 */
void continue_scroll(int x, int y);

/**
 * @brief End event emit
 */
void end_scroll(int x, int y);

#endif
