#include <complex.h>
#include <stddef.h>
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
        char linecopy[strlen(line)];
        strcpy(linecopy, line);

        linePosition++;
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char* group = strtok(line, "=");
        char* item = strtok(NULL, "\n");

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
            int idx = hellwm_config_check_character_in_line(linecopy,',');
            if (idx==-1)
            {
                hellwm_log(HELLWM_ERROR, "config parse error in file: %s, at line %d", filename,linePosition);
                continue;
            }
            
            char value[strlen(linecopy)-idx+1];

            memcpy(value,linecopy+idx+1,strlen(linecopy)-idx);

            if (key)
            {
                temp_group->items = realloc(temp_group->items, (temp_group->count + 1) * sizeof(hellwm_config_item));

                strncpy(temp_group->items[temp_group->count].key, key, sizeof(temp_group->items[temp_group->count].key));

                strncpy(temp_group->items[temp_group->count].value, value, sizeof(temp_group->items[temp_group->count].value));
                temp_group->count++;
            }
            printf("NAME: %s\nKEY: %s\nVALUE: %s\n",temp_group->name,
                    temp_group->items[temp_group->count-1].key,temp_group->items[temp_group->count-1].value);
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
