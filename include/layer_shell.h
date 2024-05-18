#ifndef LAYER_SHELL_H
#define LAYER_SHELL_H
#include <stdbool.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "../include/server.h"

enum { LBg, LBottom, LTile, LFloat, LTop, LFS, LOverlay, NB_OF_LAYERS };

/* zwlr_layer_shell_v1::layer { background, bottom, top, overlay } */
const int layermap[] = { LBg, LBottom, LTop, LOverlay };

/* Client type */
enum { XDGShell, LayerShell, X11 };

struct hellwm_layer_surface
{
  int mapped;
  struct wl_list link;
  struct wlr_box geom;
  unsigned int layer_type;
  struct wlr_scene_tree *scene;
  struct wlr_scene_tree *popups;
  struct hellwm_toplevel *toplevel;
  struct wlr_layer_surface_v1 *layer_surface;
  struct wlr_scene_layer_surface_v1 *scene_layer;
  
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener surface_commit;
};

static void handle_layer_shell_surface(struct wl_listener *listener, void *data);
void layer_surface_commit(struct wl_listener *listener, void *data);
void arrange_layers(struct hellwm_toplevel *toplevel);
void arrangelayer(struct hellwm_toplevel *toplevel, struct wl_list *list, struct wlr_box *usable_area, int exclusive);

#endif
