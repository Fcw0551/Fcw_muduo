#pragma once
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include "Fcw_Log.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include "Fcw_NoBlock.hpp"

#define MAC_LISTEN 5
class Socket{
    private:
        int _sockfd;
    public:
    Socket();
    Socket(int sockfd);

    int getfd();
    bool createTcp();
    //bind
    bool bindTcp(const std::string &ip,size_t port=8080);
    //服务端监听套接字
    bool listenTcp(int backlog=MAC_LISTEN);
    //客户端调用连接服务器
    bool connectTcp(const std::string &ip,size_t port=8080);

    //服务端调用
    //输入输出型参数，可以通过此参数拿到客户端的ip+端口
    int acceptTcp(struct sockaddr_in *addr=NULL);
    //接收数据
    ssize_t recv(void*buf,size_t len,int flag=0);
    ssize_t recvNoBlock(void*buf,size_t len);
    // 发送数据
    ssize_t send(const void *buf, size_t len, int flag = 0);
    ssize_t sendNoBlock(void *buf, size_t len);
    // 关闭套接字
    void close();
    // 设置套接字阻塞属性-- 设置为⾮阻塞
    void NonBlock();


    // 创建⼀个服务端连接
    bool createServer(uint16_t port,const std::string &ip = "0.0.0.0",bool block_flag = false);
    // 创建⼀个客⼾端连接
    bool createClient(uint16_t port, const std::string &ip);
    // 设置套接字选项---开启地址端⼝重⽤
    void reuseAddress();
};