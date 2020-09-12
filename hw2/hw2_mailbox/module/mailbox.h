#ifndef MAILBOX_H
#define MAILBOX_H

#include "def.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/spinlock_types.h>

#include <linux/slab.h>

#define STAGE_WAIT -3
#define STAGE_DONE -4

struct mail_t {
	union {
		char query_word[32];
		unsigned int word_count;
	} data;
	char file_path[4096];
};

struct mailbox_head_t {
	int count;
	spinlock_t mylock;
	struct list_head head;
};

struct mailbox_entry_t {
	int flag;
	struct mail_t mail;
	struct list_head entry;
};

#endif
