
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

#ifdef XWAYLAND
#include <wlr/xwayland/shell.h>
#include <wlr/xwayland/xwayland.h>
#endif

#include "../include/server.h"

#if XWAYLAND

static void server_handle_xwayland_surface(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, new_xwayland_surface);
   struct wlr_xwayland_surface *surface = data;

	hellwm_log(HELLWM_LOG, "New xwayland surface: title=%s, class=%s, instance=%s",
		surface->title, surface->class, surface->instance);
	wlr_xwayland_surface_ping(surface);

	struct hellwm_xwayland_surface *hellwm_surface =
		calloc(1, sizeof(struct hellwm_xwayland_surface));
	if (hellwm_surface == NULL) {
		return;
	}/*

	view_init(&hellwm_surface->view, &view_impl, HELLWM_XWAYLAND_VIEW, server);
	hellwm_surface->view.box.x = surface->x;
	hellwm_surface->view.box.y = surface->y;
	hellwm_surface->xwayland_surface = surface;

	view_set_title(&hellwm_surface->view, surface->title);
	view_set_app_id(&hellwm_surface->view, surface->class);

	hellwm_surface->destroy.notify = xwayland_handle_destroy;
	wl_signal_add(&surface->events.destroy, &hellwm_surface->destroy);

	hellwm_surface->map.notify = handle_map;
	wl_signal_add(&surface->events.map, &hellwm_surface->map);

	hellwm_surface->unmap.notify = handle_unmap;
	wl_signal_add(&surface->events.unmap, &hellwm_surface->unmap);
	
	hellwm_surface->request_move.notify = handle_request_move;
	wl_signal_add(&surface->events.request_move, &hellwm_surface->request_move);
	
	hellwm_surface->request_resize.notify = handle_request_resize;

	wl_signal_add(&surface->events.request_resize,
		&hellwm_surface->request_resize);
	hellwm_surface->request_maximize.notify = handle_request_maximize;
	
	wl_signal_add(&surface->events.request_maximize,
		&hellwm_surface->request_maximize);
	hellwm_surface->request_fullscreen.notify = handle_request_fullscreen;
	
	wl_signal_add(&surface->events.request_fullscreen,
		&hellwm_surface->request_fullscreen);
	hellwm_surface->set_title.notify = handle_set_title;
	
	wl_signal_add(&surface->events.set_title, &hellwm_surface->set_title);
	hellwm_surface->set_class.notify = handle_set_class;
	
	wl_signal_add(&surface->events.set_class,
			&hellwm_surface->set_class);
	*/
}

/*
static void xwayland_handle_destroy(struct wl_listener *listener, void *data)
{
	struct hellwm_xwayland_surface *hellwm_surface =
		wl_container_of(listener, hellwm_surface, destroy);
	view_destroy(&hellwm_surface->view);
}
*/

static void server_xwayland_ready(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, xwayland_ready);
   struct wlr_seat *wlr_seat = server->seat; 

   wlr_xwayland_set_seat(server->xwayland, wlr_seat);
   hellwm_log(HELLWM_INFO, "XWayland is ready");
}
#endif 
