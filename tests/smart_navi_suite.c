
#include "screen_reader_spi.h"
#include "flat_navi.h"
#include <check.h>
#include <stdio.h>
#include <atspi/atspi.h>
#include <Eina.h>

static AtspiAccessible *root;
static AtspiAccessible *child1;
static AtspiAccessible *child2;
static AtspiAccessible *child3;
static AtspiAccessible *child4;
static FlatNaviContext *ctx;

void setup(void)
{
    Service_Data *data = get_pointer_to_service_data_struct();
    data->information_level = 1;
    data->run_service = 1;
    data->voice_type = TTS_VOICE_TYPE_FEMALE;
    data->reading_speed = 2;
    data->tracking_signal_name = FOCUS_CHANGED_SIG;

    //Set by tts
    data->tts = NULL;
    data->available_languages = NULL;

    //Actions to do when tts state is 'ready'
    data->say_text = false;
    data->update_language_list = false;

    data->text_to_say_info = NULL;
}

void teardown(void)
{
}

void setup_flat_navi()
{
    setup();
    root = atspi_create_accessible();
    root->role = ATSPI_ROLE_APPLICATION;
    child1 = atspi_create_accessible();
    child2 = atspi_create_accessible();
    child3 = atspi_create_accessible();
    child4 = atspi_create_accessible();
    child1->role = ATSPI_ROLE_PUSH_BUTTON;
    child2->role = ATSPI_ROLE_ICON;
    child3->role = ATSPI_ROLE_CHECK_BOX;
    child4->role = ATSPI_ROLE_ENTRY;
    atspi_accessible_add_child(root, child1);
    atspi_accessible_add_child(root, child2);
    atspi_accessible_add_child(root, child3);
    atspi_accessible_add_child(root, child4);
    eina_init();
    atspi_alloc_memory();
    ctx = flat_navi_context_create(root);
}

void teardown_flat_navi()
{
    flat_navi_context_free(ctx);
    atspi_free_memory();
    eina_shutdown();
    atspi_delete_accessible(root);
    teardown();
}

START_TEST(spi_init_null_parameter)
{
    spi_init(NULL);
}
END_TEST

START_TEST(spi_init_service_data_parameter)
{
    Service_Data *data = get_pointer_to_service_data_struct();
    spi_init(data);
}
END_TEST

START_TEST(spi_on_state_change_name)
{
    Service_Data *sd = get_pointer_to_service_data_struct();
    AtspiEvent event;
    AtspiAccessible accessible;
    event.type = "test_event";
    sd->tracking_signal_name = "test_event";
    event.detail1 = 1;
    accessible.name = "test_name";
    accessible.role = ATSPI_ROLE_ICON;
    accessible.description = NULL;
    event.source = &accessible;
    char *return_value = spi_event_get_text_to_read(&event, sd);
    fail_if(!return_value || strcmp(return_value, "test_name, Icon"));
    free(return_value);
}
END_TEST

START_TEST(spi_on_state_change_description)
{
    Service_Data *sd = get_pointer_to_service_data_struct();
    AtspiEvent event;
    AtspiAccessible accessible;
    event.type = "test_event";
    sd->tracking_signal_name = "test_event";
    event.detail1 = 1;
    accessible.name = "test_name";
    accessible.description = "test description";
    accessible.role = ATSPI_ROLE_ICON;
    event.source = &accessible;
    char *return_value = spi_event_get_text_to_read(&event, sd);
    fail_if(!return_value || strcmp(return_value, "test_name, Icon, test description"));
    free(return_value);
}
END_TEST

START_TEST(spi_on_state_change_role)
{
    Service_Data *sd = get_pointer_to_service_data_struct();
    AtspiEvent event;
    AtspiAccessible accessible;
    event.type = "test_event";
    sd->tracking_signal_name = "test_event";
    event.detail1 = 1;
    accessible.role = ATSPI_ROLE_ICON;
    accessible.name = NULL;
    accessible.description = NULL;
    event.source = &accessible;
    char *return_value = spi_event_get_text_to_read(&event, sd);
    fail_if(!return_value || strcmp(return_value, atspi_accessible_get_role_name(&accessible, NULL)));
    free(return_value);
}
END_TEST

START_TEST(spi_on_caret_move)
{
    Service_Data *sd = get_pointer_to_service_data_struct();
    AtspiEvent event;
    AtspiAccessible accessible;
    event.type = "object:text-caret-moved";
    accessible.name = "test_name";
    event.source = &accessible;
    atspi_alloc_memory();
    char *return_value = spi_event_get_text_to_read(&event, sd);
    atspi_free_memory();
    fail_if(!return_value || strcmp(return_value, "AtspiText text"));
    free(return_value);
}
END_TEST

START_TEST(spi_on_value_changed)
{
Service_Data *sd = get_pointer_to_service_data_struct();
    AtspiEvent event;
    AtspiAccessible accessible;
    event.type = VALUE_CHANGED_SIG;
    accessible.name = "test_name";
    event.source = &accessible;
    atspi_alloc_memory();
    char *return_value = spi_event_get_text_to_read(&event, sd);
    atspi_free_memory();
    fail_if(!return_value || strcmp(return_value, "1.00"));
    free(return_value);
}
END_TEST

START_TEST(spi_flat_navi_context_create_null_parameter)
{
    FlatNaviContext *test_ctx = flat_navi_context_create(NULL);
    fail_if(!test_ctx);
    free(test_ctx);
}
END_TEST

START_TEST(spi_flat_navi_context_create_valid_parameter)
{
    FlatNaviContext *test_ctx = flat_navi_context_create(root);
    fail_if(!test_ctx);
    flat_navi_context_free(test_ctx);
}
END_TEST

START_TEST(spi_flat_navi_context_get_current_null_parameter)
{
    AtspiAccessible *current = flat_navi_context_current_get(NULL);
    fail_if(current);
}
END_TEST

START_TEST(spi_flat_navi_context_get_current_valid_parameter)
{
    AtspiAccessible *current = flat_navi_context_current_get(ctx);
    fail_if(!current || current != child1);
}
END_TEST

START_TEST(spi_flat_navi_context_next_null_parameter)
{
    AtspiAccessible *next = flat_navi_context_next(NULL);
    fail_if(next);
}
END_TEST

START_TEST(spi_flat_navi_context_next_valid_parameter)
{
    AtspiAccessible *next = flat_navi_context_next(ctx);
    fail_if(!next || next != child2);
}
END_TEST

START_TEST(spi_flat_navi_context_prev_null_parameter)
{
    AtspiAccessible *prev = flat_navi_context_prev(NULL);
    fail_if(prev);
}
END_TEST

START_TEST(spi_flat_navi_context_prev_valid_parameter)
{
    AtspiAccessible *prev = flat_navi_context_prev(ctx);
    fail_if(prev);
    flat_navi_context_current_set(ctx, child4);
    prev = flat_navi_context_prev(ctx);
    fail_if(!prev || prev != child3);
}
END_TEST

START_TEST(spi_flat_navi_context_last_null_parameter)
{
    AtspiAccessible *last = flat_navi_context_last(NULL);
    fail_if(last);
}
END_TEST

START_TEST(spi_flat_navi_context_last_valid_parameter)
{
    AtspiAccessible *last = flat_navi_context_last(ctx);
    fail_if(!last || last != child2);
}
END_TEST

START_TEST(spi_flat_navi_context_first_null_parameter)
{
    AtspiAccessible *first = flat_navi_context_first(NULL);
    fail_if(first);
}
END_TEST

START_TEST(spi_flat_navi_context_first_valid_parameter)
{
    AtspiAccessible *first = flat_navi_context_first(ctx);
    fail_if(!first || first != child1);
}
END_TEST

START_TEST(spi_flat_navi_context_current_set_null_parameters)
{
    Eina_Bool ret = flat_navi_context_current_set(NULL, NULL);
    fail_if(ret != EINA_FALSE);
    ret = flat_navi_context_current_set(ctx, NULL);
    fail_if(ret != EINA_FALSE);
    AtspiAccessible *last = flat_navi_context_last(ctx);
    ret = flat_navi_context_current_set(NULL, last);
    fail_if(ret != EINA_FALSE);
}
END_TEST

START_TEST(spi_flat_navi_context_current_set_valid_parameters)
{
    AtspiAccessible *last = flat_navi_context_last(ctx);
    Eina_Bool ret = flat_navi_context_current_set(ctx, last);
    AtspiAccessible *current = flat_navi_context_current_get(ctx);
    fail_if(ret != EINA_TRUE || current != last);
}
END_TEST

START_TEST(spi_flat_navi_context_line_prev_null_parameter)
{
    AtspiAccessible *prev = flat_navi_context_line_prev(NULL);
    fail_if(prev);
}
END_TEST

START_TEST(spi_flat_navi_context_line_prev_valid_parameter)
{
    AtspiAccessible *prev = flat_navi_context_line_prev(ctx);
    fail_if(prev);
    flat_navi_context_current_set(ctx, child4);
    prev = flat_navi_context_line_prev(ctx);
    fail_if(!prev || prev != child1);
}
END_TEST

START_TEST(spi_flat_navi_context_line_next_null_parameter)
{
    AtspiAccessible *next = flat_navi_context_line_next(NULL);
    fail_if(next);
}
END_TEST

START_TEST(spi_flat_navi_context_line_next_valid_parameter)
{
    AtspiAccessible *next = flat_navi_context_line_next(ctx);
    fail_if(!next || next != child3);
}
END_TEST

START_TEST(spi_flat_navi_context_line_first_null_parameter)
{
    AtspiAccessible *first = flat_navi_context_line_first(NULL);
    fail_if(first);
}
END_TEST

START_TEST(spi_flat_navi_context_line_first_valid_parameter)
{
    AtspiAccessible *first = flat_navi_context_line_first(ctx);
    fail_if(!first || first != child1);
}
END_TEST

START_TEST(spi_flat_navi_context_line_last_null_parameter)
{
    AtspiAccessible *last = flat_navi_context_line_last(NULL);
    fail_if(last);
}
END_TEST

START_TEST(spi_flat_navi_context_line_last_valid_parameter)
{
    AtspiAccessible *last = flat_navi_context_line_last(ctx);
    fail_if(!last || last != child3);
}
END_TEST

Suite *screen_reader_suite(void)
{
    Suite *s;
    TCase *tc_spi_screen_reader_init;
    TCase *tc_spi_screen_reader_on_state_changed;
    TCase *tc_spi_screen_reader_on_caret_move;
    TCase *tc_spi_screen_reader_on_access_value;
    TCase *tc_spi_screen_reader_flat_navi;

    s = suite_create("Screen reader");
    tc_spi_screen_reader_init = tcase_create("tc_spi_screen_reader_init");
    tc_spi_screen_reader_on_state_changed = tcase_create("tc_spi_screen_reader_on_state_changed");
    tc_spi_screen_reader_on_caret_move = tcase_create("tc_spi_screen_reader_on_caret_move");
    tc_spi_screen_reader_on_access_value = tcase_create("tc_spi_screen_reader_on_access_value");
    tc_spi_screen_reader_flat_navi = tcase_create("tc_scpi_screen_reader_flat_navi");

    tcase_add_checked_fixture(tc_spi_screen_reader_init, setup, teardown);
    tcase_add_checked_fixture(tc_spi_screen_reader_on_state_changed, setup, teardown);
    tcase_add_checked_fixture(tc_spi_screen_reader_on_caret_move, setup, teardown);
    tcase_add_checked_fixture(tc_spi_screen_reader_on_access_value, setup, teardown);
    tcase_add_checked_fixture(tc_spi_screen_reader_flat_navi, setup_flat_navi, teardown_flat_navi);

    tcase_add_test(tc_spi_screen_reader_init, spi_init_null_parameter);
    tcase_add_test(tc_spi_screen_reader_init, spi_init_service_data_parameter);
    tcase_add_test(tc_spi_screen_reader_on_state_changed, spi_on_state_change_name);
    tcase_add_test(tc_spi_screen_reader_on_state_changed, spi_on_state_change_description);
    tcase_add_test(tc_spi_screen_reader_on_state_changed, spi_on_state_change_role);
    tcase_add_test(tc_spi_screen_reader_on_caret_move, spi_on_caret_move);
    tcase_add_test(tc_spi_screen_reader_on_access_value, spi_on_value_changed);

    suite_add_tcase(s, tc_spi_screen_reader_init);
    suite_add_tcase(s, tc_spi_screen_reader_on_state_changed);
    suite_add_tcase(s, tc_spi_screen_reader_on_caret_move);
    suite_add_tcase(s, tc_spi_screen_reader_on_access_value);
    suite_add_tcase(s, tc_spi_screen_reader_flat_navi);

    return s;
}

int main()
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = screen_reader_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
