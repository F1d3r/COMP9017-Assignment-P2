#ifndef HELPER_H
#define HELPER_H

#include "document.h"


bool check_integer(char* str);

void resolve_command(char* command_input, char** command, char** arg1, char** arg2, char** arg3);

bool check_command_insert(document* doc, char* arg1, char* arg2);

bool check_command_delete(document* doc, char* arg1, char* arg2);

bool check_command_bold(document* doc, char* arg1, char* arg2);

bool check_command_newline(document* doc, char* arg1);

#endif
