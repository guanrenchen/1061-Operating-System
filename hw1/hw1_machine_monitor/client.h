#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

#include "protocol.h"

void printOption();
void sendRequest(int sockfd);
void getResponse(int sockfd);

#endif
