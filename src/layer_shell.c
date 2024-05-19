#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <GLES2/gl2.h>
#include <wayland-egl.h>
#include <wayland-util.h>
#include <wlr/util/log.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-server-core.h>
#include <linux/input-event-codes.h>
#include <wlr/types/wlr_output_layout.h>

#ifdef XWAYLAND
#include <wlr/xwayland/shell.h>
#include <wlr/xwayland/xwayland.h>
#endif

#include "../include/server.h"
#include "../include/layer_shell.h"
#include "../wlr-layer-shell-unstable-v1-protocol.h"

struct wlr_scene_tree *layers[7];

void hellwm_layer_shell_init()
{
	for (int i = 0; i < 7; i++)
		layers[i] = malloc(sizeof(struct wlr_scene_tree));
}

static void handle_layer_shell_surface(struct wl_listener *listener, void *data)
{
	struct hellwm_server *server = wl_container_of(listener, server, layer_shell);
	struct hellwm_layer_surface *hwm_layer;

	struct wlr_layer_surface_v1 *layer_surface = data;
	struct wlr_surface *surface = layer_surface->surface;
	struct wlr_scene_tree *scene_layer = layers[layermap[layer_surface->pending.layer]];
	struct wlr_layer_surface_v1_state old_state;

	hellwm_log(HELLWM_LOG, "new layer surface: namespace %s layer %d anchor %" PRIu32
			" size %" PRIu32 "x%" PRIu32 " margin %" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",",
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
		layer_surface->output =	wlr_output_layout_get_center_output(server->output_layout);
	}

	enum zwlr_layer_shell_v1_layer layer_type = layer_surface->pending.layer;
	struct wlr_scene_tree *output_layer = hwm_layer->scene;

	struct wlr_scene_layer_surface_v1 *scene_surface =
		wlr_scene_layer_surface_v1_create(output_layer, layer_surface);
	if (!scene_surface) {
		hellwm_log(HELLWM_ERROR, "Could not allocate a layer_surface_v1");
		return;
	}

	struct hellwm_toplevel *toplevel = calloc(1, sizeof(*toplevel));

	if (!toplevel) {
		hellwm_log(HELLWM_ERROR, "Could not allocate a toplevel");
		return;
	}

	toplevel->scene = scene_surface;
	toplevel->layer_surface = layer_surface;
	toplevel->layer_surface = layer_surface;

	toplevel->output->wlr_output = layer_surface->output;

	hwm_layer->surface_commit.notify = layer_surface_commit;
	wl_signal_add(&surface->events.commit, &hwm_layer->surface_commit);
	
	hwm_layer->map.notify = layer_surface_map;
	wl_signal_add(&surface->events.map, &hwm_layer->map);
}


void layer_surface_commit(struct wl_listener *listener, void *data)
{
	struct hellwm_layer_surface *surface= wl_container_of(listener, surface, surface_commit);

	struct wlr_layer_surface_v1 *layer_surface = surface->layer_surface;
	if (!layer_surface->initialized)
	{
		return;
	}

	uint32_t committed = layer_surface->current.committed;
	
	if (committed & WLR_LAYER_SURFACE_V1_STATE_LAYER)
	{
		enum zwlr_layer_shell_v1_layer layer_type = layer_surface->current.layer;
		struct wlr_scene_tree *output_layer = surface->toplevel->scene_tree;
		wlr_scene_node_reparent(&surface->scene->node, output_layer);
	}

	if (layer_surface->initial_commit || committed || layer_surface->surface->mapped != surface->mapped)
	{
		surface->mapped = layer_surface->surface->mapped;
		arrange_layers(surface->toplevel->output, surface);
	}	
}

void arrange_layers(struct hellwm_output *output, struct hellwm_layer_surface *hwm_layer)
{
	struct wlr_box usable_area = { 0 };
	wlr_output_effective_resolution(output->wlr_output,
			&usable_area.width, &usable_area.height);
	const struct wlr_box full_area = usable_area;

	for (int i = 0; i < 7; i++)
	{
		wlr_scene_layer_surface_v1_configure(hwm_layer->scene_layer, &full_area, &usable_area);
	}
}

void layer_surface_map(struct wl_listener *listener, void *data)
{
	struct hellwm_layer_surface *hwm_layer = wl_container_of(listener, hwm_layer, map);
	struct wlr_layer_surface_v1 *layer_surface = hwm_layer->layer_surface;

	if (hwm_layer->layer_surface->current.keyboard_interactive)
	{
		
	}
}
