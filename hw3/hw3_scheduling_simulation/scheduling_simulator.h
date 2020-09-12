#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ucontext.h>

#include <signal.h>

#include <sys/time.h>

#include "task.h"

#define STACKSIZE 16384
#define MAX_NAME_LENGTH 1024

enum TASK_STATE {
	TASK_RUNNING,
	TASK_READY,
	TASK_WAITING,
	TASK_TERMINATED
};

struct tasknode {
	int pid, quantum, q_time, s_time;
	enum TASK_STATE state;
	char name[MAX_NAME_LENGTH];
	struct tasknode *prev, *next, *prev_p, *next_p;
	ucontext_t uctx;
};

void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);

void wakeup_pid(int pid);
int task_create(char *task_name);
void prompt();
void schedule();
void reschedule();
void nothing();
void setup_context(ucontext_t *uctx, size_t stacksize, ucontext_t *uc_link,
                   void *func);
void handler(int sig);

#endif
