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
