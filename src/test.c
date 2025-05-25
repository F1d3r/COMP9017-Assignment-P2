#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#include "../libs/document.h"

#define CMD_LEN 256



void main(){
    log* log_head = init_log();
    char msg[CMD_LEN] = "Version 0\nEdit Fider INSERT 0 12 SUCCESS\nEdit Fider INSERT 0 34 SUCCESS\nEdit Fider INSERT 0 THIS SUCCESS\nEND\n";
    printf("Origin msg:\n%s", msg);
    log* mylog = get_log(msg);

    print_log(mylog);

}