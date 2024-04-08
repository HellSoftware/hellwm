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

static void
server_new_layer_surface(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, new_layer_surface);
   struct wlr_layer_surface_v1 *layer_surface = data;
	hellwm_log(HELLWM_LOG, "New layer surface with namespace: %s", layer_surface->namespace);
;	
	hellwm_log(
			HELLWM_LOG,
			"Current Awidth = %u, Aheight = %u, Dwidth = %u, Dheight = %u",
			layer_surface->current.actual_width,
			layer_surface->current.actual_height,
			layer_surface->current.desired_width,
			layer_surface->current.desired_height
			);
	
	if (!layer_surface->output)
	{
		hellwm_log(HELLWM_LOG,"Layer surface not specified output, choosing by deafult");
		return;
	}
	//wlr_layer_surface_v1_configure(layer_surface, 500,500);
}
