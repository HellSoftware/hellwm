#ifndef HELLWM_CONFIG_H 
#define HELLWM_CONFIG_H 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void hellwm_config_kbind_give();

int hellwm_config_parse_line(char *line);

void hellwm_config_load();

#endif
