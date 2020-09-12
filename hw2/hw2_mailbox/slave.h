#ifndef SLAVE_H
#define SLAVE_H

#include "mail.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

unsigned int count_word_in_file(const char *query, const char *path);

#endif
