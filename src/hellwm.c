#include <lua.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <getopt.h>
#include <lualib.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <complex.h>
#include <lauxlib.h>
#include <pthread.h>
#include <stdbool.h>
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

#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include "../include/xwayland.h"
#include "../src/xwayland.c"

#endif

#include "../include/server.h"
#include "../include/config.h"

// problem with linking :/
#include "./server.c"
#include "./config.c"

#define cmp(x) strcmp(argv[i], x) == 0

char * configPath = "~/.config/hellwm/config.lua";

void hellwm_print_usage(int argc, char*argv[]);

int main(int argc, char *argv[])
{
	hellwm_print_usage(argc, argv);
	hellwm_log_flush();

	struct hellwm_server server = {NULL};

	server.configPath = configPath;

	/* Lua setup */
	hellwm_config_setup(&server);
	
	/* Setup all necessary stuff for running server */
	hellwm_setup(&server);

	/* Start Wayland Compositor */
	hellwm_log(HELLWM_INFO,"Started HellWM Wayland Session at %s", server.socket);
	wl_display_run(server.wl_display);

	hellwm_log(HELLWM_INFO, "Closed HellWM Wayland Session");

	hellwm_destroy_everything(&server);

	return 0;
}

void hellwm_print_usage(int argc, char*argv[])
{
	for (int i = 0; i < argc; i++)
	{
		if (cmp("-c"))
		{
			if (i+1 >= argc)
			{
				fprintf(stderr, "Usage: %s no argument provided", argv[0]);
				exit(1);
			}
			configPath = argv[i+1];
			i++;
		}
	}
}
