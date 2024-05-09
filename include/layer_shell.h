#ifndef LAYER_SHELL_H
#define LAYER_SHELL_H
#include <stdbool.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#ifdef XWAYLAND
#include <wlr/xwayland/shell.h>
#include <wlr/xwayland/xwayland.h>
#endif

static void server_new_layer_surface(struct wl_listener *listener, void *data);
static struct hellwm_toplevel *hellwm_layer_surface_create(struct wlr_scene_layer_surface_v1 *scene);
static struct wlr_scene_tree *hellwm_layer_get_scene(struct hellwm_output *output, enum zwlr_layer_shell_v1_layer type);

#if XWAYLAND
static void server_new_xwayland_surface(struct wl_listener *listener, void *data);
static void server_xwayland_ready(struct wl_listener *listener, void *data);
#endif 

#endif
