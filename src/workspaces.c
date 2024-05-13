#include <iso646.h>
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
/*	TODO: add borders */

struct hellwm_toplevel *prev = NULL;

/* TODO - ADD IT TO TREE */
void spiralTiling(struct hellwm_toplevel *new_toplevel)
{
	struct hellwm_server *server = new_toplevel->server;

   int direction = 0;
	
	struct wlr_output *output = wlr_output_layout_get_center_output(server->output_layout);
	int sWidth = output->width;
	int sHeight = output->height;

	int width = sWidth, height = sHeight;

	int n = wl_list_length(&server->toplevels);
	int length = wl_list_length(&server->toplevels);

	int x=0, y=0; 
	for (int i = 0; i < n+1; i++)
	{
		x = sWidth-width;
		y = sHeight-height;
	
		
		if (direction == 0)
		{
			width = width/2;
			direction = 1;
		}
		else
		{
			height = height/2;
			direction = 0;
		}
	}
		
	hellwm_log(HELLWM_LOG, "%d - Tiling: %d %d %d %d", n , x, y, width, height);
	wlr_scene_node_set_position(&new_toplevel->scene_tree->node, x, y);
	wlr_xdg_toplevel_set_size(new_toplevel->xdg_toplevel,  width,  height);

	hellwm_log("", "");
}
