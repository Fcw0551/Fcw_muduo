#pragma once 
#include<sys/epoll.h>
#include "Fcw_Log.hpp"
#include <cassert>
#include <unordered_map>
#include <vector>

class Channel;


//当Poller检测到 fd 有事件就绪时，会通知Channel，再由Channel触发对应的回调函数。
#define EPOLLENEVENTS 1024   // epoll最多监听1024个事件

class Poller{
    private:
    int _epfd;
    std::unordered_map<int, Channel*> _channels;
    epoll_event _evs[EPOLLENEVENTS];

    //更新事件
//  EPOLL_CTL_ADD：添加 fd 到 epoll 实例（监控室加新 FD）；

// EPOLL_CTL_DEL：从 epoll 实例删除 fd（监控室移除 FD）；

// EPOLL_CTL_MOD：修改 fd 的监听事件（改 FD 的监控规则）
    void update(Channel *channel, int op);
    
    //判断事件是否已经添加到epoll模型
    bool hasChannel(Channel *channel);

    public:
    Poller();

    //添加或修改监控事件
    void updateEvent(Channel *channel);
    // 移除监控
    void removeEvent(Channel *channel);

    // 开始监控，返回就绪事件
    //通过输入输出型参数返回出去
    //内部设置好就绪事件
    void poll(std::vector<Channel*>*active);
};