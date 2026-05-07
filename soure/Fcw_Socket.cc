#include "../include/Fcw_Socket.hpp"

    Socket::Socket()
    :_sockfd(-1){
    }
    Socket::Socket(int sockfd)
    :_sockfd(sockfd){
    }

    int Socket::getfd(){
        return _sockfd;
    }
    bool Socket::createTcp(){
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(_sockfd < 0){
            ERR_LOG("Create socket error");
            return false;
        }
        return true;
    }
    //bind
    bool Socket::bindTcp(const std::string &ip,size_t port){
        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        if(bind(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
            ERR_LOG("Bind error");
            return false;
        }
        return true;
    }
    //服务端监听套接字
    bool Socket::listenTcp(int backlog){
        if(listen(_sockfd, backlog)<0){
            ERR_LOG("Listen error");
            return false;
        }
        return true;
    }
    //客户端调用连接服务器
    bool Socket::connectTcp(const std::string &ip,size_t port){
        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        if(connect(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
            ERR_LOG("Connect error");
            return false;
        }
        return true;
    }

    //服务端调用
    //输入输出型参数，可以通过此参数拿到客户端的ip+端口
    int Socket::acceptTcp(struct sockaddr_in *addr){
        bzero(&addr, sizeof(addr));
        socklen_t len = sizeof(addr);
        int newfd = accept(_sockfd, (struct sockaddr*)&addr, &len);
        if(newfd < 0){
            ERR_LOG("Accept error");
            return -1;
        }
        return newfd;
    }
    //接收数据
    ssize_t Socket::recv(void*buf,size_t len,int flag){
        // int ret=::recv(_sockfd, buf, len, flag);
        // if(ret <= 0){
        //     if(errno == EAGAIN||errno==EINTR){
        //         //1、非阻塞中缓冲区无数据
        //         //2、被信号打断
        //         return 0;
        //     }
        //     ERR_LOG("Recv error");
        //     return -1;
        // }
        // return ret;
        while (true)
        {
            ssize_t ret = ::recv(_sockfd, buf, len, flag);
            if (ret > 0)
            {
                return ret; // 成功读取
            }
            if (ret == 0)
            {
                return 0; // 对端正常关闭
            }
            // ret < 0
            if (errno == EINTR)
            {
                continue; // 被信号中断，重试
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return 0; // 非阻塞模式下暂无数据
            }
            // 真实错误
            ERR_LOG("Recv error: %s", strerror(errno));
            return -1;
        }
    }
    ssize_t Socket::recvNoBlock(void*buf,size_t len){
        return Socket::recv(buf,len,MSG_DONTWAIT);
    }
    // 发送数据
    ssize_t Socket::send(const void *buf, size_t len, int flag ){
        // ssize_t send(int sockfd, void *data, size_t len, int flag);
        ssize_t ret =::send(_sockfd, buf, len, flag);
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            ERR_LOG("SOCKET SEND FAILED!!");
            return -1;
        }
        return ret; // 实际发送的数据⻓度
    }
    ssize_t Socket::sendNoBlock(void *buf, size_t len){
        if (len == 0)
            return 0;
        return Socket::send(buf, len, MSG_DONTWAIT); // MSG_DONTWAIT 表⽰当前发送为⾮阻塞
    }
    // 关闭套接字
    void Socket::close(){
        if (_sockfd != -1)
        {
            ::close(_sockfd);
            _sockfd = -1;
        }
    }
    // 设置套接字阻塞属性-- 设置为⾮阻塞
    void Socket::NonBlock()
    {
        setNoBlock(_sockfd);
    }


    // 创建⼀个服务端连接
    bool Socket::createServer(uint16_t port,const std::string &ip,bool block_flag ){
        // 1. 创建套接字，2. 绑定地址，3. 开始监听，4. 设置⾮阻塞， 5. 启动地址重⽤
        if (createTcp() == false)
            return false;
        if (block_flag)
            NonBlock();
        reuseAddress();
        if (bindTcp(ip, port) == false)
            return false;
        if (listenTcp() == false)
            return false;
        return true;
    }
    // 创建⼀个客⼾端连接
    bool Socket::createClient(uint16_t port, const std::string &ip)
    {
        // 1. 创建套接字，2.指向连接服务器
        if (createTcp() == false)
            return false;
        if (connectTcp(ip, port) == false)
            return false;
        return true;
    }
    // 设置套接字选项---开启地址端⼝重⽤
    void Socket::reuseAddress()
    {
        // int setsockopt(int fd, int leve, int optname, void *val, intvallen)
        int val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val,sizeof(int));
        val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val,sizeof(int));
    }