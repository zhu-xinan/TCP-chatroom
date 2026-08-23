#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

// 客户端节点（链表）
typedef struct Client {
    int sockfd;
    char name[32];
    struct Client* next;
} Client;

Client* head = NULL;          // 客户端链表头
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  // 互斥锁

// 广播消息给所有客户端
void broadcast(char* msg, int sender_fd) {
    pthread_mutex_lock(&lock);
    Client* cur = head;
    while (cur) {
        if (cur->sockfd != sender_fd) {
            send(cur->sockfd, msg, strlen(msg), 0);
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&lock);
}

// 私聊消息
void private_message(char* msg, int sender_fd) {
    // 格式：@username 消息内容
    char target_name[32];
    char content[BUFFER_SIZE];
    if (sscanf(msg, "@%s %[^\n]", target_name, content) != 2) {
        char* err = "格式错误：请用 @用户名 消息内容\n";
        send(sender_fd, err, strlen(err), 0);
        return;
    }

    pthread_mutex_lock(&lock);
    Client* cur = head;
    while (cur) {
        if (cur->sockfd != sender_fd && strcmp(cur->name, target_name) == 0) {
            char buffer[BUFFER_SIZE];
            sprintf(buffer, "[私聊] %s\n", content);
            send(cur->sockfd, buffer, strlen(buffer), 0);
            pthread_mutex_unlock(&lock);
            return;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&lock);

    char* err = "用户不存在\n";
    send(sender_fd, err, strlen(err), 0);
}

// 处理单个客户端
void* handle_client(void* arg) {
    int sockfd = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char name[32];
    int len;

    // 第一步：接收用户名
    len = recv(sockfd, name, sizeof(name) - 1, 0);
    if (len <= 0) {
        close(sockfd);
        return NULL;
    }
    name[len] = '\0';

    // 加入链表
    Client* new_client = (Client*)malloc(sizeof(Client));
    new_client->sockfd = sockfd;
    strcpy(new_client->name, name);
    new_client->next = NULL;

    pthread_mutex_lock(&lock);
    new_client->next = head;
    head = new_client;
    pthread_mutex_unlock(&lock);

    // 广播上线通知
    char join_msg[BUFFER_SIZE];
    sprintf(join_msg, "[系统] %s 进入聊天室\n", name);
    broadcast(join_msg, sockfd);

    // 主循环：接收并转发消息
    while ((len = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[len] = '\0';

        // 私聊指令检测
        if (buffer[0] == '@') {
            private_message(buffer, sockfd);
        } else {
            // 群聊：带上用户名
            char full_msg[BUFFER_SIZE + 32];
            sprintf(full_msg, "%s: %s", name, buffer);
            broadcast(full_msg, sockfd);
        }
    }

    // 客户端断开
    close(sockfd);

    // 从链表移除
    pthread_mutex_lock(&lock);
    Client* cur = head;
    if (cur == new_client) {
        head = cur->next;
    } else {
        while (cur && cur->next != new_client) cur = cur->next;
        if (cur) cur->next = new_client->next;
    }
    pthread_mutex_unlock(&lock);
    free(new_client);

    char quit_msg[BUFFER_SIZE];
    sprintf(quit_msg, "[系统] %s 已退出\n", name);
    broadcast(quit_msg, sockfd);

    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("聊天室服务器已启动，端口: %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int* client_fd = (int*)malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (*client_fd < 0) {
            perror("accept");
            free(client_fd);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_fd);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}