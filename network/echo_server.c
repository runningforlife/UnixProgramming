#include "socket_header.h"

const static int SERVER_PORT = 2100;
const static int MAX_PENDING = 10;
const static int MAX_RCV_BUF = 256;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char buf[MAX_RCV_BUF];

    int socket_fd;
    struct sockaddr_in sa;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("fail to create socket");
        exit(EXIT_FAILURE);   
    }

    sa.sin_family = AF_INET;
    /** htonl/htons 将host字节序转为network字节序 */
    sa.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY表示该端口可接受任何入境消息 */
    sa.sin_port = htons(atoi(argv[1]));

    /* 将socket与地址进行绑定*/
    if (bind(socket_fd, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
        perror("fail to bind socket address\n");   
        goto errout;
    }

    /* 监听socket */
    if (listen(socket_fd, MAX_PENDING) < 0) {
        perror("fail to listen port\n");  
        goto errout; 
    }

    printf("server is started\n");

    for(; ;)
    {
        /* accept any requests from clients */
        int connect_fd = accept(socket_fd, NULL, NULL);
        if (connect_fd < 0) {
            perror("fail to accept");
            goto errout;      
        }

        /* keep receving message from client */
        while(1) {
            if (read(connect_fd, buf, MAX_RCV_BUF) < 0) {
                perror("fail to read");  
                break;         
            }

            buf[strlen(buf)] = 0;
            printf("Client: %s\n", buf);
        
            if (write(connect_fd, buf, strlen(buf)) < 0) {
                perror("fail to send to client\n");
                break;
            }

            if (strncmp(buf, "quit", 4) == 0) {
                printf("talk is done*_*\n");
                break;
            }
        }

        close(connect_fd);
    }

errout:
    close(socket_fd);
    exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}
