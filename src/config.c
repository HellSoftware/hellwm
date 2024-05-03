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

#include "../include/config.h"
#include "../include/server.h"
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

void hellwm_config_apply_to_server(lua_State *L, struct hellwm_config_pointers *config_pointer)
{
    hellwm_config_keyboard_set(L,config_pointer);
}

void hellwm_config_keyboard_set(lua_State *L, struct hellwm_config_pointers *config_pointer)
{
    hellwm_luaGetTable(L, (char*)hellwm_config_groups_arr[HELLWM_CONFIG_KEYBOARD]);
    char * rules   = hellwm_luaGetField(L, "rules", LUA_TSTRING);
    char * model   = hellwm_luaGetField(L, "model", LUA_TSTRING);
    char * layout  = hellwm_luaGetField(L, "layout", LUA_TSTRING);
    char * variant = hellwm_luaGetField(L, "variant", LUA_TSTRING);
    char * options = hellwm_luaGetField(L, "options", LUA_TSTRING);

    int delay =  tINT hellwm_luaGetField(L, "delay", LUA_TNUMBER));
    int rate = tINT hellwm_luaGetField(L, "rate", LUA_TNUMBER));

    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    struct xkb_rule_names rule_names = 
    {
   		.rules = rules,
   		.model = model,
   		.layout = layout,
   		.variant = variant,
   		.options = options 
    };

	 struct xkb_keymap *keymap = xkb_keymap_new_from_names(
           context,
           &rule_names,
           XKB_KEYMAP_COMPILE_NO_FLAGS);
	
    for (int i = 0; i < config_pointer->server->keyboard_list->count-1; ++i)
    {
        wlr_keyboard_set_keymap(config_pointer->server->keyboard_list->keyboards[i]->wlr_keyboard,keymap);
        wlr_keyboard_set_repeat_info(config_pointer->server->keyboard_list->keyboards[i]->wlr_keyboard,rate,delay);
    }
	 
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);

    hellwm_log(
            HELLWM_LOG,
            "Keyboard Config: name: %s, delay: %d, rate: %d, rules: %s, model: %s, layout: %s, variant: %s, options: %s ",
            config_pointer->server->keyboard_list->keyboards[0]->wlr_keyboard->base.name,
            delay, 
            rate, 
            rules, 
            model, 
            layout, 
            variant, 
            options
    );
    lua_pop(L,1); 
}
