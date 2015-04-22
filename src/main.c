#include <appcore-efl.h>
#include <eldbus-1/Eldbus.h>
#include "navigator.h"
#include "window_tracker.h"
#include "gesture_tracker.h"
#include "logger.h"
#include "screen_reader.h"

#define A11Y_BUS "org.a11y.Bus"
#define A11Y_INTERFACE "org.a11y.Bus"
#define A11Y_PATH "/org/a11y/bus"
#define A11Y_GET_ADDRESS "GetAddress"

Eldbus_Connection *a11y_conn;

static void _init_modules(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   printf("START:%s", __func__);
   const char *a11y_bus_address = NULL;

   logger_init();
   if(!eldbus_message_arguments_get(msg, "s", &a11y_bus_address))
      ERROR("error geting arguments _init_modules");
   Eldbus_Connection *a11y_conn = eldbus_address_connection_get(a11y_bus_address);

   gesture_tracker_init(a11y_conn);
   navigator_init();
}

static int app_create(void *data)
{
    printf("START:%s", __func__);
    eldbus_init();
    elm_init(0, NULL);

    Eldbus_Connection *session_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
    Eldbus_Object *a11y_obj = eldbus_object_get(session_conn, A11Y_BUS, A11Y_PATH);
    Eldbus_Proxy *manager = eldbus_proxy_get(a11y_obj, A11Y_INTERFACE);
    eldbus_proxy_call(manager, A11Y_GET_ADDRESS, _init_modules, data, -1, "");

    screen_reader_create_service(data);

    return 0;
}

static int app_terminate(void *data)
{
    printf("START:%s", __func__);
    screen_reader_terminate_service(data);

    eldbus_connection_unref(a11y_conn);
    gesture_tracker_shutdown();
    navigator_shutdown();
    eldbus_shutdown();
    logger_shutdown();
    return 0;
}

int main(int argc, char **argv)
{
    DEBUG("Screen Reader Main Function Start");
    unsetenv("ELM_ATSPI_MODE");

    struct appcore_ops ops =
    {
        .create = app_create,
        .terminate = app_terminate,
        .pause = NULL,
        .resume = NULL,
        .reset = NULL
    };

    ops.data = get_pointer_to_service_data_struct();

    return appcore_efl_main("Smart Navigation", &argc, &argv, &ops);
}
