/* client side of communication tool with the HellWM Wayland Compositor */
#ifndef HELLCLI_HELLCLI_H
#define HELLCLI_HELLCLI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define HELLWM_SOCK "/tmp/hellwm" 

void hellcli_print_usage();
int main(int argc, char *argv[]);

#endif
