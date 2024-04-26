#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

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

int luaRunFromFile(lua_State *L, char *filename)
{
    if (luaL_dofile(L, filename) == LUA_OK)
    {
        lua_pop(L, lua_gettop(L));
        return 0;
    }
    return 1;
}

void test()
{
    lua_State *L = initLua();
    luaRunCode(L,"print('Hello, World')");
    luaRunCode(L, "test_config.lua");
    lua_close(L);
}
