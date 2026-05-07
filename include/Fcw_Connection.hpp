#pragma once 
#include "Fcw_Socket.hpp"
#include "Fcw_Buffer.hpp"
#include "Fcw_Channel.hpp"
#include "Fcw_EventLoop.hpp"
#include "Fcw_Log.hpp"
#include "Fcw_Any.hpp"
#include <memory>

class Connection;
// DISCONECTED -- 连接关闭状态； CONNECTING -- 连接建⽴成功-待处理状态
// CONNECTED -- 连接建⽴完成，各种设置已完成，可以通信的状态； DISCONNECTING -- 待关闭状态
typedef enum
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
} ConnStatu;

using PtrConnection = std::shared_ptr<Connection>;

//此类的作用依托整个系统，对外提供服务，外界通过这个类去跟底层的eventLoop沟通
class Connection : public std::enable_shared_from_this<Connection>
{
private:
    uint64_t _connid;              // 连接的唯⼀ID，便于连接的管理和查询
    // uint64_t _timer_id;         // 定时器ID，必须是唯⼀的，这块为了简化操作使⽤conn_id作为定时器ID
    int _sockfd;                   // 连接关联的⽂件描述符
    bool _enableInactiveRelease;   // 连接是否启动⾮活跃销毁的判断标志，默认为false ,是否要加入定时器
    EventLoop *_loop;              // 连接所关联的⼀个EventLoop
    ConnStatu _statu;              // 连接状态
    Socket _socket;                // 套接字操作管理
    Channel _channel;              // 连接的事件管理
    Buffer _inBuffer;             // 输⼊缓冲区---存放从socket中读取到的数据
    Buffer _outBuffer;            // 输出缓冲区---存放要发送给对端的数据
    Any _context;                  // 请求的接收处理上下⽂
   
    /*这四个回调函数，是让服务器模块来设置的（其实服务器模块的处理回调也是组件使⽤者设置的）*/
    /*换句话说，这⼏个回调都是组件使⽤者使⽤的*/
    using ConnectedCallback = std::function<void(const PtrConnection &)>;
    using MessageCallback = std::function<void(const PtrConnection &,Buffer*)>;
    using ClosedCallback = std::function<void(const PtrConnection &)>;
    using AnyEventCallback = std::function<void(const PtrConnection &)>;
    ConnectedCallback _connectedCallback;
    MessageCallback _messageCallback;
    ClosedCallback _closedCallback;
    AnyEventCallback _eventCallback;
    /*组件内的连接关闭回调--组件内设置的，因为服务器组件内会把所有的连接管理起来，⼀旦某个连接要关闭*/
    /*就应该从管理的地⽅移除掉⾃⼰的信息*/
    ClosedCallback _serverClosedCallback;

private:
    /*五个channel的事件回调函数*/

    // 描述符可读事件触发后调⽤的函数，接收socket数据放到接收缓冲区中，然后调⽤_messageCallback 
    void handleRead();
    // 描述符可写事件触发后调⽤的函数，将发送缓冲区中的数据进⾏发送
    void handleWrite();
    
    // 描述符触发挂断事件
    void handleClose();
    
    // 描述符触发任意事件: 
    //1. 刷新连接的活跃度--延迟定时销毁任务； 2. 调⽤组件使⽤者的任意事件回调
    void handleEvent();
   
    void handleError();
    // 连接获取之后，所处的状态下要进⾏各种设置（启动读监控,调⽤回调函数）
    void establishedInLoop();
    // 这个接⼝才是实际的释放接⼝
    void releaseInLoop();
    
    // 这个接⼝并不是实际的发送接⼝，⽽只是把数据放到了发送缓冲区，启动了可写事件监控
    void sendInLoop(Buffer &buf);
    // 这个关闭操作并⾮实际的连接释放操作，需要判断还有没有数据待处理，待发送
    void shutdownInLoop();
    // 启动⾮活跃连接超时释放规则
    void enableInactiveReleaseInLoop(int sec);
    //取消非活跃链接
    void cancelInactiveReleaseInLoop();
    void upgradeInLoop(const Any &context,const ConnectedCallback &conn,
                       const MessageCallback &msg,
                       const ClosedCallback &closed,
                       const AnyEventCallback &event);

public:
    Connection(EventLoop *loop, uint64_t connid, int sockfd);
    ~Connection();
    // 获取sockfd描述符
    int getfd();
    // 获取连接ID
    uint64_t getConnectid() ;
    // 是否处于CONNECTED状态
    bool isConnected();
    // 设置上下⽂--连接建⽴完成时进⾏调⽤
    void setContext(const Any &context);
    // 获取上下⽂，返回的是指针
    Any *getContext();

    //业务函数的回调设置
    void setConnectedCallback(const ConnectedCallback &cb);
    void setMessageCallback(const MessageCallback &cb);
    void setClosedCallback(const ClosedCallback &cb);
    void setAnyEventCallback(const AnyEventCallback &cb) ;
    void setSrvClosedCallback(const ClosedCallback &cb);

    // 连接建⽴就绪后，投递到线程eventloop中，启动读监控，然后回调_connectedCallback
    void established();

    // 发送数据，将数据放到发送缓冲区，启动写事件监控
    void send(const char *data, size_t len);


    // 提供给组件使⽤者的关闭接⼝--并不实际关闭，需要判断有没有数据待处理
    void shutdown();
    void release();
    
    // 启动⾮活跃销毁，并定义多⻓时间⽆通信就是⾮活跃，添加定时任务
    void enableInactiveRelease(int sec);

    // 取消⾮活跃销毁
    void cancelInactiveRelease();

    // 切换协议---重置上下⽂以及阶段性回调处理函数 -- 但是这个接⼝必须在EventLoop线程中⽴即执⾏
    // 防备新的事件触发后，处理的时候，切换任务还没有被执⾏--会导致数据使⽤原协议处理了。
    void upgrade(const Any &context, const ConnectedCallback &conn, const MessageCallback &msg,
                 const ClosedCallback &closed, const AnyEventCallback &event);
};
