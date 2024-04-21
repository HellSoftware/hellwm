#ifndef SERVER_H
#define SERVER_H

#include <assert.h>
#include <complex.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wchar.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "config.h"

#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#endif

#define HELLWM_INFO  "INFO"
#define HELLWM_ERROR "ERROR"
#define HELLWM_LOG   "LOG"


enum hellwm_cursor_mode
{
	HELLWM_CURSOR_PASSTHROUGH,
	HELLWM_CURSOR_MOVE,
	HELLWM_CURSOR_RESIZE,
};

#if XWAYLAND
struct hellwm_xwayland
{
	struct wlr_xwayland *wlr_xwayland;
	struct wlr_xcursor_manager *xcursor_manager;
};
#endif

struct hellwm_server
{
	struct wl_display *wl_display;
	struct wlr_backend *backend;
	struct wlr_renderer *renderer;
	struct wlr_allocator *allocator;
	struct wlr_compositor *compositor;
	struct wlr_scene *scene;
	struct wlr_scene_output_layout *scene_layout;

	struct wlr_xdg_shell *xdg_shell;
	struct wl_listener new_xdg_toplevel;
	struct wl_listener new_xdg_popup;
	struct wl_list toplevels;

	struct wlr_layer_shell_v1 *layer_shell;
	struct wl_listener new_layer_surface;

#if XWAYLAND
	struct hellwm_xwayland xwayland;
	struct wl_listener xwayland_surface;
	struct wl_listener xwayland_ready;
#endif

	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *cursor_mgr;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wlr_seat *seat;
	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener request_set_selection;
	struct wl_list keyboards;
	enum hellwm_cursor_mode cursor_mode;
	struct hellwm_toplevel *grabbed_toplevel;
	double grab_x, grab_y;
	struct wlr_box grab_geobox;
	uint32_t resize_edges;

	struct wlr_output_layout *output_layout;
	//struct hellwm_outputs_list *outputs_list;
	struct wl_listener new_output;
	struct wl_list outputs;
	const char *socket;
	struct hellwm_toplevel_list *alltoplevels;

	struct hellwm_config_storage config_storage;
};

struct hellwm_output
{
	struct {
		struct wlr_scene_tree *shell_background;
		struct wlr_scene_tree *shell_overlay;
		struct wlr_scene_tree *shell_bottom;
		struct wlr_scene_tree *fullscreen;
		struct wlr_scene_tree *shell_top;
		struct wlr_scene_tree *tiling;
	} layers;

	struct wl_list link;
	struct wl_listener frame;
	struct wl_listener destroy;
	struct hellwm_server *server;
	struct wlr_output *wlr_output;
	struct wl_listener request_state;
};

struct hellwm_outputs_list 
{
	int count;
	struct hellwm_output **outputs;
};

struct hellwm_toplevel
{
	struct wl_list link;
	struct hellwm_server *server;
	struct wlr_xdg_toplevel *xdg_toplevel;
	struct wlr_scene_tree *scene_tree;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;

	struct hellwm_output *output;
	struct wlr_scene_layer_surface_v1 *scene;
	struct wlr_layer_surface_v1 *layer_surface;
};

struct hellwm_popup
{
	struct wlr_xdg_popup *xdg_popup;
	struct wlr_scene_tree *scene;
	struct hellwm_toplevel *toplevel;

	struct wl_listener new_popup;
	struct wl_listener commit;
	struct wl_listener destroy;
};

struct hellwm_keyboard
{
	struct wl_list link;
	struct hellwm_server *server;
	struct wlr_keyboard *wlr_keyboard;

	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_listener destroy;
};

struct hellwm_toplevel_list
{
	struct hellwm_toplevel_list_element **list;
	int32_t size;
	int32_t current_id;
	int32_t last_id;
};

struct hellwm_toplevel_list_element
{
	struct hellwm_toplevel *toplevel;
	int32_t position;
};
void hellwm_toplevel_add_to_list(struct hellwm_server *server, struct hellwm_toplevel *new_toplevel);
void hellwm_log(char *logtype, char *format, ...);
void hellwm_log_flush();
void hellwm_toplevel_remove_from_list(struct wlr_xdg_toplevel *toplevel);
void hellwm_toggle_fullscreen_toplevel(struct hellwm_server *server);
void hellwm_setup(struct hellwm_server *server);
void hellwm_destroy_everything(struct hellwm_server *server);
void hellwm_print_usage(int *argc, char**argv[]);
static void exec_cmd(char *command);
static void hellwm_resize_toplevel_by(struct hellwm_server *server, int32_t w, int32_t h);
static void focus_toplevel(struct hellwm_toplevel *toplevel, struct wlr_surface *surface) ;
static void destroy_toplevel(struct hellwm_server *server) ;
static void keyboard_handle_modifiers(struct wl_listener *listener, void *data);
static bool handle_keybinding(struct hellwm_server *server, xkb_keysym_t sym);
static void keyboard_handle_key(struct wl_listener *listener, void *data);
static void keyboard_handle_destroy(struct wl_listener *listener, void *data);
static void server_new_keyboard(struct hellwm_server *server, struct wlr_input_device *device);
static void server_new_pointer(struct hellwm_server *server, struct wlr_input_device *device);
static void server_new_input(struct wl_listener *listener, void *data);
static void seat_request_cursor(struct wl_listener *listener, void *data);
static void seat_request_set_selection(struct wl_listener *listener, void *data);
static void reset_cursor_mode(struct hellwm_server *server);
static void process_cursor_move(struct hellwm_server *server, uint32_t time);
static void process_cursor_resize(struct hellwm_server *server, uint32_t time);
static void process_cursor_motion(struct hellwm_server *server, uint32_t time);
static void server_cursor_motion(struct wl_listener *listener, void *data);
static void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
static void server_cursor_button(struct wl_listener *listener, void *data);
static void server_cursor_axis(struct wl_listener *listener, void *data);
static void server_cursor_frame(struct wl_listener *listener, void *data);
static void output_frame(struct wl_listener *listener, void *data);
static void output_request_state(struct wl_listener *listener, void *data);
static void output_destroy(struct wl_listener *listener, void *data);
static void server_new_output(struct wl_listener *listener, void *data);
static void xdg_toplevel_map(struct wl_listener *listener, void *data) ;
static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) ;
static void xdg_toplevel_commit(struct wl_listener *listener, void *data) ;
static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) ;
static void begin_interactive(struct hellwm_toplevel *toplevel, enum hellwm_cursor_mode mode, uint32_t edges);
static void xdg_toplevel_request_move(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data);
static void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data);
static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) ;
static void xdg_popup_commit(struct wl_listener *listener, void *data);
static void xdg_popup_destroy(struct wl_listener *listener, void *data);
static void server_new_xdg_popup(struct wl_listener *listener, void *data);
static struct hellwm_toplevel *desktop_toplevel_at(struct hellwm_server *server, double lx, double ly,struct wlr_surface **surface, double *sx, double *sy);

#endif
