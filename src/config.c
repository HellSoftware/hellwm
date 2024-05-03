#include <linux/limits.h>
#include <lua.h>
#include <stdio.h>
#include <endian.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>
#include <strings.h>
#include <xkbcommon/xkbcommon.h>

#include "../include/server.h"
#include "../include/config.h"
#include "../include/lua/luaUtil.h"

/* purpose of this define is just because this thing down there looks cool */
#define boolean bool

#define tINT *((int *) 
#define tCHAR *((char *) 
#define tFLOAT *((float *) 
#define tDOUBLE *((double *) 
#define tBOOLEAN *((boolean *)

void hellwm_config_setup(lua_State *L, char *configPath)
{
    L = hellwm_luaInit();
    hellwm_luaLoadFile(L, configPath);
}

void hellwm_config_apply_to_server(hellwm_config *config, struct hellwm_config_storage *storage, lua_State *L)
{
    char *tableName = "keyboard";

    hellwm_luaGetTable(L, tableName);
    char * layout = hellwm_luaGetField(L, "layout", LUA_TSTRING);
    int delay =  tINT hellwm_luaGetField(L, "delay", LUA_TNUMBER));
    int rate = *((int *)hellwm_luaGetField(L, "rate", LUA_TNUMBER));

    hellwm_log(HELLWM_LOG, "Lua config: %d %d", delay, rate);
    
    lua_pop(L,1); 
}
