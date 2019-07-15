// simple client
#include "socket_header.h"


/* client side */

const static char* SERVER_ADDR = "127.0.0.1";
const static int MAX_BUF_SIZE = 256;

void killHandler(int sigType);
void registerSigHandler();

int socket_fd;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    registerSigHandler();

    struct sockaddr_in sa;
    int res;

    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET; /* address familiy*/
    sa.sin_port = htons(atoi(argv[1])); /* address port */
    sa.sin_addr.s_addr = inet_addr(SERVER_ADDR); /* internet address */

    if (connect(socket_fd, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
        perror("connect failure");
        close(socket_fd);
        exit(0);
    }

    printf("Client: server is connected\n");

    char buf[MAX_BUF_SIZE];   
    while(1) {
        //scanf("%s\n", buf);
        fgets(buf, MAX_BUF_SIZE, stdin);
        if (strncmp(buf, "quit", 4) == 0 || strncmp(buf, "q", 1) == 0) {
            printf("quit\n");
            write(socket_fd, "quit", 4);
            break;       
        } else {
            printf("Client: %s\n", buf);
            int len = strlen(buf);
            if (send(socket_fd, buf, len, 0) != len) {
                printf("client: send message error\n");
                break;           
            }

            memset(buf, 0, MAX_BUF_SIZE);
            if (recv(socket_fd, buf, MAX_BUF_SIZE, 0) < 0) {
                printf("fail to receive message from server\n");
                break;
            }         
            printf("Server:%s", buf);
        }
    }

    close(socket_fd);

    return 0;
}

void killHandler(int sigType)
{
    printf("signal %d received\n", sigType);

    write(socket_fd, "quit", 4);
    if (socket_fd > 0) {
        close(socket_fd);
    }

    exit(0);
}

void registerSigHandler()
{
    struct sigaction sa;
    sa.sa_handler = killHandler;
    if (sigfillset(&sa.sa_mask) < 0) {
        printf("fail to mask signal\n");
    }

    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, 0) < 0) {
        printf("sigaction() fail\n");
    }
}
