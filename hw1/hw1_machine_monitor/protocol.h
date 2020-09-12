#ifndef HW1_PROTCL_H
#define HW1_PROTCL_H

#define PORT 8000
#define ADDRESS "127.0.0.1"

struct monitor_protocol {
	int pid;
	char code;
};

struct data_package {
	int flag;
	char msg[64];
};

#endif
