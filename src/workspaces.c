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

struct hellwm_tile_tree **to_erase;
int to_erase_count = 0;

void hellwm_tile_tree_add_to_erase(struct hellwm_tile_tree *root)
{
	if (to_erase == NULL)
	{
		to_erase = (struct hellwm_tile_tree **)malloc(sizeof(struct hellwm_tile_tree *));
	}
	else
	{
		to_erase = (struct hellwm_tile_tree **)realloc(to_erase, sizeof(struct hellwm_tile_tree *) * (to_erase_count + 1));
	}

	if (to_erase == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Failed to allocate mem to add to erase");
		return;
	}

	to_erase[to_erase_count] = root;
	to_erase_count++;
}

void hellwm_tile_erase()
{
	if (to_erase == NULL)
	{
		return;
	}

	for (int i = 0; i < to_erase_count; i++)
	{
		free(to_erase[i]->toplevel);
		
		if (to_erase[i]->left != NULL)
		{
			to_erase[i]->left = to_erase[i]->parent;
			to_erase[i] = to_erase[i]->left;
		}
		else if (to_erase[i]->right != NULL)
		{
			to_erase[i]->right = to_erase[i]->parent;
			to_erase[i] = to_erase[i]->right;
		}
		else
		{
			to_erase[i] = to_erase[i]->parent;
			free(to_erase[i]);
		}
		
		to_erase_count--;
	}
}

struct hellwm_tile_tree* hellwm_tile_tree_node_create(struct hellwm_tile_tree *root, struct hellwm_toplevel *toplevel, bool left)
{
	if (root == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Root node is null");
		return NULL;
	}

	struct hellwm_tile_tree *new_node = malloc(sizeof(struct hellwm_tile_tree *));

  	if (new_node == NULL)
  	{
  	   hellwm_log(HELLWM_ERROR, "Failed to create new node");
  	   return NULL;
  	}

  	new_node->toplevel = toplevel;

	if (root->parent == NULL)
	{
		new_node->x = 0;
		new_node->y = 0;

		new_node->width = root->width;
		new_node->height = root->height;
	}
	else
	{
		if	(left)
		{
			new_node->x = root->width/2;
			new_node->y = root->y;

			new_node->width = root->width/2;
			new_node->height = root->height;
		}
		else
		{
			new_node->x = root->x;
			new_node->y = root->height/2;

			new_node->width = root->width;
			new_node->height = root->height/2;
		}
	}

  	new_node->left = NULL;
  	new_node->right = NULL;

	new_node->parent = root;

	hellwm_log(HELLWM_INFO, "Created new node at %d, %d, with width %d and height %d, and name %s", new_node->x, new_node->y, new_node->width, new_node->height, toplevel->xdg_toplevel->title);

  	return new_node;
}

void hellwm_tile_tree_insert_toplevel(struct hellwm_tile_tree* root, struct hellwm_toplevel* toplevel, bool left)
{
	hellwm_log(HELLWM_LOG, "hellwm_tile_tree_insert_toplevel()");
	if (root == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Root is NULL");
		return;
	}

	if (left)
	{
		if (root->left == NULL)
		{
			root->left = hellwm_tile_tree_node_create(root, toplevel, left);
			return;
		}
		else
		{
			hellwm_tile_tree_insert_toplevel(root->left, toplevel, left);
			return;
		}
	}
	else
	{
		if (root->right == NULL)
		{
			root->right = hellwm_tile_tree_node_create(root, toplevel, left);
			return;
		}
		else
		{
			hellwm_tile_tree_insert_toplevel(root->right, toplevel, left);
			return;
		}
	}
	return;
}

void hellwm_tile_tree_preorderTraversal(struct hellwm_tile_tree *root)
{
	if (root == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Root is NULL");
		return;
	}

	if (root->toplevel == NULL && root->parent != NULL)
	{
		hellwm_log(HELLWM_LOG, "added to erase");
		hellwm_tile_tree_add_to_erase(root);
	}
	else
	{
		wlr_xdg_toplevel_set_size(root->toplevel->xdg_toplevel, root->width, root->height);
		wlr_scene_node_set_position(&root->toplevel->scene_tree->node, root->x, root->y);
	}
	
	hellwm_tile_tree_preorderTraversal(root->left);
	hellwm_tile_tree_preorderTraversal(root->right);
}

void hellwm_tile(struct hellwm_toplevel *hellwm_toplevel)
{
	struct hellwm_server *server = hellwm_toplevel->server;
	struct wlr_output *output = wlr_output_layout_get_center_output(server->output_layout);
	
	struct hellwm_tile_tree *tree = server->tile_tree;

	if (tree == NULL)
	{
		hellwm_log(HELLWM_ERROR, "Tree is NULL");
		return;
	}

	hellwm_tile_tree_insert_toplevel(tree, hellwm_toplevel, false);
}
