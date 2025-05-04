#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "ftp.h"

#define DEFAULT_PORT 21
#define DEFAULT_ROOT_DIR "/tmp/ftproot"

void print_usage(const char *program_name);
void signal_handler(int sig);

#endif /* MAIN_H */
