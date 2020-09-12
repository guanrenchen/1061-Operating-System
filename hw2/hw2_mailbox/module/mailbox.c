#include "mailbox.h"

MODULE_LICENSE("Dual BSD/GPL");

static void get_process_name(char *ouput_name);
static ssize_t mailbox_read(struct kobject *kobj,
                            struct kobj_attribute *attr, char *buf);
static ssize_t mailbox_write(struct kobject *kobj,
                             struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobject *hw2_kobject;
static struct kobj_attribute mailbox_attribute
    = __ATTR(mailbox, 0660, mailbox_read, mailbox_write);

static int num_entry_max = 2;
module_param(num_entry_max, int, S_IRUGO);

static struct mailbox_head_t mailbox;

static void get_process_name(char *ouput_name)
{
	memcpy(ouput_name, current->comm, sizeof(current->comm));
}

static ssize_t mailbox_read(struct kobject *kobj,
                            struct kobj_attribute *attr, char *buf)
{
	struct mailbox_entry_t *mbox, *tmp;
	ssize_t read_size;

	spin_lock(&mailbox.mylock);


	if(mailbox.count==0) {
		//printk("Mailbox empty triggered (%s)\n", current->comm);
		spin_unlock(&mailbox.mylock);
		return ERR_EMPTY;
	}

	list_for_each_entry_safe(mbox, tmp, &mailbox.head, entry) {

		if(	(strcmp(current->comm,"master")==0 && mbox->flag==STAGE_DONE) ||
		    (strcmp(current->comm,"slave")==0 && mbox->flag==STAGE_WAIT) ) {
			read_size = sizeof(mbox->mail.data) + strlen(mbox->mail.file_path);
			/*
			printk("READER : %s\n", current->comm);
			printk("\t\tFLAG  : %s\n", (mbox->flag==STAGE_WAIT)? "WAIT": "DONE");
			(mbox->flag)==STAGE_WAIT ?
			printk("\t\tQUERY : %s\n", (char *)&mbox->mail.data):
			printk("\t\tCOUNT : %u\n", *(unsigned int *)&mbox->mail.data);
			printk("\t\tPATH  : %s\n", mbox->mail.file_path);
			printk("\t\tBYTES : %ld\n", read_size);
			*/
			memcpy(buf, &mbox->mail, read_size);
			list_del(&mbox->entry);
			kfree(mbox);
			--mailbox.count;
			spin_unlock(&mailbox.mylock);
			return read_size;
		}
	}
	spin_unlock(&mailbox.mylock);
	return ERR_EMPTY;
}

static ssize_t mailbox_write(struct kobject *kobj,
                             struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct mailbox_entry_t *mbox;

	spin_lock(&mailbox.mylock);

	if(	(strcmp(current->comm,"master")==0 && mailbox.count<num_entry_max-1)
	    || 	(strcmp(current->comm,"slave")==0 && mailbox.count<num_entry_max) ) {

		mbox = kmalloc(sizeof(struct mailbox_entry_t), GFP_KERNEL);
		memset(mbox, 0, sizeof(struct mailbox_entry_t));
		mbox->flag = strcmp(current->comm,"master")==0? STAGE_WAIT: STAGE_DONE;
		memcpy(&mbox->mail, buf, count);
		INIT_LIST_HEAD(&mbox->entry);
		list_add(&(mbox->entry), &(mailbox.head));
		++mailbox.count;
		/*
		printk("WRITER : %s\n", current->comm);
		printk("\t\tFLAG  : %s\n", (mbox->flag==STAGE_WAIT)? "WAIT": "DONE");
		(mbox->flag)==STAGE_WAIT ?
		printk("\t\tQUERY : %s\n", (char *)&mbox->mail.data):
		printk("\t\tCOUNT : %u\n", *(unsigned int *)&mbox->mail.data);
		printk("\t\tPATH  : %s\n", mbox->mail.file_path);
		printk("\t\tBYTES : %ld\n", count);
		*/
		spin_unlock(&mailbox.mylock);
		return count;
	} else {
		//printk("Mailbox full triggered (%s)\n", current->comm);
		spin_unlock(&mailbox.mylock);
		return ERR_FULL;
	}

}

static int __init mailbox_init(void)
{
	printk("Insert\n");
	hw2_kobject = kobject_create_and_add("hw2", kernel_kobj);
	sysfs_create_file(hw2_kobject, &mailbox_attribute.attr);

	INIT_LIST_HEAD(&mailbox.head);
	spin_lock_init(&mailbox.mylock);
	mailbox.count = 0;

	return 0;
}

static void __exit mailbox_exit(void)
{
	printk("Remove\n");
	kobject_put(hw2_kobject);
}

module_init(mailbox_init);
module_exit(mailbox_exit);
