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
#include <wlr/util/log.h>
#include "../wlr-layer-shell-unstable-v1-protocol.h"

#include "../include/layer_shell.h"
#include "../include/server.h"
#include "../include/config.h"

static void
server_new_layer_surface(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, new_layer_surface);
   struct wlr_layer_surface_v1 *layer_surface = data;
	hellwm_log("New layer surface with namespace ", layer_surface->namespace);
}
