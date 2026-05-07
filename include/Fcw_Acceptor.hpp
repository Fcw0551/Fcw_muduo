#pragma once
#include "Fcw_Channel.hpp"
#include "Fcw_Socket.hpp"
#include "Fcw_Log.hpp"
#include <assert.h>
//主线程进行套接字的监听
class Acceptor{
    private:
    Socket _listenfd;
    EventLoop* _loop;
    Channel _channel;

using AcceptCallback = std::function<void(int)>;
AcceptCallback _accept_callback;
    void handleRead();
    int cretateServer(uint16_t port);

    public:
    Acceptor(EventLoop *loop,uint16_t port);
    void setAcceptCallback(const AcceptCallback &cb);
    void listen();
};