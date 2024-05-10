#ifndef LAYER_SHELL_H
#define LAYER_SHELL_H
#include <stdbool.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_layer_shell_v1.h>

static void handle_layer_shell_surface(struct wl_listener *listener, void *data);
static void hellwm_layer_surface_create(struct wlr_scene_layer_surface_v1 *scene, struct hellwm_toplevel *_toplevel);
static struct wlr_scene_tree *hellwm_layer_get_scene(struct hellwm_output *output, enum zwlr_layer_shell_v1_layer type);

#endif
