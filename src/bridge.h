#pragma once
#include <emscripten/fetch.h>

char *get_file_async();
char *read_async();
char get_char_async(bool wait = false);
void send_ping_and_char_async(char *command, char c);
void send_ping_async(char *command);