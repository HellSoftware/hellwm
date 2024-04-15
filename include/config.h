#ifndef CONFIG_H
#define CONFIG_H
#include <assert.h>
#include <complex.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
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

#include "server.h"

/* 
 * in future hellwm_config_items will be swapped with
 * hellwm_config_group that will contain items for every group,
 * because it will be easier to use with many configuration keywords
 *
 * GROUP=KEYWORD,VALUE
 * bind=SUPER+Return,kitty
*/

const char *hellwm_config_groups_arr[] ={
 "source",
 "bind",
 "monitor",
 "keyboard"
};

enum hellwm_config_groups_types{
    HELLWM_CONFIG_SOURCE=0,
    HELLWM_CONFIG_BIND=1,
    HELLWM_CONFIG_MONITOR=2,
    HELLWM_CONFIG_KEYBOARD=3
};

/* monitor releated section */
typedef struct {
   char *name;
	bool monitor_as;
	float monitor_scale;
	int32_t monitor_transform;
	int32_t monitor_aspect_ratio;
   struct wlr_output_mode *monitor_mode;
} hellwm_config_storage_monitor;

/* keyboard releated section */
typedef struct {
   char *name;
	char **keyboard_binds;
	int keyboard_binds_count;
   int keyboard_repeat_rate;
   int keyboard_repeat_delay;
	struct xkb_rule_names keyboard_rule_names;
} hellwm_config_storage_keyboard;

struct hellwm_config_storage
{
    /* 
     * binds can be same for all keyboards, 
     * but you can specify diffrent binds for specific one //TODO
    */
    hellwm_config_storage_keyboard *keyboards;
    xkb_keysym_t *keyboard_binds_key;
    char **keyboard_binds_content;
    uint32_t keyboard_binds_count;

    /* 
     * for monitor name (e.g: DP-1) set options
     * if not specified it's set to preferred
    */
    hellwm_config_storage_monitor *monitors;
};

typedef struct {
    char key[256];
    char value[256];
} hellwm_config_item;

typedef struct{
    hellwm_config_item *items;
    char *name;
    int count;
} hellwm_config_group;

typedef struct {
    hellwm_config_group *groups;
    int count;
} hellwm_config;

void hellwm_config_apply_to_server(hellwm_config *config, struct hellwm_config_storage *storage);
void hellwm_config_print(hellwm_config *config);
void hellwm_config_setup(hellwm_config *config);
void hellwm_config_load(const char* filename, hellwm_config* config);
int hellwm_config_check_character_in_line(char *line, char character);
const char* hellwm_config_get_value(const hellwm_config* config, const char* key);
hellwm_config_group *hellwm_config_search_in_group_by_name(hellwm_config *config, char*query);

#endif
