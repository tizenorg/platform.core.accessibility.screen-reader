/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This file is a modified version of BSD licensed file and
 * licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please, see the LICENSE file for the original copyright owner and
 * license.
 */

#include "dbus_gesture_adapter.h"
#include "logger.h"

#include <Eldbus.h>

#define E_A11Y_SERVICE_BUS_NAME "com.samsung.EModule"
#define E_A11Y_SERVICE_NAVI_IFC_NAME "com.samsung.GestureNavigation"
#define E_A11Y_SERVICE_NAVI_OBJ_PATH "/com/samsung/GestureNavigation"

static Eldbus_Connection *conn;
static Eldbus_Service_Interface *service;

enum _Signals
{
   GESTURE_DETECTED = 0,
};

static const Eldbus_Signal _signals[] = {
   [GESTURE_DETECTED] = {"GestureDetected", ELDBUS_ARGS({"siiiiu", NULL}), 0},
   {NULL, ELDBUS_ARGS({NULL, NULL}), 0}
};

static const Eldbus_Service_Interface_Desc desc = {
   E_A11Y_SERVICE_NAVI_IFC_NAME, NULL, _signals, NULL, NULL, NULL
};

void _on_get_a11y_address(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   const char *sock_addr;
   const char *errname, *errmsg;
   Eldbus_Connection *session = data;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERROR("GetAddress failed: %s %s", errname, errmsg);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "s", &sock_addr) || !sock_addr)
     {
        ERROR("Could not get A11Y Bus socket address.");
        goto end;
     }

   if (!(conn = eldbus_address_connection_get(sock_addr)))
     {
        ERROR("Failed to connect to %s", sock_addr);
        goto end;
     }

   if (!(service = eldbus_service_interface_register(conn, E_A11Y_SERVICE_NAVI_OBJ_PATH, &desc)))
     {
        ERROR("Failed to register %s interface", E_A11Y_SERVICE_NAVI_IFC_NAME);
        eldbus_connection_unref(conn);
        conn = NULL;
        goto end;
     }

   eldbus_name_request(conn, E_A11Y_SERVICE_BUS_NAME, ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE, NULL, NULL);

end:
   eldbus_connection_unref(session);
}

void dbus_gesture_adapter_init(void)
{
   Eldbus_Connection *session;
   Eldbus_Message *msg;

   eldbus_init();

   if (!(session = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION)))
     {
        ERROR("Unable to get session bus");
        return;
     }

   if (!(msg = eldbus_message_method_call_new("org.a11y.Bus", "/org/a11y/bus",
                                      "org.a11y.Bus", "GetAddress")))
     {
        ERROR("DBus message allocation failed");
        goto fail_msg;
     }

   if (!eldbus_connection_send(session, msg, _on_get_a11y_address, session, -1))
     {
        ERROR("Message send failed");
        goto fail_send;
     }

   return;

fail_send:
   eldbus_message_unref(msg);
fail_msg:
   eldbus_connection_unref(session);
}

void dbus_gesture_adapter_shutdown(void)
{
   if (service) eldbus_service_object_unregister(service);
   if (conn) eldbus_connection_unref(conn);

   conn = NULL;
   service = NULL;

   eldbus_shutdown();
}

static const char *_gesture_enum_to_string(Gesture g)
{
   switch(g)
     {
      case ONE_FINGER_HOVER:
         return "OneFingerHover";
      case TWO_FINGERS_HOVER:
         return "TwoFingersHover";
      case THREE_FINGERS_HOVER:
         return "ThreeFingersHover";
      case ONE_FINGER_FLICK_LEFT:
         return "OneFingerFlickLeft";
      case ONE_FINGER_FLICK_RIGHT:
         return "OneFingerFlickRight";
      case ONE_FINGER_FLICK_UP:
         return "OneFingerFlickUp";
      case ONE_FINGER_FLICK_DOWN:
         return "OneFingerFlickDown";
      case TWO_FINGERS_FLICK_UP:
         return "TwoFingersFlickUp";
      case TWO_FINGERS_FLICK_DOWN:
         return "TwoFingersFlickDown";
      case TWO_FINGERS_FLICK_LEFT:
         return "TwoFingersFlickLeft";
      case TWO_FINGERS_FLICK_RIGHT:
         return "TwoFingersFlickRight";
      case THREE_FINGERS_FLICK_LEFT:
         return "ThreeFingersFlickLeft";
      case THREE_FINGERS_FLICK_RIGHT:
         return "ThreeFingersFlickRight";
      case THREE_FINGERS_FLICK_UP:
         return "ThreeFingersFlickUp";
      case THREE_FINGERS_FLICK_DOWN:
         return "ThreeFingersFlickDown";
      case ONE_FINGER_SINGLE_TAP:
         return "OneFingerSingleTap";
      case ONE_FINGER_DOUBLE_TAP:
         return "OneFingerDoubleTap";
      case ONE_FINGER_TRIPLE_TAP:
         return "OneFingerTripleTap";
      case TWO_FINGERS_SINGLE_TAP:
         return "TwoFingersSingleTap";
      case TWO_FINGERS_DOUBLE_TAP:
         return "TwoFingersDoubleTap";
      case TWO_FINGERS_TRIPLE_TAP:
         return "TwoFingersTripleTap";
      case THREE_FINGERS_SINGLE_TAP:
         return "ThreeFingersSingleTap";
      case THREE_FINGERS_DOUBLE_TAP:
         return "ThreeFingersDoubleTap";
      case THREE_FINGERS_TRIPLE_TAP:
         return "ThreeFingersTripleTap";
      case ONE_FINGER_FLICK_LEFT_RETURN:
         return "OneFingerFlickLeftReturn";
      case ONE_FINGER_FLICK_RIGHT_RETURN:
         return "OneFingerFlickRightReturn";
      case ONE_FINGER_FLICK_UP_RETURN:
         return "OneFingerFlickUpReturn";
      case ONE_FINGER_FLICK_DOWN_RETURN:
         return "OneFingerFlickDownReturn";
      case TWO_FINGERS_FLICK_LEFT_RETURN:
         return "TwoFingersFlickLeftReturn";
      case TWO_FINGERS_FLICK_RIGHT_RETURN:
         return "TwoFingersFlickRightReturn";
      case TWO_FINGERS_FLICK_UP_RETURN:
         return "TwoFingersFlickUpReturn";
      case TWO_FINGERS_FLICK_DOWN_RETURN:
         return "TwoFingersFlickDownReturn";
      case THREE_FINGERS_FLICK_LEFT_RETURN:
         return "ThreeFingersFlickLeftReturn";
      case THREE_FINGERS_FLICK_RIGHT_RETURN:
         return "ThreeFingersFlickRightReturn";
      case THREE_FINGERS_FLICK_UP_RETURN:
         return "ThreeFingersFlickUpReturn";
      case THREE_FINGERS_FLICK_DOWN_RETURN:
         return "ThreeFingersFlickDownReturn";
      default:
         ERROR("unhandled gesture enum");
         return NULL;
     }
}

void dbus_gesture_adapter_emit(const Gesture_Info *info)
{
   const char *name;

   if (!service)
     return;

   name = _gesture_enum_to_string(info->type);
   if (!name)
     return;

   if (!eldbus_service_signal_emit(service, GESTURE_DETECTED, name, info->x_beg, info->y_beg,
                                   info->x_end, info->y_end, info->state))
     {
        ERROR("Unable to send GestureDetected signal");
     }
   else
     DEBUG("Successfullt send GestureDetected singal");
}
