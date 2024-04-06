#include <complex.h>
#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
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
            if (addElement(&sourceFilesToLoad,&sourceFilesCount,item)==1)
            {
                perror("malloc() failed, skipping loading from sourcefile: \n");
                //hellwm_log(HELLWM_ERROR,"malloc() failed, skipping loading from sourcefile: %s\n",item);
                continue;
            }
            continue;
        }
 
        if (group && item)
        {
            hellwm_config_group* temp_group = hellwm_config_search_in_group_by_name(config, group);
            if (temp_group==NULL)
            {
                //error here probably TODO
                config->groups = realloc(config->groups, (config->count + 1) * sizeof(hellwm_config_group));
                config->groups[config->count].name=group;
                config->count=config->count+1;
                hellwm_log(HELLWM_LOG,"hellwm_config_search_in_group_by_name == NULL, created new group: %s. TOTAL SIZE: %d", 
                        config->groups[config->count-1].name, config->count);
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

hellwm_config_group *hellwm_config_search_in_group_by_name(hellwm_config *config, char*query)
{
    for (int i=0;i<config->count;i++)
    {
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
