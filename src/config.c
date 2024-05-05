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
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "../include/config.h"
#include "../include/server.h"
#include "../include/lua/luaUtil.h"

/* purpose of this boolean define is just because this thing down there looks with it cool :) */
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

void hellwm_config_set_monitor(lua_State *L, struct wlr_output *output)
{
    char name[32]="";
    char *output_name = output->name;
    strcat(name,hellwm_config_groups_arr[HELLWM_CONFIG_MONITOR]);
    strcat(name,"_");

    for(int i=0;output_name[i]!='\0';i++)
    {
            if(output_name[i]=='-')
            {
                output_name[i] = '_';
            }
            else
            {
                continue;
            }
    } 
    strcat(name,output_name);
    
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
    
    if (hellwm_luaGetTable(L, name))
    {
        int32_t  width      =  tFLOAT    hellwm_luaGetField(L, "width", LUA_TNUMBER));
        int32_t  height     =  tFLOAT    hellwm_luaGetField(L, "height", LUA_TNUMBER));
        int32_t  hz         =  tFLOAT    hellwm_luaGetField(L, "hz", LUA_TNUMBER));
        int32_t  transfrom  =  tFLOAT    hellwm_luaGetField(L, "transfrom", LUA_TNUMBER));
        float    scale      =  tFLOAT    hellwm_luaGetField(L, "scale", LUA_TNUMBER));
        bool     vrr        =  tBOOLEAN  hellwm_luaGetField(L, "vrr", LUA_TBOOLEAN));
        
        switch (transfrom)
        {
          case 0:
            wlr_output_state_set_transform(&state, WL_OUTPUT_TRANSFORM_NORMAL);
            break;
         
          case 1:
            wlr_output_state_set_transform(&state, WL_OUTPUT_TRANSFORM_90);
            break;
         
          case 2:
            wlr_output_state_set_transform(&state, WL_OUTPUT_TRANSFORM_180);
            break;
         
          case 3:
            wlr_output_state_set_transform(&state, WL_OUTPUT_TRANSFORM_270);
            break;
        
          default:
            wlr_output_state_set_transform(&state, WL_OUTPUT_TRANSFORM_NORMAL);
            break;
        }

        if (width != 0 && height != 0) 
        {
            /* This function expect refresh rate as mhz, so we have to multiply it by 1000000 */
            wlr_output_state_set_custom_mode(&state, width, height, hz*1000000 );
        }
        else
        {
            hellwm_log(HELLWM_INFO, "Width or Height of %s output is not provided, set to preffered",name);
        }
        if (scale != 0)
        {
            wlr_output_state_set_scale(&state, scale);
        }

        /* Set variable refresh rate, if it's not set vrr = false */
        wlr_output_state_set_adaptive_sync_enabled(&state, vrr);
    }
    else
    {
        hellwm_log(HELLWM_LOG, "Could not access table: %s", name); 
    }

    /* Enable output */
    wlr_output_state_set_enabled(&state, true);

    if (wlr_output_commit_state(output, &state)==false)
    {
       hellwm_log(HELLWM_ERROR, "Cannot commit changes to output %s, compositor may not work properly",name);
    }

    hellwm_log(HELLWM_LOG, 
               "Output %s - set to: %"PRId32"x%"PRId32"@%f, scale: %f, VRR: %d",
               name,
               output->width, 
               output->height, 
               (float)(output->refresh)/(1000.f), // it's easier to read logfiles, just it 
               output->scale,
               output->adaptive_sync_status
               );
    wlr_output_state_finish(&state);

    lua_pop(L, 1);
}

void hellwm_config_set_keyboard(lua_State *L, struct wlr_keyboard *keyboard)
{
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    
    struct xkb_rule_names rule_names = 
    {
      	.rules = NULL,
      	.model = NULL,
      	.layout = "us",
      	.variant = NULL,
      	.options = NULL 
    };

	 struct xkb_keymap *keymap = xkb_keymap_new_from_names(
           context,
           &rule_names,
           XKB_KEYMAP_COMPILE_NO_FLAGS);

    if (hellwm_luaGetTable(L, (char*)hellwm_config_groups_arr[HELLWM_CONFIG_KEYBOARD]))
    {
        char * rules   = hellwm_luaGetField(L, "rules", LUA_TSTRING);
        char * model   = hellwm_luaGetField(L, "model", LUA_TSTRING);
        char * layout  = hellwm_luaGetField(L, "layout", LUA_TSTRING);
        char * variant = hellwm_luaGetField(L, "variant", LUA_TSTRING);
        char * options = hellwm_luaGetField(L, "options", LUA_TSTRING);

        int delay =  tFLOAT hellwm_luaGetField(L, "delay", LUA_TNUMBER));
        int rate = tFLOAT hellwm_luaGetField(L, "rate", LUA_TNUMBER));

        rule_names.rules   = rules;
        rule_names.model   = model;
        rule_names.layout  = layout;
        rule_names.variant = variant;
        rule_names.options = options;

        wlr_keyboard_set_repeat_info(keyboard,rate,delay);

        hellwm_log(
                HELLWM_LOG,
                "New Keyboard: %s, layout: %s",
                keyboard->base.name,
                layout 
        );
    }
    else
    {
        hellwm_log(HELLWM_INFO, "Cannot find keyboard configuration in config file. Keyboard set by default"); 
    }

    wlr_keyboard_set_keymap(keyboard,keymap);

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);

    lua_pop(L,1); 
}

void hellwm_config_reload_keyboards(lua_State *L, struct hellwm_server *server)
{
    struct hellwm_keyboard *keyboard, *tmp;
    wl_list_for_each_safe(keyboard, tmp, &server->keyboards, link)
    {
       hellwm_log(HELLWM_INFO, "KBD: %s",keyboard->wlr_keyboard->base.name); 
    }

    lua_pop(L,1); 
}

