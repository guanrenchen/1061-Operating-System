#include "client.h"

int main( int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in server_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	struct hostent *server = gethostbyname(ADDRESS);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ADDRESS);
	server_addr.sin_port = htons(PORT);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("ERROR while connecting");
		exit(1);
	}

	printf("Client ready\n");
	while(1) {
		printOption();
		sendRequest(sockfd);
		getResponse(sockfd);
	}
	close(sockfd);

	return 0;
}

void printOption()
{
	printf("======================================\n");
	printf("(a) list all process ids\n");
	printf("(b) thread's IDs\n");
	printf("(c) child's PIDs\n");
	printf("(d) process name\n");
	printf("(e) state of process(D,R,S,T,t,W,X,Z)\n");
	printf("(f) command line of executing process\n");
	printf("(g) parent's PID\n");
	printf("(h) all ancients of PIDs\n");
	printf("(i) virtual memory size(VmSize)\n");
	printf("(j) physical memory size(VmRSS)\n");
	printf("(k) exit\n\n");
}

void sendRequest(int sockfd)
{
	struct monitor_protocol req;
	memset(&req, 0, sizeof(req));

	stdin = freopen(NULL,"r",stdin);
	printf("which? ");
	req.code = getchar();

	if (req.code=='a') {
		req.pid = -1;
	} else if ('a'<req.code && req.code<'k') {
		stdin = freopen(NULL,"r",stdin);
		printf("pid? ");
		if(scanf("%d", &req.pid))
			while(getchar()!='\n');
	} else if (req.code == 'k') {
		close(sockfd);
		exit(1);
	}

	if (write(sockfd, &req, sizeof(req)) < 0) {
		perror("ERROR while writing to server");
		exit(1);
	}

	printf("\n");
}

void getResponse(int sockfd)
{
	int n;
	struct data_package pkg;
	memset(&pkg, 0, sizeof(pkg));

	while ( (n=read(sockfd,&pkg,sizeof(pkg))) ) {
		printf("%s", pkg.msg);
		if(pkg.flag==0) break;
	}
	if( n==0 ) {
		perror("ERROR : Disconnected from server");
		close(sockfd);
		exit(1);
	} else if(n==-1) {
		perror("ERROR : while reading from server");
		close(sockfd);
		exit(1);
	}

	printf("\n\n");

}
