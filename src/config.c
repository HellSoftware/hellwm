#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    int linePosition=0;
    while (fgets(line, sizeof(line), file)) 
    {
        linePosition++;
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char* group = strtok(line, "=");
        char* item = strtok(NULL, "\n");

        hellwm_log(HELLWM_LOG, "LINE: %d \t group: %s \t item: %s", linePosition, group, item);

        if (!strcmp(group,"source"))
        {
            hellwm_config_load(item, config);
            continue;
        }
        
        if (group && item)
        {
            hellwm_config_group* temp_group = hellwm_config_search_in_group_by_name(config, group);
            if (temp_group==NULL)
            {
                config->groups = realloc(config->groups, (config->count + 1) * sizeof(hellwm_config_group));
                config->count++;
                config->groups[config->count-1].name=group;
            }
            temp_group = hellwm_config_search_in_group_by_name(config, group);
           
            if (temp_group==NULL)
            {
                hellwm_log(HELLWM_ERROR, "Cannot find group. Line %d, File%s",linePosition,filename);
                continue;
            }
            
            char* key = strtok(item, ",");
            char* value;// = strtok(item, "\n");
            memcpy(value,line+strlen(key),strlen(line)-strlen(key));
            
            hellwm_log("HELLWM_LOG", "LINE: %d \t key: %s \t value:%s", linePosition,key,value);

            if (key && value)
            {
                temp_group->items = realloc(temp_group->items, (temp_group->count + 1) * sizeof(hellwm_config_item));

                strncpy(temp_group->items[temp_group->count].key, key, sizeof(temp_group->items[temp_group->count].key));

                strncpy(temp_group->items[temp_group->count].value, value, sizeof(temp_group->items[temp_group->count].value));
                temp_group->count++;
            }
         }
    }
    fclose(file);
}

hellwm_config_group *hellwm_config_search_in_group_by_name(hellwm_config *config, char*query)
{
    for (int i=0;i<config->count;i++)
    {
        if (config->groups[i].name==query)
        {
            return &config->groups[i];
        }
    }
    return NULL;
}

int hellwm_config_check_character_in_line(char *line, char character)
{
    for (int i=0;i<strlen(line);i++)
    {
        if (line[i]==character)
        {
            return i;
        }
    }
    return -1;
}

void hellwm_config_apply(hellwm_config *config, struct hellwm_server *server)
{
    
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
