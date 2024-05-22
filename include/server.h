#ifndef SERVER_H
#define SERVER_H

#include <lua.h>
#include <time.h>
#include <stdio.h>
#include <wayland-util.h>
#include <wchar.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
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
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_server_decoration.h>

#include "config.h"
#include "lua/luaUtil.h"


#ifdef XWAYLAND
#include <xcb/xproto.h>
#include <wlr/xwayland/shell.h>
#include <wlr/xwayland/xwayland.h>
#include "../include/xwayland.h"
#endif

#define HELLWM_INFO  "INFO"
#define HELLWM_ERROR "ERROR"
#define HELLWM_LOG   "LOG"
#define HELLWM_DEBUG "DEBUG"

enum hellwm_cursor_mode
{
	HELLWM_CURSOR_PASSTHROUGH,
	HELLWM_CURSOR_MOVE,
	HELLWM_CURSOR_RESIZE,
};

struct hellwm_server
{
	lua_State *L;

	char *configPath;
	const char *socket;

	double grab_x, grab_y;

	uint32_t resize_edges;

	enum hellwm_cursor_mode cursor_mode;

	struct wlr_seat *seat;
	struct wl_list outputs;
	struct wlr_scene *scene;
	struct wl_list keyboards;
	struct wl_list toplevels;
	struct wl_list xtoplevels;
	struct wlr_cursor *cursor;
	struct wlr_box grab_geobox;
	struct wlr_backend *backend;
	struct wl_listener new_input;
	struct wl_display *wl_display;
	struct wl_listener new_output;
	struct wlr_renderer *renderer;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;
	struct wlr_allocator *allocator;
	struct wlr_xdg_shell *xdg_shell;
	struct wl_listener cursor_button;
	struct wl_listener cursor_motion;
	struct wl_listener new_xdg_popup;
	struct wl_listener request_cursor;
	struct hellwm_tile_tree *tile_tree;
	struct hellwm_config_binds *keybinds;
	struct wlr_compositor *compositor;
	struct wl_listener new_xdg_toplevel;
	struct wl_listener new_layer_surface;
	struct wlr_layer_shell_v1 *layer_shell;
	struct wlr_xcursor_manager *cursor_mgr;
	struct wlr_output_layout *output_layout;
	struct hellwm_toplevel *grabbed_toplevel;
	struct wl_listener request_set_selection;
	struct wl_listener cursor_motion_absolute;
	struct wlr_scene_output_layout *scene_layout;
	struct hellwm_config_pointers *config_pointer;
	struct wlr_xdg_decoration_manager_v1 *xdg_decoration_manager;	

	xcb_atom_t atoms[ATOM_LAST];
	struct wlr_xwayland *xwayland;
	struct wl_listener xwayland_ready;
	struct wl_listener new_xwayland_surface;
};

struct hellwm_tile_tree
{
	struct hellwm_toplevel *toplevel;
	int width, height;
	int direction;
	int x, y;
	
	struct hellwm_tile_tree *parent;
	struct hellwm_tile_tree *right;
	struct hellwm_tile_tree *left;
};

struct hellwm_output
{
	struct wl_list link;
	struct wl_listener frame;
	struct wl_listener destroy;
	struct hellwm_server *server;
	struct wlr_output *wlr_output;
	struct wl_listener request_state;
};

struct hellwm_toplevel
{
	struct wl_list link;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener destroy;
	struct hellwm_server *server;
	struct wl_listener request_move;
	struct wl_listener request_title;
	struct wl_listener request_app_id;
	struct wl_listener request_resize;
	struct wlr_scene_tree *scene_tree;
	struct hellwm_tile_tree* tile_node;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
	struct wl_listener destroy_decoration;
	struct wlr_xdg_toplevel *xdg_toplevel;
	struct wl_listener set_decoration_mode;
	struct wlr_xdg_toplevel_decoration_v1 *decoration; 
};

struct hellwm_popup
{
	struct wlr_scene_tree *scene;
	struct wlr_xdg_popup *xdg_popup;
	struct hellwm_toplevel *toplevel;

	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_listener new_popup;
};

struct hellwm_keyboard
{
	struct wl_list link;
	struct hellwm_server *server;
	struct wlr_keyboard *wlr_keyboard;

	struct wl_listener key;
	struct wl_listener destroy;
	struct wl_listener modifiers;
};

struct hellwm_toplevel_list
{
	int32_t size;
	int32_t last_id;
	int32_t current_id;
	struct hellwm_toplevel_list_element **list;
};

struct hellwm_toplevel_list_element
{
	int32_t position;
	struct hellwm_toplevel *toplevel;
};

void hellwm_log_flush();
void focus_next(struct hellwm_server *server);
void hellwm_setup(struct hellwm_server *server);
void hellwm_log(char *logtype, char *format, ...);
void toggle_fullscreen(struct hellwm_server *server);
void hellwm_config_reload(struct hellwm_server *server);
void hellwm_destroy_everything(struct hellwm_server *server);
void hellwm_toggle_fullscreen_toplevel(struct hellwm_server *server);
void hellwm_toplevel_remove_from_list(struct wlr_xdg_toplevel *toplevel);
void hellwm_toplevel_add_to_list(struct hellwm_server *server, struct hellwm_toplevel *new_toplevel);

static void exec(char *command);
static void kill_active(struct hellwm_server *server);
static void reset_cursor_mode(struct hellwm_server *server);
static void output_frame(struct wl_listener *listener, void *data);
static void output_destroy(struct wl_listener *listener, void *data);
static void server_new_input(struct wl_listener *listener, void *data);
static void xdg_popup_commit(struct wl_listener *listener, void *data);
static void xdg_toplevel_map(struct wl_listener *listener, void *data);
static void server_new_output(struct wl_listener *listener, void *data);
static void xdg_popup_destroy(struct wl_listener *listener, void *data);
static void server_cursor_axis(struct wl_listener *listener, void *data);
static void keyboard_handle_key(struct wl_listener *listener, void *data);
static void seat_request_cursor(struct wl_listener *listener, void *data);
static void server_cursor_frame(struct wl_listener *listener, void *data);
static void xdg_toplevel_commit(struct wl_listener *listener, void *data);
static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) ;
static void output_request_state(struct wl_listener *listener, void *data);
static void server_cursor_button(struct wl_listener *listener, void *data);
static void server_cursor_motion(struct wl_listener *listener, void *data);
static void server_new_xdg_popup(struct wl_listener *listener, void *data);
static void xdg_toplevel_destroy(struct wl_listener *listener, void *data);
static void xdg_handle_decoration(struct wl_listener *listener, void *data);
static void process_cursor_move(struct hellwm_server *server, uint32_t time);
static void xdg_toplevel_set_title(struct wl_listener *listener, void *data);
static void xdg_toplevel_set_app_id(struct wl_listener *listener, void *data);
static bool handle_keybinding(struct hellwm_server *server, xkb_keysym_t sym);
static void keyboard_handle_destroy(struct wl_listener *listener, void *data);
static void server_new_xdg_toplevel(struct wl_listener *listener, void *data);
static void process_cursor_motion(struct hellwm_server *server, uint32_t time);
static void process_cursor_resize(struct hellwm_server *server, uint32_t time);
static void keyboard_handle_modifiers(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_move(struct wl_listener *listener, void *data);
static void seat_request_set_selection(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) ;
static void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data);
void xdg_toplevel_decoration_request_destroy(struct wl_listener *listener, void *data);
static void focus_toplevel(struct hellwm_toplevel *toplevel, struct wlr_surface *surface);
static void hellwm_resize_toplevel_by(struct hellwm_server *server, int32_t w, int32_t h);
static void server_new_touch(struct hellwm_server *server, struct wlr_input_device *device);
static void server_new_tablet(struct hellwm_server *server, struct wlr_input_device *device);
static void server_new_pointer(struct hellwm_server *server, struct wlr_input_device *device);
static void server_new_keyboard(struct hellwm_server *server, struct wlr_input_device *device);
void xdg_toplevel_decoration_request_decoration_mode(struct wl_listener *listener, void *data);
static void begin_interactive(struct hellwm_toplevel *toplevel, enum hellwm_cursor_mode mode, uint32_t edges);
static struct hellwm_toplevel *desktop_toplevel_at(struct hellwm_server *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy);

#endif
