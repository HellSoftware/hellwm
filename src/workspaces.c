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

struct hellwm_tile_tree *hellwm_tile_setup(struct wlr_output *output)
{
	struct hellwm_tile_tree *rsetup = malloc(sizeof(struct hellwm_tile_tree));

	if (rsetup == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Failed to allocate memory for root inside hellwm_tile_setup().");
		return NULL;
	}

	rsetup->parent = NULL; /* This one NULL mean that the what ever branch is the root*/
	rsetup->left = NULL;
	rsetup->right = NULL;
	rsetup->x = 0;
	rsetup->y = 0;
	rsetup->direction = 0;
	rsetup->toplevel = NULL;

	rsetup->width = output->width;
	rsetup->height = output->height;

	return rsetup;
}

struct hellwm_tile_tree *hellwm_tile_farthest(struct hellwm_tile_tree *root, bool left, int i)
{
	i++;
	hellwm_log(HELLWM_DEBUG, "Inside hellwm_tile_farthest() - %d", i);
	if (root == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Root is NULL inside hellwm_tile_farthest().");
		return NULL;
	}

	if (left)
	{
		if (root->left == NULL)
		{
			return root;
		}
		return hellwm_tile_farthest(root->left, left, i);
	}
	else
	{
		if (root->right == NULL)
		{
			return root;
		}
		return hellwm_tile_farthest(root->right, left, i);
	}
}

void hellwm_tile_insert_toplevel(struct hellwm_tile_tree *node, struct hellwm_toplevel *new_toplevel, bool left)
{
	struct wlr_output *output = wlr_output_layout_get_center_output(new_toplevel->server->output_layout);
	
	if (node == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Node is NULL inside hellwm_tile_insert_toplevel().");
		return;
	}

	struct hellwm_tile_tree *branch = malloc(sizeof(struct hellwm_tile_tree));

	if (branch == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Failed to allocate memory for branch inside hellwm_tile_insert_toplevel().");
		return;
	}

	int new_width = node->width;
	int new_height = node->height;

	int direction = 0;

	int x = node->x;
	int y = node->y;
	
	if (node->parent != NULL)
	{
		if (node->direction == 0)
		{
			new_width = new_width/2;
			direction = 1;

			x = node->x+new_width;
			y = y;
		}
		else
		{
			new_height = new_height/2;
			direction = 0;

			x = x;
			y = node->y+new_height;
		}
		wlr_xdg_toplevel_set_size(node->toplevel->xdg_toplevel, new_width, new_height);
	}

	branch->parent = node;
	branch->left = NULL;
	branch->right = NULL;
	
	branch->width = new_width;
	branch->height = new_height;

	branch->x = x;
	branch->y = y;
	
	branch->direction = direction;
	branch->toplevel = new_toplevel;

	new_toplevel->tile_node = branch;

	if (left)
	{
		node->left = branch;
	}
	else
	{
		node->right = branch;
	}

	wlr_scene_node_set_position(&branch->toplevel->scene_tree->node, x, y);
	wlr_xdg_toplevel_set_size(new_toplevel->xdg_toplevel, new_width, new_height);

	hellwm_log(HELLWM_DEBUG, "Inserting toplevel to the %p branch at %d %d with %d %d",branch, x, y, new_width, new_height);
}

void hellwm_tile_free_all(struct hellwm_tile_tree *root)
{
	if (root == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Root is NULL inside hellwm_tile_free_all().");
		return;
	}

	if (root->parent != NULL)
	{
		hellwm_log(HELLWM_DEBUG, "Root's parent is not NULL inside hellwm_tile_free_all().");
		return;
	}

	if (root->left != NULL)
		hellwm_tile_free_all(root->left);

	if (root->right != NULL)
		hellwm_tile_free_all(root->right);
}

/* Free node of destroyed toplevel and everything 'bellow' this tree is re-added again */
void hellmw_tile_erase_toplevel(struct hellwm_toplevel *toplevel)
{
	if (toplevel == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Toplevel is NULL inside hellwm_tile_erase_toplevel().");
		return;
	}

	/* TODO - fix this */
	hellwm_tile_free_all(toplevel->server->tile_tree);

	struct hellwm_toplevel *tp;
	wl_list_for_each(tp, &toplevel->server->toplevels,link)
	{
		if (toplevel != tp)
			hellwm_tile_insert_toplevel(toplevel->server->tile_tree, tp, false);
	}
}
