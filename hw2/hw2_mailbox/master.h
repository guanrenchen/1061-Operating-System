#ifndef MASTER_H
#define MASTER_H

#include "mail.h"

#include <ctype.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

void count_query_in_all_dir(const char *top_dir, const char *query,
                            unsigned int *query_count);
void count_query_in_dir(const char *top_dir, const char *query,
                        unsigned int *query_count);

#endif
