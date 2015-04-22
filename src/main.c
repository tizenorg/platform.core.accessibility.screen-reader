#include <app.h>
#include "navigator.h"
#include "window_tracker.h"
#include "logger.h"
#include "screen_reader.h"


static bool app_create(void *data)
{
    atspi_init();
    navigator_init();
    screen_reader_create_service(data);

    return true;
}

static void app_terminate(void *data)
{
    screen_reader_terminate_service(data);
    navigator_shutdown();
}

int main(int argc, char **argv)
{
    app_create(get_pointer_to_service_data_struct());

    GMainLoop *ml = g_main_loop_new(NULL, FALSE);
    g_main_loop_run (ml);

    app_terminate(get_pointer_to_service_data_struct());

    return 0;
}
