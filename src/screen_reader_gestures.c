/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "screen_reader_gestures.h"
#include "logger.h"

#include <Ecore.h>
#include <Ecore_Input.h>
#include <Ecore_X.h>

static GestureCB _global_cb;
static void *_global_data;
static Ecore_Window win;
static Ecore_Event_Handler *property_changed_hld;

struct _Gestures_Config
{
   // minimal required length of flick gesture (in pixels)
   int one_finger_flick_min_length;
   // maximal time of gesture
   int one_finger_flick_max_time;
   // timeout period to activate hover gesture (first longpress timeout)
   double one_finger_hover_longpress_timeout;
   // to activate flick gesture by 2 fingers (it is hotfix - gestures need serious refactoring)
   int two_finger_flick_to_scroll_timeout;
   // after mowing this pixels flick two finger flick to scroll gesture is started
   int two_finger_flick_to_scroll_min_length;
   // tap timeout - maximal ammount of time allowed between seqiential taps
   double one_finger_tap_timeout;
   // tap radius(in pixels)
   int one_finger_tap_radius;
};
typedef struct _Gestures_Config Gestures_Config;

typedef enum
{
   FLICK_DIRECTION_UNDEFINED,
   FLICK_DIRECTION_DOWN,
   FLICK_DIRECTION_UP,
   FLICK_DIRECTION_LEFT,
   FLICK_DIRECTION_RIGHT,
   FLICK_DIRECTION_DOWN_RETURN,
   FLICK_DIRECTION_UP_RETURN,
   FLICK_DIRECTION_LEFT_RETURN,
   FLICK_DIRECTION_RIGHT_RETURN,
} flick_direction_e;

typedef enum
{
   GESTURE_NOT_STARTED = 0, // Gesture is ready to start
   GESTURE_ONGOING,         // Gesture in progress.
   GESTURE_FINISHED,        // Gesture finished - should be emited
   GESTURE_ABORTED          // Gesture aborted
} gesture_state_e;

typedef enum
{
   ONE_FINGER_GESTURE = 1,
   TWO_FINGERS_GESTURE,
   THREE_FINGERS_GESTURE
} gesture_type_e;

struct _Cover
{
   Ecore_X_Window  win;  /**< Input window covering given zone */
   unsigned int    n_taps; /**< Number of fingers touching screen */
   unsigned int    event_time;

   struct
   {
      gesture_state_e state;     // current state of gesture
      unsigned int timestamp[3]; // time of gesture;
      int finger[3];             // finger number which initiates gesture
      int x_org[3], y_org[3];    // coorinates of finger down event
      int x_end[3], y_end[3];    // coorinates of finger up event
      flick_direction_e dir;     // direction of flick
      int n_fingers;             // number of fingers in gesture
      int n_fingers_left;        // number of fingers in gesture
      //         still touching screen
      Eina_Bool finger_out[3];   // finger is out of the finger boundary
      Eina_Bool return_flick[3];
      Eina_Bool flick_to_scroll;
   } flick_gesture;

   struct
   {
      gesture_state_e state;   // currest gesture state
      int x[2], y[2];
      int n_fingers;
      int finger[2];
      unsigned int timestamp; // time of gesture;
      unsigned int last_emission_time; // last time of gesture emission
      Ecore_Timer *timer;
      Eina_Bool longpressed;
   } hover_gesture;

   struct
   {
      Eina_Bool started; // indicates if taps recognition process has started
      Eina_Bool pressed; // indicates if finger is down
      int n_taps;        // number of taps captures in sequence
      int finger[3];        // device id of finget
      Ecore_Timer *timer;  // sequence expiration timer
      int x_org[3], y_org[3];    // coordinates of first tap
      gesture_type_e tap_type;
   } tap_gesture_data;
};
typedef struct _Cover Cover;

Gestures_Config *_e_mod_config;
static Ecore_X_Window scrolled_win;
static int rx, ry;
static Eina_List *handlers;
static Cover *cov;
static int win_angle;

static void _hover_event_emit(Cover *cov, int state);

static void _event_emit(Gesture g, int x, int y, int x_e, int y_e, int state, int event_time)
{
   Gesture_Info *info = calloc(sizeof(Gesture_Info), 1);
   EINA_SAFETY_ON_NULL_RETURN(info);

   info->type = g;
   info->x_beg = x;
   info->x_end = x_e;
   info->y_beg = y;
   info->y_end = y_e;
   info->state = state;
   info->event_time = event_time;

   if (_global_cb)
      _global_cb(_global_data, info);
   free(info);
}

static void
_flick_gesture_mouse_down(Ecore_Event_Mouse_Button *ev, Cover *cov)
{
   if (cov->flick_gesture.state == GESTURE_NOT_STARTED)
      {
         cov->flick_gesture.state = GESTURE_ONGOING;
         cov->flick_gesture.finger[0] = ev->multi.device;
         cov->flick_gesture.x_org[0] = ev->root.x;
         cov->flick_gesture.y_org[0] = ev->root.y;
         cov->flick_gesture.timestamp[0] = ev->timestamp;
         cov->flick_gesture.flick_to_scroll = EINA_FALSE;
         cov->flick_gesture.n_fingers = 1;
         cov->flick_gesture.n_fingers_left = 1;
         cov->flick_gesture.dir = FLICK_DIRECTION_UNDEFINED;
         cov->flick_gesture.finger_out[0] = EINA_FALSE;
         cov->flick_gesture.return_flick[0] = EINA_FALSE;
      }
   else if (cov->flick_gesture.state == GESTURE_ONGOING)
      {
         // abort gesture if too many fingers touched screen
         if ((cov->n_taps > 3) || (cov->flick_gesture.n_fingers > 2))
            {
               cov->flick_gesture.state = GESTURE_ABORTED;
               return;
            }

         cov->flick_gesture.x_org[cov->flick_gesture.n_fingers] = ev->root.x;
         cov->flick_gesture.y_org[cov->flick_gesture.n_fingers] = ev->root.y;
         cov->flick_gesture.timestamp[cov->flick_gesture.n_fingers] = ev->timestamp;
         cov->flick_gesture.finger[cov->flick_gesture.n_fingers] = ev->multi.device;
         cov->flick_gesture.n_fingers++;
         cov->flick_gesture.n_fingers_left++;
         if (cov->flick_gesture.n_fingers < 3) /* n_fingers == 3 makes out of bounds write */
           {
              cov->flick_gesture.finger_out[cov->flick_gesture.n_fingers] = EINA_FALSE;
              cov->flick_gesture.return_flick[cov->flick_gesture.n_fingers] = EINA_FALSE;
           }
      }
}

static Eina_Bool
_flick_gesture_time_check(unsigned int event_time, unsigned int gesture_time)
{
   DEBUG("Flick time: %d", event_time - gesture_time);
   if ((event_time - gesture_time) < _e_mod_config->one_finger_flick_max_time * 2) //Double time because of the possible of return flick
      return EINA_TRUE;
   else
      return EINA_FALSE;
}

static Eina_Bool
_flick_gesture_length_check(int x, int y, int x_org, int y_org)
{
   int dx = x - x_org;
   int dy = y - y_org;

   if ((dx * dx + dy * dy) > (_e_mod_config->one_finger_flick_min_length *
                              _e_mod_config->one_finger_flick_min_length))
      return EINA_TRUE;
   else
      return EINA_FALSE;
}

static flick_direction_e
_flick_gesture_direction_get(int x, int y, int x_org, int y_org)
{
   int dx = x - x_org;
   int dy = y - y_org;
   int tmp;

   switch(win_angle)
      {
      case 90:
         tmp = dx;
         dx = -dy;
         dy = tmp;
         break;
      case 270:
         tmp = dx;
         dx = dy;
         dy = -tmp;
         break;
      }

   if ((dy < 0) && (abs(dx) < -dy))
      return FLICK_DIRECTION_UP;
   if ((dy > 0) && (abs(dx) < dy))
      return FLICK_DIRECTION_DOWN;
   if ((dx > 0) && (dx > abs(dy)))
      return FLICK_DIRECTION_RIGHT;
   if ((dx < 0) && (-dx > abs(dy)))
      return FLICK_DIRECTION_LEFT;

   return FLICK_DIRECTION_UNDEFINED;
}

static void
_flick_event_emit(Cover *cov)
{
   int ax, ay, axe, aye, i, type = -1;
   ax = ay = axe = aye = 0;

   for (i = 0; i < cov->flick_gesture.n_fingers; i++)
      {
         ax += cov->flick_gesture.x_org[i];
         ay += cov->flick_gesture.y_org[i];
         axe += cov->flick_gesture.x_end[i];
         aye += cov->flick_gesture.y_end[i];
      }

   ax /= cov->flick_gesture.n_fingers;
   ay /= cov->flick_gesture.n_fingers;
   axe /= cov->flick_gesture.n_fingers;
   aye /= cov->flick_gesture.n_fingers;

   if (cov->flick_gesture.dir == FLICK_DIRECTION_LEFT)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_LEFT;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_LEFT;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_LEFT;
      }
   else if (cov->flick_gesture.dir == FLICK_DIRECTION_RIGHT)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_RIGHT;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_RIGHT;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_RIGHT;
      }
   else if (cov->flick_gesture.dir == FLICK_DIRECTION_UP)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_UP;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_UP;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_UP;
      }
   else if (cov->flick_gesture.dir == FLICK_DIRECTION_DOWN)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_DOWN;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_DOWN;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_DOWN;
      }
   else if (cov->flick_gesture.dir == FLICK_DIRECTION_DOWN_RETURN)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_DOWN_RETURN;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_DOWN_RETURN;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_DOWN_RETURN;
      }
   else if (cov->flick_gesture.dir == FLICK_DIRECTION_UP_RETURN)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_UP_RETURN;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_UP_RETURN;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_UP_RETURN;
      }
   else if (cov->flick_gesture.dir == FLICK_DIRECTION_LEFT_RETURN)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_LEFT_RETURN;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_LEFT_RETURN;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_LEFT_RETURN;
      }
   else if (cov->flick_gesture.dir == FLICK_DIRECTION_RIGHT_RETURN)
      {
         if (cov->flick_gesture.n_fingers == 1)
            type = ONE_FINGER_FLICK_RIGHT_RETURN;
         if (cov->flick_gesture.n_fingers == 2)
            type = TWO_FINGERS_FLICK_RIGHT_RETURN;
         if (cov->flick_gesture.n_fingers == 3)
            type = THREE_FINGERS_FLICK_RIGHT_RETURN;
      }
   DEBUG("FLICK GESTURE: N: %d F: %d", cov->flick_gesture.n_fingers, cov->flick_gesture.dir);
   _event_emit(type, ax, ay, axe, aye, 2, cov->event_time);
}

static void
_flick_gesture_mouse_up(Ecore_Event_Mouse_Button *ev, Cover *cov)
{
   if (cov->flick_gesture.state == GESTURE_ONGOING)
      {
         int i;
         // check if fingers match
         for (i = 0; i < cov->flick_gesture.n_fingers; i++)
            {
               if (cov->flick_gesture.finger[i] == ev->multi.device)
                  break;
            }
         if (i == cov->flick_gesture.n_fingers)
            {
               DEBUG("Finger id not recognized. Gesture aborted.");
               cov->flick_gesture.state = GESTURE_ABORTED;
               goto end;
            }
        if (ev->multi.device == 1 && cov->flick_gesture.flick_to_scroll)//we react only on second finger
            {
               DEBUG("Flick gesture was interpreted as scroll so we aborting it.");
               end_scroll(ev->x, ev->y);
               cov->flick_gesture.flick_to_scroll  = EINA_FALSE;
               cov->flick_gesture.state = GESTURE_ABORTED;
               goto end;
            }
         // check if flick for given finger is valid
         if (!_flick_gesture_time_check(ev->timestamp,
                                        cov->flick_gesture.timestamp[i]))
            {
               DEBUG("finger flick gesture timeout expired. Gesture aborted.");
               cov->flick_gesture.state = GESTURE_ABORTED;
               goto end;
            }

         // check minimal flick length
         if (!_flick_gesture_length_check(ev->root.x, ev->root.y,
                                          cov->flick_gesture.x_org[i],
                                          cov->flick_gesture.y_org[i]))
            {
               if (!cov->flick_gesture.finger_out[i])
                  {
                     DEBUG("Minimal gesture length not reached and no return flick. Gesture aborted.");
                     cov->flick_gesture.state = GESTURE_ABORTED;
                     goto end;
                  }
               cov->flick_gesture.return_flick[i] = EINA_TRUE;
            }

         flick_direction_e s = cov->flick_gesture.return_flick[i] ?
                               cov->flick_gesture.dir :
                               _flick_gesture_direction_get(ev->root.x, ev->root.y,
                                     cov->flick_gesture.x_org[i],
                                     cov->flick_gesture.y_org[i]);

         cov->flick_gesture.n_fingers_left--;

         if ((cov->flick_gesture.dir == FLICK_DIRECTION_UNDEFINED ||
               cov->flick_gesture.dir > FLICK_DIRECTION_RIGHT)
               && cov->flick_gesture.return_flick[i] == EINA_FALSE)
            {
               DEBUG("Flick gesture");
               cov->flick_gesture.dir = s;
            }

         // gesture is valid only if all flicks are in same direction
         if (cov->flick_gesture.dir != s)
            {
               DEBUG("Flick in different direction. Gesture aborted.");
               cov->flick_gesture.state = GESTURE_ABORTED;
               goto end;
            }

         cov->flick_gesture.x_end[i] = ev->root.x;
         cov->flick_gesture.y_end[i] = ev->root.y;

         if (!cov->flick_gesture.n_fingers_left)
            {
               _flick_event_emit(cov);
               cov->flick_gesture.state = GESTURE_NOT_STARTED;
            }
      }

end:
   // if no finger is touching a screen, gesture will be reseted.
   if ((cov->flick_gesture.state == GESTURE_ABORTED) && (cov->n_taps == 0))
      {
         DEBUG("Restet flick gesture");
         cov->flick_gesture.state = GESTURE_NOT_STARTED;
      }
}
static Eina_Bool _flick_to_scroll_gesture_conditions_met(Ecore_Event_Mouse_Move *ev, int gesture_timestamp, int dx, int dy)
{
    if (ev->timestamp - gesture_timestamp > _e_mod_config->two_finger_flick_to_scroll_timeout)
        if (abs(dx) > _e_mod_config->two_finger_flick_to_scroll_min_length ||
            abs(dy) > _e_mod_config->two_finger_flick_to_scroll_min_length)
            return EINA_TRUE;

    return EINA_FALSE;
}

static void
_flick_gesture_mouse_move(Ecore_Event_Mouse_Move *ev, Cover *cov)
{
   if (cov->flick_gesture.state == GESTURE_ONGOING)
      {
         int i;
         for(i = 0; i < cov->flick_gesture.n_fingers; ++i)
            {
               if (cov->flick_gesture.finger[i] == ev->multi.device)
                  break;
            }
         if (i == cov->flick_gesture.n_fingers)
            {
               if (cov->flick_gesture.n_fingers >= 3)   //that is because of the EFL bug. Mouse move event before mouse down(!)
                  {
                     ERROR("Finger id not recognized. Gesture aborted.");
                     cov->flick_gesture.state = GESTURE_ABORTED;
                     return;
                  }
            }
         DEBUG("i: %i, ev->multi.device: %i", i, ev->multi.device);

         int dx = ev->root.x - cov->flick_gesture.x_org[i];
         int dy = ev->root.y - cov->flick_gesture.y_org[i];
         int tmp;

         switch(win_angle)
            {
            case 90:
               tmp = dx;
               dx = -dy;
               dy = tmp;
               break;
            case 270:
               tmp = dx;
               dx = dy;
               dy = -tmp;
               break;
            }
         if ( i == 1)
         {
             if (cov->flick_gesture.flick_to_scroll || _flick_to_scroll_gesture_conditions_met(ev, cov->flick_gesture.timestamp[i], dx, dy))
               {
                 if (!cov->flick_gesture.flick_to_scroll) {
                    start_scroll(ev->x, ev->y);
                    cov->flick_gesture.flick_to_scroll = EINA_TRUE;
                 } else {
                    continue_scroll(ev->x, ev->y);
                 }
                 return;
              }
         }

         if(!cov->flick_gesture.finger_out[i])
            {
               if (abs(dx) > _e_mod_config->one_finger_flick_min_length)
                  {
                     cov->flick_gesture.finger_out[i] = EINA_TRUE;
                     if (dx > 0)
                        {
                           if (cov->flick_gesture.dir == FLICK_DIRECTION_UNDEFINED ||
                                 cov->flick_gesture.dir == FLICK_DIRECTION_RIGHT_RETURN)
                              {
                                 cov->flick_gesture.dir = FLICK_DIRECTION_RIGHT_RETURN;
                              }
                           else
                              {
                                 ERROR("Invalid direction, abort");
                                 cov->flick_gesture.state = GESTURE_ABORTED;
                              }
                        }
                     else
                        {
                           if (cov->flick_gesture.dir == FLICK_DIRECTION_UNDEFINED ||
                                 cov->flick_gesture.dir == FLICK_DIRECTION_LEFT_RETURN)
                              {
                                 cov->flick_gesture.dir = FLICK_DIRECTION_LEFT_RETURN;
                              }
                           else
                              {
                                 ERROR("Invalid direction, abort");
                                 cov->flick_gesture.state = GESTURE_ABORTED;
                              }
                        }
                     return;
                  }

               else if (abs(dy) > _e_mod_config->one_finger_flick_min_length)
                  {
                     cov->flick_gesture.finger_out[i] = EINA_TRUE;
                     if (dy > 0)
                        {
                           if (cov->flick_gesture.dir == FLICK_DIRECTION_UNDEFINED ||
                                 cov->flick_gesture.dir == FLICK_DIRECTION_DOWN_RETURN)
                              {
                                 cov->flick_gesture.dir = FLICK_DIRECTION_DOWN_RETURN;
                              }
                           else
                              {
                                 ERROR("Invalid direction, abort");
                                 cov->flick_gesture.state = GESTURE_ABORTED;
                              }
                        }
                     else
                        {
                           if (cov->flick_gesture.dir == FLICK_DIRECTION_UNDEFINED ||
                                 cov->flick_gesture.dir == FLICK_DIRECTION_UP_RETURN)
                              {
                                 cov->flick_gesture.dir = FLICK_DIRECTION_UP_RETURN;
                              }
                           else
                              {
                                 ERROR("Invalid direction, abort");
                                 cov->flick_gesture.state = GESTURE_ABORTED;
                              }
                        }
                     return;
                  }
            }
      }
   return;
}

static Eina_Bool
_on_hover_timeout(void *data)
{
   Cover *cov = data;
   DEBUG("Hover timer expierd");

   cov->hover_gesture.longpressed = EINA_TRUE;
   cov->hover_gesture.timer = NULL;

   if (cov->hover_gesture.last_emission_time == -1)
      {
         _hover_event_emit(cov, 0);
         cov->hover_gesture.last_emission_time = cov->event_time;
      }
   return EINA_FALSE;
}

static void
_hover_gesture_timer_reset(Cover *cov, double time)
{
   DEBUG("Hover timer reset");
   cov->hover_gesture.longpressed = EINA_FALSE;
   if (cov->hover_gesture.timer)
      {
         ecore_timer_reset(cov->hover_gesture.timer);
         return;
      }
   cov->hover_gesture.timer = ecore_timer_add(time, _on_hover_timeout, cov);
}

static void
_hover_gesture_mouse_down(Ecore_Event_Mouse_Button *ev, Cover *cov)
{
   if (cov->hover_gesture.state == GESTURE_NOT_STARTED &&
         cov->n_taps == 1)
      {
         cov->hover_gesture.state = GESTURE_ONGOING;
         cov->hover_gesture.timestamp = ev->timestamp;
         cov->hover_gesture.last_emission_time = -1;
         cov->hover_gesture.x[0] = ev->root.x;
         cov->hover_gesture.y[0] = ev->root.y;
         cov->hover_gesture.finger[0] = ev->multi.device;
         cov->hover_gesture.n_fingers = 1;
         _hover_gesture_timer_reset(cov, _e_mod_config->one_finger_hover_longpress_timeout);
      }
   if (cov->hover_gesture.state == GESTURE_ONGOING &&
         cov->n_taps == 2)
      {
         if (cov->hover_gesture.longpressed)
            {
               _hover_event_emit(cov, 2);
               goto abort;
            }
         cov->hover_gesture.timestamp = -1;
         cov->hover_gesture.last_emission_time = -1;
         cov->hover_gesture.x[1] = ev->root.x;
         cov->hover_gesture.y[1] = ev->root.y;
         cov->hover_gesture.finger[1] = ev->multi.device;
         cov->hover_gesture.n_fingers = 2;
         _hover_gesture_timer_reset(cov, _e_mod_config->one_finger_hover_longpress_timeout);
      }
   // abort gesture if more then 2 fingers touched screen
   if ((cov->hover_gesture.state == GESTURE_ONGOING) &&
         cov->n_taps > 2)
      {
         DEBUG("More then 2 finged. Abort hover gesture");
         _hover_event_emit(cov, 2);
         goto abort;
      }
   return;

abort:
   cov->hover_gesture.state = GESTURE_ABORTED;
   if (cov->hover_gesture.timer)
      ecore_timer_del(cov->hover_gesture.timer);
   cov->hover_gesture.timer = NULL;
}

static void
_hover_gesture_mouse_up(Ecore_Event_Mouse_Button *ev, Cover *cov)
{
   int i;
   if (cov->hover_gesture.state == GESTURE_ONGOING)
      {

         for (i = 0; i < cov->hover_gesture.n_fingers; i++)
            {
               if (cov->hover_gesture.finger[i] == ev->multi.device)
                  break;
            }
         if (i == cov->hover_gesture.n_fingers)
            {
               DEBUG("Invalid finger id: %d", ev->multi.device);
               return;
            }
         else
            {
               cov->hover_gesture.state = GESTURE_ABORTED;
               if (cov->hover_gesture.timer)
                  ecore_timer_del(cov->hover_gesture.timer);
               cov->hover_gesture.timer = NULL;
               // aditionally emit event to complete sequence
               if (cov->hover_gesture.longpressed)
                  _hover_event_emit(cov, 2);
            }
      }
   // reset gesture only if user released all his fingers
   if (cov->n_taps == 0)
      cov->hover_gesture.state = GESTURE_NOT_STARTED;
}

static void _get_root_coords(Ecore_X_Window win, int *x, int *y)
{
   Ecore_X_Window root = ecore_x_window_root_first_get();
   Ecore_X_Window parent = ecore_x_window_parent_get(win);
   int wx, wy;

   if (x) *x = 0;
   if (y) *y = 0;

   while (parent && (root != parent))
      {
         ecore_x_window_geometry_get(parent, &wx, &wy, NULL, NULL);
         if (x) *x += wx;
         if (y) *y += wy;
         parent = ecore_x_window_parent_get(parent);
      }
}

Ecore_X_Window top_window_get (int x, int y)
{
   Ecore_X_Window wins[1] = { win };
   Ecore_X_Window under = ecore_x_window_at_xy_with_skip_get(x, y, wins, sizeof(wins)/sizeof(wins[0]));
   if (under)
      {
         _get_root_coords(under, &rx, &ry);
         DEBUG("Recieved window with coords:%d %d", rx, ry);
         return under;
      }
   return 0;
}

void
start_scroll(int x, int y)
{
   Ecore_X_Window wins[1] = { win };
   Ecore_X_Window under = ecore_x_window_at_xy_with_skip_get(x, y, wins, sizeof(wins)/sizeof(wins[0]));
   _get_root_coords(under, &rx, &ry);
   DEBUG("Starting scroll: %d %d", x - rx, y - ry);
   ecore_x_mouse_in_send(under, x - rx, y - ry);
   ecore_x_window_focus(under);
   ecore_x_mouse_down_send(under, x - rx, y - ry, 1);
   scrolled_win = under;
}

void
continue_scroll(int x, int y)
{
   DEBUG("Continuing scroll: %d %d", x - rx, y - ry);
   ecore_x_mouse_move_send(scrolled_win, x - rx, y - ry);
}

void
end_scroll(int x, int y)
{
   DEBUG("Ending scroll : %d %d", x - rx, y - ry);
   ecore_x_mouse_up_send(scrolled_win, x - rx, y - ry, 1);
   ecore_x_mouse_out_send(scrolled_win, x - rx, y - ry);
}

static unsigned int
_win_angle_get(void)
{
   Ecore_X_Window root, first_root;
   int ret;
   int count;
   int angle = 0;
   unsigned char *prop_data = NULL;

   first_root = ecore_x_window_root_first_get();
   root = ecore_x_window_root_get(first_root);
   ret = ecore_x_window_prop_property_get(root, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
                   ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);

   if (ret && prop_data)
      memcpy (&angle, prop_data, sizeof (int));

   if (prop_data)
      free (prop_data);

   return angle;
}

static void
_hover_event_emit(Cover *cov, int state)
{
   Ecore_X_Window root;
   int ax = 0, ay = 0, j, tmp, w, h;

   for (j = 0; j < cov->hover_gesture.n_fingers; j++)
      {
         ax += cov->hover_gesture.x[j];
         ay += cov->hover_gesture.y[j];
      }

   ax /= cov->hover_gesture.n_fingers;
   ay /= cov->hover_gesture.n_fingers;

   win_angle = _win_angle_get();
   switch(win_angle)
      {
      case 90:
         root = ecore_x_window_root_first_get();
         ecore_x_window_geometry_get(root, NULL, NULL, &w, &h);
         tmp = ax;
         ax = h - ay;
         ay = tmp;
         break;

      case 270:
         root = ecore_x_window_root_first_get();
         ecore_x_window_geometry_get(root, NULL, NULL, &w, &h);
         tmp = ax;
         ax = ay;
         ay = w - tmp;
         break;
      }

   switch (cov->hover_gesture.n_fingers)
      {
      case 1:
         INFO("ONE FINGER HOVER");
         _event_emit(ONE_FINGER_HOVER, ax, ay, ax, ay, state, cov->event_time);
         break;
      default:
         break;
      }
}

static void
_hover_gesture_mouse_move(Ecore_Event_Mouse_Move *ev, Cover *cov)
{
   if (cov->hover_gesture.state == GESTURE_ONGOING)
      {
         // check fingers
         int i;
         if (!cov->hover_gesture.longpressed)
            return;

         for (i = 0; i < cov->hover_gesture.n_fingers; i++)
            {
               if (cov->hover_gesture.finger[i] == ev->multi.device)
                  break;
            }
         if (i == cov->hover_gesture.n_fingers)
            {
               DEBUG("Invalid finger id: %d", ev->multi.device);
               return;
            }
         cov->hover_gesture.x[i] = ev->root.x;
         cov->hover_gesture.y[i] = ev->root.y;
         _hover_event_emit(cov, 1);
      }
}

static void
_tap_event_emit(Cover *cov, int state)
{
   switch (cov->tap_gesture_data.n_taps)
      {
      case 1:
         if(cov->tap_gesture_data.tap_type == ONE_FINGER_GESTURE)
            {
               DEBUG("ONE_FINGER_SINGLE_TAP");
               _event_emit(ONE_FINGER_SINGLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           state, cov->event_time);
            }
         else if(cov->tap_gesture_data.tap_type == TWO_FINGERS_GESTURE)
            {
               DEBUG("TWO_FINGERS_SINGLE_TAP");
               _event_emit(TWO_FINGERS_SINGLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[1], cov->tap_gesture_data.y_org[1],
                           state, cov->event_time);
            }
         else if(cov->tap_gesture_data.tap_type == THREE_FINGERS_GESTURE)
            {
               DEBUG("THREE_FINGERS_SINGLE_TAP");
               _event_emit(THREE_FINGERS_SINGLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[2], cov->tap_gesture_data.y_org[2],
                           state, cov->event_time);
            }
         else
            {
               ERROR("Unknown tap");
            }
         break;
      case 2:
         if(cov->tap_gesture_data.tap_type == ONE_FINGER_GESTURE)
            {
               DEBUG("ONE_FINGER_DOUBLE_TAP");
               _event_emit(ONE_FINGER_DOUBLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           state, cov->event_time);
            }
         else if(cov->tap_gesture_data.tap_type == TWO_FINGERS_GESTURE)
            {
               DEBUG("TWO_FINGERS_DOUBLE_TAP");
               _event_emit(TWO_FINGERS_DOUBLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[1], cov->tap_gesture_data.y_org[1],
                           state, cov->event_time);
            }
         else if(cov->tap_gesture_data.tap_type == THREE_FINGERS_GESTURE)
            {
               DEBUG("THREE_FINGERS_DOUBLE_TAP");
               _event_emit(THREE_FINGERS_DOUBLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[2], cov->tap_gesture_data.y_org[2],
                           state, cov->event_time);
            }
         else
            {
               ERROR("Unknown tap");
            }
         break;
      case 3:
         if(cov->tap_gesture_data.tap_type == ONE_FINGER_GESTURE)
            {
               DEBUG("ONE_FINGER_TRIPLE_TAP");
               _event_emit(ONE_FINGER_TRIPLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           state, cov->event_time);
            }
         else if(cov->tap_gesture_data.tap_type == TWO_FINGERS_GESTURE)
            {
               DEBUG("TWO_FINGERS_TRIPLE_TAP");
               _event_emit(TWO_FINGERS_TRIPLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[1], cov->tap_gesture_data.y_org[1],
                           state, cov->event_time);
            }
         else if(cov->tap_gesture_data.tap_type == THREE_FINGERS_GESTURE)
            {
               DEBUG("THREE_FINGERS_TRIPLE_TAP");
               _event_emit(THREE_FINGERS_TRIPLE_TAP,
                           cov->tap_gesture_data.x_org[0], cov->tap_gesture_data.y_org[0],
                           cov->tap_gesture_data.x_org[2], cov->tap_gesture_data.y_org[2],
                           state, cov->event_time);
            }
         else
            {
               ERROR("Unknown tap");
            }
         break;
      default:
         ERROR("Unknown tap");
         break;
      }
}

static Eina_Bool
_on_tap_timer_expire(void *data)
{
   Cover *cov = data;
   DEBUG("Timer expired");

   if (cov->tap_gesture_data.started && !cov->tap_gesture_data.pressed)
      _tap_event_emit(cov,2);
   else
      _tap_event_emit(cov,3);

   // finish gesture
   cov->tap_gesture_data.started = EINA_FALSE;
   cov->tap_gesture_data.timer = NULL;
   cov->tap_gesture_data.tap_type = ONE_FINGER_GESTURE;
   cov->tap_gesture_data.finger[0] = -1;
   cov->tap_gesture_data.finger[1] = -1;
   cov->tap_gesture_data.finger[2] = -1;

   return EINA_FALSE;
}

static int _tap_gesture_finger_check(Cover *cov, int x, int y)
{
   int dx = x - cov->tap_gesture_data.x_org[0];
   int dy = y - cov->tap_gesture_data.y_org[0];

   if (cov->tap_gesture_data.finger[0] != -1 &&
         (dx * dx + dy * dy < _e_mod_config->one_finger_tap_radius *
          _e_mod_config->one_finger_tap_radius))
      {
         return 0;
      }

   dx = x - cov->tap_gesture_data.x_org[1];
   dy = y - cov->tap_gesture_data.y_org[1];
   if (cov->tap_gesture_data.finger[1] != -1 &&
         (dx * dx + dy * dy < _e_mod_config->one_finger_tap_radius *
          _e_mod_config->one_finger_tap_radius))
      {
         return 1;
      }

   dx = x - cov->tap_gesture_data.x_org[2];
   dy = y - cov->tap_gesture_data.y_org[2];
   if (cov->tap_gesture_data.finger[2] != -1 &&
         (dx * dx + dy * dy < _e_mod_config->one_finger_tap_radius *
          _e_mod_config->one_finger_tap_radius))
      {
         return 2;
      }

   return -1;
}

static void
_tap_gestures_mouse_down(Ecore_Event_Mouse_Button *ev, Cover *cov)
{
   if (cov->n_taps > 4)
      {
         ERROR("Too many fingers");
         return;
      }

   cov->tap_gesture_data.pressed = EINA_TRUE;

   if (cov->tap_gesture_data.started == EINA_FALSE)
      {
         DEBUG("First finger down");
         cov->tap_gesture_data.started = EINA_TRUE;
         cov->tap_gesture_data.finger[0] = ev->multi.device;
         cov->tap_gesture_data.x_org[0] = ev->root.x;
         cov->tap_gesture_data.y_org[0] = ev->root.y;
         cov->tap_gesture_data.finger[1] = -1;
         cov->tap_gesture_data.finger[2] = -1;
         cov->tap_gesture_data.n_taps = 0;
         cov->tap_gesture_data.timer = ecore_timer_add(
                                          _e_mod_config->one_finger_tap_timeout,
                                          _on_tap_timer_expire, cov);
         cov->tap_gesture_data.tap_type = ONE_FINGER_GESTURE;
      }

   else
      {
         if (ev->multi.device == cov->tap_gesture_data.finger[0])
            {
               DEBUG("First finger down");

               if (_tap_gesture_finger_check(cov, ev->root.x, ev->root.y) == -1)
                  {
                     ERROR("Abort gesture");
                     cov->tap_gesture_data.started = EINA_FALSE;
                     ecore_timer_del(cov->tap_gesture_data.timer);
                     cov->tap_gesture_data.timer = NULL;
                     cov->tap_gesture_data.tap_type = ONE_FINGER_GESTURE;
                     cov->tap_gesture_data.finger[0] = -1;
                     cov->tap_gesture_data.finger[1] = -1;
                     cov->tap_gesture_data.finger[2] = -1;
                     _tap_gestures_mouse_down(ev, cov);
                     return;
                  }

               cov->tap_gesture_data.x_org[0] = ev->root.x;
               cov->tap_gesture_data.y_org[0] = ev->root.y;
            }
         else if (cov->tap_gesture_data.finger[1] == -1 ||
                  cov->tap_gesture_data.finger[1] == ev->multi.device)
            {
               DEBUG("Second finger down");
               cov->tap_gesture_data.finger[1] = ev->multi.device;

               cov->tap_gesture_data.x_org[1] = ev->root.x;
               cov->tap_gesture_data.y_org[1] = ev->root.y;
               if (cov->tap_gesture_data.tap_type < TWO_FINGERS_GESTURE)
                  cov->tap_gesture_data.tap_type = TWO_FINGERS_GESTURE;
            }
         else if (cov->tap_gesture_data.finger[2] == -1 ||
                  cov->tap_gesture_data.finger[2] == ev->multi.device)
            {
               DEBUG("Third finger down");
               cov->tap_gesture_data.finger[2] = ev->multi.device;

               cov->tap_gesture_data.x_org[2] = ev->root.x;
               cov->tap_gesture_data.y_org[2] = ev->root.y;
               if (cov->tap_gesture_data.tap_type < THREE_FINGERS_GESTURE)
                  cov->tap_gesture_data.tap_type = THREE_FINGERS_GESTURE;
            }
         else
            {
               ERROR("Unknown finger down");
            }
         ecore_timer_reset(cov->tap_gesture_data.timer);
      }
}

static void
_tap_gestures_mouse_up(Ecore_Event_Mouse_Button *ev, Cover *cov)
{
   if (cov->tap_gesture_data.timer)
      {
         cov->tap_gesture_data.pressed = EINA_FALSE;

         if (ev->multi.device == cov->tap_gesture_data.finger[0])
            {
               DEBUG("First finger up");

               int dx = ev->root.x - cov->tap_gesture_data.x_org[0];
               int dy = ev->root.y - cov->tap_gesture_data.y_org[0];

               if((dx * dx + dy * dy) < _e_mod_config->one_finger_tap_radius *
                     _e_mod_config->one_finger_tap_radius)
                  {
                     if (cov->n_taps == 0)
                        {
                           cov->tap_gesture_data.n_taps++;
                        }
                  }
               else
                  {
                     ERROR("Abort gesture");
                     cov->tap_gesture_data.started = EINA_FALSE;
                  }
            }
         else if (ev->multi.device == cov->tap_gesture_data.finger[1])
            {
               DEBUG("Second finger up");

               int dx = ev->root.x - cov->tap_gesture_data.x_org[1];
               int dy = ev->root.y - cov->tap_gesture_data.y_org[1];

               if((dx * dx + dy * dy) < _e_mod_config->one_finger_tap_radius *
                     _e_mod_config->one_finger_tap_radius)
                  {
                     if (cov->n_taps == 0)
                        {
                           cov->tap_gesture_data.n_taps++;
                        }
                  }
               else
                  {
                     ERROR("Abort gesture");
                     cov->tap_gesture_data.started = EINA_FALSE;
                  }
            }
         else if (ev->multi.device == cov->tap_gesture_data.finger[2])
            {
               DEBUG("Third finger up");

               int dx = ev->root.x - cov->tap_gesture_data.x_org[2];
               int dy = ev->root.y - cov->tap_gesture_data.y_org[2];

               if((dx * dx + dy * dy) < _e_mod_config->one_finger_tap_radius *
                     _e_mod_config->one_finger_tap_radius)
                  {
                     if (cov->n_taps == 0)
                        {
                           cov->tap_gesture_data.n_taps++;
                        }
                  }
               else
                  {
                     ERROR("Abort gesture");
                     cov->tap_gesture_data.started = EINA_FALSE;
                  }
            }
         else
            {
               ERROR("Unknown finger up, abort gesture");
               cov->tap_gesture_data.started = EINA_FALSE;
            }
      }
}

static void
_tap_gestures_move(Ecore_Event_Mouse_Move *ev, Cover *cov)
{
   int i;
   for (i = 0; i < sizeof(cov->tap_gesture_data.finger)/sizeof(cov->tap_gesture_data.finger[0]); i++)
      {
         if (ev->multi.device == cov->tap_gesture_data.finger[i])
            {
               int dx = ev->root.x - cov->tap_gesture_data.x_org[i];
               int dy = ev->root.y - cov->tap_gesture_data.y_org[i];

               if((dx * dx + dy * dy) > _e_mod_config->one_finger_tap_radius *
                     _e_mod_config->one_finger_tap_radius)
                  {
                     DEBUG("abort tap gesutre");
                     cov->tap_gesture_data.started = EINA_FALSE;
                     ecore_timer_del(cov->tap_gesture_data.timer);
                     cov->tap_gesture_data.timer = NULL;
                     cov->tap_gesture_data.tap_type = ONE_FINGER_GESTURE;
                     cov->tap_gesture_data.finger[0] = -1;
                     cov->tap_gesture_data.finger[1] = -1;
                     cov->tap_gesture_data.finger[2] = -1;
                  }
               break;
            }
      }
}

static Eina_Bool
_cb_mouse_down(void    *data EINA_UNUSED,
               int      type EINA_UNUSED,
               void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;

   cov->n_taps++;
   cov->event_time = ev->timestamp;

   DEBUG("mouse down: multi.device: %d, taps: %d", ev->multi.device, cov->n_taps);

   win_angle = _win_angle_get();

   _flick_gesture_mouse_down(ev, cov);
   _hover_gesture_mouse_down(ev, cov);
   _tap_gestures_mouse_down(ev, cov);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_up(void    *data EINA_UNUSED,
             int      type EINA_UNUSED,
             void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;

   cov->n_taps--;
   cov->event_time = ev->timestamp;

   DEBUG("mouse up, multi.device: %d, taps: %d", ev->multi.device, cov->n_taps);

   _flick_gesture_mouse_up(ev, cov);
   _hover_gesture_mouse_up(ev, cov);
   _tap_gestures_mouse_up(ev, cov);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_move(void    *data EINA_UNUSED,
               int      type EINA_UNUSED,
               void    *event)
{
   Ecore_Event_Mouse_Move *ev = event;

   cov->event_time = ev->timestamp;

   _flick_gesture_mouse_move(ev, cov);
   _hover_gesture_mouse_move(ev, cov);
   _tap_gestures_move(ev, cov);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_gesture_input_win_create(void)
{
   int w, h;

   if (!win)
     {
        Ecore_Window root = ecore_x_window_root_first_get();
        if (!root)
          return EINA_FALSE;
        ecore_x_window_geometry_get(root, NULL, NULL, &w, &h);
        win = ecore_x_window_input_new(root, 0, 0, w, h);
     }
   if (!win)
     return EINA_FALSE;

   ecore_x_input_multi_select(win);
   ecore_x_window_show(win);
   ecore_x_window_raise(win);

   // restet gestures
   memset(cov, 0x0, sizeof(Cover));

   return EINA_TRUE;
}

static Eina_Bool
_win_property_changed(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Property *wp = event;

   if (wp->atom != ECORE_X_ATOM_NET_CLIENT_LIST_STACKING)
     return EINA_TRUE;

   _gesture_input_win_create();

   return EINA_TRUE;
}

static Eina_Bool
_gestures_input_window_init(void)
{
   Ecore_Window root = ecore_x_window_root_first_get();
   if (!root)
     {
        ERROR("No root window found. Is Window manager running?");
        return EINA_FALSE;
     }
   ecore_x_event_mask_set(root, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
   property_changed_hld = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _win_property_changed, NULL);

   return _gesture_input_win_create();
}

static void
_gestures_input_widnow_shutdown(void)
{
   ecore_event_handler_del(property_changed_hld);
   if (win) ecore_x_window_free(win);
   win = 0;
}

Eina_Bool screen_reader_gestures_init(void)
{
   ecore_init();
   ecore_x_init(NULL);

   cov = calloc(sizeof(Cover), 1);

   if (!_gestures_input_window_init())
     {
        free(cov);
        return EINA_FALSE;
     }

   _e_mod_config = calloc(sizeof(Gestures_Config), 1);
   _e_mod_config->one_finger_flick_min_length = 100;
   _e_mod_config->one_finger_flick_max_time = 400;
   _e_mod_config->two_finger_flick_to_scroll_timeout = 100;
   _e_mod_config->two_finger_flick_to_scroll_min_length = 50;
   _e_mod_config->one_finger_hover_longpress_timeout = 0.81;
   _e_mod_config->one_finger_tap_timeout = 0.4;
   _e_mod_config->one_finger_tap_radius = 100;

   handlers = eina_list_append(NULL, ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, _cb_mouse_move, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, _cb_mouse_up, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, _cb_mouse_down, NULL));

   return EINA_TRUE;
}

void screen_reader_gestures_shutdown(void)
{
   Ecore_Event_Handler *hdlr;
   EINA_LIST_FREE(handlers, hdlr)
   {
      ecore_event_handler_del(hdlr);
   }
   _gestures_input_widnow_shutdown();

   ecore_x_shutdown();
   ecore_shutdown();
   free(_e_mod_config);
   free(cov);
}

void screen_reader_gestures_tracker_register(GestureCB cb, void *data)
{
   _global_cb = cb;
   _global_data = data;
}
