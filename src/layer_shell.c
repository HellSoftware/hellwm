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

static void handle_layer_shell_surface(struct wl_listener *listener, void *data)
{
	//struct hellwm_server *server = wl_container_of(listener, server, layer_shell);
	struct hellwm_layer_surface *hwm_layer;

	struct wlr_layer_surface_v1 *layer_surface = data;
	struct wlr_surface *surface = layer_surface->surface;
	struct wlr_scene_tree *scene_layer = layers[layermap[layer_surface->pending.layer]];
	struct wlr_layer_surface_v1_state old_state;

	if (!layer_surface->output)
	{
		//layer_surface->output =	wlr_output_layout_get_center_output(server->output_layout);
	}

	hwm_layer = layer_surface->data = calloc(1, sizeof(*hwm_layer)); 
	hwm_layer->layer_type = LayerShell; 

	//wl_signal_add(&surface->events.commit, &hwm_layer->surface_commit);
	//hwm_layer->surface_commit.notify = layer_surface_commit;
}


void layer_surface_commit(struct wl_listener *listener, void *data)
{
	struct hellwm_layer_surface *hwm_layer = wl_container_of(listener, hwm_layer, surface_commit); 
	struct wlr_layer_surface_v1 *layer_surface = hwm_layer->layer_surface; 

	struct wlr_scene_tree *scene_layer = layers[layermap[layer_surface->current.layer]];

	if (layer_surface->current.committed == 0 && hwm_layer->mapped == layer_surface->surface->mapped)
	{
		return;
	}

	hwm_layer->mapped = layer_surface->surface->mapped;

	if (scene_layer != hwm_layer->scene->node.parent)
	{
		wlr_scene_node_reparent(&hwm_layer->scene->node, scene_layer);
		wl_list_remove(&hwm_layer->link);
		wl_list_insert(&hwm_layer->toplevel->layers[layer_surface->current.layer], &hwm_layer->link);
		wlr_scene_node_reparent(&hwm_layer->popups->node, (layer_surface->current.layer < ZWLR_LAYER_SHELL_V1_LAYER_TOP ? layers[LTop] : scene_layer));
	}

	arrange_layers(hwm_layer->toplevel);
}

void arrange_layers(struct hellwm_toplevel *toplevel)
{
	int i;
	struct wlr_box usable_area;
	usable_area.width = toplevel->xdg_toplevel->current.width;
	usable_area.height = toplevel->xdg_toplevel->current.height;

	usable_area.x = toplevel->scene->tree->node.x;
	usable_area.y = toplevel->scene->tree->node.y;

	struct hellwm_layer_surface *hwm_surface;

	uint32_t layers_above_shell[] =
	{
		ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
		ZWLR_LAYER_SHELL_V1_LAYER_TOP,
	};

	if (!toplevel->output->wlr_output->enabled)
	{
		return;
	}
	
	for (i = 3; i >= 0; i--)
	{
		arrangelayer(toplevel, &toplevel->layers[i], &usable_area, 1);
	}

	for (i = 3; i >= 0; i--)
	{
		arrangelayer(toplevel, &toplevel->layers[i], &usable_area, 0);
	}

	focus_toplevel(toplevel, hwm_surface->layer_surface->surface);
}
void arrangelayer(struct hellwm_toplevel *toplevel, struct wl_list *list, struct wlr_box *usable_area, int exclusive)
{
	struct hellwm_layer_surface *hwm_layer_surface;
	struct wlr_box full_area;
	full_area.width = toplevel->output->wlr_output->width;
	full_area.height = toplevel->output->wlr_output->height;
	full_area.x = 0;
	full_area.y = 0;

	wl_list_for_each(hwm_layer_surface, list, link)
	{
		struct wlr_layer_surface_v1 *layer_surface = hwm_layer_surface->layer_surface;

		if (exclusive != (layer_surface->current.exclusive_zone > 0))
			continue;

		wlr_scene_layer_surface_v1_configure(hwm_layer_surface->scene_layer,
				&full_area, usable_area
				);

		wlr_scene_node_set_position(
				&hwm_layer_surface->popups->node,
				hwm_layer_surface->scene->node.x,
				hwm_layer_surface->scene->node.y
				);

		hwm_layer_surface->geom.x = hwm_layer_surface->scene->node.x;
		hwm_layer_surface->geom.y = hwm_layer_surface->scene->node.y;
	}
}
