#ifndef CONFIG_H
#define CONFIG_H
#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>
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

#include "lua/luaUtil.h"

const char *hellwm_config_groups_arr[] = 
{
 "source",
 "bind",
 "monitor",
 "keyboard",
 "autostart"
};

enum hellwm_config_groups_types
{
    HELLWM_CONFIG_SOURCE=0,
    HELLWM_CONFIG_BIND=1,
    HELLWM_CONFIG_MONITOR=2,
    HELLWM_CONFIG_KEYBOARD=3,
    HELLWM_CONFIG_AUTOSTART=4
};

struct hellwm_config_pointers
{
	struct hellwm_server *server;
};

struct hellwm_config_binds
{
    struct hellwm_config_one_bind **binds;
    int32_t count;
};

struct hellwm_config_one_bind
{
    xkb_keysym_t key;
    char *val;
};

static int hellwm_c_bind(lua_State *L);
void hellwm_config_setup(struct hellwm_server *server);
void hellwm_config_bind_free_array(struct hellwm_config_binds *binds);
void hellwm_config_set_monitor(lua_State *L, struct wlr_output *output);
void hellwm_config_set_keyboard(lua_State *L, struct wlr_keyboard *keyboard);
void hellwm_config_binds_load(lua_State *L, struct hellwm_config_binds *binds);
void hellwm_config_reload_keyboards(lua_State *L, struct hellwm_server *server);
void hellwm_config_bind_add(const char *key, const char *val);

#endif
