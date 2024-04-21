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
#include "../wlr-layer-shell-unstable-v1-protocol.h"

#include "../include/layer_shell.h"
#include "../include/server.h"

static void
server_new_layer_surface(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, new_layer_surface);
   struct wlr_layer_surface_v1 *layer_surface = data;
	
	hellwm_log(HELLWM_LOG, "\n\nnew layer surface: namespace %s layer %d anchor %" PRIu32
			" size %" PRIu32 "x%" PRIu32 " margin %" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",\n",
		layer_surface->namespace,
		layer_surface->pending.layer,
		layer_surface->pending.anchor,
		layer_surface->pending.desired_width,
		layer_surface->pending.desired_height,
		layer_surface->pending.margin.top,
		layer_surface->pending.margin.right,
		layer_surface->pending.margin.bottom,
		layer_surface->pending.margin.left);

	if (!layer_surface->output)
	{
		hellwm_log(HELLWM_LOG,"Layer surface not specified output, choosing by deafult");
		layer_surface->output = wlr_output_layout_get_center_output(server->output_layout); 
	}

	struct hellwm_output *output = layer_surface->output->data;	
	//struct wlr_scene_tree *output_layer = hellwm_layer_get_scene(output,layer_surface->current.layer);
	struct wlr_scene_tree *output_layer = &server->scene->tree;

	struct wlr_scene_layer_surface_v1 *scene_surface = wlr_scene_layer_surface_v1_create(output_layer,layer_surface);
	if (!scene_surface)
	{
		hellwm_log(HELLWM_ERROR, "failed to allocate wlr_scene_layer_surface");
	}
	
	struct hellwm_toplevel *toplevel = hellwm_layer_surface_create(scene_surface);
	if (!toplevel)
	{
		wlr_layer_surface_v1_destroy(layer_surface);
		hellwm_log(HELLWM_ERROR, "failed to allocate toplevel");
	}

	toplevel->output = output;

	toplevel->commit.notify = xdg_toplevel_commit;
	wl_signal_add(
			&layer_surface->surface->events.commit,
			&toplevel->commit);

	toplevel->map.notify = xdg_toplevel_map;
	wl_signal_add(
			&layer_surface->surface->events.map,
			&toplevel->map);

	toplevel->unmap.notify = xdg_toplevel_unmap;
	wl_signal_add(
			&layer_surface->surface->events.unmap,
			&toplevel->unmap
			);

	/* TODO: POPUPS */
}

static struct 
hellwm_toplevel *hellwm_layer_surface_create(struct wlr_scene_layer_surface_v1 *scene)
{
	struct hellwm_toplevel *toplevel = calloc(1, sizeof(*toplevel));
	if (!toplevel) {
		hellwm_log(HELLWM_ERROR, "calloc() toplevel");
		return NULL;
	}

	toplevel->scene_tree = scene->tree;
	toplevel->scene = scene;
	toplevel->layer_surface = scene->layer_surface;
	toplevel->layer_surface->data = toplevel;

	return toplevel;
}

static struct
wlr_scene_tree *hellwm_layer_get_scene(
		struct hellwm_output *output,
		enum zwlr_layer_shell_v1_layer type)
{
	switch (type) {
	case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
		return output->layers.shell_background;
	case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
		return output->layers.shell_bottom;
	case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
		return output->layers.shell_top;
	case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
		return output->layers.shell_overlay;
	}

	hellwm_log(HELLWM_ERROR, "Unknown layer shell type");
	return NULL;
}
