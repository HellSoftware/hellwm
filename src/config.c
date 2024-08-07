#include <lauxlib.h>
#include <linux/limits.h>
#include <lua.h>
#include <stdint.h>
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
#include "../include/lua/lua_util.h"
#include "../include/lua/exposed_functions.h"

#include "../src/lua/exposed_functions.c"

#define defaultconfigPATH "config/config.lua"

/* purpose of this boolean define is just because this thing down there looks with it cool :) */
#define boolean bool

struct hellwm_config_binds global_keybinds;
struct hellwm_server *global_server;

void hellwm_config_setup(struct hellwm_server *server)
{
    server->L = hellwm_luaInit();
    global_server = server; 
    
    global_server->keybinds=&global_keybinds;
    hellwm_get_Server_g(server);

    hellwm_lua_expose_functions(server);

    if (access(server->configPath, F_OK) == -1)
    {
        hellwm_log(HELLWM_ERROR, "Could not find %s, using default path instead", server->configPath);
        hellwm_luaLoadFile(server->L, defaultconfigPATH);
    }

    hellwm_luaLoadFile(server->L, server->configPath);

    hellwm_config_set_decoration(server->L);
}

void hellwm_lua_expose_functions(struct hellwm_server *server)
{
    hellwm_lua_expose_function(server, hellwm_c_log                , "log");
    hellwm_lua_expose_function(server, hellwm_c_env                , "env"); 
    hellwm_lua_expose_function(server, hellwm_c_bind               , "bind");
    hellwm_lua_expose_function(server, hellwm_c_destroy            , "destroy");
    hellwm_lua_expose_function(server, hellwm_c_resize_toplevel_by , "resize_by");
    hellwm_lua_expose_function(server, hellwm_c_cursor_move        , "cursor_move");
    hellwm_lua_expose_function(server, hellwm_c_cursor_set_position, "cursor_set_position");

    hellwm_lua_expose_function(server, hellwm_c_exec               , FUNCTION_NAME(exec));
    hellwm_lua_expose_function(server, hellwm_c_log_flush          , FUNCTION_NAME(log_flush));
    hellwm_lua_expose_function(server, hellwm_c_focus_next         , FUNCTION_NAME(focus_next));
    hellwm_lua_expose_function(server, hellwm_c_kill_active        , FUNCTION_NAME(kill_active));
    hellwm_lua_expose_function(server, hellwm_c_config_reload      , FUNCTION_NAME(config_reload));
    hellwm_lua_expose_function(server, hellwm_c_toggle_fullscreen  , FUNCTION_NAME(toggle_fullscreen));
}

void hellwm_config_bind_add(const char *key, void *val, bool isFunc)
{
    const char *valueString = "";
    if (!isFunc)
    {
        valueString = (const char *) valueString;
        struct hellwm_config_one_bind *new_bind = malloc(sizeof(struct hellwm_config_one_bind));
        if (new_bind==NULL)
        {
            hellwm_log(HELLWM_ERROR, "Failed to allocate memory for new_bind inside hellwm_config_bind_add().");
            free(new_bind);
            return;
        }
        new_bind->key = xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE);
        new_bind->val = strdup(val);

        global_keybinds.binds = realloc(global_keybinds.binds, (global_keybinds.count + 1) * sizeof(struct hellwm_config_one_bind*)); 

        if (global_keybinds.binds==NULL)
        {
            hellwm_log(HELLWM_ERROR, "Failed to reallocate memory for global_keybinds->binds inside hellwm_config_bind_add()");
            return;
        }
        global_keybinds.binds[global_keybinds.count] = new_bind;
        global_keybinds.count++;
        
        hellwm_log(HELLWM_LOG, "New BIND: [%s] = %s",key,global_keybinds.binds[global_keybinds.count-1]->val);
    }
    else
    {
        int num_args = lua_gettop(global_server->L);
        int num_additional_args = num_args - 2;

        struct hellwm_config_one_fbind *new_bind = malloc(sizeof(struct hellwm_config_one_fbind));
        if (new_bind==NULL)
        {
            hellwm_log(HELLWM_ERROR, "Failed to allocate memory for new_bind inside hellwm_config_bind_add().");
            free(new_bind);
            return;
        }
        new_bind->key = xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE);
        new_bind->val = val;

        global_keybinds.fbinds= realloc(global_keybinds.fbinds, (global_keybinds.fcount + 1) * sizeof(struct hellwm_config_one_fbind*)); 

        if (global_keybinds.fbinds==NULL)
        {
            hellwm_log(HELLWM_ERROR, "Failed to reallocate memory for global_keybinds->binds inside hellwm_config_bind_add()");
            return;
        }
        global_keybinds.fbinds[global_keybinds.fcount] = new_bind;
        global_keybinds.fcount++;

        hellwm_log(HELLWM_LOG, "New BIND: [%s] = %d - %d", key, num_args, num_additional_args);
    }
}

void hellwm_config_bind_free_array(struct hellwm_config_binds *binds)
{
    for (size_t i = 0; i < binds->count; ++i)
    {
        free((char *)binds->binds[i]->val);
        free(binds->binds[i]);
    }
    free(binds->binds);
    free(binds);
}

void hellwm_config_set_decoration(lua_State *L)
{
    if (hellwm_luaGetTable(L, (char*)hellwm_config_groups_arr[HELLWM_CONFIG_DECORATION]))
    {
        uint8_t window_decoration_mode = (int)hellwm_luaGetFieldInteger(L, "window_decoration_mode");

        global_server->default_decoration_mode = window_decoration_mode;
    }
    else /* Set to default */
    {
        global_server->default_decoration_mode = 2; /* Server side */ 
    }

    hellwm_log(
                HELLWM_LOG,
                "Decoration settings: window_decoration_mode: %d",
                global_server->default_decoration_mode 
        );

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
    wlr_output_state_set_enabled(&state, true);
    struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
    
    int lx = 0, ly = 0;

    if (hellwm_luaGetTable(L, name))
    {
        int   width      = hellwm_luaGetFieldInteger(L, "width");
        int   height     = hellwm_luaGetFieldInteger(L, "height");
        int   hz         = hellwm_luaGetFieldInteger(L, "hz");
        int   transfrom  = hellwm_luaGetFieldInteger(L, "transfrom");
       
              lx         = hellwm_luaGetFieldInteger(L, "x");
              ly         = hellwm_luaGetFieldInteger(L, "y");
       
        float scale      = hellwm_luaGetFieldFloat(L, "scale");
        bool  vrr        = hellwm_luaGetFieldBool(L, "vrr");
        
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
 
        if (scale != 0)
        {
            wlr_output_state_set_scale(&state, scale);
        }

        /* Set variable refresh rate, if it's not set vrr = false */
        wlr_output_state_set_adaptive_sync_enabled(&state, vrr);
        
        if (width != 0 && height != 0) 
        {
            /* TODO: fix refresh rate - cannot set ANY value exept 0 */
            wlr_output_state_set_custom_mode(&state, width, height, hz);
        }
        else
        {
            hellwm_log(HELLWM_INFO, "Width or Height of %s output is not provided, set to preffered",name);
        }

    }
    else
    {
        hellwm_log(HELLWM_LOG, "Could not access table: %s", name); 
    }

    struct wlr_output_layout_output *l_output = wlr_output_layout_add(global_server->output_layout, output, lx, ly);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(global_server->scene, output);
    wlr_scene_output_layout_add_output(global_server->scene_layout, l_output, scene_output);
    
    if (wlr_output_test_state(output, &state))
    {
        wlr_output_commit_state(output, &state);
    }
    else
    {
       hellwm_log(HELLWM_ERROR, "Cannot commit changes to output %s, compositor may not work properly",name);
    }

    hellwm_log(HELLWM_LOG, 
           "Output %s - set to: %"PRId32"x%"PRId32"@%f, on pos: %dx%d, scale: %f, VRR: %d",
           name,
           output->width, 
           output->height, 
           lx, ly,
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

    if (hellwm_luaGetTable(L, (char*)hellwm_config_groups_arr[HELLWM_CONFIG_KEYBOARD]))
    {
        char * rules   = hellwm_luaGetFieldString(L, "rules");
        char * model   = hellwm_luaGetFieldString(L, "model");
        char * layout  = hellwm_luaGetFieldString(L, "layout");
        char * variant = hellwm_luaGetFieldString(L, "variant");
        char * options = hellwm_luaGetFieldString(L, "options");

        int delay = hellwm_luaGetFieldInteger(L, "delay");
        int rate =  hellwm_luaGetFieldInteger(L, "rate");

        rule_names.rules   = rules;
        rule_names.model   = model;
        rule_names.layout  = layout;
        rule_names.variant = variant;
        rule_names.options = options;

        wlr_keyboard_set_repeat_info(keyboard,rate,delay);

        hellwm_log(
                HELLWM_DEBUG,
                "New Keyboard: %s, layout: %s, delay: %d, rate: %d",
                keyboard->base.name,
                layout, 
                delay,
                rate
        );
    }
    else
    {
        hellwm_log(HELLWM_INFO, "Cannot find keyboard configuration in config file. Keyboard set by default"); 
    }
    
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(
        context,
        &rule_names,
        XKB_KEYMAP_COMPILE_NO_FLAGS);

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

