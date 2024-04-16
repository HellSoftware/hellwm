#include <assert.h>
#include <complex.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wchar.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>
#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#endif

#include "../include/server.h"
#include "../include/config.h"
#include "../include/layer_shell.h"

// problem with linking :/
#include "./server.c"
#include "./layer_shell.c"
#include "./config.c"

void hellwm_print_usage(int *argc, char**argv[])
{
	// TODO add usage options under -h
}

int main(int argc, char *argv[])
{
	hellwm_print_usage(&argc, &argv);

	hellwm_log_flush();

	struct hellwm_server server = {NULL};

	hellwm_setup(&server);

	hellwm_log(HELLWM_INFO,"Started HellWM Wayland Session at %s", server.socket);
	wl_display_run(server.wl_display);
	hellwm_log(HELLWM_INFO, "Close HellWM Wayland Session");

	hellwm_destroy_everything(&server);

	return 0;
}
