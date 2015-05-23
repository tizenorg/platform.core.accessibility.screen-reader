#include <appcore-efl.h>
#include <Elementary.h>
#include <eldbus-1/Eldbus.h>
#include "navigator.h"
#include "window_tracker.h"
#include "logger.h"
#include "screen_reader.h"
#include "screen_reader_gestures.h"


static int app_create(void *data)
{
    eldbus_init();
    elm_init(0, NULL);

    logger_init();
    screen_reader_gestures_init();
    screen_reader_create_service(data);
    navigator_init();

    return 0;
}

static int app_terminate(void *data)
{
    screen_reader_terminate_service(data);

    navigator_shutdown();
    screen_reader_gestures_shutdown();
    eldbus_shutdown();
    logger_shutdown();
    return 0;
}

int main(int argc, char **argv)
{
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
