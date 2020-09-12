#include "scheduling_simulator.h"

int pid = -1;
sigset_t sigset_sigalrm;
sig_atomic_t flag_SIGTSTP = 0;
struct tasknode *task = NULL;
struct tasknode *origin = NULL;
ucontext_t *uctx_cur;
ucontext_t uctx_prompt;
ucontext_t uctx_schedule;
ucontext_t uctx_reschedule;
ucontext_t uctx_nothing;

void hw_suspend(int msec_10)
{
	sigprocmask(SIG_BLOCK, &sigset_sigalrm, NULL);

	struct tasknode *task_old = task;

	if(task->pid == task->next->pid)
		task = NULL;
	else
		task = task->prev;

	task_old->q_time = 0;
	task_old->s_time = msec_10 * 10;
	task_old->state = TASK_WAITING;
	task_old->prev->next = task_old->next;
	task_old->next->prev = task_old->prev;
	task_old->next = NULL;
	task_old->prev = NULL;

	ucontext_t *temp = uctx_cur;
	uctx_cur = &uctx_nothing;
	swapcontext(temp, uctx_cur);

	sigprocmask(SIG_UNBLOCK, &sigset_sigalrm, NULL);
}

void hw_wakeup_pid(int pid)
{
	sigprocmask(SIG_BLOCK, &sigset_sigalrm, NULL);
	wakeup_pid(pid);
	sigprocmask(SIG_UNBLOCK, &sigset_sigalrm, NULL);
}

int hw_wakeup_taskname(char *task_name)
{
	sigprocmask(SIG_BLOCK, &sigset_sigalrm, NULL);
	int count = 0;
	struct tasknode *cur;
	for (cur=origin; cur; cur=cur->next_p) {
		if (strcmp(cur->name, task_name)==0) {
			wakeup_pid(cur->pid);
			++count;
		}
	}
	sigprocmask(SIG_UNBLOCK, &sigset_sigalrm, NULL);
	return count;
}

int hw_task_create(char *task_name)
{
	sigprocmask(SIG_BLOCK, &sigset_sigalrm, NULL);
	int new_pid = task_create(task_name);
	sigprocmask(SIG_UNBLOCK, &sigset_sigalrm, NULL);
	return new_pid; // the pid of created task_cur name
}

void wakeup_pid(int pid)
{
	struct tasknode *cur;
	for(cur=origin; cur; cur=cur->next_p)
		if(cur->pid==pid)
			break;
	if(cur && cur->state==TASK_WAITING) {
		cur->q_time = 0;
		cur->s_time = 0;
		cur->state = TASK_READY;
		if(task) {
			cur->next = task;
			cur->prev = task->prev;
			cur->next->prev = cur;
			cur->prev->next = cur;
		} else {
			task = cur;
			task->prev = task;
			task->next = task;
		}
	}
}

int task_create(char *task_name)
{
	void (*func)(void);
	if(strcmp(task_name, "task1")==0)
		func = &task1;
	else if(strcmp(task_name, "task2")==0)
		func = &task2;
	else if(strcmp(task_name, "task3")==0)
		func = &task3;
	else if(strcmp(task_name, "task4")==0)
		func = &task4;
	else if(strcmp(task_name, "task5")==0)
		func = &task5;
	else if(strcmp(task_name, "task6")==0)
		func = &task6;
	else
		return -1;

	int new_pid = ++pid;

	struct tasknode *new_task;
	new_task = malloc(sizeof(struct tasknode));
	memset(new_task, 0, sizeof(struct tasknode));

	new_task->pid = new_pid;
	new_task->quantum = 10;
	new_task->q_time = 0;
	new_task->s_time = 0;
	new_task->state = TASK_READY;
	strncpy(new_task->name, task_name, sizeof(new_task->name));
	new_task->prev = NULL;
	new_task->next = NULL;
	new_task->prev_p = NULL;
	new_task->next_p = NULL;

	if(task) {
		new_task->prev = task->prev;
		new_task->next = task;
		new_task->prev->next = new_task;
		new_task->next->prev = new_task;
	} else {
		task = new_task;
		task->prev = task;
		task->next = task;
	}

	if(origin) {
		struct tasknode *tail;
		for(tail=origin; tail->next_p; tail=tail->next_p);
		tail->next_p = new_task;
		new_task->prev_p = tail;
	} else {
		origin = new_task;
	}

	setup_context(&new_task->uctx, STACKSIZE, &uctx_reschedule, func);

	return new_pid;
}

void prompt()
{
	flag_SIGTSTP = 0;

	char command[1024];
	const char delim[3] = " \n";
	char *token;

	while(1) {
		memset(command, 0, sizeof(command));
		printf("$ ");
		fgets(command, sizeof(command), stdin);

		token = strtok(command, delim);
		if(!token) continue;

		if ( strcmp(token, "add")==0 ) {
			token = strtok(NULL, delim);
			if(!token) continue;
			int new_pid = task_create(token);

			if(new_pid==-1) continue;

			token = strtok(NULL, delim);
			if(!token || strcmp(token,"-t")!=0)	continue;

			token = strtok(NULL, delim);
			if(!token || strcmp(token,"L")!=0) continue;

			struct tasknode *cur;
			for(cur=origin; cur; cur=cur->next_p)
				if(cur->pid == new_pid)
					break;
			cur->quantum = 20;
		} else if( strcmp(token, "remove")==0 ) {
			// REMOVE pid
			token = strtok(NULL, delim);
			if(!token) continue;

			int target_pid = atoi(token);

			struct tasknode *cur;
			for(cur=origin; cur; cur=cur->next_p)
				if(cur->pid == target_pid)
					break;

			if(!cur) continue;

			if(origin == cur) {
				origin = origin->next_p;
			}

			switch(cur->state) {
			case TASK_RUNNING:
				if (cur->pid == cur->next->pid) {
					task = NULL;
				} else {
					task = task->next;
				}
			case TASK_READY:
				cur->prev->next = cur->next;
				cur->next->prev = cur->prev;
			case TASK_WAITING:
			case TASK_TERMINATED:
				if(cur->prev_p)
					cur->prev_p->next_p = cur->next_p;
				if(cur->next_p)
					cur->next_p->prev_p = cur->prev_p;
			}

			free(cur->uctx.uc_stack.ss_sp);
			free(cur);

		} else if( strcmp(token, "ps")==0 ) {
			struct tasknode *cur;
			for(cur=origin; cur; cur=cur->next_p) {
				printf("\t%d\t", cur->pid);
				printf("%s\t", cur->name);
				switch(cur->state) {
				case TASK_READY:
					printf("TASK_READY\t");
					break;
				case TASK_RUNNING:
					printf("TASK_RUNNING\t");
					break;
				case TASK_WAITING:
					printf("TASK_WAITING\t");
					break;
				case TASK_TERMINATED:
					printf("TASK_TERMINATED\t");
					break;
				}
				printf("%d\n", cur->q_time);
				fflush(stdout);
			}
		} else if( strcmp(token, "start")==0 ) {
			printf("Simulating...\n");
			fflush(stdout);
			flag_SIGTSTP = 0;
			swapcontext(&uctx_prompt, &uctx_schedule);
			printf("\n");
			fflush(stdout);
		} else {
			printf("INVALID COMMAND\n");
		}
	}

}

void schedule()
{
	if (flag_SIGTSTP==1) {
		setcontext(&uctx_prompt);
	}

	struct tasknode *cur;

	int all_terminate = 1;
	for(cur=origin; cur; cur=cur->next_p) {
		if(cur->state != TASK_TERMINATED) {
			all_terminate = 0;
			break;
		}
	}
	if (all_terminate==1) {
		setcontext(&uctx_prompt);
	}

	int quantum = 10;
	if(task) {
		if(task->state==TASK_RUNNING) {
			task->state = TASK_READY;
			task = task->next;
		}
		task->state = TASK_RUNNING;
		quantum = task->quantum;
		uctx_cur = &task->uctx;
	} else {
		quantum = 10;
		uctx_cur = &uctx_nothing;
	}

	for(cur=origin; cur; cur=cur->next_p) {
		switch(cur->state) {
		case TASK_RUNNING:
			cur->q_time = 0;
			cur->s_time = 0;
			break;
		case TASK_READY:
			cur->q_time += quantum;
			break;
		case TASK_WAITING:
			cur->s_time -= quantum;
			if(cur->s_time<=0)
				wakeup_pid(cur->pid);
			break;
		case TASK_TERMINATED:
			break;
		}
	}

	ualarm(quantum*1000, 0);
	setcontext(uctx_cur);
}

void reschedule()
{
	task->q_time = 0;
	task->s_time = 0;
	task->state = TASK_TERMINATED;

	//free(task->uctx.uc_stack.ss_sp);

	if(task->pid == task->next->pid) {
		task = NULL;
	} else {
		struct tasknode *temp = task->prev;

		task->prev->next = task->next;
		task->next->prev = task->prev;
		task->prev = NULL;
		task->next = NULL;

		task = temp;
	}

	uctx_cur = &uctx_nothing;
	setcontext(uctx_cur);
}

void nothing()
{
	while(1);
}

void setup_context(ucontext_t *uctx, size_t stacksize, ucontext_t *uc_link,
                   void *func)
{
	getcontext(uctx);
	uctx->uc_stack.ss_sp = malloc(stacksize);
	uctx->uc_stack.ss_size = stacksize;
	uctx->uc_link = uc_link;
	sigemptyset(&uctx->uc_sigmask);
	makecontext(uctx, func, 0);
}

void handler(int sig)
{
	switch(sig) {
	case SIGALRM:
		swapcontext(uctx_cur, &uctx_schedule);
		break;
	case SIGTSTP:
		flag_SIGTSTP = 1;
		break;
	}
}

int main()
{
	// make signal mask for critical section
	sigemptyset(&sigset_sigalrm);
	sigaddset(&sigset_sigalrm, SIGALRM);

	// assign signal handler
	signal(SIGALRM, handler);
	signal(SIGTSTP, handler);

	// make uctx for prompt
	setup_context(&uctx_prompt, STACKSIZE, NULL, prompt);
	sigaddset(&uctx_prompt.uc_sigmask, SIGALRM);

	// make uctx for schedule
	setup_context(&uctx_schedule, STACKSIZE, NULL, schedule);
	sigaddset(&uctx_schedule.uc_sigmask, SIGALRM);

	// make uctx for reschedule
	setup_context(&uctx_reschedule, STACKSIZE, &uctx_schedule, reschedule);
	sigaddset(&uctx_reschedule.uc_sigmask, SIGALRM);

	// make uctx for nothing
	setup_context(&uctx_nothing, STACKSIZE, NULL, nothing);

	setcontext(&uctx_prompt);
	return 0;
}
