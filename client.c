#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int sockfd;

// 接收线程：持续打印服务器发来的消息
void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    int len;
    while ((len = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[len] = '\0';
        printf("%s", buffer);
        fflush(stdout);   // 强制立即输出
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("用法: ./client 服务器IP\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }

    char name[32];
    printf("请输入昵称: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    send(sockfd, name, strlen(name), 0);

    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_messages, NULL);
    pthread_detach(recv_tid);

    char buffer[BUFFER_SIZE];
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "/quit") == 0) break;
        send(sockfd, buffer, strlen(buffer), 0);
    }

    close(sockfd);
    return 0;
}