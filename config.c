#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum hellwm_kbind_type {
	KBIND_CMD,
	KBIND_KILL,
	KBIND_EXIT,
	KBIND_RELOAD,
	KBIND_MAXW,
};

static void hellwm_config_kbind_give()
{
	
}

static int hellwm_config_parse_line(char *line)
{
	char *type;
	char *keybind;

	type = strtok(line,",");
	printf("%s\n",type);
	
	memcpy(line + strlen(type), line, strlen(line));		

	keybind = strtok(line,",");
	printf("%s\n",type);

	return 0;
}

static void hellwm_config_load()
{
	FILE* fconfig;
	fconfig = fopen("config.conf","r");
	fseek(fconfig,0L,SEEK_END);
	int buffer_size = ftell(fconfig);
	fseek(fconfig, 0, SEEK_SET);
	
	char line[256];
	while (fgets(line, sizeof(line), fconfig)) 
	{
       hellwm_config_parse_line(line);
   }
   fclose(fconfig);
}

int main()
{
  hellwm_config_load();
  return 0;
}
