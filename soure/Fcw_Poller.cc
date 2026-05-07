#include "../include/Fcw_Poller.hpp"
#include "../include/Fcw_Channel.hpp"
// 更新事件
//  EPOLL_CTL_ADD：添加 fd 到 epoll 实例（监控室加新 FD）；

// EPOLL_CTL_DEL：从 epoll 实例删除 fd（监控室移除 FD）；

// EPOLL_CTL_MOD：修改 fd 的监听事件（改 FD 的监控规则）
void Poller::update(Channel *channel, int op){
    int fd = channel->getFd();
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = channel->getEvents();
    int ret = epoll_ctl(_epfd, op, fd, &ev);
    if (ret < 0)
    {
        ERR_LOG("epoll_ctl error");
    }
}

// 判断事件是否已经添加到epoll模型
bool Poller::hasChannel(Channel *channel){
    auto it = _channels.find(channel->getFd());
    if (it == _channels.end())
        return false;
    return true;
}

Poller::Poller(){
    _epfd = epoll_create(EPOLLENEVENTS);
    if (_epfd < 0)
    {
        ERR_LOG("epoll_create error");
        exit(1); // 程序退出
    }
}

// 添加或修改监控事件
void Poller::updateEvent(Channel *channel){
    bool ret = hasChannel(channel);
    if (ret == false)
    {
        // 不存在则添加
        _channels.insert(std::make_pair(channel->getFd(), channel));
        return update(channel, EPOLL_CTL_ADD);
    }
    return update(channel, EPOLL_CTL_MOD);
}
// 移除监控
void Poller::removeEvent(Channel *channel){
    // auto it = _channels.find(channel->getFd());
    // if (it != _channels.end())
    // {
    //     _channels.erase(it);
    // }
    // update(channel, EPOLL_CTL_DEL);

    int fd = channel->getFd();
    auto it = _channels.find(fd);
    if (it == _channels.end())
    {
        return; // 已经移除过，避免重复删除
    }
    _channels.erase(it);
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr) < 0)
    {
        if (errno != EBADF)
        {
            ERR_LOG("epoll_ctl DEL error, fd: %d, errno: %d (%s)", fd, errno, strerror(errno));
        }
        // EBADF 是正常现象（连接已关闭），忽略
    }
}

// 开始监控，返回就绪事件
// 通过输入输出型参数返回出去
// 内部设置好就绪事件
void Poller::poll(std::vector<Channel *> *active){
    // int epoll_wait(int epfd, struct epoll_event *evs, intmaxevents, int timeout)
    //-1表示永久阻塞
    int nfds = epoll_wait(_epfd, _evs, EPOLLENEVENTS, -1);
    if (nfds < 0)
    {
        if (errno == EINTR)
        {
            DBG_LOG("EPOLL WAIT EINTR\n");
            return;
        }
        ERR_LOG("EPOLL WAIT ERROR:%s\n", strerror(errno));
        abort(); // 退出程序
    }
    for (int i = 0; i < nfds; i++)
    {
        auto it = _channels.find(_evs[i].data.fd); // 通过fd找到对应的channel
        assert(it != _channels.end());
        it->second->setREvents(_evs[i].events); // 设置实际就绪的事件
        active->push_back(it->second);
    }
    return;
}