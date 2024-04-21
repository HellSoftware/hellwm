#ifndef SERV_HELLCLI_H
#define SERV_HELLCLI_H

/* Server side of communication tool with the HellWM Wayland Compositor */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define HELLWM_SOCK "/tmp/hellwm"

/*
 * for example: hellcli kill active 
 * kill - group_keyword of hellcli_cmd structure
 * active - pointer 'var' in hellcli_cmd_filler
 *
 * we will assign destroy_toplevel() to the assignedCommand
 * and put 'active' toplevel provided from server
 */

typedef void(*assignedCommand)(void);

typedef struct {
    char *group_keyword;
    char *name;
    void *variable;
    assignedCommand cmd;
} hellcli_cmd;

// temp. main function for testing purpose
void hellcli_serv();
void test_func()

#endif
