#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon.h>

#include "../../include/server.h"
#include "../../include/lua/luaUtil.h"

lua_State *initLua()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    return L;
}

int luaRunCode(lua_State *L, char *code)
{
  if (luaL_loadstring(L, code) == LUA_OK)
  {
    if (lua_pcall(L, 0, 0, 0) == LUA_OK)
    {
      lua_pop(L, lua_gettop(L));
      return 0;
    }
    else
    {
      return 1;
    }
  }
  return 1;
}

int luaFromFile(lua_State *L, char *filename)
{
    if (luaL_dofile(L, filename) == LUA_OK)
    {
        lua_pop(L, lua_gettop(L));
        return 0;
    }
    return 1;
}

void luaReadTable(lua_State *L, char *name)
{
  lua_getglobal(L,name); 
  if (lua_istable(L, -1))
  {
    while (lua_next(L, -1) != 0)
    {
      const char *key = lua_tostring(L, -2);
      const char *valueType = lua_typename(L, lua_type(L, -1));
      printf("Key: %s, Value Type: %s\n", key, valueType);
      lua_pop(L, 1);
    }
  }
  else
  {
    printf("Not a table");
  }
}

void luaSetKeyboard(lua_State *L, struct xkb_context *context, struct xkb_keymap *keymap)
{
  context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	struct xkb_rule_names rule_names = {
		.rules = NULL,
		.model = NULL,
		.layout = "pl",
		.variant = NULL,
		.options = NULL
	};

	keymap = xkb_keymap_new_from_names(context, &rule_names,
		XKB_KEYMAP_COMPILE_NO_FLAGS);
}

void test()
{
    lua_State *L = initLua();
    luaRunCode(L,"print('Hello, World')");
    luaRunCode(L, "test_config.lua");
    lua_close(L);
}
