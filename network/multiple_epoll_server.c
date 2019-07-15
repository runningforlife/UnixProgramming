#include "socket_header.h"
#include<pthread.h>
#include<sys/epoll.h>

#define MAX_PORTS 20
#define MAX_BUF_SIZE 256


int createServerSocket(int port);
int acceptConnecton(int sockFd);
void* handleRequest(void* arg);

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: %s port1 port2 port3 ...\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    struct epoll_event polledEv[MAX_PORTS], readyEv[MAX_PORTS];

    int i;
    for (i = 0; i < argc; ++i) {
        polledEv[i].data.fd = -1;
    }

    int epollFd = epoll_create1(0);
    if (epollFd < 0) {
        perror("fail to create epoll for stdin");
        exit(EXIT_FAILURE);
    }

    polledEv[0].data.fd = STDIN_FILENO;
    polledEv[0].events = EPOLLIN;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &polledEv[0]) < 0) {
        perror("epoll_ctl: stdin");
        exit(EXIT_FAILURE);
    }

    for (i = 1; i < argc; ++i) {
        int sockFd = createServerSocket(atoi(argv[i]));
        if (sockFd < 0) {
           perror("fail to create socket");
           continue; 
        } 

        polledEv[i].data.fd = sockFd;
        polledEv[i].events = EPOLLIN;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, sockFd, &polledEv[i]) < 0) {
            perror("epoll_ctl");
            close(sockFd);
            exit(EXIT_FAILURE);
        }
    }

    int running = 1;
    while (running) {
        int ready = epoll_wait(epollFd, readyEv, MAX_PORTS, 5);
        if (ready < 0) {
            perror("epoll_wait");
            goto errout;
        } else if (ready == 0) {
            continue;
        } else {
            for (i = 0; i < ready; ++i) {
                int fd = acceptConnecton(readyEv[i].data.fd);
                if (fd < 0) {
                    perror("fail to accept request");
                    continue;
                } 

                pthread_t tid;
                if (pthread_create(&tid, NULL, handleRequest, (void*)&fd) != 0) {
                    perror("fail to create handle thread");
                    close(fd);
                    goto errout;
                }
            }
        }
    }

errout:
    for (i = 0; i < argc; ++i) {
        close(polledEv[i].data.fd);
    }
    exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}

int createServerSocket(int port)
{
    if (port < 0) return -1;

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("fail to create server socket");
        return sockfd;
    }

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("fail to bind socket");
        return -1;
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen()");
        return -1;
    }

    printf("server started on port %d\n", port);

    return sockfd;
}

int acceptConnecton(int sockfd) 
{
    return accept(sockfd, NULL, NULL);
}

void* handleRequest(void* arg)
{
    int fd = *(int*)arg;

    if (fd < 0) {
        perror("invalid socket fd");
        pthread_exit(NULL);
    }

    pthread_detach(pthread_self());

    printf("handle request over %d\n", fd);

    char buf[MAX_BUF_SIZE];

    while(1) {
        memset(buf, 0, MAX_BUF_SIZE);

        if (recv(fd, buf, MAX_BUF_SIZE, 0) < 0) {
            perror("recv()");
            continue;
        }

        printf("Client: %s\n", buf);

        if (strcmp(buf, "quit") == 0 || strcmp(buf, "q") == 0) {
            printf("Client disconnection, exit\n");
            close(fd);
            pthread_exit(NULL);
        }

        int len = strlen(buf);
        if (send(fd, buf, len, 0) != len) {
            perror("send()");
            continue;
        }
    }

    //pthread_exit(NULL);
}