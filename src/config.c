#include <complex.h>
#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "../include/server.h"
#include "../include/config.h"
#include "../include/repeatable_functions.h"

/* because of bad linking :/ */
#include "./repeatable_functions.c"

void hellwm_config_load(const char* filename, hellwm_config* config)
{
    int sourceFilesCount=0;
    char** sourceFilesToLoad=NULL;

    FILE* file = fopen(filename, "r");
    if (file == NULL)
    {
        hellwm_log(HELLWM_ERROR,
                "Failed to open HellWM configuration file: %s\n",
                filename);
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

        if (!strcmp(group,hellwm_config_groups_arr[HELLWM_CONFIG_SOURCE]))
        {
            if (addElement(&sourceFilesToLoad,&sourceFilesCount,item)==1)
            {
                hellwm_log(HELLWM_ERROR,"malloc() failed, skipping loading from sourcefile: %s\n",item);
                continue;
            }
            continue;
        }
 
        if (group && item)
        {
            hellwm_config_group *temp_group  = hellwm_config_search_in_group_by_name(config, group);

            if (temp_group==NULL)
            {
                // error here probably TODO
                
                config->groups = realloc(config->groups, (config->count + 1) * sizeof(hellwm_config_group));
                config->count=config->count+1;
                config->groups[config->count-1].name=group;
                config->groups[config->count-1].count=0;
                config->groups[config->count-1].items=NULL;
                
                //add_config_group(&config->groups, &config->count, NULL);
                //config->groups[config->count-1].name=group;

                hellwm_log(HELLWM_LOG,
                        "group %s does not exists, creating new one. TOTAL SIZE: %d", 
                        config->groups[config->count-1].name,
                        config->count);
            }
        
            temp_group = hellwm_config_search_in_group_by_name(config, group);

            if (temp_group==NULL)
            {
                hellwm_log(
                        HELLWM_ERROR, 
                        "Cannot find group named: %s. Line: %d, File: %s",
                        group,
                        linePosition,
                        filename);
                continue;
            }
            
            char* key = strtok(item, ",");
            int idx = hellwm_config_check_character_in_line(linecopy,',');
            if (idx==-1)
            {
                hellwm_log(HELLWM_ERROR,
                        "config parse error. File: %s, Line %d", 
                        filename,
                        linePosition);
                continue;
            }
            
            char value[strlen(linecopy)-idx+1];

            memcpy(value,linecopy+idx+1,strlen(linecopy)-idx);

            if (key)
            {
                printf("\n\n%s, %d\n",temp_group->name,temp_group->count);
                temp_group->items = realloc(temp_group->items, (temp_group->count + 1) * sizeof(hellwm_config_item));
                temp_group->count++;
                strncpy(temp_group->items[temp_group->count-1].key, key, sizeof(temp_group->items[temp_group->count-1].key));
                strncpy(temp_group->items[temp_group->count-1].value, value, sizeof(temp_group->items[temp_group->count-1].value));
            }
            printf("NAME: %s\nKEY: %s\nVALUE: %s\n\n",temp_group->name,
                    temp_group->items[temp_group->count-1].key,temp_group->items[temp_group->count-1].value);
         }
    }
    fclose(file);

    for (int i=0;i<sourceFilesCount;i++)
    {
        hellwm_config_load(sourceFilesToLoad[i],config);
    }
}

void hellwm_config_print(hellwm_config *config)
{
    printf("ALL GROUPS COUNT: %d",config->count);

    for(int i=0;i<config->count;i++)
    {
        printf("\n\nGROUP: %s\n",config->groups[i].name);
        for(int j=0;j<config->groups[i].count;j++)
        {
           printf("\t%s: %s\n",config->groups[i].items[j].key, config->groups[i].items[j].value); 
        }
    }
}

void hellwm_config_setup(hellwm_config *config)
{
    for (int i=0;i<sizeof(hellwm_config_groups_arr)/sizeof(hellwm_config_groups_arr[0]);i++)
    {
        config->groups = realloc(
                config->groups,
                (config->count+1) * sizeof(hellwm_config_group)
            );
        if (config->groups==NULL)
        {
            hellwm_log(HELLWM_ERROR,"realloc() failed");
            continue;
        }
        config->count++;
        config->groups[config->count-1].name=(char*)hellwm_config_groups_arr[i];
        config->groups[config->count-1].count=0;
        config->groups[config->count-1].items=NULL;
    }
    hellwm_config_print(config);
}

/* this return index of group found under query */
hellwm_config_group *hellwm_config_search_in_group_by_name(hellwm_config *config, char*query)
{
    for (int i=0;i<config->count;i++)
    {
        //printf("search in group: %s\n", config->groups[i].name); 
        if (strcmp(config->groups[i].name,query)==0)
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
