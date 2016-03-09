#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>
#include <stdio.h>

#include "logger.h"
#include "lua_engine.h"

#include <string.h>

#define ACCESSIBLE_CLASS_NAME "Accessible"
#define VALUE_CLASS_NAME "Value"
#define SELECTION_CLASS_NAME "Selection"

#define GERROR_CHECK(err) \
	if (err) { lua_error(L); printf("GError: %s\n", err->message); g_error_free(err); }

static lua_State *L;


static void *_pop_class_obj(lua_State *L, int index, int narg, const char *class)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void **ret = lua_touserdata(L, index);
	if (!ret) luaL_typerror(L, narg, class);
	return *ret;
}

static void _push_class_obj(lua_State *L, void *obj, const char *class)
{
	void **ptr = lua_newuserdata(L, sizeof(void*));
	*ptr = obj;
	luaL_getmetatable(L, class);
	lua_setmetatable(L, -2);
}

static int _accessible_name(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, -1, 1, ACCESSIBLE_CLASS_NAME);
	GError *err = NULL;
	if (!obj) return 0;

	char *name = atspi_accessible_get_name(obj, &err);
	GERROR_CHECK(err);

	lua_pushstring(L, name);
	g_free(name);
	return 1;
}

static int _accessible_description(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	GError *err = NULL;
	if (!obj)
		return 0;
	char *name = atspi_accessible_get_description(obj, &err);
	GERROR_CHECK(err);

	lua_pushstring(L, name);
	g_free(name);
	return 1;
}

static int l_get_root(lua_State *L) {
	AtspiAccessible *root = atspi_get_desktop(0);
	_push_class_obj(L, root, ACCESSIBLE_CLASS_NAME);
	return 1;
}

static int l_get_translation(lua_State *L) {
	const char *str = lua_tostring(L, -1);
	lua_pushstring(L, _(str));
	return 1;
}

static int _accessible_children(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	GError *err = NULL;
	if (!obj)
		return 0;
	int i, n = atspi_accessible_get_child_count(obj, &err);
	GERROR_CHECK(err);

	lua_createtable(L, n, 0);

	for (i = 0; i < n; i++)
	{
		AtspiAccessible *child = atspi_accessible_get_child_at_index(obj, i, &err);
		if (!child) continue;
		GERROR_CHECK(err);
		lua_pushinteger(L, i + 1);
		_push_class_obj(L, child, ACCESSIBLE_CLASS_NAME);
		lua_settable(L, -3);
	}
	return 1;
}

static int _accessible_parent(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	GError *err = NULL;
	if (!obj) return 0;

	AtspiAccessible *parent = atspi_accessible_get_parent(obj, &err);
	if (!parent) return 0;
	GERROR_CHECK(err);
	_push_class_obj(L, parent, ACCESSIBLE_CLASS_NAME);

	return 1;
}

static int _accessible_role(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	GError *err = NULL;
	if (!obj) return 0;

	AtspiRole role = atspi_accessible_get_role(obj, &err);
	GERROR_CHECK(err);

	lua_pushinteger(L, role);

	return 1;
}

static int _accessible_role_name(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	GError *err = NULL;
	if (!obj) {
		return 0;
	}
	char *name = atspi_accessible_get_localized_role_name(obj, &err);
	GERROR_CHECK(err);

	lua_pushstring(L, name);
	g_free(name);
	return 1;
}

static int _accessible_is(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	AtspiStateType type = lua_tonumber(L, -1);

	AtspiStateSet *ss = atspi_accessible_get_state_set(obj);

	if (atspi_state_set_contains(ss, type))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	return 1;
}

static int _accessible_index(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	GError *err = NULL;
	if (!obj) return 0;

	int idx = atspi_accessible_get_index_in_parent(obj, &err);
	GERROR_CHECK(err);

	lua_pushinteger(L, idx);
	return 1;
}

static int _accessible_relations(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	AtspiRelationType type = lua_tonumber(L, -1);
	GError *err = NULL;
	int i, j;

	lua_newtable(L);
	if (!obj) return 1;
	GArray *rels = atspi_accessible_get_relation_set(obj, &err);
	GERROR_CHECK(err);
	if (!rels) return 1;

	for (i = 0; i < rels->len; i++)
	{
		AtspiRelation *rel = g_array_index(rels, AtspiRelation*, i);
		if (atspi_relation_get_relation_type(rel) == type)
		{
			for (j = 0; j < atspi_relation_get_n_targets(rel); j++)
			{
				AtspiAccessible *target = atspi_relation_get_target(rel, j);
				if (!target) continue;
				lua_pushinteger(L, j);
				_push_class_obj(L, target, ACCESSIBLE_CLASS_NAME);
				lua_settable(L, -3);
			}
		}
		g_object_unref(rel);
	}
	g_array_free(rels, TRUE);

	return 1;
}

static int _accessible_value(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	AtspiValue *val = atspi_accessible_get_value_iface(obj);
	if (!val)
	{
		lua_pushnil(L);
		return 1;
	}
	_push_class_obj(L, val, VALUE_CLASS_NAME);
	return 1;
}

static int _accessible_selection(lua_State *L) {
	AtspiAccessible *obj = _pop_class_obj(L, 1, 1, ACCESSIBLE_CLASS_NAME);
	AtspiSelection *sel = atspi_accessible_get_selection_iface(obj);
	if (!sel)
	{
		lua_pushnil(L);
		return 1;
	}
	_push_class_obj(L, sel, SELECTION_CLASS_NAME);
	return 1;
}

static int _gobject_unref(lua_State *L) {
#ifdef SCREEN_READER_FLAT_NAVI_TEST_DUMMY_IMPLEMENTATION
	return 0;
#else
	void **ret = lua_touserdata(L, -1);
	g_object_unref(*ret);
	return 0;
#endif
}

static const luaL_reg _meta_methods[] = {
	{ "__gc", _gobject_unref},
	{ NULL, NULL}
};

static const luaL_reg _accessible_methods[] = {
	{ "name", _accessible_name },
	{ "description", _accessible_description },
	{ "children", _accessible_children },
	{ "parent", _accessible_parent },
	{ "role", _accessible_role },
	{ "inRelation", _accessible_relations },
	{ "roleName", _accessible_role_name },
	{ "is", _accessible_is },
	{ "index", _accessible_index },
	{ "value", _accessible_value },
	{ "selection", _accessible_selection },
	{ NULL, NULL}
};

static int _value_current_value(lua_State *L) {
	AtspiValue *val = _pop_class_obj(L, -1, 1, VALUE_CLASS_NAME);
	GError *err = NULL;

	double cv = atspi_value_get_current_value(val, &err);
	GERROR_CHECK(err);

	lua_pushnumber(L, cv);

	return 1;
}

static int _value_range(lua_State *L) {
	AtspiValue *val = _pop_class_obj(L, -1, 1, VALUE_CLASS_NAME);
	GError *err = NULL;
	double max, min;

	max = atspi_value_get_maximum_value(val, &err);
	GERROR_CHECK(err);

	min = atspi_value_get_minimum_value(val, &err);
	GERROR_CHECK(err);

	lua_pushnumber(L, min);
	lua_pushnumber(L, max);

	return 2;
}

static int _value_increment(lua_State *L) {
	AtspiValue *val = _pop_class_obj(L, 1, 1, VALUE_CLASS_NAME);
	GError *err = NULL;

	double mi = atspi_value_get_minimum_increment(val, &err);
	GERROR_CHECK(err);

	lua_pushnumber(L, mi);

	return 1;
}

static const luaL_reg _value_methods[] = {
	{ "current", _value_current_value },
	{ "range", _value_range },
	{ "increment", _value_increment },
	{ NULL, NULL}
};

static int _selection_is_selected(lua_State *L) {
	AtspiSelection *sel = _pop_class_obj(L, 1, 1, SELECTION_CLASS_NAME);
	GError *err = NULL;
	int id = lua_tointeger(L, -1);

	gboolean ret = atspi_selection_is_child_selected(sel, id, &err);
	GERROR_CHECK(err);

	lua_pushboolean(L, ret);

	return 1;
}

static int _selection_count(lua_State *L) {
	AtspiSelection *sel = _pop_class_obj(L, 1, 1, SELECTION_CLASS_NAME);
	GError *err = NULL;

	int mi = atspi_selection_get_n_selected_children(sel, &err);
	GERROR_CHECK(err);

	lua_pushinteger(L, mi);

	return 1;
}

static const luaL_reg _selection_methods[] = {
	{ "count", _selection_count },
	{ "isSelected", _selection_is_selected },
	{ NULL, NULL}
};

static void _register_role_enums(lua_State *L)
{
	lua_pushinteger(L, ATSPI_ROLE_INVALID);
	lua_setglobal(L, "INVALID");
	lua_pushinteger(L, ATSPI_ROLE_ACCELERATOR_LABEL);
	lua_setglobal(L, "ACCELERATOR_LABEL");
	lua_pushinteger(L, ATSPI_ROLE_ALERT);
	lua_setglobal(L, "ALERT");
	lua_pushinteger(L, ATSPI_ROLE_ANIMATION);
	lua_setglobal(L, "ANIMATION");
	lua_pushinteger(L, ATSPI_ROLE_ARROW);
	lua_setglobal(L, "ARROW");
	lua_pushinteger(L, ATSPI_ROLE_CALENDAR);
	lua_setglobal(L, "CALENDAR");
	lua_pushinteger(L, ATSPI_ROLE_CANVAS);
	lua_setglobal(L, "CANVAS");
	lua_pushinteger(L, ATSPI_ROLE_CHECK_BOX);
	lua_setglobal(L, "CHECK_BOX");
	lua_pushinteger(L, ATSPI_ROLE_CHECK_MENU_ITEM);
	lua_setglobal(L, "CHECK_MENU_ITEM");
	lua_pushinteger(L, ATSPI_ROLE_COLOR_CHOOSER);
	lua_setglobal(L, "COLOR_CHOOSER");
	lua_pushinteger(L, ATSPI_ROLE_COLUMN_HEADER);
	lua_setglobal(L, "COLUMN_HEADER");
	lua_pushinteger(L, ATSPI_ROLE_COMBO_BOX);
	lua_setglobal(L, "COMBO_BOX");
	lua_pushinteger(L, ATSPI_ROLE_DATE_EDITOR);
	lua_setglobal(L, "DATE_EDITOR");
	lua_pushinteger(L, ATSPI_ROLE_DESKTOP_ICON);
	lua_setglobal(L, "DESKTOP_ICON");
	lua_pushinteger(L, ATSPI_ROLE_DESKTOP_FRAME);
	lua_setglobal(L, "DESKTOP_FRAME");
	lua_pushinteger(L, ATSPI_ROLE_DIAL);
	lua_setglobal(L, "DIAL");
	lua_pushinteger(L, ATSPI_ROLE_DIALOG);
	lua_setglobal(L, "DIALOG");
	lua_pushinteger(L, ATSPI_ROLE_DIRECTORY_PANE);
	lua_setglobal(L, "DIRECTORY_PANE");
	lua_pushinteger(L, ATSPI_ROLE_DRAWING_AREA);
	lua_setglobal(L, "DRAWING_AREA");
	lua_pushinteger(L, ATSPI_ROLE_FILE_CHOOSER);
	lua_setglobal(L, "FILE_CHOOSER");
	lua_pushinteger(L, ATSPI_ROLE_FILLER);
	lua_setglobal(L, "FILLER");
	lua_pushinteger(L, ATSPI_ROLE_FOCUS_TRAVERSABLE);
	lua_setglobal(L, "FOCUS_TRAVERSABLE");
	lua_pushinteger(L, ATSPI_ROLE_FONT_CHOOSER);
	lua_setglobal(L, "FONT_CHOOSER");
	lua_pushinteger(L, ATSPI_ROLE_FRAME);
	lua_setglobal(L, "FRAME");
	lua_pushinteger(L, ATSPI_ROLE_GLASS_PANE);
	lua_setglobal(L, "GLASS_PANE");
	lua_pushinteger(L, ATSPI_ROLE_HTML_CONTAINER);
	lua_setglobal(L, "HTML_CONTAINER");
	lua_pushinteger(L, ATSPI_ROLE_ICON);
	lua_setglobal(L, "ICON");
	lua_pushinteger(L, ATSPI_ROLE_IMAGE);
	lua_setglobal(L, "IMAGE");
	lua_pushinteger(L, ATSPI_ROLE_INTERNAL_FRAME);
	lua_setglobal(L, "INTERNAL_FRAME");
	lua_pushinteger(L, ATSPI_ROLE_LABEL);
	lua_setglobal(L, "LABEL");
	lua_pushinteger(L, ATSPI_ROLE_LAYERED_PANE);
	lua_setglobal(L, "LAYERED_PANE");
	lua_pushinteger(L, ATSPI_ROLE_LIST);
	lua_setglobal(L, "LIST");
	lua_pushinteger(L, ATSPI_ROLE_LIST_ITEM);
	lua_setglobal(L, "LIST_ITEM");
	lua_pushinteger(L, ATSPI_ROLE_MENU);
	lua_setglobal(L, "MENU");
	lua_pushinteger(L, ATSPI_ROLE_MENU_BAR);
	lua_setglobal(L, "MENU_BAR");
	lua_pushinteger(L, ATSPI_ROLE_MENU_ITEM);
	lua_setglobal(L, "MENU_ITEM");
	lua_pushinteger(L, ATSPI_ROLE_OPTION_PANE);
	lua_setglobal(L, "OPTION_PANE");
	lua_pushinteger(L, ATSPI_ROLE_PAGE_TAB);
	lua_setglobal(L, "PAGE_TAB");
	lua_pushinteger(L, ATSPI_ROLE_PAGE_TAB_LIST);
	lua_setglobal(L, "PAGE_TAB_LIST");
	lua_pushinteger(L, ATSPI_ROLE_PANEL);
	lua_setglobal(L, "PANEL");
	lua_pushinteger(L, ATSPI_ROLE_PASSWORD_TEXT);
	lua_setglobal(L, "PASSWORD_TEXT");
	lua_pushinteger(L, ATSPI_ROLE_POPUP_MENU);
	lua_setglobal(L, "POPUP_MENU");
	lua_pushinteger(L, ATSPI_ROLE_PROGRESS_BAR);
	lua_setglobal(L, "PROGRESS_BAR");
	lua_pushinteger(L, ATSPI_ROLE_PUSH_BUTTON);
	lua_setglobal(L, "PUSH_BUTTON");
	lua_pushinteger(L, ATSPI_ROLE_RADIO_BUTTON);
	lua_setglobal(L, "RADIO_BUTTON");
	lua_pushinteger(L, ATSPI_ROLE_RADIO_MENU_ITEM);
	lua_setglobal(L, "RADIO_MENU_ITEM");
	lua_pushinteger(L, ATSPI_ROLE_ROOT_PANE);
	lua_setglobal(L, "ROOT_PANE");
	lua_pushinteger(L, ATSPI_ROLE_ROW_HEADER);
	lua_setglobal(L, "ROW_HEADER");
	lua_pushinteger(L, ATSPI_ROLE_SCROLL_BAR);
	lua_setglobal(L, "SCROLL_BAR");
	lua_pushinteger(L, ATSPI_ROLE_SCROLL_PANE);
	lua_setglobal(L, "SCROLL_PANE");
	lua_pushinteger(L, ATSPI_ROLE_SEPARATOR);
	lua_setglobal(L, "SEPARATOR");
	lua_pushinteger(L, ATSPI_ROLE_SLIDER);
	lua_setglobal(L, "SLIDER");
	lua_pushinteger(L, ATSPI_ROLE_SPIN_BUTTON);
	lua_setglobal(L, "SPIN_BUTTON");
	lua_pushinteger(L, ATSPI_ROLE_SPLIT_PANE);
	lua_setglobal(L, "SPLIT_PANE");
	lua_pushinteger(L, ATSPI_ROLE_STATUS_BAR);
	lua_setglobal(L, "STATUS_BAR");
	lua_pushinteger(L, ATSPI_ROLE_TABLE);
	lua_setglobal(L, "TABLE");
	lua_pushinteger(L, ATSPI_ROLE_TABLE_CELL);
	lua_setglobal(L, "TABLE_CELL");
	lua_pushinteger(L, ATSPI_ROLE_TABLE_COLUMN_HEADER);
	lua_setglobal(L, "TABLE_COLUMN_HEADER");
	lua_pushinteger(L, ATSPI_ROLE_TABLE_ROW_HEADER);
	lua_setglobal(L, "TABLE_ROW_HEADER");
	lua_pushinteger(L, ATSPI_ROLE_TEAROFF_MENU_ITEM);
	lua_setglobal(L, "TEAROFF_MENU_ITEM");
	lua_pushinteger(L, ATSPI_ROLE_TERMINAL);
	lua_setglobal(L, "TERMINAL");
	lua_pushinteger(L, ATSPI_ROLE_TEXT);
	lua_setglobal(L, "TEXT");
	lua_pushinteger(L, ATSPI_ROLE_TOGGLE_BUTTON);
	lua_setglobal(L, "TOGGLE_BUTTON");
	lua_pushinteger(L, ATSPI_ROLE_TOOL_BAR);
	lua_setglobal(L, "TOOL_BAR");
	lua_pushinteger(L, ATSPI_ROLE_TOOL_TIP);
	lua_setglobal(L, "TOOL_TIP");
	lua_pushinteger(L, ATSPI_ROLE_TREE);
	lua_setglobal(L, "TREE");
	lua_pushinteger(L, ATSPI_ROLE_TREE_TABLE);
	lua_setglobal(L, "TREE_TABLE");
	lua_pushinteger(L, ATSPI_ROLE_UNKNOWN);
	lua_setglobal(L, "UNKNOWN");
	lua_pushinteger(L, ATSPI_ROLE_VIEWPORT);
	lua_setglobal(L, "VIEWPORT");
	lua_pushinteger(L, ATSPI_ROLE_WINDOW);
	lua_setglobal(L, "WINDOW");
	lua_pushinteger(L, ATSPI_ROLE_EXTENDED);
	lua_setglobal(L, "EXTENDED");
	lua_pushinteger(L, ATSPI_ROLE_HEADER);
	lua_setglobal(L, "HEADER");
	lua_pushinteger(L, ATSPI_ROLE_FOOTER);
	lua_setglobal(L, "FOOTER");
	lua_pushinteger(L, ATSPI_ROLE_PARAGRAPH);
	lua_setglobal(L, "PARAGRAPH");
	lua_pushinteger(L, ATSPI_ROLE_RULER);
	lua_setglobal(L, "RULER");
	lua_pushinteger(L, ATSPI_ROLE_APPLICATION);
	lua_setglobal(L, "APPLICATION");
	lua_pushinteger(L, ATSPI_ROLE_AUTOCOMPLETE);
	lua_setglobal(L, "AUTOCOMPLETE");
	lua_pushinteger(L, ATSPI_ROLE_EDITBAR);
	lua_setglobal(L, "EDITBAR");
	lua_pushinteger(L, ATSPI_ROLE_EMBEDDED);
	lua_setglobal(L, "EMBEDDED");
	lua_pushinteger(L, ATSPI_ROLE_ENTRY);
	lua_setglobal(L, "ENTRY");
	lua_pushinteger(L, ATSPI_ROLE_CHART);
	lua_setglobal(L, "CHART");
	lua_pushinteger(L, ATSPI_ROLE_CAPTION);
	lua_setglobal(L, "CAPTION");
	lua_pushinteger(L, ATSPI_ROLE_DOCUMENT_FRAME);
	lua_setglobal(L, "DOCUMENT_FRAME");
	lua_pushinteger(L, ATSPI_ROLE_HEADING);
	lua_setglobal(L, "HEADING");
	lua_pushinteger(L, ATSPI_ROLE_PAGE);
	lua_setglobal(L, "PAGE");
	lua_pushinteger(L, ATSPI_ROLE_SECTION);
	lua_setglobal(L, "SECTION");
	lua_pushinteger(L, ATSPI_ROLE_REDUNDANT_OBJECT);
	lua_setglobal(L, "REDUNDANT_OBJECT");
	lua_pushinteger(L, ATSPI_ROLE_FORM);
	lua_setglobal(L, "FORM");
	lua_pushinteger(L, ATSPI_ROLE_LINK);
	lua_setglobal(L, "LINK");
	lua_pushinteger(L, ATSPI_ROLE_INPUT_METHOD_WINDOW);
	lua_setglobal(L, "INPUT_METHOD_WINDOW");
	lua_pushinteger(L, ATSPI_ROLE_TABLE_ROW);
	lua_setglobal(L, "TABLE_ROW");
	lua_pushinteger(L, ATSPI_ROLE_TREE_ITEM);
	lua_setglobal(L, "TREE_ITEM");
	lua_pushinteger(L, ATSPI_ROLE_DOCUMENT_SPREADSHEET);
	lua_setglobal(L, "DOCUMENT_SPREADSHEET");
	lua_pushinteger(L, ATSPI_ROLE_DOCUMENT_PRESENTATION);
	lua_setglobal(L, "DOCUMENT_PRESENTATION");
	lua_pushinteger(L, ATSPI_ROLE_DOCUMENT_TEXT);
	lua_setglobal(L, "DOCUMENT_TEXT");
	lua_pushinteger(L, ATSPI_ROLE_DOCUMENT_WEB);
	lua_setglobal(L, "DOCUMENT_WEB");
	lua_pushinteger(L, ATSPI_ROLE_DOCUMENT_EMAIL);
	lua_setglobal(L, "DOCUMENT_EMAIL");
	lua_pushinteger(L, ATSPI_ROLE_COMMENT);
	lua_setglobal(L, "COMMENT");
	lua_pushinteger(L, ATSPI_ROLE_LIST_BOX);
	lua_setglobal(L, "LIST_BOX");
	lua_pushinteger(L, ATSPI_ROLE_GROUPING);
	lua_setglobal(L, "GROUPING");
	lua_pushinteger(L, ATSPI_ROLE_IMAGE_MAP);
	lua_setglobal(L, "IMAGE_MAP");
	lua_pushinteger(L, ATSPI_ROLE_NOTIFICATION);
	lua_setglobal(L, "NOTIFICATION");
	lua_pushinteger(L, ATSPI_ROLE_INFO_BAR);
	lua_setglobal(L, "INFO_BAR");
	lua_pushinteger(L, ATSPI_ROLE_LEVEL_BAR);
	lua_setglobal(L, "LEVEL_BAR");
	lua_pushinteger(L, ATSPI_ROLE_TITLE_BAR);
	lua_setglobal(L, "TITLE_BAR");
	lua_pushinteger(L, ATSPI_ROLE_BLOCK_QUOTE);
	lua_setglobal(L, "BLOCK_QUOTE");
	lua_pushinteger(L, ATSPI_ROLE_AUDIO);
	lua_setglobal(L, "AUDIO");
	lua_pushinteger(L, ATSPI_ROLE_VIDEO);
	lua_setglobal(L, "VIDEO");
	lua_pushinteger(L, ATSPI_ROLE_DEFINITION);
	lua_setglobal(L, "DEFINITION");
	lua_pushinteger(L, ATSPI_ROLE_ARTICLE);
	lua_setglobal(L, "ARTICLE");
	lua_pushinteger(L, ATSPI_ROLE_LANDMARK);
	lua_setglobal(L, "LANDMARK");
	lua_pushinteger(L, ATSPI_ROLE_LOG);
	lua_setglobal(L, "LOG");
	lua_pushinteger(L, ATSPI_ROLE_MARQUEE);
	lua_setglobal(L, "MARQUEE");
	lua_pushinteger(L, ATSPI_ROLE_MATH);
	lua_setglobal(L, "MATH");
	lua_pushinteger(L, ATSPI_ROLE_RATING);
	lua_setglobal(L, "RATING");
	lua_pushinteger(L, ATSPI_ROLE_TIMER);
	lua_setglobal(L, "TIMER");
	lua_pushinteger(L, ATSPI_ROLE_STATIC);
	lua_setglobal(L, "STATIC");
	lua_pushinteger(L, ATSPI_ROLE_MATH_FRACTION);
	lua_setglobal(L, "MATH_FRACTION");
	lua_pushinteger(L, ATSPI_ROLE_MATH_ROOT);
	lua_setglobal(L, "MATH_ROOT");
	lua_pushinteger(L, ATSPI_ROLE_SUBSCRIPT);
	lua_setglobal(L, "SUBSCRIPT");
	lua_pushinteger(L, ATSPI_ROLE_SUPERSCRIPT);
	lua_setglobal(L, "SUPERSCRIPT");
	lua_pushinteger(L, ATSPI_ROLE_LAST_DEFINED);
	lua_setglobal(L, "LAST_DEFINED");

	// relations enums
	lua_pushinteger(L, ATSPI_RELATION_NULL);
	lua_setglobal(L, "RELATION_NULL");
	lua_pushinteger(L, ATSPI_RELATION_LABEL_FOR);
	lua_setglobal(L, "RELATION_LABEL_FOR");
	lua_pushinteger(L, ATSPI_RELATION_LABELLED_BY);
	lua_setglobal(L, "RELATION_LABELLED_BY");
	lua_pushinteger(L, ATSPI_RELATION_CONTROLLER_FOR);
	lua_setglobal(L, "RELATION_CONTROLLER_FOR");
	lua_pushinteger(L, ATSPI_RELATION_CONTROLLED_BY);
	lua_setglobal(L, "RELATION_CONTROLLED_BY");
	lua_pushinteger(L, ATSPI_RELATION_MEMBER_OF);
	lua_setglobal(L, "RELATION_MEMBER_OF");
	lua_pushinteger(L, ATSPI_RELATION_TOOLTIP_FOR);
	lua_setglobal(L, "RELATION_TOOLTIP_FOR");
	lua_pushinteger(L, ATSPI_RELATION_NODE_CHILD_OF);
	lua_setglobal(L, "RELATION_NODE_CHILD_OF");
	lua_pushinteger(L, ATSPI_RELATION_NODE_PARENT_OF);
	lua_setglobal(L, "RELATION_NODE_PARENT_OF");
	lua_pushinteger(L, ATSPI_RELATION_EXTENDED);
	lua_setglobal(L, "RELATION_EXTENDED");
	lua_pushinteger(L, ATSPI_RELATION_FLOWS_TO);
	lua_setglobal(L, "RELATION_FLOWS_TO");
	lua_pushinteger(L, ATSPI_RELATION_FLOWS_FROM);
	lua_setglobal(L, "RELATION_FLOWS_FROM");
	lua_pushinteger(L, ATSPI_RELATION_SUBWINDOW_OF);
	lua_setglobal(L, "RELATION_SUBWINDOW_OF");
	lua_pushinteger(L, ATSPI_RELATION_EMBEDS);
	lua_setglobal(L, "RELATION_EMBEDS");
	lua_pushinteger(L, ATSPI_RELATION_EMBEDDED_BY);
	lua_setglobal(L, "RELATION_EMBEDDED_BY");
	lua_pushinteger(L, ATSPI_RELATION_POPUP_FOR);
	lua_setglobal(L, "RELATION_POPUP_FOR");
	lua_pushinteger(L, ATSPI_RELATION_PARENT_WINDOW_OF);
	lua_setglobal(L, "RELATION_PARENT_WINDOW_OF");
	lua_pushinteger(L, ATSPI_RELATION_DESCRIPTION_FOR);
	lua_setglobal(L, "RELATION_DESCRIPTION_FOR");
	lua_pushinteger(L, ATSPI_RELATION_DESCRIBED_BY);
	lua_setglobal(L, "RELATION_DESCRIBED_BY");
	lua_pushinteger(L, ATSPI_RELATION_LAST_DEFINED);
	lua_setglobal(L, "RELATION_LAST_DEFINED");

	// state enums
	lua_pushinteger(L, ATSPI_STATE_INVALID);
	lua_setglobal(L, "INVALID");
	lua_pushinteger(L, ATSPI_STATE_ACTIVE);
	lua_setglobal(L, "ACTIVE");
	lua_pushinteger(L, ATSPI_STATE_ARMED);
	lua_setglobal(L, "ARMED");
	lua_pushinteger(L, ATSPI_STATE_BUSY);
	lua_setglobal(L, "BUSY");
	lua_pushinteger(L, ATSPI_STATE_CHECKED);
	lua_setglobal(L, "CHECKED");
	lua_pushinteger(L, ATSPI_STATE_COLLAPSED);
	lua_setglobal(L, "COLLAPSED");
	lua_pushinteger(L, ATSPI_STATE_DEFUNCT);
	lua_setglobal(L, "DEFUNCT");
	lua_pushinteger(L, ATSPI_STATE_EDITABLE);
	lua_setglobal(L, "EDITABLE");
	lua_pushinteger(L, ATSPI_STATE_ENABLED);
	lua_setglobal(L, "ENABLED");
	lua_pushinteger(L, ATSPI_STATE_EXPANDABLE);
	lua_setglobal(L, "EXPANDABLE");
	lua_pushinteger(L, ATSPI_STATE_EXPANDED);
	lua_setglobal(L, "EXPANDED");
	lua_pushinteger(L, ATSPI_STATE_FOCUSABLE);
	lua_setglobal(L, "FOCUSABLE");
	lua_pushinteger(L, ATSPI_STATE_FOCUSED);
	lua_setglobal(L, "FOCUSED");
	lua_pushinteger(L, ATSPI_STATE_HAS_TOOLTIP);
	lua_setglobal(L, "HAS_TOOLTIP");
	lua_pushinteger(L, ATSPI_STATE_HORIZONTAL);
	lua_setglobal(L, "HORIZONTAL");
	lua_pushinteger(L, ATSPI_STATE_ICONIFIED);
	lua_setglobal(L, "ICONIFIED");
	lua_pushinteger(L, ATSPI_STATE_MODAL);
	lua_setglobal(L, "MODAL");
	lua_pushinteger(L, ATSPI_STATE_MULTI_LINE);
	lua_setglobal(L, "MULTI_LINE");
	lua_pushinteger(L, ATSPI_STATE_MULTISELECTABLE);
	lua_setglobal(L, "MULTISELECTABLE");
	lua_pushinteger(L, ATSPI_STATE_OPAQUE);
	lua_setglobal(L, "OPAQUE");
	lua_pushinteger(L, ATSPI_STATE_PRESSED);
	lua_setglobal(L, "PRESSED");
	lua_pushinteger(L, ATSPI_STATE_RESIZABLE);
	lua_setglobal(L, "RESIZABLE");
	lua_pushinteger(L, ATSPI_STATE_SELECTABLE);
	lua_setglobal(L, "SELECTABLE");
	lua_pushinteger(L, ATSPI_STATE_SELECTED);
	lua_setglobal(L, "SELECTED");
	lua_pushinteger(L, ATSPI_STATE_SENSITIVE);
	lua_setglobal(L, "SENSITIVE");
	lua_pushinteger(L, ATSPI_STATE_SHOWING);
	lua_setglobal(L, "SHOWING");
	lua_pushinteger(L, ATSPI_STATE_SINGLE_LINE);
	lua_setglobal(L, "SINGLE_LINE");
	lua_pushinteger(L, ATSPI_STATE_STALE);
	lua_setglobal(L, "STALE");
	lua_pushinteger(L, ATSPI_STATE_TRANSIENT);
	lua_setglobal(L, "TRANSIENT");
	lua_pushinteger(L, ATSPI_STATE_VERTICAL);
	lua_setglobal(L, "VERTICAL");
	lua_pushinteger(L, ATSPI_STATE_VISIBLE);
	lua_setglobal(L, "VISIBLE");
	lua_pushinteger(L, ATSPI_STATE_MANAGES_DESCENDANTS);
	lua_setglobal(L, "MANAGES_DESCENDANTS");
	lua_pushinteger(L, ATSPI_STATE_INDETERMINATE);
	lua_setglobal(L, "INDETERMINATE");
	lua_pushinteger(L, ATSPI_STATE_REQUIRED);
	lua_setglobal(L, "REQUIRED");
	lua_pushinteger(L, ATSPI_STATE_TRUNCATED);
	lua_setglobal(L, "TRUNCATED");
	lua_pushinteger(L, ATSPI_STATE_ANIMATED);
	lua_setglobal(L, "ANIMATED");
	lua_pushinteger(L, ATSPI_STATE_INVALID_ENTRY);
	lua_setglobal(L, "INVALID_ENTRY");
	lua_pushinteger(L, ATSPI_STATE_SUPPORTS_AUTOCOMPLETION);
	lua_setglobal(L, "SUPPORTS_AUTOCOMPLETION");
	lua_pushinteger(L, ATSPI_STATE_SELECTABLE_TEXT);
	lua_setglobal(L, "SELECTABLE_TEXT");
	lua_pushinteger(L, ATSPI_STATE_IS_DEFAULT);
	lua_setglobal(L, "IS_DEFAULT");
	lua_pushinteger(L, ATSPI_STATE_VISITED);
	lua_setglobal(L, "VISITED");
	lua_pushinteger(L, ATSPI_STATE_HIGHLIGHTED);
	lua_setglobal(L, "HIGHLIGHTED");
	lua_pushinteger(L, ATSPI_STATE_LAST_DEFINED);
	lua_setglobal(L, "LAST_DEFINED");
}

static void _register_class(lua_State *L, const char *class_name, const luaL_Reg *funcs)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, class_name);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	luaI_openlib(L, 0, _meta_methods, 0);

	lua_pop(L, 1);

	luaI_openlib(L, 0, funcs, 0);
	lua_pop(L, 1);
}

static int _extensions_register(lua_State *L)
{
	_register_class(L, ACCESSIBLE_CLASS_NAME, _accessible_methods);
	_register_class(L, VALUE_CLASS_NAME, _value_methods);
	_register_class(L, SELECTION_CLASS_NAME, _selection_methods);

	_register_role_enums(L);

	lua_register(L, "desktop", l_get_root);
	lua_register(L, "T", l_get_translation);

	return 0;
}

int lua_engine_init(const char *script)
{
	L = luaL_newstate();

	luaL_openlibs(L);

	DEBUG("Initializing lua engine with script file: %s", script);

	if (_extensions_register(L))
	{
		ERROR("Failed to load screen-reader lua extensions");
		lua_close(L);
		return -1;
	}
	if (luaL_loadfile(L, script))
	{
		ERROR("Script loading failed: %s\n", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}
	if (lua_pcall(L, 0, 0, 0))
	{
		ERROR("Failed to init lua script: %s", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}
	DEBUG("Lua engine successfully inited.");
	return 0;
}

void lua_engine_shutdown(void)
{
	lua_close(L);
}

char *lua_engine_describe_object(AtspiAccessible *obj)
{
	const char *ret = NULL;

	// get 'describeObject' function and push arguments on stack
	lua_getglobal(L, "describeObject");

	_push_class_obj(L, obj, ACCESSIBLE_CLASS_NAME);

	// launch function
	if (lua_pcall(L, 1, 1, 0))
	{
		ERROR("Failed to run function 'describeObject': %s", lua_tostring(L, -1));
		goto end;
	}
	// get result from lua stack
	if (!lua_isstring(L, -1))
	{
		ERROR("Function 'describeObject' do not returned string value.");
		goto end;
	}
	ret = lua_tostring(L, -1);

end:
	// unref all AtspiAccessible references
	lua_gc(L, LUA_GCCOLLECT, 1);
	lua_pop(L, 1);
	return ret ? strdup(ret) : NULL;
}
