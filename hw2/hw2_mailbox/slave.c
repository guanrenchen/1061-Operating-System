#include "slave.h"

int main(int argc, char **argv)
{
	int fd;
	unsigned int word_count;
	struct mail_t mail;

	while(1) {
		while ((fd = open(MAILBOX_PATH, O_RDONLY, S_IRUSR)) == -1);
		memset(&mail, 0, sizeof(mail));
		while (receive_from_fd(fd, &mail) < 0);
		close(fd);
		//printf("Mail recv (slave)\n");

		word_count = count_word_in_file((char *)&mail.data, mail.file_path);
		memset(&mail.data, 0, sizeof(mail.data));
		memcpy(&mail.data, &word_count, sizeof(unsigned int));

		while ((fd = open(MAILBOX_PATH, O_WRONLY, S_IWUSR)) == -1);
		while(send_to_fd(fd, &mail)<0);
		close(fd);
		//printf("Mail sent (slave)\n");
	}
}

unsigned int count_word_in_file(const char* query, const char* path)
{
	int fd;
	char *buf, *pos;
	long length;
	unsigned int count;

	while ((fd = open(path, O_RDONLY, S_IRUSR)) == -1);
	length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	buf = malloc(length);
	if (buf) {
		read(fd, buf, length);
	}
	close(fd);

	if (buf) {
		for (int i=0; buf[i]; ++i)
			buf[i] = tolower(buf[i]);
		pos = buf;
		count = 0;
		while ( (pos = strstr(pos, query)) ) {
			++pos;
			++count;
		}
		return count;
	}

	return 0;
}

int send_to_fd(int sysfs_fd, struct mail_t *mail)
{
	lseek(sysfs_fd, 0, SEEK_SET);
	int ret_val = write(sysfs_fd, mail, sizeof(mail->data)+strlen(mail->file_path));
	/*
	if (ret_val == ERR_FULL) {
		printf("Mailbox full (slave)\n");
	} else {
		printf("send_to_fd (slave)\n");
		printf("\tCOUNT : %u\n", *(unsigned int *)&mail->data);
		printf("\tPATH  : %s\n", mail->file_path);
		printf("\tBYTES : %d\n", ret_val);
	}
	*/
	return ret_val;
}

int receive_from_fd(int sysfs_fd, struct mail_t *mail)
{
	lseek(sysfs_fd, 0, SEEK_SET);
	int ret_val = read(sysfs_fd, mail, sizeof(struct mail_t));
	/*
	if (ret_val == ERR_EMPTY) {
		printf("Mailbox empty (slave)\n");
	} else {
		printf("receive_from_fd (slave)\n");
		printf("\tQUERY : %s\n", (char *)&mail->data);
		printf("\tPATH  : %s\n", mail->file_path);
		printf("\tBYTES : %d\n", ret_val);
	}
	*/
	return ret_val;
}
