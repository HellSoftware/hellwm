#ifndef LAYER_SHELL_H
#define LAYER_SHELL_H
#include <stdbool.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include "./server.h"

static void server_new_layer_surface(struct wl_listener *listener, void *data);

#endif
