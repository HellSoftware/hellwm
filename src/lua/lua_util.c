#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon.h>

#include "../../include/server.h"
#include "../../include/lua/lua_util.h"

lua_State *hellwm_luaInit()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    return L;
}

int hellwm_luaLoadFile(lua_State *L, char *filename)
{
  if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0))
  {
      hellwm_log(HELLWM_ERROR,
          "%s: %s",
          filename,
          lua_tostring(L, -1)
      );
      return 1;
  }
  return 0;
}

int hellwm_luaGetTable(lua_State *L, char *tableName)
{
  lua_getglobal(L, tableName);
  if (!lua_istable(L, -1))
  {
    hellwm_log(HELLWM_ERROR, "%s is not a table", tableName);
    return false;
  }
  return true;
}

/* 
 * Assuming table is at top of stack,
 * we just use lua_getglobal() before calling this function
 */ 
char* hellwm_luaGetFieldString(lua_State *L, char *fieldName)
{
  char *string_val = "";

  lua_pushstring(L, fieldName);
  lua_gettable(L, -2);

  if (lua_isstring(L, -1))
  {
    string_val = lua_tostring(L, -1);
  }

  lua_pop(L,1);
  return string_val;
}

bool hellwm_luaGetFieldBool(lua_State *L, char *fieldName)
{
  bool bool_val = 0;
  lua_pushstring(L, fieldName);
  lua_gettable(L, -2);

   if (lua_isboolean(L, -1))
   {
     bool_val = lua_toboolean(L,-1); 
   }

  lua_pop(L,1);
  return bool_val;
}

int hellwm_luaGetFieldInteger(lua_State *L, char *fieldName)
{
  int int_val = 0;

  lua_pushstring(L, fieldName);
  lua_gettable(L, -2);

  if (lua_isnumber(L, -1))
  {
    int_val = lua_tonumber(L,-1); 
  }
  
  lua_pop(L,1);
  return int_val;
}

float hellwm_luaGetFieldFloat(lua_State *L, char *fieldName)
{
  float float_val = 0;

  lua_pushstring(L, fieldName);
  lua_gettable(L, -2);

  if (lua_isnumber(L, -1))
  {
    float float_val = lua_tonumber(L,-1); 
  }
  
  lua_pop(L,1);
  return float_val;
}

void hellwm_luaGetTableField(lua_State *L, char *tableName, char *fieldName)
{
  lua_getglobal(L, tableName);
  lua_pushstring(L, fieldName);
  lua_gettable(L, -2);

  printf("%s - %s: %f",tableName, fieldName, lua_tonumber(L, -1));

  lua_pop(L,1);
  lua_pop(L,1);
}
