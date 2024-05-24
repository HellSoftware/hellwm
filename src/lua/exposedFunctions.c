#include <lauxlib.h>
#include <lua.h>
#include <time.h>
#include <stdio.h>
#include <wayland-util.h>
#include <wchar.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>
#include <pthread.h>
#include <stdbool.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_seat.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <xkbcommon/xkbcommon.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_keyboard.h>
#include <wayland-server-protocol.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_subcompositor.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>

#include "../../include/server.h"

/* Get name of the function */
#define FUNCTION_NAME(func) #func

struct hellwm_server *global_server;

void hellwm_get_Server_g(struct hellwm_server *server)
{
   global_server = server;
}

void hellwm_lua_expose_function(struct hellwm_server *server, void *function, char *name) 
{
   lua_pushcfunction(server->L, function);
   lua_setglobal(server->L, name);

   hellwm_log(HELLWM_LOG, "LUA: Exposed function: %s ", name);
}

static int hellwm_c_bind(lua_State *L)
{

   const char *key = luaL_checkstring(L, 1);
   
   if (lua_isfunction(L, 2))
   {
      hellwm_config_bind_add(key, lua_tocfunction(L, 2), true);
   }
   else
   {
      hellwm_config_bind_add(key, (char*)luaL_checkstring(L,2), false);
   }

   return 0;
}

void hellwm_c_resize_toplevel_by(lua_State *L)
{
   int32_t w = luaL_checkinteger(L, 1);
   int32_t h = luaL_checkinteger(L, 2);
   
   hellwm_resize_toplevel_by(global_server, w, h);
}

void hellwm_c_config_reload()
{
   hellwm_config_reload(global_server);
}

void hellwm_c_env()
{
   char *env = (char *)luaL_checkstring(global_server->L, 1);
   char *val = (char *)luaL_checkstring(global_server->L, 2);
   setenv(env, val, 1);
}

void hellwm_c_toggle_fullscreen()
{
   toggle_fullscreen(global_server);
}

void hellwm_c_focus_next()
{
   focus_next(global_server);
}

void hellwm_c_kill_active()
{
   kill_active(global_server);
}

void hellwm_c_destroy()
{
   hellwm_destroy_everything(global_server);
}

void hellwm_c_exec(lua_State *L)
{
     exec((char*)luaL_checkstring(L, 1)); 
}

void hellwm_c_log(lua_State *L)
{
    char *log = (char *)luaL_checkstring(L, 1);

    hellwm_log("LUA", log);
}

void hellwm_c_log_flush(lua_State *L)
{
    hellwm_log_flush();
}
