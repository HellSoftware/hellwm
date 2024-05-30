#ifndef LUAUTIL_H
#define LUAUTIL_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

lua_State *hellwm_luaInit();
int hellwm_luaLoadFile(lua_State *L, char *filename);
int hellwm_luaGetTable(lua_State *L, char *tableName);
void *hellwm_luaGetField(lua_State *L, char *fieldName, int lua_type);
void *hellwm_luaGetField(lua_State *L, char *fieldName, int lua_variableType);

#endif
