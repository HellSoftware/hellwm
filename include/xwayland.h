#ifndef SWAY_XWAYLAND_H
#define SWAY_XWAYLAND_H

#include <linux/input-event-codes.h>
#include <assert.h>
#include <GLES2/gl2.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/util/log.h>
#include <xcb/xcb_icccm.h>

#include <wlr/xwayland/shell.h>
#include <wlr/xwayland/xwayland.h>

#include "../include/server.h"

enum atom_name
{
	NET_WM_WINDOW_TYPE_NORMAL,
	NET_WM_WINDOW_TYPE_DIALOG,
	NET_WM_WINDOW_TYPE_UTILITY,
	NET_WM_WINDOW_TYPE_TOOLBAR,
	NET_WM_WINDOW_TYPE_SPLASH,
	NET_WM_WINDOW_TYPE_MENU,
	NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
	NET_WM_WINDOW_TYPE_POPUP_MENU,
	NET_WM_WINDOW_TYPE_TOOLTIP,
	NET_WM_WINDOW_TYPE_NOTIFICATION,
	NET_WM_STATE_MODAL,
	ATOM_LAST,
};

struct hellwm_xwayland_toplevel
{
	struct hellwm_server *server;
	struct wlr_xwayland_surface *xwayland_surface;
	struct wl_list link;

	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_listener associate;
	struct wl_listener request_role;
	struct wl_listener request_move;
	struct wl_listener request_title;
	struct wl_listener request_app_id;
	struct wl_listener request_resize;
	struct wl_listener request_activate;
	struct wl_listener request_maximize;
	struct wl_listener request_geometry;
	struct wl_listener request_configure;
	struct wl_listener request_fullscreen;
};

static void xhandle_map(struct wl_listener *listener, void *  data);
static void xhandle_unmap(struct wl_listener *listener, void * data);
static void xhandle_destroy(struct wl_listener *listener, void *data);
static void xhandle_set_role(struct wl_listener *listener, void *data);
static void xhandle_associate(struct wl_listener *listener, void *data);
static void xhandle_set_title(struct wl_listener *listener,  void *data);
static void xhandle_configure(struct wl_listener *listener,   void *data);
static void xhandle_set_app_id(struct wl_listener *listener,   void *data);
static void xhandle_set_geometry(struct wl_listener *listener, void * data);
static void handle_surface_commit(struct wl_listener *listener ,void * data);
static void server_xwayland_ready(struct wl_listener *listener, void  * data);
static void xhandle_request_activate(struct wl_listener *listener, void *data);
static void xhandle_request_maximize(struct wl_listener *listener, void  *data);
static void server_handle_xwayland_surface(struct wl_listener*listener,void*data);

#endif
