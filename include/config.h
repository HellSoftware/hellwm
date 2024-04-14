#ifndef CONFIG_H
#define CONFIG_H

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
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

void hellwm_config_setup(hellwm_config *config);
void hellwm_config_print(hellwm_config *config);
void hellwm_config_load(const char* filename, hellwm_config* config);
int hellwm_config_check_character_in_line(char *line, char character);
const char* hellwm_config_get_value(const hellwm_config* config, const char* key);
hellwm_config_group *hellwm_config_search_in_group_by_name(hellwm_config *config, char*query);

#endif
