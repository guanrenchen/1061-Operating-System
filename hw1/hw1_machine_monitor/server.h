#ifndef SERVER_H
#define SERVER_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#include <ctype.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "protocol.h"

struct proc_node {
	int pid, ppid;
	struct proc_node *prev, *next;
};

void *handleConnection(void *ptr);
void *handleRequest(void *ptr);
void sendData(int sockfd, struct monitor_protocol req);

int checkPidExist(int pid);
void sendNumericFilename(int sockfd, const char *dir);
void sendProcData(int sockfd, int pid, const char *key);
void sendChildrenProcess(int sockfd, int pid);
void sendCmdline(int sockfd, int pid);
void sendAncientProcess(int sockfd, int pid);

void buildProcList(struct proc_node *procList);
void sendPackage(int sockfd, int flag, const char *msg);

void writeProcToBuffer(char *buffer, int pid, const char *key);

#endif
