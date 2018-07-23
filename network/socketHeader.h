#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include<sys/select.h>
#include<sys/time.h>
#include<stddef.h>

#include<sys/un.h>
#include<sys/stat.h>
#include<errno.h>

/** a well known UNIX socket path */
#define SERVER_NAME  "/var/tmp/unix.socket"
