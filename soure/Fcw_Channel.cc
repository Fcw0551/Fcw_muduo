#include "../include/Fcw_Channel.hpp"
#include "../include/Fcw_EventLoop.hpp"
Channel::Channel(EventLoop *loop, int fd)
    : _fd(fd)
    , _events(0)
    , _revents(0)
    , _loop(loop) {
    }

int Channel::getFd(){
    return _fd;
}

// 获取想要监控的事件，Poller会根据这个事件来注册fd
uint32_t Channel::getEvents(){
    return _events;
}

// 设置实际就绪的事件，就绪之后需要注册回来
void Channel::setREvents(uint32_t events){
    _revents = events;
}
void Channel::setReadCallback(const EventCallback &cb){
    _readCallback = cb;
}
void Channel::setWriteCallback(const EventCallback &cb){
    _writeCallback = cb;
}
void Channel::setErrorCallback(const EventCallback &cb){
    _errorCallback = cb;
}
void Channel::setCloseCallback(const EventCallback &cb){
    _closeCallback = cb;
}
void Channel::setEventCallback(const EventCallback &cb){
    _eventCallback = cb;
}
// 当前是否监控了可读
bool Channel::readAble(){
    return (_events & EPOLLIN);
}
// 当前是否监控了可写
bool Channel::writeAble(){
    return (_events & EPOLLOUT);
}

// 移除监控
void Channel::remove(){
    return _loop->removeEvent(this);
}

// 更新事件监听
void Channel::update(){
    return _loop->updateEvent(this);
}

// 启动事件监听，调用Poller的update
//  启动读事件监控
void Channel::enableRead(){
    _events |= EPOLLIN;
    update();
}
// 启动写事件监控
void Channel::enableWrite(){
    _events |= EPOLLOUT;
    update();
}
// 关闭读事件监控
void Channel::disableRead(){
    _events &= ~EPOLLIN;
    update();
}
// 关闭写事件监控
void Channel::disableWrite(){
    _events &= ~EPOLLOUT;
    update();
}
// 关闭所有事件监控
void Channel::disableAll(){
    _events = 0;
    update();
}

// 事件处理，⼀旦连接触发了事件，就调⽤这个函数，⾃⼰触发了什么事件如何处理⾃⼰决定
void Channel::handleEvent(){
    // 第一步：优先处理错误/挂断等致命事件（优先级最高）
    if (_revents & (EPOLLERR | EPOLLHUP))
    {
        // 先执行错误回调
        if (_errorCallback)
            _errorCallback();
        // 挂断事件触发关闭回调（EPOLLHUP是连接完全挂断）
        if (_closeCallback)
            _closeCallback();
        // 致命事件处理后，直接返回（避免后续无效处理）
        return;
    }

    // 第二步：处理可读/紧急数据/对端关闭写半连接事件
    if (_revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if (_readCallback)
            _readCallback();
        // EPOLLRDHUP是对端关闭写半连接，触发关闭回调
        if ((_revents & EPOLLRDHUP) && _closeCallback)
            _closeCallback();
    }

    // 第三步：处理可写事件（非致命，单独判断）
    if (_revents & EPOLLOUT)
    {
        if (_writeCallback)
            _writeCallback();
    }

    // 第四步：通用事件回调（最后执行）
    if (_eventCallback)
        _eventCallback();
}