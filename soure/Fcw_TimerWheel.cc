#include "../include/Fcw_TimerWheel.hpp"
#include "../include/Fcw_EventLoop.hpp"

//timer的实现
Timer::Timer(uint64_t id, int timeout, const onTimerCallback &timerCallback)
    : _timeout(timeout)
    , _id(id)
    , _timerCallback(timerCallback){
    _cancel = false;
}
Timer::~Timer(){
    if (_releaseCallback)
    {
        // 通过释放任务把定时队列中的weak_ptr给释放掉(清理相关信息)
        _releaseCallback();
    }
    if (_timerCallback && !_cancel)
    {
        // 超时后如果没有被取消，则需要执行的任务
        _timerCallback();
    }
}
// 获取定时任务的定时时间
int Timer::getTimeout(){
    return _timeout;
}
// 取消定时任务
void Timer::cancel(){
    _cancel = true;
}
// 获取定时任务id
uint64_t Timer::getId(){
    return _id;
}
// 设置定时任务的回调函数
void Timer::setOnTimerCallback(const onTimerCallback &callback){
    _timerCallback = callback;
}
// 设置定时任务的释放函数
void Timer::setReleaseCallback(const releaseCallback &callback){
    _releaseCallback = callback;
}





//timerwheel的实现
void TimerWheel::delTimer(uint64_t id){
    auto it = _timers.find(id);
    if (it != _timers.end()){
        _timers.erase(it);
    }
}
// 这里用1s是因为时间轮是1s1s的走动的
int TimerWheel::createTimerfd(){
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd < 0){
        ERR_LOG("timerfd_create error!");
        abort();
    }
    // int timerfd_settime(int fd, int flags, struct itimerspec *new,struct itimerspec *old);
    struct itimerspec itime;
    itime.it_value.tv_sec = 1;
    itime.it_value.tv_nsec = 0; // 第⼀次超时时间为1s后
    itime.it_interval.tv_sec = 1;
    itime.it_interval.tv_nsec = 0; // 第⼀次超时后，每次超时的间隔时
    timerfd_settime(timerfd, 0, &itime, NULL);
    return timerfd;
}

int TimerWheel::readTimefdCount(){
    uint64_t times;
    // 有可能因为其他描述符的事件处理花费事件⽐较⻓，然后在处理定时器描述符事件的时候，有可能就已经超时了很多次
    // read读取到的数据times就是从上⼀次read之后超时的次数
    int ret = read(_timerfd, &times, 8);
    if (ret < 0)
    {
        ERR_LOG("read timerfd error!");
        abort();
    }
    return times;
}
// 走到哪就执行哪里的定时任务
void TimerWheel::runTimerTask(){
    _tick = (_tick + 1) % _capacity; // 指针往后移动一格
    // std::cout<<"秒："<<_tick<<std::endl;
    _buckets[_tick].clear(); // 清空当前槽的定时任务，清空掉直到shared_ptr为0自动释放
}
void  TimerWheel::onTime(){
    // 根据实际超时的次数，执⾏对应的秒针走动
    int times = readTimefdCount();
    for (int i = 0; i < times; i++){
        TimerWheel::runTimerTask();
    }
}
// 添加定时任务
void TimerWheel::addTimer(uint64_t id, int timeout, const onTimerCallback &callback){
    if (timeout <= 0 || timeout > MAX_TIMEOUT){
        // 对于小于0或者大于最大时间的都不支持
        return;
    }
    // 初始化定时任务
    ptrTimer timer(new Timer(id, timeout, callback));
    // 超时后执行的任务
    timer->setOnTimerCallback(callback);
    // 定时任务释放函数(这个是为了清理weakptr和id对应关系)
    timer->setReleaseCallback(std::bind(&TimerWheel::delTimer, this, id));

    _timers[id] = weakTimer(timer);
    _buckets[(_tick + timeout) % _capacity].push_back(timer);
}
// 根据id刷新定时任务
void TimerWheel::refreshTimer(uint64_t id){
    auto it = _timers.find(id);
    if (it != _timers.end())
    {
        int timeout = it->second.lock()->getTimeout();
        _buckets[(_tick + timeout) % _capacity].push_back(it->second.lock());
        //  std::cout<<"桶："<<(_tick+timeout)%_capacity<<std::endl;
    }
}
// 删除定时任务
void TimerWheel::cancelTimer(uint64_t id){
    auto it = _timers.find(id);
    if (it != _timers.end()){
        it->second.lock()->cancel();
    }
}
TimerWheel::TimerWheel(EventLoop *loop)
    : _tick(0)
    ,_capacity(MAX_TIMEOUT)
    ,_buckets(_capacity), _loop(loop), _timerfd(createTimerfd()) // 创建定时器
    ,_timerChannel(new Channel(loop, _timerfd)){
    // 给定时器设置一个可读事件回调,并且设置成读事件
    _timerChannel->setReadCallback(std::bind(&TimerWheel::onTime, this));
    _timerChannel->enableRead();
}
// 这个接⼝存在线程安全问题--这个接⼝实际上不能被外界使⽤者调⽤，只能在模块内，在对应的EventLoop线程中使用
// 判断定时任务是否存在，非线程安全
bool TimerWheel::hasTimer(uint64_t id){
    auto it = _timers.find(id);
    if (it != _timers.end()){
        return true;
    }
    return false;
}

/*定时器中有个_timers成员，定时器信息的操作有可能在多线程中进⾏，因此需要考虑线程安全问题*/
/*如果不想加锁，那就一个线程对应一个定时器管理即可*/
void TimerWheel::timerAdd(uint64_t id, uint32_t timeout, const onTimerCallback &cb){
    _loop->runInLoop(std::bind(&TimerWheel::addTimer, this, id, timeout, cb));
}
// 刷新/延迟定时任务
void TimerWheel::timerRefresh(uint64_t id){
    _loop->runInLoop(std::bind(&TimerWheel::refreshTimer, this, id));
}
void TimerWheel::timerCancel(uint64_t id){
    _loop->runInLoop(std::bind(&TimerWheel::cancelTimer, this, id));
}