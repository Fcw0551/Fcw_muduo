#include "../include/Fcw_Acceptor.hpp"
//主线程进行套接字的监听
    void Acceptor::handleRead(){
        int newfd = _listenfd.acceptTcp();
        if (newfd < 0){
            return;
        }
        if (_accept_callback)
            _accept_callback(newfd);
        
    }
    int Acceptor::cretateServer(uint16_t port){
        bool ret = _listenfd.createServer(port);
        assert(ret == true);
        return _listenfd.getfd();
    }

    Acceptor::Acceptor(EventLoop *loop,uint16_t port)
    :_loop(loop)
    ,_listenfd(cretateServer(port))
    ,_channel(loop, _listenfd.getfd())
    {
        _channel.setReadCallback(std::bind(&Acceptor::handleRead,this));
    }
    void Acceptor::setAcceptCallback(const AcceptCallback &cb) {
         _accept_callback = cb; }
    void Acceptor::listen(){
        _channel.enableRead();
    }