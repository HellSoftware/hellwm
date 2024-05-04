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
    hellwm_config_set_keyboard(L,config_pointer);
    // TODO not working yet - hellwm_config_set_monitor(L, config_pointer);
}

void hellwm_config_set_monitor(lua_State *L, struct hellwm_config_pointers *config_pointer)
{
    for (int i=0;i<config_pointer->server->output_list->count;i++)
    {
        char name[32]="";
        struct hellwm_output *output = config_pointer->server->output_list->outputs[i];
        char *output_name = output->wlr_output->name;
        strcat(name,hellwm_config_groups_arr[HELLWM_CONFIG_MONITOR]);
        strcat(name,"_");
        for(i=0;output_name[i]!='\0';i++)
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
        
        if (!hellwm_luaGetTable(L, name))
        {
            int width      =  tINT   hellwm_luaGetField(L, "width", LUA_TNUMBER));
            int height     =  tINT   hellwm_luaGetField(L, "height", LUA_TNUMBER));
            int hz         =  tINT   hellwm_luaGetField(L, "hz", LUA_TNUMBER));
            int transfrom  =  tINT   hellwm_luaGetField(L, "transfrom", LUA_TNUMBER));
            int scale      =  tFLOAT hellwm_luaGetField(L, "scale", LUA_TNUMBER));

            struct wlr_output_state state;
            wlr_output_state_init(&state);
           	wlr_output_state_set_enabled(&state, true);
            struct wlr_output_mode *mode = wlr_output_preferred_mode(output->wlr_output);

            if (width!=0)
            {
                width=mode->width;
            }
            if (height!=0)
            {
                height=mode->height;
            }
            if (hz!=0)
            {
                hz=mode->refresh;
            }
            if (scale==0)
            {
                scale=1;
            }
            
            wlr_output_state_set_custom_mode(&state, width, height, hz);
            wlr_output_state_set_scale(&state, scale);
           
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
            wlr_output_commit_state(output->wlr_output, &state);
           	wlr_output_state_finish(&state);
        }
        else
        {
            hellwm_log(HELLWM_LOG, "Could not get table: %s", name); 
        }
        lua_pop(L, 1);
    }
}

void hellwm_config_set_keyboard(lua_State *L, struct hellwm_config_pointers *config_pointer)
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
	
    for (int i = 0; i < config_pointer->server->keyboard_list->count; ++i)
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
