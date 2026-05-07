#pragma once
#include <functional>
#include <sys/epoll.h>
class EventLoop;
class Poller;
class Channel
{
private:
    int _fd;
    EventLoop *_loop;
    uint32_t _events;  // 当前需要监控的事件
    uint32_t _revents; // 当前就绪后的事件
    using EventCallback = std::function<void()>;
    EventCallback _readCallback;  // 可读事件被触发的回调函数
    EventCallback _writeCallback; // 可写事件被触发的回调函数
    EventCallback _errorCallback; // 错误事件被触发的回调函数
    EventCallback _closeCallback; // 连接断开事件被触发的回调函数
    EventCallback _eventCallback; // 任意事件被触发的回调函数
public:
    Channel(EventLoop *loop, int fd);
    int getFd();
    // 获取想要监控的事件，Poller会根据这个事件来注册fd
    uint32_t getEvents();

    // 设置实际就绪的事件，就绪之后需要注册回来 
    void setREvents(uint32_t events);
    void setReadCallback(const EventCallback &cb);
    void setWriteCallback(const EventCallback &cb);
    void setErrorCallback(const EventCallback &cb);
    void setCloseCallback(const EventCallback &cb);
    void setEventCallback(const EventCallback &cb);
    // 当前是否监控了可读
    bool readAble();
    // 当前是否监控了可写
    bool writeAble();
    // 移除监控
    void remove();

    // 更新事件监听
    void update();


    //启动事件监听，调用Poller的update
    // 启动读事件监控
    void enableRead();
    // 启动写事件监控
    void enableWrite();
    // 关闭读事件监控
    void disableRead();
    // 关闭写事件监控
    void disableWrite();
    // 关闭所有事件监控
    void disableAll();
  

    // 事件处理，⼀旦连接触发了事件，就调⽤这个函数，⾃⼰触发了什么事件如何处理⾃⼰决定 
    void handleEvent();
};
