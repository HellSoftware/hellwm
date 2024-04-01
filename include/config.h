#ifndef CONFIG_H
#define CONFIG_H

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

typedef struct {
    char key[256];
    char value[256];
} hellwm_config_item;

typedef struct {
    hellwm_config_item *items;
    int count;
} hellwm_config_group;

/* 
 * in future hellwm_config_items will be swapped with
 * hellwm_config_group that will contain items for every group,
 * because it will be easier to use with many configuration keywords
 *
 * GROUP = KEYWORD, VALUE
 * bind = SUPER+Return, exec kitty
*/
typedef struct {
    hellwm_config_item* items;
    hellwm_config_group* groups; 
    int count;
} hellwm_config;

void hellwm_config_load(const char* filename, hellwm_config* config);

const char* hellwm_config_get_value(const hellwm_config* config, const char* key);

#endif
