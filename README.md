# TCP 聊天室

这是一个用 C 语言实现的多线程 TCP 聊天室程序。

## 功能
- 支持多客户端同时在线聊天
- 服务端广播消息给所有在线客户端
- 客户端之间支持私聊功能
- 基于 pthread 实现多线程并发

## 使用方法
1. 编译服务端：`gcc server.c -o server -lpthread`
2. 编译客户端：`gcc client.c -o client -lpthread`
3. 先运行服务端：`./server`
4. 再运行客户端：`./client 127.0.0.1`
