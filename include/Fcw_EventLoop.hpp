#pragma once
#include "Fcw_Poller.hpp"
#include "Fcw_TimerWheel.hpp"
#include "Fcw_Log.hpp"
#include <thread>
#include <memory>
#include <functional>
#include <mutex>
#include <sys/eventfd.h>
#include <condition_variable>

class Channel;
class EventLoop
{
private:
    using Functor = std::function<void()>;
    std::thread::id _threadid;              // 线程ID
    int _eventfd;                           // eventfd唤醒IO事件监控有可能导致的阻塞
    std::vector<Functor> _tasks;            // 任务池
    std::mutex _mutex;                      // 实现任务池操作的线程安全
    std::unique_ptr<Channel> _eventChannel; //事件投递模块
    Poller _poller;                         // 事件监控模块
    TimerWheel _timerWheel;                 //定时器模块
public:
    // 执⾏任务池中的所有任务
    void runAllTask();
    //这个函数的作用就是让别的线程能够唤醒此线程，不要阻塞在epoll_wait
    static int createEventFd();
    //read让重置通知
    void readEventfd();
    // 唤醒eventfd
    void weakupEventFd();

public:
    EventLoop();
    // 三步⾛--事件监控-》就绪事件处理-》执⾏任务
    void start();
    // ⽤于判断当前线程是否是EventLoop对应的线程；
    bool isInLoop();
    void assertInLoop();
    // 判断将要执⾏的任务是否处于当前线程中，如果是则执⾏，不是则压⼊队列。
    void runInLoop(const Functor &cb);
    // 将操作压⼊任务池
    void queueInLoop(const Functor &cb);
    // 添加/修改描述符的事件监控
    void updateEvent(Channel *channel);
    // 移除描述符的监控
    void removeEvent(Channel *channel);

    //定时器的功能
    void timerAdd(uint64_t id, uint32_t delay, const onTimerCallback &cb) ;
    void timerRefresh(uint64_t id);
    void timerCancel(uint64_t id) ;
    bool hasTimer(uint64_t id) ;
};

class LoopThread
{
private:
    /*⽤于实现_loop获取的同步关系，避免线程创建了，但是_loop还没有实例化之前去获取_loop*/
    std::mutex _mutex;             // 互斥锁
    std::condition_variable _cond; // 条件变量
    EventLoop* _loop;              // EventLoop指针变量，这个对象需要在线程内实例化
    std::thread _thread;           // EventLoop对应的线程
private:
    /*实例化 EventLoop 对象，唤醒_cond上有可能阻塞的线程，并且开始运⾏EventLoop模块的功能*/
    void threadEntry();

public:
    /*创建线程，设定线程⼊⼝函数*/
    LoopThread();
    /*返回当前线程关联的EventLoop对象指针*/
    EventLoop* getLoop();
};
class LoopThreadPool
{
private:
    int _threadCount;
    int _nextIndex;
    EventLoop *_baseloop;
    std::vector<LoopThread*> _threads;
    std::vector<EventLoop*>  _loops;

public:
    LoopThreadPool(EventLoop *baseloop) ;
    void setThreadCount(int count);
    void create();
    EventLoop *nextLoop();
};