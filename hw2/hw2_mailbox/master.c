#include "master.h"

int main(int argc, char **argv)
{
	char query[32], query_low[32];
	char top_dir[4096];
	int num_slave = 1;

	int query_get = 0;
	int target_dir_get = 0;

	if (argc != 5 && argc != 7) {
		printf("ERROR : only 4 or 6 arguments allowed%d\n", argc-1);
		exit(1);
	}
	for (int i = 1; i < argc; i+=2) {
		if (strcmp(argv[i], "-q")==0) {
			memset(query, 0, sizeof(query));
			memset(query_low, 0, sizeof(query_low));
			strncpy(query, argv[i + 1], sizeof(query));
			for (int i=0; query[i]; ++i)
				query_low[i] = tolower(query[i]);
			query_get = 1;
		} else if (strcmp(argv[i], "-d") == 0) {
			strncpy(top_dir, argv[i + 1], sizeof(top_dir));
			if ( (char)top_dir[strlen(top_dir)-1] != '/') {
				strncat(top_dir, "/", 1);
			}
			target_dir_get = 1;
		} else if (strcmp(argv[i], "-s") == 0) {
			num_slave = atoi(argv[i + 1]);
			if (num_slave<=0)
				num_slave = 1;
		} else {
			perror("ERROR : only -q, -d, -s allowed\n");
			return 0;
		}
	}
	if (query_get==0 || target_dir_get==0) {
		perror("Mandatory arguments not satisfied\n");
		return 0;
	}

	int pid;
	int slave_pids[num_slave];
	for (int i = 0; i < num_slave; ++i) {
		pid = fork();
		if (pid>0) {
			//master
			slave_pids[i] = pid;
		} else if (pid ==0) {
			//slave
			execv("./slave", (char *[]) {
				"./slave", NULL
			});
			exit(1);
		} else {
			perror("Fork failed");
			abort();
		}
	}

	unsigned int query_count = 0;
	count_query_in_all_dir(top_dir, query_low, &query_count);
	printf("The total number of the query word \"%s\" is :%u\n", query,
	       query_count);
	for (int i = 0; i < num_slave; ++i) {
		kill(slave_pids[i], SIGKILL);
	}
	return 0;
}

void count_query_in_all_dir(const char* top_dir, const char* query,
                            unsigned int *query_count)
{

	count_query_in_dir(top_dir, query, query_count);

	glob_t pglob;
	char pattern[4096];
	memset(pattern, 0, sizeof(pattern));
	strncpy(pattern, top_dir, sizeof(pattern));
	strncat(pattern, "*/", 2);
	glob(pattern, GLOB_MARK | GLOB_ERR, NULL, &pglob);
	for (int i = 0; i<pglob.gl_pathc; ++i) {
		count_query_in_all_dir(pglob.gl_pathv[i], query, query_count);
	}
	globfree(&pglob);
}

void count_query_in_dir(const char* top_dir, const char* query,
                        unsigned int *query_count)
{
	glob_t pglob;
	unsigned int file_count;
	char pattern[4096];
	strncpy(pattern, top_dir, sizeof(pattern));
	strncat(pattern, "*.txt", 5);
	glob(pattern, GLOB_ERR, NULL, &pglob);
	file_count = pglob.gl_pathc;

	char file_paths[file_count][4096];
	for (int i = 0; i<file_count; ++i) {
		memset(file_paths[i], 0, sizeof(file_paths[i]));
		strncpy(file_paths[i], pglob.gl_pathv[i], sizeof(file_paths[i]));
	}

	globfree(&pglob);

	int fd;
	struct mail_t mail;
	unsigned int file_done = 0;
	unsigned int file_sent = 0;
	while (1) {
		if (file_done==file_count) {
			break;
		} else {
			while ((fd = open(MAILBOX_PATH, O_RDONLY, S_IRUSR)) == -1);
			memset(&mail, 0, sizeof(mail));
			if (receive_from_fd(fd, &mail) >= 0) {
				//printf("Mail recv (master)\n");
				*query_count += *(unsigned int *)&mail.data;
				++file_done;
			}
			close(fd);
		}

		if (file_sent==file_count) {
			continue;
		} else {
			memset(&mail, 0, sizeof(mail));
			memcpy(&mail.data, query, strlen(query));
			memcpy(mail.file_path, file_paths[file_sent], sizeof(mail.file_path));

			while ((fd = open(MAILBOX_PATH, O_WRONLY, S_IWUSR)) == -1);
			if (send_to_fd(fd, &mail)>=0) {
				//printf("Mail sent (master)\n");
				++file_sent;
			}
			close(fd);
		}
	}
	return;
}

int send_to_fd(int sysfs_fd, struct mail_t *mail)
{
	lseek(sysfs_fd, 0, SEEK_SET);
	int ret_val = write(sysfs_fd, mail, sizeof(mail->data)+strlen(mail->file_path));
	/*
	if (ret_val == -1) {
		printf("\tMailbox full (master)\n");
	} else {
		printf("send_to_fd (master)\n");
		printf("\tQUERY : %s\n", (char *)&mail->data);
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
	if (ret_val == -1) {
		printf("Mailbox empty (master)\n");
	} else {
		printf("receive_from_fd (master)\n");
		printf("\tCOUNT : %u\n", *(unsigned int *)&mail->data);
		printf("\tPATH  : %s\n", mail->file_path);
		printf("\tBYTES : %d\n", ret_val);
	}
	*/
	return ret_val;
}
