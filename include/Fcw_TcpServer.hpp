#pragma once 
#include "Fcw_Log.hpp"
#include  "Fcw_Channel.hpp"
#include "Fcw_Acceptor.hpp"
#include "Fcw_Connection.hpp"

class TcpServer
{
private:
    uint64_t _nextid;             // 这是⼀个⾃动增⻓的连接ID，
    int _port;
    int _timeout;                  // 这是⾮活跃连接的统计时间---多⻓时间⽆通信就是⾮活跃连接
    bool _enableInactiveRelease;   // 是否启动了⾮活跃连接超时销毁的判断标志
    EventLoop _baseloop;           // 这是主线程的EventLoop对象，负责监听事件的处理
    Acceptor _acceptor;            // 这是监听套接字的管理对象
    LoopThreadPool _threadpool;    // 这是从属EventLoop线程池
    
    // 保存管理所有连接对应的shared_ptr对象
    std::unordered_map<uint64_t, PtrConnection> _conns;               
    using connectedCallback = std::function<void(const PtrConnection &)>;
    using messageCallback = std::function<void(const PtrConnection &,Buffer *)>;
    using closedCallback = std::function<void(const PtrConnection &)>;
    using anyEventCallback = std::function<void(const PtrConnection &)>;
    using functor = std::function<void()>;
    connectedCallback _connectedCallback;
    messageCallback _messageCallback;
    closedCallback _closedCallback;
    anyEventCallback _eventCallback;

private:
    void runAfterInLoop(const functor &task, int delay)
    {
        _nextid++;
        _baseloop.timerAdd(_nextid, delay, task);
    }

    // 为新连接构造⼀个Connection进⾏管理
    void newConnection(int fd)
    {
        _nextid++;
        PtrConnection conn(new Connection(_threadpool.nextLoop(), _nextid, fd));
        conn->setMessageCallback(_messageCallback);
        conn->setClosedCallback(_closedCallback);
        conn->setConnectedCallback(_connectedCallback);
        conn->setAnyEventCallback(_eventCallback);
        conn->setSrvClosedCallback(std::bind(&TcpServer::removeConnection,this, std::placeholders::_1));
        // 启动⾮活跃超时销毁
        if (_enableInactiveRelease)
            conn -> enableInactiveRelease(_timeout);
        conn->established(); // 就绪初始化
        _conns.insert(std::make_pair(_nextid, conn));
    }
    void removeConnectionInLoop(const PtrConnection &conn)
    {
        int id = conn->getConnectid();
        auto it = _conns.find(id);
        if (it != _conns.end())
        {
            _conns.erase(it);
        }
    }
    // 从管理Connection的_conns中移除连接信息
    void removeConnection(const PtrConnection &conn)
    {
        _baseloop.runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this, conn));
    }

public:
    TcpServer(int port) : _port(port),
                          _nextid(0),
                          _enableInactiveRelease(false),
                          _acceptor(&_baseloop, port),
                          _threadpool(&_baseloop)
    {
        _acceptor.setAcceptCallback(std::bind(&TcpServer::newConnection,this, std::placeholders::_1));
        _acceptor.listen(); // 将监听套接字挂到baseloop上
    }
    void setThreadCount(int count) { 
        return _threadpool.setThreadCount(count); 
    }
    void setConnectedCallback(const connectedCallback &cb){
        _connectedCallback = cb;
    }
    void setMessageCallback(const messageCallback &cb) { 
        _messageCallback = cb; 
    }
    void setClosedCallback(const closedCallback &cb) { 
        _closedCallback =cb;
    }
    void setAnyEventCallback(const anyEventCallback &cb) { 
        _eventCallback = cb;
    }
    void enableInactiveRelease(int timeout){
        _timeout = timeout;
        _enableInactiveRelease = true;
    }
    // ⽤于添加⼀个定时任务
    void runAfter(const functor &task, int delay){
        _baseloop.runInLoop(std::bind(&TcpServer::runAfterInLoop, this, task, delay));
    }
    void start(){
        _threadpool.create();
        _baseloop.start();
    }
};