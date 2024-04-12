#ifndef REPEATABLE_FUNCTIONS_H
#define REPEATABLE_FUNCTIONS_H

#include "config.h"
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

int addElement(char*** arr, int* size, const char* newElement);
int add_config_group(hellwm_config_group** arr, int* size, const hellwm_config_group* newElement);
#endif
