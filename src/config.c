#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include "../include/server.h"
#include "../include/config.h"

void hellwm_config_load(const char* filename, hellwm_config* config)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL)
    {
        hellwm_log(HELLWM_ERROR, "Failed to open HellWM configuration file: %s\n", filename);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");
        
        if (!strcmp(key,"source"))
        {
            hellwm_config_load(value, config);
            continue;
        }
        
        if (key && value)
        {
            config->items = realloc(config->items, (config->count + 1) * sizeof(hellwm_config_item));
            strncpy(config->items[config->count].key, key, sizeof(config->items[config->count].key));
            strncpy(config->items[config->count].value, value, sizeof(config->items[config->count].value));
            config->count++;
        }
    }
    fclose(file);
}

const char* hellwm_config_get_value(const hellwm_config* config, const char* key)
{
    for (int i = 0; i < config->count; i++) {
        if (strcmp(config->items[i].key, key) == 0) {
            printf("\n%s - %s",key,config->items[i].value);
            //return hellwm_config->items[i].value;
        }
    }
    return NULL;
}

/*int main()
{
    hellwm_config hellwm_config = {NULL, 0};
    hellwm_config_load("config.conf", &hellwm_config);

    for (int i=0;i<hellwm_config.count;i++)
    {
        printf("%s: %s\n",hellwm_config.items[i].key,hellwm_config.items[i].value);
    }

    free(hellwm_config.items);
    return 0;
}*/
