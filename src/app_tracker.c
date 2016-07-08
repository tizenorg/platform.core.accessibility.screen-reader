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

#include "app_tracker.h"
#include "screen_reader_tts.h"
#include "logger.h"
#include <ctype.h>
#ifndef X11_ENABLED
#include "keyboard_tracker.h"
#endif

typedef struct {
	AtspiAccessible *root;
	GList *base_roots;
	GList *callbacks;
	guint timer;
} SubTreeRootData;

typedef struct {
	AppTrackerEventCB func;
	AtspiAccessible *root;
	void *user_data;
} EventCallbackData;

#define APP_TRACKER_INVACTIVITY_TIMEOUT 200

static int _init_count;
static GList *_roots;
static AtspiEventListener *_listener;
static AppTrackerEventCB _new_obj_highlighted_callback;
extern bool keyboard_feedback;

static int _is_descendant(AtspiAccessible * ancestor, AtspiAccessible * descendant)
{
	return 1;

#if 0
	do {
		if (ancestor == descendant)
			return 1;
	}
	while ((descendant = atspi_accessible_get_parent(descendant, NULL)) != NULL);

	return 0;
#endif
}

static Eina_Bool _object_has_modal_state(AtspiAccessible * obj)
{
	if (!obj)
		return EINA_FALSE;

	Eina_Bool ret = EINA_FALSE;

	AtspiStateSet *ss = atspi_accessible_get_state_set(obj);

	if (atspi_state_set_contains(ss, ATSPI_STATE_MODAL))
		ret = EINA_TRUE;
	g_object_unref(ss);
	return ret;
}

static Eina_Bool _object_has_showing_state(AtspiAccessible * obj)
{
	if (!obj)
		return EINA_FALSE;

	Eina_Bool ret = EINA_FALSE;

	AtspiStateSet *ss = atspi_accessible_get_state_set(obj);

	if (atspi_state_set_contains(ss, ATSPI_STATE_SHOWING))
		ret = EINA_TRUE;
	g_object_unref(ss);
	return ret;
}

static Eina_Bool _object_has_highlighted_state(AtspiAccessible * obj)
{
	if (!obj)
		return EINA_FALSE;

	Eina_Bool ret = EINA_FALSE;

	AtspiStateSet *ss = atspi_accessible_get_state_set(obj);

	if (atspi_state_set_contains(ss, ATSPI_STATE_HIGHLIGHTED))
		ret = EINA_TRUE;
	g_object_unref(ss);
	return ret;
}

static void _subtree_callbacks_call(SubTreeRootData * std)
{
	DEBUG("START");
	GList *l;
	EventCallbackData *ecd;

	for (l = std->callbacks; l != NULL; l = l->next) {
		ecd = l->data;
		ecd->func(std->root, ecd->user_data);
	}
	DEBUG("END");
}

static gboolean _on_timeout_cb(gpointer user_data)
{
	DEBUG("START");
	SubTreeRootData *std = user_data;

	_subtree_callbacks_call(std);

	std->timer = 0;
	DEBUG("END");
	return FALSE;
}

static void _print_event_object_info(const AtspiEvent * event)
{
	gchar *name = atspi_accessible_get_name(event->source, NULL), *role = atspi_accessible_get_role_name(event->source, NULL);

	DEBUG("signal:%s, name: %s, role: %s, detail1:%i, detail2: %i", event->type, name, role, event->detail1, event->detail2);
	g_free(name);
	g_free(role);
}

static void _read_value(AtspiValue * value)
{
	if (!value)
		return;

	gdouble current_val = atspi_value_get_current_value(value, NULL);
	gdouble max_val = atspi_value_get_maximum_value(value, NULL);
	gdouble min_val = atspi_value_get_minimum_value(value, NULL);

	int proc = ((current_val - min_val) / fabs(max_val - min_val)) * 100;

	char buf[256] = "\0";
	snprintf(buf, sizeof(buf), "%d percent", proc);
	DEBUG("has value %s", buf);
	tts_speak(buf, EINA_TRUE);
}

static void _on_atspi_event_cb(const AtspiEvent * event)
{
	GList *l;
	SubTreeRootData *std;

	if (!event)
		return;
	if (!event->source) {
		ERROR("empty event source");
		return;
	}

	if ((atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_DESKTOP_FRAME)) {
		return;
	}

	_print_event_object_info(event);
#ifndef X11_ENABLED
	if (!strcmp(event->type, "object:bounds-changed")
		&& (atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_INPUT_METHOD_WINDOW)) {
		AtspiRect *rect;
		rect = (AtspiRect *)g_value_get_boxed(&event->any_data);
		if (rect) {
			keyboard_geometry_set(rect->x, rect->y, rect->width, rect->height);
			DEBUG("keyboard_geometry: %d %d %d %d", rect->x, rect->y, rect->width, rect->height);
		}
	}
#endif
	if (keyboard_feedback) {
		if (!strcmp(event->type, "object:text-changed:insert") && (atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_ENTRY)) {
			char buf[256] = "\0";
			const gchar *text = NULL;
			text = g_value_get_string(&event->any_data);
			if ((event->detail2 == 1) && isupper((int)*text)) {
			   strncat(buf, _("IDS_CAPITAL"), sizeof(buf) - strlen(buf) - 1);
			   strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
			}
			strncat(buf, text, sizeof(buf) - strlen(buf) - 1);
			tts_speak(buf, EINA_TRUE);
		}
		if (!strcmp(event->type, "object:text-changed:delete") && (atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_ENTRY)) {
			char buf[256] = "\0";
			const gchar *text = NULL;
			text = g_value_get_string(&event->any_data);
			if ((event->detail2 == 1) && isupper((int)*text)) {
				strncat(buf, _("IDS_CAPITAL"), sizeof(buf) - strlen(buf) - 1);
				strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
			}
			strncat(buf, text, sizeof(buf) - strlen(buf) - 1);
			strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
			strncat(buf, _("IDS_DELETED"), sizeof(buf) - strlen(buf) - 1);
			if (event->detail1 == 0) {
				strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);//entry should be empty, need to find/get more detail here
				strncat(buf, _("IDS_ALL_CHARACTERS_DELETED"), sizeof(buf) - strlen(buf) - 1);
			}
			tts_speak(buf, EINA_TRUE);
		}
	}
	if (!strcmp(event->type, "object:property-change:accessible-name") && _object_has_highlighted_state(event->source)) {
		gchar *name = atspi_accessible_get_name(event->source, NULL);
		DEBUG("New name for object, read:%s", name);
		tts_speak (name, EINA_TRUE);
		g_free(name);
		return;
	}
	// for reading slider and spinner value changes
	if (!strcmp(event->type, "object:property-change:accessible-value")) {
		AtspiRole role = atspi_accessible_get_role(event->source, NULL);
		if (role == ATSPI_ROLE_SLIDER) {
			AtspiValue *value_interface = atspi_accessible_get_value_iface(event->source);
			_read_value(value_interface);
			g_object_unref(value_interface);
		}
		else if (role == ATSPI_ROLE_FILLER) {
			gchar *name = atspi_accessible_get_name(event->source, NULL);
			tts_speak (name, EINA_TRUE);
			g_free(name);
		}
	}
	//

	if (!strcmp(event->type, "object:state-changed:animated") && (atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_LIST_ITEM)) {
		GError *err = NULL;
		char buf[256] = "\0";
		gint idx = atspi_accessible_get_index_in_parent(event->source, &err);
		if (event->detail1)
			snprintf(buf, sizeof(buf), _("IDS_TRAIT_REORDER_DRAG_START"), idx + 1);
		else
			snprintf(buf, sizeof(buf), _("IDS_TRAIT_REORDER_DRAG_STOP"), idx + 1);
		tts_speak(buf, EINA_TRUE);
		g_error_free(err);
	}

	if (!strcmp(event->type, "object:state-changed:checked")) {
		char buf[256] = "\0";
		gchar *name = atspi_accessible_get_name(event->source, NULL);
		strncat(buf, name, sizeof(buf) - strlen(buf) - 1);
		strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
		if (event->detail1)
			strncat(buf, _("IDS_TRAIT_CHECK_BOX_SELECTED"), sizeof(buf) - strlen(buf) - 1);
		else
			strncat(buf, _("IDS_TRAIT_CHECK_BOX_NOT_SELECTED"), sizeof(buf) - strlen(buf) - 1);
		tts_speak(buf, EINA_TRUE);
		g_free(name);
	}

	if (!strcmp(event->type, "object:state-changed:selected") && (atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_MENU_ITEM)) {
		char buf[256] = "\0";
		char tab_index[16] = "\0";
		AtspiAccessible *parent = atspi_accessible_get_parent(event->source, NULL);
		int children_count = atspi_accessible_get_child_count(parent, NULL);
		int index = atspi_accessible_get_index_in_parent(event->source, NULL);
		gchar *name = atspi_accessible_get_name(event->source, NULL);
		strncat(buf, name, sizeof(buf) - strlen(buf) - 1);
		strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
		snprintf(tab_index, sizeof(tab_index), _("IDS_TRAIT_MENU_ITEM_TAB_INDEX"), index + 1, children_count);
		strncat(buf, tab_index, sizeof(buf) - strlen(buf) - 1);
		strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
		if (event->detail1) {
			strncat(buf, _("IDS_TRAIT_ITEM_SELECTED"), sizeof(buf) - strlen(buf) - 1);
		}
		else strncat(buf, _("IDS_TRAIT_ITEM_SELECT"), sizeof(buf) - strlen(buf) - 1);
		tts_speak(buf, EINA_TRUE);
		g_object_unref(parent);
		g_free(name);
	}

	AtspiAccessible *new_highlighted_obj = NULL;

	if (!strcmp(event->type, "object:state-changed:highlighted"))
		new_highlighted_obj = event->source;
	else if (!strcmp(event->type, "object:active-descendant-changed"))
		new_highlighted_obj = atspi_accessible_get_child_at_index(event->source, event->detail1, NULL);

	if (new_highlighted_obj && _new_obj_highlighted_callback && _object_has_highlighted_state(new_highlighted_obj)) {
		DEBUG("HIGHLIGHTED OBJECT IS ABOUT TO CHANGE");
		_new_obj_highlighted_callback(new_highlighted_obj, NULL);
		g_object_unref(new_highlighted_obj);
		new_highlighted_obj = NULL;
	}

	if (!strcmp("object:state-changed:showing", event->type) ||
		!strcmp("object:state-changed:visible", event->type) ||
		!strcmp("object:state-changed:defunct", event->type)) {
		for (l = _roots; l != NULL; l = l->next) {
			std = l->data;

			GList *l_base_root = g_list_last(std->base_roots);

			if (!_object_has_showing_state(std->root) && l_base_root->data ){
				std->root = l_base_root->data;
				std->base_roots = g_list_remove(std->base_roots, l_base_root->data);
			}


			if (_is_descendant(std->root, event->source)) {
				if (std->timer)
					g_source_remove(std->timer);

				DEBUG("Before Checking if modal is showing");
				if (_object_has_modal_state(event->source)) {
					DEBUG("Object is modal");
					std->base_roots = g_list_append(std->base_roots, std->root);
					std->root = event->source;
				}

				std->timer = g_timeout_add(APP_TRACKER_INVACTIVITY_TIMEOUT, _on_timeout_cb, std);
			}
		}
	}
}

static int _app_tracker_init_internal(void)
{
	DEBUG("START");
	_new_obj_highlighted_callback = NULL;
	_listener = atspi_event_listener_new_simple(_on_atspi_event_cb, NULL);

	atspi_event_listener_register(_listener, "object:state-changed:showing", NULL);
	atspi_event_listener_register(_listener, "object:state-changed:visible", NULL);
	atspi_event_listener_register(_listener, "object:state-changed:defunct", NULL);
	atspi_event_listener_register(_listener, "object:state-changed:highlighted", NULL);
	atspi_event_listener_register(_listener, "object:state-changed:animated", NULL);
	atspi_event_listener_register(_listener, "object:state-changed:checked", NULL);
	atspi_event_listener_register(_listener, "object:state-changed:selected", NULL);
	atspi_event_listener_register(_listener, "object:bounds-changed", NULL);
	atspi_event_listener_register(_listener, "object:visible-data-changed", NULL);
	atspi_event_listener_register(_listener, "object:active-descendant-changed", NULL);
	atspi_event_listener_register(_listener, "object:property-change", NULL);
	atspi_event_listener_register(_listener, "object:text-changed:insert", NULL);
	atspi_event_listener_register(_listener, "object:text-changed:delete", NULL);

	return 0;
}

static void _free_callbacks(gpointer data)
{
	g_free(data);
}

static void _free_rootdata(gpointer data)
{
	SubTreeRootData *std = data;
	g_list_free_full(std->callbacks, _free_callbacks);
	if (std->timer)
		g_source_remove(std->timer);
	g_free(std);
}

static void _app_tracker_shutdown_internal(void)
{
	atspi_event_listener_deregister(_listener, "object:state-changed:showing", NULL);
	atspi_event_listener_deregister(_listener, "object:state-changed:visible", NULL);
	atspi_event_listener_deregister(_listener, "object:state-changed:highlighted", NULL);
	atspi_event_listener_deregister(_listener, "object:state-changed:animated", NULL);
	atspi_event_listener_deregister(_listener, "object:state-changed:checked", NULL);
	atspi_event_listener_deregister(_listener, "object:bounds-changed", NULL);
	atspi_event_listener_deregister(_listener, "object:state-changed:defunct", NULL);
	atspi_event_listener_deregister(_listener, "object:visible-data-changed", NULL);
	atspi_event_listener_deregister(_listener, "object:active-descendant-changed", NULL);
	atspi_event_listener_deregister(_listener, "object:property-change", NULL);

	g_object_unref(_listener);
	_listener = NULL;
	_new_obj_highlighted_callback = NULL;
	g_list_free_full(_roots, _free_rootdata);
	_roots = NULL;
}

int app_tracker_init(void)
{
	DEBUG("START");
	if (!_init_count)
		if (_app_tracker_init_internal())
			return -1;
	return ++_init_count;
}

void app_tracker_shutdown(void)
{
	if (_init_count == 1)
		_app_tracker_shutdown_internal();
	if (--_init_count < 0)
		_init_count = 0;
}

void app_tracker_callback_register(AtspiAccessible * app, AppTrackerEventCB cb, void *user_data)
{
	DEBUG("START");
	SubTreeRootData *rd = NULL;
	EventCallbackData *cd;
	GList *l;

	if (!_init_count || !cb)
		return;

	for (l = _roots; l != NULL; l = l->next) {
		rd = l->data;
		if (((SubTreeRootData *) l->data)->root == app) {
			rd = l->data;
			break;
		}
	}

	if (!rd) {
		rd = g_new(SubTreeRootData, 1);
		rd->root = app;
		rd->callbacks = NULL;
		rd->timer = 0;
		_roots = g_list_append(_roots, rd);
		rd->base_roots = NULL;
		rd->base_roots = g_list_append(rd->base_roots, NULL);
	}

	cd = g_new(EventCallbackData, 1);
	cd->func = cb;
	cd->user_data = user_data;

	rd->callbacks = g_list_append(rd->callbacks, cd);
	DEBUG("END");
}

void app_tracker_new_obj_highlighted_callback_register(AppTrackerEventCB cb)
{
	_new_obj_highlighted_callback = cb;
}

void app_tracker_callback_unregister(AtspiAccessible * app, AppTrackerEventCB cb, void *user_data)
{
	DEBUG("START");
	GList *l;
	EventCallbackData *ecd;
	SubTreeRootData *std = NULL;

	for (l = _roots; l != NULL; l = l->next) {
		if (((SubTreeRootData *) l->data)->root == app || ((SubTreeRootData *) l->data)->base_roots->data == app) {
			std = l->data;
			break;
		}
	}

	if (!std)
		return;

	for (l = std->callbacks; l != NULL; l = l->next) {
		ecd = l->data;
		if ((ecd->func == cb) && (ecd->user_data == user_data)) {
			std->callbacks = g_list_delete_link(std->callbacks, l);
			break;
		}
	}

	if (!std->callbacks) {
		if (std->timer)
			g_source_remove(std->timer);
		_roots = g_list_remove(_roots, std);
		g_free(std);
	}
}

void app_tracker_new_obj_highlighted_callback_unregister(AppTrackerEventCB cb)
{
	_new_obj_highlighted_callback = NULL;
}
