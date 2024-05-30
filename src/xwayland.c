#include <linux/input-event-codes.h>
#include <GLES2/gl2.h>
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
#include "../include/xwayland.h"

static const char *atom_map[ATOM_LAST] = {
	[NET_WM_WINDOW_TYPE_NORMAL] = "_NET_WM_WINDOW_TYPE_NORMAL",
	[NET_WM_WINDOW_TYPE_DIALOG] = "_NET_WM_WINDOW_TYPE_DIALOG",
	[NET_WM_WINDOW_TYPE_UTILITY] = "_NET_WM_WINDOW_TYPE_UTILITY",
	[NET_WM_WINDOW_TYPE_TOOLBAR] = "_NET_WM_WINDOW_TYPE_TOOLBAR",
	[NET_WM_WINDOW_TYPE_SPLASH] = "_NET_WM_WINDOW_TYPE_SPLASH",
	[NET_WM_WINDOW_TYPE_MENU] = "_NET_WM_WINDOW_TYPE_MENU",
	[NET_WM_WINDOW_TYPE_DROPDOWN_MENU] = "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
	[NET_WM_WINDOW_TYPE_POPUP_MENU] = "_NET_WM_WINDOW_TYPE_POPUP_MENU",
	[NET_WM_WINDOW_TYPE_TOOLTIP] = "_NET_WM_WINDOW_TYPE_TOOLTIP",
	[NET_WM_WINDOW_TYPE_NOTIFICATION] = "_NET_WM_WINDOW_TYPE_NOTIFICATION",
	[NET_WM_STATE_MODAL] = "_NET_WM_STATE_MODAL",
};

static void server_handle_xwayland_surface(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, new_xwayland_surface);
   struct wlr_xwayland_surface *xwayland_surface = data;	

	wlr_xwayland_surface_ping(xwayland_surface);

   struct hellwm_xwayland_toplevel *xtoplevel = calloc(1, sizeof(*xtoplevel));
   
	xtoplevel->server = server;
	xtoplevel->xwayland_surface = xwayland_surface;

   xwayland_surface->data = xtoplevel;

   wl_signal_add(&xwayland_surface->events.associate, &xtoplevel->associate);
   xtoplevel->associate.notify = xhandle_associate;

   wl_signal_add(&xwayland_surface->events.destroy, &xtoplevel->destroy);
   xtoplevel->destroy.notify = xhandle_destroy;

   wl_signal_add(&xwayland_surface->events.request_configure, &xtoplevel->request_configure);
   xtoplevel->request_configure.notify = xhandle_configure;
   
   wl_signal_add(&xwayland_surface->events.set_title, &xtoplevel->request_title);
   xtoplevel->request_title.notify = xhandle_set_title;

   wl_signal_add(&xwayland_surface->events.set_class, &xtoplevel->request_app_id);
   xtoplevel->request_app_id.notify = xhandle_set_app_id;

   wl_signal_add(&xwayland_surface->events.set_role, &xtoplevel->request_role);
   xtoplevel->request_role.notify = xhandle_set_role;

   wl_signal_add(&xwayland_surface->events.set_geometry, &xtoplevel->request_geometry);
   xtoplevel->request_geometry.notify = xhandle_set_geometry;

   wl_signal_add(&xwayland_surface->events.request_activate, &xtoplevel->request_activate);
   xtoplevel->request_activate.notify = xhandle_request_activate;

   wl_signal_add(&xwayland_surface->events.request_maximize, &xtoplevel->request_maximize);
   xtoplevel->request_maximize.notify = xhandle_request_maximize;
}

static void xhandle_request_maximize(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, request_maximize);
   // TODO
   return;
}

static void xhandle_request_activate(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, request_activate);
   wlr_xwayland_surface_activate(xtoplevel->xwayland_surface, true);
}

static void xhandle_set_geometry(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, request_geometry);
   int x = xtoplevel->xwayland_surface->x;
   int y = xtoplevel->xwayland_surface->y;
   int width = xtoplevel->xwayland_surface->width;
   int height = xtoplevel->xwayland_surface->height;

   wlr_xwayland_surface_configure(xtoplevel->xwayland_surface, x, y, width, height);

   hellwm_log(HELLWM_INFO, "xwayland set geometry: %dx%d - %d, %d", width, height, x, y);
}

static void xhandle_set_role(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, request_role);
   const char *role = xtoplevel->xwayland_surface->role;
   hellwm_log(HELLWM_INFO, "xwayland set role: %s", role);
}

static void handle_surface_commit(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, commit);

   int new_width = xtoplevel->xwayland_surface->width;
   int new_height = xtoplevel->xwayland_surface->height;
   int new_x = xtoplevel->xwayland_surface->x;
   int new_y = xtoplevel->xwayland_surface->y;

   hellwm_log(HELLWM_LOG, "xwayland surface commit: %dx%d - %d, %d", new_width, new_height, new_x, new_y);
}

static void xhandle_associate(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, associate);
	struct wlr_xwayland_surface *xsurface = xtoplevel->xwayland_surface;

	wl_signal_add(&xsurface->surface->events.unmap, &xtoplevel->unmap);
	xtoplevel->unmap.notify = xhandle_unmap;

	wl_signal_add(&xsurface->events.map_request, &xtoplevel->map);
	xtoplevel->map.notify = xhandle_map;
   
   wlr_xwayland_surface_configure(xsurface, 0, 0, 0, 0);
}

static void xhandle_set_app_id(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, request_app_id);
   const char *app_id = xtoplevel->xwayland_surface->class;
   hellwm_log(HELLWM_INFO, "xwayland set app_id: %s", app_id);
}

static void xhandle_set_title(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, request_title);
   const char *title = xtoplevel->xwayland_surface->title;
   hellwm_log(HELLWM_INFO, "xwayland set title: %s", title);
}

static void xhandle_configure(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, request_configure);
   wlr_xwayland_surface_configure(xtoplevel->xwayland_surface, 0, 0, xtoplevel->xwayland_surface->width, xtoplevel->xwayland_surface->height);
}

static void xhandle_map(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, map);
   struct wlr_xwayland_surface *xwayland_surface = xtoplevel->xwayland_surface;
   wl_list_insert(&xtoplevel->server->xtoplevels, &xtoplevel->link);

   wl_signal_add(&xwayland_surface->surface->events.commit, &xtoplevel->commit);
   xtoplevel->commit.notify = handle_surface_commit;
}

static void xhandle_unmap(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xwayland_toplevel = wl_container_of(listener, xwayland_toplevel, unmap);

   wl_list_remove(&xwayland_toplevel->link);
}

static void xhandle_destroy(struct wl_listener *listener, void *data)
{
   struct hellwm_xwayland_toplevel *xtoplevel = wl_container_of(listener, xtoplevel, destroy);
   wl_list_remove(&xtoplevel->link);
   free(xtoplevel);
}

static void server_xwayland_ready(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, xwayland_ready);
   struct wlr_seat *wlr_seat = server->seat; 

   wlr_xwayland_set_seat(server->xwayland, wlr_seat);

   xcb_connection_t *xcb_conn = xcb_connect(NULL, NULL);
   int result = xcb_connection_has_error(xcb_conn);
	if (result)
   {
      hellwm_log(HELLWM_ERROR, "XCB failed to connect: %d", result);
		return;
	}

   xcb_intern_atom_cookie_t cookies[ATOM_LAST];
	for (size_t i = 0; i < ATOM_LAST; i++)
   {
		cookies[i] =
			xcb_intern_atom(xcb_conn, 0, strlen(atom_map[i]), atom_map[i]);
	}
	for (size_t i = 0; i < ATOM_LAST; i++)
   {
		xcb_generic_error_t *error = NULL;
		xcb_intern_atom_reply_t *reply =
			xcb_intern_atom_reply(xcb_conn, cookies[i], &error);
		if (reply != NULL && error == NULL) {
			server->atoms[i] = reply->atom;
		}
		free(reply);

		if (error != NULL)
      {
			hellwm_log(
              HELLWM_LOG,
               "error atom %s, X11 error code %d",
               atom_map[i],
               error->error_code);
			free(error);
			break;
		}
	}

  	xcb_disconnect(xcb_conn);

   hellwm_log(HELLWM_INFO, "XWayland is ready");
}
