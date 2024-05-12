#include <lua.h>
#include <time.h>
#include <stdio.h>
#include <wayland-cursor.h>
#include <wayland-util.h>
#include <wchar.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>
#include <pthread.h>
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
#include <wayland-server-protocol.h>
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

#ifdef XWAYLAND
#include <xcb/xcb.h>
#include <wlr/xwayland.h>
#include <xcb/xcb_icccm.h>
#endif

#include "../include/server.h"

/* TODO - add workspaces, !layout (reminder) */

void hellwm_tile(struct hellwm_toplevel *toplevel)
{
	struct hellwm_server *server = toplevel->server;
	struct wlr_output *output = wlr_output_layout_get_center_output(server->output_layout);
	int output_width = output->width;
	int output_height = output->height;
	
	/*	TODO: add borders */
	int i=0;
	int length = wl_list_length(&server->toplevels);

	wl_list_for_each(toplevel, &server->toplevels, link)
	{
		wlr_scene_node_set_position(&toplevel->scene_tree->node, i*output_width/length, 0);
		wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, output_width/length, output_height);

		i++;
	}
}
