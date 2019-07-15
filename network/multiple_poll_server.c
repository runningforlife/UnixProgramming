#include "socket_header.h"
#include<pthread.h>
#include<poll.h>

#define MAX_PORTS  20
#define MAX_BUF_SIZE 256
#define MAX_POLL_TIMEOUT 5


int createServerSocket(int port);
int acceptConnection(int sock);
void* handleRequst(void* arg);


int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: %s port1, port2, ...", argv[0]);
        exit(EXIT_SUCCESS);
    }

    struct pollfd pollFds[MAX_PORTS];

    pollFds[0].fd = STDIN_FILENO;
    pollFds[0].events = POLLIN;

    int maxFds = argc;
    int i;
    for (i = 1; i < argc; ++i) {
        pollFds[i].fd = createServerSocket(atoi(argv[i]));
        pollFds[i].events = POLLIN;
    }


    char buf[MAX_BUF_SIZE];

    int running = 1;
    while (running) {
        int ready = poll(pollFds, maxFds, MAX_POLL_TIMEOUT);

        if (ready < 0) {
            perror("fail to poll");
            goto errout;
        } else if (ready == 0) {
            //printf("timeout\n");
            continue;
        } else {
            memset(buf, 0, MAX_BUF_SIZE);

            for (i = 0; i < maxFds; ++i) {
                if (pollFds[i].fd > 0 && (pollFds[i].events & POLLIN)) {
                    if (pollFds[i].fd == STDIN_FILENO) {
                        // read from stdin stream
                        if (fgets(buf, MAX_BUF_SIZE, stdin) == NULL) {
                            perror("fail to read stdin");
                            continue;
                        }

                        int len = strlen(buf);
                        if (send(pollFds[i].fd, buf, len , 0) != len) {
                            perror("fail to send data");
                            continue;
                        }
                    } else {
                        int fd = acceptConnection(pollFds[i].fd);
                        if (fd < 0) {
                            perror("fail to accept connection");
                        } else {
                            pthread_t tid;
                            if (pthread_create(&tid, NULL, handleRequst, (void*)&fd) != 0) {
                                perror("fail to create handle thread");
                                close(fd);
                                goto errout;
                            }
                        }
                    }
                }
            }
        }
    }

errout:
    for (i = 0; i < maxFds; ++i) {
        close(pollFds[i].fd);
    }
    exit(EXIT_FAILURE);

    exit(0);
}

int createServerSocket(int port) 
{
    if (port < 0) return -1;

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("fail to create socket");
        return -1;
    }

    struct sockaddr_in sockaddr;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("fail to bind");
        return -1;
    }

    if (listen(sockfd, 5) < 0) {
        perror("fail to listen");
        return -1;
    }

    return sockfd;
}

int acceptConnection(int sock)
{
    return accept(sock, NULL, NULL);
}

void* handleRequst(void* arg)
{
    int sockfd = *((int*)arg);
    if (sockfd < 0) {
        perror("invalid socket fd");
        pthread_exit(NULL);
    }

    pthread_detach(pthread_self());

    printf("handle request over port = %d\n", sockfd);

    char buf[MAX_BUF_SIZE];

    while(1) {
        memset(buf, 0, MAX_BUF_SIZE);

        if (recv(sockfd, buf, MAX_BUF_SIZE, 0) < 0) {
            perror("fail to receive");
            break;
        }    

        printf("Client: %s\n", buf);

        if (strcmp(buf, "quit") == 0 || strcmp(buf, "q") == 0) {
            printf("Client disconnection, exit...");
            close(sockfd);
            pthread_exit(NULL);
        }

        int len = strlen(buf);
        buf[len] = 0;
        if (len > 0 && send(sockfd, buf, len , 0) != len) {
            perror("fail to send data");
            break;
        }        
    }

    close(sockfd);
    pthread_exit(NULL);
}