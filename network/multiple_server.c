#include "socket_header.h"
#include<pthread.h>

#define MAX_PORTS 100
#define DEFAULT_TIMEOUT 15 //15s
#define MAX_BUF_SIZE 256

int createServerSocket(int port);
int acceptConnection(int sockFd);
void handleRequest(void*);
void fatal(char* error);

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage %s <port_1> <port_2> ... \n", argv[0]);
        exit(0);
    }

    /* io multiplex */
    fd_set sockset;
    struct timeval timeout;
    /* is server running */
    int running = 1;
    /* max file descriptors of socket */
    int maxFd = -1; 
    int* serverSock;
    int portNum = 0;

    serverSock = (int*)malloc((argc - 1) * sizeof(int));

    for (int i = 1; i < argc; ++i) {
        int port = atoi(argv[i]);
        serverSock[portNum++] = createServerSocket(port);
        maxFd = serverSock[portNum-1] > maxFd ? serverSock[portNum-1] : maxFd;
    }

    printf("server started, hit RETURN to shotdown\n");
    while(running) {
        /* this must be reset for every time select() is called */
        FD_ZERO(&sockset);
        FD_SET(STDIN_FILENO, &sockset);

        for (int p = 0; p < portNum; ++p) {
            FD_SET(serverSock[p], &sockset);
        }

        /* timeout for select IO */
        /* must be called every time select() is called */
        timeout.tv_sec = DEFAULT_TIMEOUT;
        timeout.tv_usec = 0;

        if (select(maxFd + 1, &sockset, NULL, NULL, &timeout) < 0) {
            printf("fail to get ready fd for %d seconds, waiting...again", DEFAULT_TIMEOUT);
        } else {
            if (FD_ISSET(STDIN_FILENO, &sockset)) {
                printf("server is gonna to shut down\n");
                running = 0;
            }

            for (int p = 0; p < portNum; ++p) {
                if (FD_ISSET(serverSock[p], &sockset)) {
                    pthread_t tid;
                    int clientFd = acceptConnection(serverSock[p]);

                    pthread_create(&tid, NULL, (void*)handleRequest, (void*)&clientFd);
                    //handleRequest(acceptConnection(serverSock[p]));
                }
            }
        }
    }

    // reclaim resources
    for (int p = 0; p < portNum; ++p) close(serverSock[p]);
    free(serverSock);
    exit(0);
}

void fatal(char* error) 
{
    perror(error);
    exit(EXIT_FAILURE);
}

int createServerSocket(int port) 
{
    struct sockaddr_in serverAddr;
    int sockfd = -1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fatal("fail to create socket\n");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(sockfd);
        fatal("fail to bind socket\n");
    }

    if (listen(sockfd, 10) < 0) {
        close(sockfd);
        fatal("fail to listen socket");
    }

    return sockfd;
}

int acceptConnection(int sockfd)
{
    return accept(sockfd, NULL, NULL);
}

void handleRequest(void* args)
{
    int fd = *((int*)args);
    if (fd < 0) {
        pthread_exit(NULL);
    }

    printf("handleRequest(): from fd=%d\n", fd);

    char buf[MAX_BUF_SIZE];

    while(1) {
        if (recv(fd, buf, MAX_BUF_SIZE, 0) < 0) {
            printf("fail to receive message from client\n");
            break;
        }

        printf("Client(%d): %s\n", fd, buf);

        int len = strlen(buf);
        buf[len] = 0;
        if (send(fd, buf, len, 0) != len) {
            printf("fail to echo");
            break;
        }
    }

    close(fd);
    pthread_exit(NULL);
}