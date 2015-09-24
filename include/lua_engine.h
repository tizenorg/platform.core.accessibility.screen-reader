#ifndef _LUA_ENGINE_H__
#define _LUA_ENGINE_H__

#include <atspi/atspi.h>

/**
 * @brief Registers additional AT-SPI2 related method on given lua state.
 *
 *
 * List of additional lua functions:
 *
 * desktop() -- global function returning desktop frame object.
 * T() -- returns translated string using gettext function.
 *
 * # Accessible object class methods
 * name() -- return string with accessible name of object.
 * description() -- return string with accessible description of object.
 * children() -- return table with accessible children references.
 * parent() -- return accessible parent reference.
 * role() -- return accessible role of object.
 * inRelation(r) -- return table with references to accessible objects in relation 'd'.
 * roleName() -- return string with name of the role.
 * is(s) -- return true is accessible object has state 's'.
 * index() -- return index of accessible object in its parent list.
 * value() -- return reference to value object or nil
 * selection() -- return reference to selection object or nil
 *
 * # Value object class methods
 * current() -- return current value of value object.
 * range() -- return minimal and maximal (tuple: min, max) values of value object.
 * increment() -- return minimal incremetation value of value object.
 *
 * # Selection object class methods
 * count() -- return number of selected children of selection object.
 * isSelected(i) -- return true if 'i' child of selection object is selected.
 *
 * @param absolute path to lua script.
 * @return 0 on success, different value on failure.
 */
int lua_engine_init(const char *script);

/*
 * @brief Create a description for given accessible object.
 *
 * Internally it calls 'describeObject' function from lua script loaded in
 * lua_engine_init
 *
 * Function 'describeObject' must have following prototype:
 *
 * function describeObject(obj)
 * 	return "some string"
 * end
 *
 * @param AtspiAccessible reference
 * @return string with a text describing object 'obj'. Should be free.
 */
char *lua_engine_describe_object(AtspiAccessible *obj);

/*
 * @brief Shutdowns lua engine
 */
void lua_engine_shutdown();

#endif
