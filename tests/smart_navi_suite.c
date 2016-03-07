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

#include "screen_reader_spi.h"
#include "flat_navi.h"
#include "lua_engine.h"
#include <check.h>
#include <stdio.h>
#include <atspi/atspi.h>
#include <Eina.h>

static AtspiAccessible *root;
static AtspiAccessible *app;
static AtspiAccessible *win;
static AtspiAccessible *ly;
static AtspiAccessible *child1;
static AtspiAccessible *child2;
static AtspiAccessible *child3;
static AtspiAccessible *child4;
static AtspiAccessible *child5;
static AtspiAccessible *child6;
static AtspiAccessible *child7;
static AtspiAccessible *child8;
static AtspiAccessible *child9;
static AtspiAccessible *child10;
static AtspiAccessible *child11;
static AtspiAccessible *child12;
static AtspiAccessible *child13;
static AtspiAccessible *child14;
static AtspiAccessible *child15;
static AtspiAccessible *child16;
static AtspiAccessible *child17;
static AtspiAccessible *child18;
static AtspiAccessible *child19;
static AtspiAccessible *child20;
static AtspiAccessible *child21;
static AtspiAccessible *child22;
static AtspiAccessible *child23;
static FlatNaviContext *ctx;

void setup(void)
{
	Service_Data *data = get_pointer_to_service_data_struct();
	data->run_service = 1;
	data->tracking_signal_name = HIGHLIGHT_CHANGED_SIG;

	//Set by tts
	data->tts = NULL;
	data->available_languages = NULL;

	//Actions to do when tts state is 'ready'
	data->say_text = false;
	data->update_language_list = false;

	data->text_to_say_info = NULL;
	data->lua_script_path = "./res/scripts/mobile.lua";
	spi_init(data);
}

void teardown(void)
{
	Service_Data *data = get_pointer_to_service_data_struct();
	spi_shutdown(data);
}

void setup_flat_navi() {
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

void setup_flat_navi2()
{
	setup();
	app = atspi_create_accessible();
	app->role = ATSPI_ROLE_APPLICATION;

	root = app;
	win = atspi_create_accessible();
	win->role = ATSPI_ROLE_WINDOW;

	child1 = atspi_create_accessible();
	child2 = atspi_create_accessible();
	child3 = atspi_create_accessible();
	child4 = atspi_create_accessible();
	child5 = atspi_create_accessible();
	child6 = atspi_create_accessible();
	child7 = atspi_create_accessible();
	child8 = atspi_create_accessible();

	child1->role = ATSPI_ROLE_FILLER;
	child2->role = ATSPI_ROLE_FILLER;
	child3->role = ATSPI_ROLE_FILLER;
	child4->role = ATSPI_ROLE_FILLER;
	child7->role = ATSPI_ROLE_FILLER;
	child8->role = ATSPI_ROLE_FILLER;

	child5->role = ATSPI_ROLE_PUSH_BUTTON;
	child5->name = "btn1";
	child6->role = ATSPI_ROLE_PUSH_BUTTON;
	child6->name = "btn2";

	atspi_accessible_add_child(app, win);
	atspi_accessible_add_child(win, child1);
	atspi_accessible_add_child(win, child7);
	atspi_accessible_add_child(win, child2);

	atspi_accessible_add_child(child1, child3);
	atspi_accessible_add_child(child2, child4);

	atspi_accessible_add_child(child7, child8);

	atspi_accessible_add_child(child3, child5);
	atspi_accessible_add_child(child4, child6);
	eina_init();
	atspi_alloc_memory();
	ctx = flat_navi_context_create(win);
}

void setup_flat_navi3()
{
	setup();
	app = atspi_create_accessible();
	app->role = ATSPI_ROLE_APPLICATION;

	root = app;
	win = atspi_create_accessible();
	win->role = ATSPI_ROLE_WINDOW;

	ly = atspi_create_accessible();
	ly->role = ATSPI_ROLE_FILLER;

	child1 = atspi_create_accessible();
	child2 = atspi_create_accessible();
	child3 = atspi_create_accessible();
	child4 = atspi_create_accessible();
	child5 = atspi_create_accessible();
	child6 = atspi_create_accessible();
	child7 = atspi_create_accessible();
	child8 = atspi_create_accessible();
	child9 = atspi_create_accessible();
	child10 = atspi_create_accessible();
	child11 = atspi_create_accessible();
	child12 = atspi_create_accessible();
	child13 = atspi_create_accessible();
	child14 = atspi_create_accessible();
	child15 = atspi_create_accessible();
	child16 = atspi_create_accessible();
	child17 = atspi_create_accessible();
	child18 = atspi_create_accessible();
	child19 = atspi_create_accessible();
	child20 = atspi_create_accessible();
	child21 = atspi_create_accessible();
	child22 = atspi_create_accessible();
	child23 = atspi_create_accessible();

	child1->role = ATSPI_ROLE_FILLER;
	child3->role = ATSPI_ROLE_FILLER;
	child4->role = ATSPI_ROLE_FILLER;
	child5->role = ATSPI_ROLE_FILLER;
	child6->role = ATSPI_ROLE_FILLER;
	child7->role = ATSPI_ROLE_FILLER;
	child8->role = ATSPI_ROLE_FILLER;
	child9->role = ATSPI_ROLE_FILLER;
	child10->role = ATSPI_ROLE_FILLER;
	child11->role = ATSPI_ROLE_FILLER;
	child12->role = ATSPI_ROLE_FILLER;
	child14->role = ATSPI_ROLE_FILLER;
	child15->role = ATSPI_ROLE_FILLER;
	child17->role = ATSPI_ROLE_FILLER;
	child18->role = ATSPI_ROLE_FILLER;
	child20->role = ATSPI_ROLE_FILLER;
	child21->role = ATSPI_ROLE_FILLER;
	child23->role = ATSPI_ROLE_FILLER;

	child2->role = ATSPI_ROLE_PUSH_BUTTON;
	child2->name = "btn1";
	child13->role = ATSPI_ROLE_PUSH_BUTTON;
	child13->name = "btn2";
	child16->role = ATSPI_ROLE_PUSH_BUTTON;
	child16->name = "btn3";
	child19->role = ATSPI_ROLE_PUSH_BUTTON;
	child19->name = "btn4";
	child22->role = ATSPI_ROLE_PUSH_BUTTON;
	child22->name = "btn5";

	atspi_accessible_add_child(app, win);
	atspi_accessible_add_child(win, ly);

	atspi_accessible_add_child(ly, child1);
	atspi_accessible_add_child(ly, child3);
	atspi_accessible_add_child(ly, child4);

	atspi_accessible_add_child(child1, child2);

	atspi_accessible_add_child(child4, child5);

	atspi_accessible_add_child(child5, child6);

	atspi_accessible_add_child(child6, child7);

	atspi_accessible_add_child(child7, child8);
	atspi_accessible_add_child(child7, child9);
	atspi_accessible_add_child(child7, child10);
	atspi_accessible_add_child(child7, child11);

	atspi_accessible_add_child(child8, child12);
	atspi_accessible_add_child(child8, child13);
	atspi_accessible_add_child(child8, child14);

	atspi_accessible_add_child(child9, child15);
	atspi_accessible_add_child(child9, child16);
	atspi_accessible_add_child(child9, child17);

	atspi_accessible_add_child(child10, child18);
	atspi_accessible_add_child(child10, child19);
	atspi_accessible_add_child(child10, child20);

	atspi_accessible_add_child(child11, child21);
	atspi_accessible_add_child(child11, child22);
	atspi_accessible_add_child(child11, child23);

	eina_init();
	atspi_alloc_memory();
	ctx = flat_navi_context_create(win);
}

void teardown_flat_navi()
{
	flat_navi_context_free(ctx);
	atspi_free_memory();
	eina_shutdown();
	atspi_delete_accessible(root);
	teardown();
}

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

END_TEST START_TEST(spi_on_state_change_description)
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

END_TEST START_TEST(spi_on_state_change_role)
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
	char *role_name = atspi_accessible_get_role_name(&accessible, NULL);
	fail_if(!return_value || (role_name && strcmp(return_value, role_name)));
	free(return_value);
	free(role_name);
}

END_TEST START_TEST(spi_on_caret_move)
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

END_TEST START_TEST(spi_on_value_changed)
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

END_TEST START_TEST(spi_flat_navi_context_create_null_parameter)
{
	FlatNaviContext *test_ctx = flat_navi_context_create(NULL);
	fail_if(test_ctx);
}

END_TEST START_TEST(spi_flat_navi_context_create_valid_parameter)
{
	FlatNaviContext *test_ctx = flat_navi_context_create(win);
	fail_if(!test_ctx);
	flat_navi_context_free(test_ctx);
}

END_TEST START_TEST(spi_flat_navi_context_get_current_null_parameter)
{
	AtspiAccessible *current = flat_navi_context_current_get(NULL);
	fail_if(current);
}

END_TEST START_TEST(spi_flat_navi_context_get_current_valid_parameter)
{
	AtspiAccessible *current = flat_navi_context_current_get(ctx);
	fail_if(!current || current != child5);
}

END_TEST START_TEST(spi_flat_navi_context_next_null_parameter)
{
	AtspiAccessible *next = flat_navi_context_next(NULL);
	fail_if(next);
}

END_TEST START_TEST(spi_flat_navi_context_next_valid_parameter)
{
	AtspiAccessible *next = flat_navi_context_next(ctx);

	fail_if(!next || next != child6);
}

END_TEST START_TEST(spi_flat_navi_context_next_valid_parameter2)
{
	AtspiAccessible *next = flat_navi_context_next(ctx);

	fail_if(!next || next != child13);
}

END_TEST START_TEST(spi_flat_navi_context_next_valid_parameter3)
{
	AtspiAccessible *next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);

	fail_if(!next || next != child16);
}

END_TEST START_TEST(spi_flat_navi_context_next_valid_parameter4)
{
	AtspiAccessible *next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);

	fail_if(!next || next != child19);
}

END_TEST START_TEST(spi_flat_navi_context_next_valid_parameter5)
{
	AtspiAccessible *next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);

	fail_if(!next || next != child22);
}

END_TEST START_TEST(spi_flat_navi_context_next_valid_parameter6)
{
	AtspiAccessible *next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);
	next = flat_navi_context_next(ctx);

	fail_if(!next || next != child2);
}

END_TEST START_TEST(spi_flat_navi_context_prev_null_parameter)
{
	AtspiAccessible *prev = flat_navi_context_prev(NULL);
	fail_if(prev);
}

END_TEST START_TEST(spi_flat_navi_context_prev_valid_parameter)
{
	AtspiAccessible *prev = flat_navi_context_prev(ctx);
	fail_if(!prev || prev != child6);
}

END_TEST START_TEST(spi_flat_navi_context_prev_valid_parameter2)
{
	AtspiAccessible *prev = flat_navi_context_prev(ctx);
	fail_if(!prev || prev != child22);
}

END_TEST START_TEST(spi_flat_navi_context_prev_valid_parameter3)
{
	AtspiAccessible *prev = flat_navi_context_prev(ctx);
	prev = flat_navi_context_prev(ctx);
	fail_if(!prev || prev != child19);
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
   fail_if(!last || last != child6);
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
   fail_if(!first || first != child5);
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

Suite * screen_reader_suite(void)
{
	Suite *s;
	TCase *tc_spi_screen_reader_on_state_changed;
	TCase *tc_spi_screen_reader_on_caret_move;
	TCase *tc_spi_screen_reader_on_access_value;
	TCase *tc_spi_screen_reader_flat_navi;
	TCase *tc_spi_screen_reader_flat_navi2;

	s = suite_create("Screen reader");
	tc_spi_screen_reader_on_state_changed = tcase_create("tc_spi_screen_reader_on_state_changed");
	tc_spi_screen_reader_on_caret_move = tcase_create("tc_spi_screen_reader_on_caret_move");
	tc_spi_screen_reader_on_access_value = tcase_create("tc_spi_screen_reader_on_access_value");
	tc_spi_screen_reader_flat_navi = tcase_create("tc_scpi_screen_reader_flat_navi");
	tc_spi_screen_reader_flat_navi2 = tcase_create("tc_scpi_screen_reader_flat_navi2");

	tcase_add_checked_fixture(tc_spi_screen_reader_on_state_changed, setup, teardown);
	tcase_add_checked_fixture(tc_spi_screen_reader_on_caret_move, setup, teardown);
	tcase_add_checked_fixture(tc_spi_screen_reader_on_access_value, setup, teardown);
	tcase_add_checked_fixture(tc_spi_screen_reader_flat_navi, setup_flat_navi2, teardown_flat_navi);
	tcase_add_checked_fixture(tc_spi_screen_reader_flat_navi2, setup_flat_navi3, teardown_flat_navi);

#if 1
	tcase_add_test(tc_spi_screen_reader_on_state_changed, spi_on_state_change_name);
	tcase_add_test(tc_spi_screen_reader_on_state_changed, spi_on_state_change_description);
	tcase_add_test(tc_spi_screen_reader_on_state_changed, spi_on_state_change_role);
#endif
	tcase_add_test(tc_spi_screen_reader_on_caret_move, spi_on_caret_move);
	tcase_add_test(tc_spi_screen_reader_on_access_value, spi_on_value_changed);

	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_create_null_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_create_valid_parameter);

	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_get_current_null_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_get_current_valid_parameter);

	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_next_null_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_next_valid_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi2, spi_flat_navi_context_next_valid_parameter2);
	tcase_add_test(tc_spi_screen_reader_flat_navi2, spi_flat_navi_context_next_valid_parameter3);
	tcase_add_test(tc_spi_screen_reader_flat_navi2, spi_flat_navi_context_next_valid_parameter4);
	tcase_add_test(tc_spi_screen_reader_flat_navi2, spi_flat_navi_context_next_valid_parameter5);
	tcase_add_test(tc_spi_screen_reader_flat_navi2, spi_flat_navi_context_next_valid_parameter6);

	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_prev_null_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_prev_valid_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi2, spi_flat_navi_context_prev_valid_parameter2);
	tcase_add_test(tc_spi_screen_reader_flat_navi2, spi_flat_navi_context_prev_valid_parameter3);

	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_last_null_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_last_valid_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_first_null_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_first_valid_parameter);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_current_set_null_parameters);
	tcase_add_test(tc_spi_screen_reader_flat_navi, spi_flat_navi_context_current_set_valid_parameters);

	suite_add_tcase(s, tc_spi_screen_reader_on_state_changed);
	suite_add_tcase(s, tc_spi_screen_reader_on_caret_move);
	suite_add_tcase(s, tc_spi_screen_reader_on_access_value);
	suite_add_tcase(s, tc_spi_screen_reader_flat_navi);
	suite_add_tcase(s, tc_spi_screen_reader_flat_navi2);

	return s;
}

int main()
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = screen_reader_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
