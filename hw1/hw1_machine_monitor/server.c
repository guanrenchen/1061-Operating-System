#include "server.h"

int main( int argc, char **argv)
{
	pthread_t server_thread;
	pthread_create(&server_thread, NULL, handleConnection, NULL);
	pthread_join(server_thread, NULL);
	printf("SERVER CLOSED\n");
	return 0;
}

void *handleConnection(void *ptr)
{
	int sockfd, newsockfd, clilen;
	struct sockaddr_in serv_addr, client_addr;

	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	/* Initialize socket structure */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	listen(sockfd,20);
	clilen = sizeof(client_addr);

	printf("Server initialized, start listening\n");

	while ( (newsockfd = accept(sockfd, (struct sockaddr *) &client_addr,
	                            (socklen_t*)&clilen)) )  {

		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}

		/* Create child process */
		pthread_t client_thread;
		pthread_create(&client_thread, NULL, handleRequest, (void*)&newsockfd);
		printf("Request handler assigned to client %d\n", newsockfd);

	} /* end of while */

	return (void*)NULL;
}

void *handleRequest(void *ptr)
{
	int n, cnt=0;
	int sockfd = *(int*)ptr;
	struct monitor_protocol req;

	memset(&req, 0, sizeof(req));
	while( (n = read(sockfd, &req, sizeof(req))) ) {
		printf("client%d request%d received : %c %d\n", sockfd, cnt++, req.code,
		       req.pid);
		sendData(sockfd, req);
		memset(&req, 0, sizeof(req));
	}

	if (n == 0) {
		printf("Client%d disconnected\n", sockfd);
	} else if(n == -1) {
		printf("Client%d failed to receive\n", sockfd);
	}

	return (void*)NULL;
}

void sendData(int sockfd, struct monitor_protocol req)
{
	if(checkPidExist(req.pid)==0 && req.code!='a') {
		sendPackage(sockfd, 0, "INVALID REQUEST");
		return;
	}

	switch(req.code) {
	case 'a': // List all process IDs
		sendPackage(sockfd, 1, "[Pid] ");
		sendNumericFilename(sockfd, "/proc");
		sendPackage(sockfd, 0, "");
		break;
	case 'b': // Thread's IDs
		sendPackage(sockfd, 1, "[Tid] ");
		char dir[64];
		sprintf(dir, "/proc/%d/task", req.pid);
		sendNumericFilename(sockfd, dir);
		sendPackage(sockfd, 0, "");
		break;
	case 'c': // child's PIDs
		sendPackage(sockfd, 1, "[Cid] ");
		sendChildrenProcess(sockfd, req.pid);
		sendPackage(sockfd, 0, "");
		break;
	case 'd': // process name
		sendPackage(sockfd, 1, "[Name] ");
		sendProcData(sockfd, req.pid, "Name");
		sendPackage(sockfd, 0, "");
		break;
	case 'e': // state of process(D,R,S,T,t,W,X,Z)
		sendPackage(sockfd, 1, "[State] ");
		sendProcData(sockfd, req.pid, "State");
		sendPackage(sockfd, 0, "");
		break;
	case 'f': // cmdline
		sendPackage(sockfd, 1, "[Cmdline] ");
		sendCmdline(sockfd, req.pid);
		sendPackage(sockfd, 0, "");
		break;
	case 'g': // parents PID
		sendPackage(sockfd, 1, "[PPid] ");
		sendProcData(sockfd, req.pid, "PPid");
		sendPackage(sockfd, 0, "");
		break;
	case 'h': // all ancestors of PIDs
		sendPackage(sockfd, 1, "[Ancients' PID] ");
		sendAncientProcess(sockfd, req.pid);
		sendPackage(sockfd, 0, "");
		break;
	case 'i': // VmSize
		sendPackage(sockfd, 1, "[VmSize] ");
		sendProcData(sockfd, req.pid, "VmSize");
		sendPackage(sockfd, 0, "");
		break;
	case 'j': // VmRSS
		sendPackage(sockfd, 1, "[VmRss] ");
		sendProcData(sockfd, req.pid, "VmRSS");
		sendPackage(sockfd, 0, "");
		break;
	default:
		sendPackage(sockfd, 0, "INVALID REQUEST");
	}
}

int checkPidExist(int pid)
{
	int result = 0;
	DIR *dp = opendir ("/proc");
	if (dp == NULL) {
		perror("ERROR : Failed to read /proc");
		return -1;
	}
	struct dirent *ep;
	while ( (ep = readdir (dp)) ) {
		if (pid == atoi(ep->d_name) && pid != 0) {
			result = 1;
			break;
		}
	}
	(void) closedir (dp);
	return result;
}

void sendNumericFilename(int sockfd, const char *dir)
{
	DIR *dp = opendir (dir);
	if (dp == NULL) {
		perror("ERROR : Failed to read /proc");
		return;
	}
	struct dirent *ep;
	while ( (ep = readdir (dp)) ) {
		if (atoi(ep->d_name)) {
			sendPackage(sockfd, 1, ep->d_name);
			sendPackage(sockfd, 1, " ");
		}
	}
	(void) closedir (dp);
}

void sendProcData(int sockfd, int pid, const char *key)
{
	FILE *fp;
	char filename[64];
	char *line = NULL;
	size_t len = 0;

	sprintf(filename, "/proc/%d/status", pid);
	fp = fopen(filename, "r");
	while ( getline(&line, &len, fp) != -1) {
		if( strstr(line, key) ) {
			sendPackage(sockfd, 1, strstr(line,"\t")+1);
			break;
		}
	}
	fclose(fp);
}

void sendChildrenProcess(int sockfd, int pid)
{
	struct proc_node *procList = malloc(sizeof(struct proc_node));;
	buildProcList(procList);

	struct proc_node *cur = procList;
	char tmp[64];
	for(cur=procList; cur!=NULL; cur=cur->next) {
		if(cur->ppid == pid) {
			memset(tmp, '\0', sizeof(tmp));
			sprintf(tmp, "%d", cur->pid);

			sendPackage(sockfd, 1, tmp);
			sendPackage(sockfd, 1, " ");
		}
	}
}

void sendCmdline(int sockfd, int pid)
{
	FILE *fp;
	char filename[64];
	char *line = NULL;
	size_t len = 0;

	sprintf(filename, "/proc/%d/cmdline", pid);
	fp = fopen(filename, "r");
	while ( getline(&line, &len, fp) != -1) {
		sendPackage(sockfd, 1, line);
	}
	fclose(fp);
}

void sendAncientProcess(int sockfd, int pid)
{
	struct proc_node *procList = malloc(sizeof(struct proc_node));;
	buildProcList(procList);

	struct proc_node *cur = procList;
	while(cur->next !=NULL) {
		cur = cur->next;
	}

	int child = pid;
	char tmp[64];
	while(cur != NULL) {
		if(cur->pid == child && cur->ppid > 0) {
			memset(tmp, '\0', sizeof(tmp));
			sprintf(tmp, "%d", cur->ppid);
			sendPackage(sockfd, 1, tmp);
			sendPackage(sockfd, 1, " ");
			child = cur->ppid;
		}
		cur = cur->prev;
	}
}

void buildProcList(struct proc_node *procList)
{
	DIR *dp = opendir ("/proc");
	if (dp == NULL) {
		perror("ERROR : Failed to read /proc");
		return;
	}

	procList->pid = -1;
	procList->ppid = -1;
	procList->prev = NULL;
	procList->next = NULL;

	int pid;
	char temp[64];
	struct dirent *ep;
	struct proc_node *cur = procList;

	while ( (ep = readdir (dp)) ) {
		if ( (pid=atoi(ep->d_name)) ) {
			memset(temp, 0, sizeof(temp));
			writeProcToBuffer(temp, pid, "PPid");

			cur->next = malloc(sizeof(struct proc_node));
			cur->next->prev = cur;
			cur->next->pid = pid;
			cur->next->ppid = atoi(temp);
			cur->next->next = NULL;
			cur = cur->next;
		}
	}
	(void) closedir (dp);
}

void sendPackage(int sockfd, int flag, const char *msg)
{
	struct data_package pkg;
	int msg_len = strlen(msg);
	int buf_len = sizeof(pkg.msg);
	int cnt = 0;

	while( cnt <= msg_len ) {
		memset(&pkg, 0, sizeof(pkg));
		pkg.flag = flag;
		strncpy(pkg.msg, (char*)(msg+cnt), buf_len);
		if (write(sockfd, &pkg, sizeof(pkg)) < 0) {
			printf("Failed to send to client%d\n", sockfd);
		}
		cnt += buf_len;
	}
}

void writeProcToBuffer(char *buffer, int pid, const char *key)
{
	FILE *fp;
	char filename[64];
	char *line = NULL;
	size_t len = 0;

	sprintf(filename, "/proc/%d/status", pid);
	fp = fopen(filename, "r");
	while ( getline(&line, &len, fp) != -1) {
		if( strstr(line, key) ) {
			strcat(buffer, strstr(line,"\t")+1);
			break;
		}
	}
	fclose(fp);
}
