#include "../include/Fcw_EventLoop.hpp"

// 执⾏任务池中的所有任务
void EventLoop::runAllTask(){
    std::vector<Functor> functor;{
        std::unique_lock<std::mutex> _lock(_mutex);
        _tasks.swap(functor);
    }
    for (auto &f : functor){
        f();
    }
    return;
}

// 这个函数的作用就是让别的线程能够唤醒此线程，不要阻塞在epoll_wait
int EventLoop::createEventFd(){
    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (efd < 0){
        ERR_LOG("CREATE EVENTFD FAILED!!");
        abort(); // 让程序异常退出
    }
    return efd;
}

// read让重置通知
void EventLoop::readEventfd(){
    uint64_t res = 0;
    int ret = read(_eventfd, &res, sizeof(res));
    if (ret < 0){
        // EINTR -- 被信号打断； EAGAIN -- 表⽰⽆数据可读
        if (errno == EINTR || errno == EAGAIN)
        {
            return;
        }
        ERR_LOG("READ EVENTFD FAILED!");
        abort();
    }
    return;
}
// 唤醒eventfd
void EventLoop::weakupEventFd(){
    uint64_t val = 1;
    int ret = write(_eventfd, &val, sizeof(val));
    if (ret < 0){
        if (errno == EINTR)
        {
            return;
        }
        ERR_LOG("READ EVENTFD FAILED!");
        abort();
    }
    return;
}

EventLoop::EventLoop()
    : _threadid(std::this_thread::get_id())
    , _eventfd(createEventFd())
    , _eventChannel(new Channel(this, _eventfd))
    , _timerWheel(this){
    // 这是注册监控，后面可以唤醒本线程
    //  给eventfd添加可读事件回调函数，读取eventfd事件通知次数
    _eventChannel->setReadCallback(std::bind(&EventLoop::readEventfd, this));
    // 启动eventfd的读事件监控
    _eventChannel->enableRead();
}
// 三步⾛--事件监控-》就绪事件处理-》执⾏任务
void EventLoop::start(){
    while (1)
    {
        // 1. 事件监控，
        std::vector<Channel *> actives;
        _poller.poll(&actives);
        // 2. 事件处理。
        for (auto &channel : actives)
        {
            channel->handleEvent();
        }
        // 3. 执⾏任务池任务
        runAllTask();
    }
}
// ⽤于判断当前线程是否是EventLoop对应的线程；
bool EventLoop::isInLoop(){
    return (_threadid == std::this_thread::get_id());
}
void EventLoop::assertInLoop(){
    assert(_threadid == std::this_thread::get_id());
}
// 判断将要执⾏的任务是否处于当前线程中，如果是则执⾏，不是则压⼊队列。
void EventLoop::runInLoop(const Functor &cb){
    if (isInLoop()){
        return cb();
    }
    return queueInLoop(cb);
}
// 将操作压⼊任务池
void EventLoop::queueInLoop(const Functor &cb){
    {
        std::unique_lock<std::mutex> _lock(_mutex);
        _tasks.push_back(cb);
    }
    // 唤醒有可能因为没有事件就绪，⽽导致的epoll阻塞；
    // 其实就是给eventfd写⼊⼀个数据，eventfd就会触发可读事件
    weakupEventFd();
}
// 添加/修改描述符的事件监控
void EventLoop::updateEvent(Channel *channel){
    return _poller.updateEvent(channel);
}
// 移除描述符的监控
void EventLoop::removeEvent(Channel *channel){
    return _poller.removeEvent(channel);
}

// 定时器的功能
void EventLoop::timerAdd(uint64_t id, uint32_t delay, const onTimerCallback &cb){
    return _timerWheel.timerAdd(id, delay, cb);
}
void EventLoop::timerRefresh(uint64_t id){
    return _timerWheel.timerRefresh(id);
}
void EventLoop::timerCancel(uint64_t id){
    return _timerWheel.timerCancel(id);
}
bool EventLoop::hasTimer(uint64_t id){
    return _timerWheel.hasTimer(id);
}



/*实例化 EventLoop 对象，唤醒_cond上有可能阻塞的线程，并且开始运⾏EventLoop模块的功能*/
void LoopThread::threadEntry(){
    //注意这里创建的是栈区
    EventLoop loop;
    {
        std::unique_lock<std::mutex> lock(_mutex); // 加锁
        _loop = &loop;
        _cond.notify_all();
    }
    loop.start();
}
/*创建线程，设定线程⼊⼝函数*/
LoopThread::LoopThread()
    : _loop(NULL)
    , _thread(std::thread(&LoopThread::threadEntry, this)) {}

/*返回当前线程关联的EventLoop对象指针*/
 EventLoop* LoopThread::getLoop(){
    EventLoop *loop = NULL;{
        std::unique_lock<std::mutex> lock(_mutex); // 加锁
        _cond.wait(lock, [&]()
                   { return _loop != NULL; }); // loop为NULL就⼀直阻塞
        loop = _loop;
    }
    return loop;
}





LoopThreadPool::LoopThreadPool(EventLoop *baseloop)
    : _threadCount(0)
    , _nextIndex(0)
    , _baseloop(baseloop) {
    }
void LoopThreadPool::setThreadCount(int count){
    _threadCount = count;
}
void LoopThreadPool::create(){
    if (_threadCount > 0){
        _threads.resize(_threadCount);
        _loops.resize(_threadCount);
        for (int i = 0; i < _threadCount; i++){
            _threads[i] = new LoopThread();
            _loops[i] = _threads[i]->getLoop();
        }
    }
    return;
}
EventLoop* LoopThreadPool::nextLoop(){
    if (_threadCount == 0){
        return _baseloop;
    }
    // 达到轮询的目的，合理分配任务到每个线程
    _nextIndex = (_nextIndex + 1) % _threadCount;
    return _loops[_nextIndex];
}