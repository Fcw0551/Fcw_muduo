#pragma once 
#include <sys/types.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Fcw_Log.hpp"
#include "Fcw_Channel.hpp"
class EventLoop;

// 定时任务类,使用bind 创建定时任务
using onTimerCallback = std::function<void()>;//到时执行任务
using releaseCallback = std::function<void()>;//清理任务
class Timer
{
private:
    int _timeout;                     // 当前定时器任务的延迟时间
    bool _cancel;                     // false表示任务正常执行，true表示任务被取消
    uint64_t _id;                     // 任务唯一标识
    onTimerCallback _timerCallback;   // 定时任务的回调函数
    releaseCallback _releaseCallback; // 定时任务的释放函数，释放一些信息
public:
    Timer(uint64_t id, int timeout, const onTimerCallback &timerCallback = nullptr);
    ~Timer();
    // 获取定时任务的定时时间
    int getTimeout();
    // 取消定时任务
    void cancel();
    // 获取定时任务id
    uint64_t getId();
    // 设置定时任务的回调函数
    void setOnTimerCallback(const onTimerCallback &callback);
    // 设置定时任务的释放函数
    void setReleaseCallback(const releaseCallback &callback);
};

#define MAX_TIMEOUT 60 // 表示时间轮的最大长度
class TimerWheel{
private:
    using weakTimer = std::weak_ptr<Timer>;  // 定时任务弱指针
    using ptrTimer = std::shared_ptr<Timer>; // 定时任务智能指针

    // 二维数组
    using bucket = std::vector<ptrTimer>;   // 时间轮的桶
    using bucketList = std::vector<bucket>; // 使用数组实现时间轮,每个槽都有bucket

    int _tick;                                       // 表示指针每秒往后移动一格（用来标注数组的下标）
    size_t _capacity;                                // 时间轮的容量，表示最大的定时时长（这里仅仅实现一个秒级别的）
    bucketList _buckets;                             // 数组实现时间轮
    std::unordered_map<uint64_t, weakTimer> _timers; // 映射关系，fd和任务   TODO非线程安全

    EventLoop* _loop; // 事件循环
    int _timerfd;     // 定时器描述符--可读事件回调就是读取计数器，执⾏定时任务，这个fd是通过函数timefd_create创建的
    std::unique_ptr<Channel> _timerChannel;
private:
    void delTimer(uint64_t id);
    // 这里用1s是因为时间轮是1s1s的走动的
    static int createTimerfd();
    int readTimefdCount();
    // 走到哪就执行哪里的定时任务
    void runTimerTask();
    void onTime();
    // 添加定时任务
    void addTimer(uint64_t id, int timeout, const onTimerCallback &callback);
    // 根据id刷新定时任务
    void refreshTimer(uint64_t id);
    // 删除定时任务
    void cancelTimer(uint64_t id);
public:
    TimerWheel(EventLoop* loop);
    // 这个接⼝存在线程安全问题--这个接⼝实际上不能被外界使⽤者调⽤，只能在模块内，在对应的EventLoop线程中使用
    // 判断定时任务是否存在，非线程安全
    bool hasTimer(uint64_t id);
    /*定时器中有个_timers成员，定时器信息的操作有可能在多线程中进⾏，因此需要考虑线程安全问题*/
    /*如果不想加锁，那就一个线程对应一个定时器管理即可*/
    void timerAdd(uint64_t id, uint32_t timeout, const onTimerCallback &cb);
    // 刷新/延迟定时任务
    void timerRefresh(uint64_t id);
    //取消定时任务
    void timerCancel(uint64_t id);
};
