#include "../include/Fcw_Connection.hpp"
#include "../include/Fcw_Socket.hpp"
#include "../include/Fcw_Buffer.hpp"
#include "../include/Fcw_Channel.hpp"
#include "../include/Fcw_EventLoop.hpp"
#include "../include/Fcw_Any.hpp"
#include <memory>
#include "../include/Fcw_Log.hpp"


    Connection::Connection(EventLoop *loop, uint64_t connid, int sockfd) 
    : _connid(connid)
    , _sockfd(sockfd)
    ,_enableInactiveRelease(false)
    , _loop(loop)
    , _statu(CONNECTING)
    ,_socket(_sockfd)
    ,_channel(loop, _sockfd)
    {
        // //注册事件的回调函数
        // _channel.setCloseCallback(std::bind(&Connection::handleClose,this));
        // _channel.setEventCallback(std::bind(&Connection::handleEvent,this));
        // _channel.setReadCallback(std::bind(&Connection::handleRead, this));
        // _channel.setWriteCallback(std::bind(&Connection::handleWrite,this));
        // _channel.setErrorCallback(std::bind(&Connection::handleError,this));
        // 注册事件的回调函数，使用 weak_ptr 防止悬空指针
        std::weak_ptr<Connection> weak_self = weak_from_this();

        _channel.setReadCallback([weak_self]()
                                 {
    if (auto self = weak_self.lock()) self->handleRead(); });

        _channel.setWriteCallback([weak_self]()
                                  {
    if (auto self = weak_self.lock()) self->handleWrite(); });

        _channel.setCloseCallback([weak_self]()
                                  {
    if (auto self = weak_self.lock()) self->handleClose(); });

        _channel.setErrorCallback([weak_self]()
                                  {
    if (auto self = weak_self.lock()) self->handleError(); });

        _channel.setEventCallback([weak_self]()
                                  {
    if (auto self = weak_self.lock()) self->handleEvent(); });
    }
    Connection::~Connection() {
        DBG_LOG("release connection:%p", this); 
    }
    /*五个channel的事件回调函数*/
    // 描述符可读事件触发后调⽤的函数，接收socket数据放到接收缓冲区中，然后调⽤_messageCallback 
    void Connection::handleRead(){
        // // 1. 接收socket的数据，放到缓冲区
        // char buf[65536];
        // ssize_t ret = _socket.recvNoBlock(buf, 65535);
        // if (ret < 0)
        // {
        //     // 出错了,不能直接关闭连接
        //     return shutdownInLoop();
        // }
        // // 这⾥的等于0表⽰的是没有读取到数据，⽽并不是连接断开了，连接断开返回的是-1
        // // 将数据放⼊输⼊缓冲区,写⼊之后顺便将写偏移向后移动
        // _inBuffer.writeAndPush(buf, ret);
        // // 2. 调⽤message_callback进⾏业务处理
        // if (_inBuffer.readSize() > 0)
        // {   
        //     // shared_from_this--从当前对象⾃⾝获取⾃⾝的shared_ptr管理对象
        //     return _messageCallback(shared_from_this(), &_inBuffer);
        // }

        char buf[65536];
        ssize_t ret = _socket.recvNoBlock(buf, 65535);
        if (ret < 0)
        {
            // 真实错误
            return shutdownInLoop();
        }
        if (ret == 0 && _inBuffer.readSize() == 0)
        {
            // 对端关闭且没有数据需要处理，直接挂断
            return shutdownInLoop();
        }
        if (ret > 0)
        {
            _inBuffer.writeAndPush(buf, ret);
        }
        // 处理业务
        if (_inBuffer.readSize() > 0)
        {
            _messageCallback(shared_from_this(), &_inBuffer);
        }
    }
    // 描述符可写事件触发后调⽤的函数，将发送缓冲区中的数据进⾏发送
    void Connection::handleWrite()
    {
        //_out_buffer中保存的数据就是要发送的数据
        ssize_t ret = _socket.sendNoBlock(_outBuffer.readPos(), _outBuffer.readSize());
        if (ret < 0)
        {
            // 发送错误就该关闭连接了，
            if (_inBuffer.readSize() > 0)
            {
                _messageCallback(shared_from_this(), &_inBuffer);
            }
            return release(); // 这时候就是实际的关闭释放操作了。
        }
        _outBuffer.moveReadOffset(ret); // 将读偏移向后移动
        if (_outBuffer.readSize() == 0)
        {
            _channel.disableWrite(); // 没有数据待发送了，关闭写事件监控
            // 如果当前是连接待关闭状态，则有数据，发送完数据释放连接，没有数据则直接释放
            if (_statu == DISCONNECTING)
            {
                return release();
            }
        }
        return;
    }
    // 描述符触发挂断事件
    void Connection::handleClose()
    {
        /*⼀旦连接挂断了，套接字就什么都⼲不了了，因此有数据待处理就处理⼀下，完毕关闭连接*/
        if (_inBuffer.readSize() > 0)
        {
            _messageCallback(shared_from_this(), &_inBuffer);
        }
        return release();
    }
    // 描述符触发出错事件
    void Connection::handleError()
    {
        return handleClose();
    }
    
    // 描述符触发任意事件: 
    //1. 刷新连接的活跃度--延迟定时销毁任务 
    //2. 调⽤组件使⽤者的任意事件回调
    void Connection::handleEvent()
    {
        if (_enableInactiveRelease == true){
            INF_LOG("刷新定时任务");
            _loop -> timerRefresh(_connid);
        }
        if (_eventCallback){
            _eventCallback(shared_from_this());
        }
    }
    // 连接获取之后，所处的状态下要进⾏各种设置（启动读监控,调⽤回调函数）
    void Connection::establishedInLoop()
    {
        // // 1. 修改连接状态； 
        // //2. 启动读事件监控； 
        // //3. 调⽤回调函数
        // assert(_statu == CONNECTING); // 当前的状态必须⼀定是上层的半连接状态
        // _statu = CONNECTED;           // 当前函数执⾏完毕，则连接进⼊已完成连接状态
        // // ⼀旦启动读事件监控就有可能会⽴即触发读事件，如果这时候启动了⾮活跃连接销毁
        // _channel.enableRead();
        // if (_connectedCallback)
        //     _connectedCallback(shared_from_this());

        assert(_statu == CONNECTING);
        _statu = CONNECTED;

        // 此时对象已由 shared_ptr 完整管理，安全获取 weak_ptr
        std::weak_ptr<Connection> weak_self = weak_from_this();

        // 重新设置所有 Channel 回调为 weak_ptr 捕获版本
        _channel.setReadCallback([weak_self]()
                                 {
        if (auto self = weak_self.lock()) self->handleRead(); });
        _channel.setWriteCallback([weak_self]()
                                  {
        if (auto self = weak_self.lock()) self->handleWrite(); });
        _channel.setCloseCallback([weak_self]()
                                  {
        if (auto self = weak_self.lock()) self->handleClose(); });
        _channel.setErrorCallback([weak_self]()
                                  {
        if (auto self = weak_self.lock()) self->handleError(); });
        _channel.setEventCallback([weak_self]()
                                  {
        if (auto self = weak_self.lock()) self->handleEvent(); });

        _channel.enableRead();
        if (_connectedCallback)
            _connectedCallback(shared_from_this());
    }
    // 这个接⼝才是实际的释放接⼝
    void Connection::releaseInLoop()
    {
        // // 1. 修改连接状态，将其置为DISCONNECTED
        // _statu = DISCONNECTED;
        // // 2. 移除连接的事件监控
        // _channel.remove();
        // // 3. 关闭描述符
        // _socket.close();
        // // 4. 如果当前定时器队列中还有定时销毁任务，则取消任务
        // if (_loop->hasTimer(_connid))
        //     cancelInactiveReleaseInLoop();
        // // 5. 调⽤关闭回调函数，避免先移除服务器管理的连接信息导致Connection被释放，再去处理会出错，因此先调⽤⽤⼾的回调函数
        // if (_closedCallback)
        //     _closedCallback(shared_from_this());
        // // 移除服务器内部管理的连接信息
        // if (_serverClosedCallback)
        //     _serverClosedCallback(shared_from_this());
        auto guard = shared_from_this(); // 确保回调执行期间对象存活
        _statu = DISCONNECTED;
        _channel.remove();
        _socket.close();
        if (_loop->hasTimer(_connid))
            cancelInactiveReleaseInLoop();
        if (_closedCallback)
            _closedCallback(guard);
        if (_serverClosedCallback)
            _serverClosedCallback(guard);
    }

    // 这个接⼝并不是实际的发送接⼝，⽽只是把数据放到了发送缓冲区，启动了可写事件监控
    void Connection::sendInLoop(Buffer &buf){
        if (_statu == DISCONNECTED)
            return;
        _outBuffer.writeBufferAndPush(buf);
        if (_channel.writeAble() == false){
            _channel.enableWrite();
        }
    }
    // 这个关闭操作并⾮实际的连接释放操作，需要判断还有没有数据待处理，待发送
    void Connection::shutdownInLoop(){
        _statu = DISCONNECTING; // 设置连接为半关闭状态
        if (_inBuffer.readSize() > 0){
            if (_messageCallback)
                _messageCallback(shared_from_this(),&_inBuffer);
        }
        // 要么就是写⼊数据的时候出错关闭，要么就是没有待发送数据，直接关闭
        if (_outBuffer.readSize() > 0){
            if (_channel.writeAble() == false){
                _channel.enableWrite();
            }
        }
        if (_outBuffer.readSize() == 0){
            release();
        }
    }
    // 启动⾮活跃连接超时释放规则
    void Connection::enableInactiveReleaseInLoop(int sec){
        // // 1. 将判断标志 _enable_inactive_release 置为true
        // _enableInactiveRelease = true;
        // // 2. 如果当前定时销毁任务已经存在，那就刷新延迟⼀下即可
        // if (_loop->hasTimer(_connid)){
        //     return _loop->timerRefresh(_connid);
        // }
        // // 3. 如果不存在定时销毁任务，则新增
        // _loop->timerAdd(_connid, sec, std::bind(&Connection::release, this));

        _enableInactiveRelease = true;
        if (_loop->hasTimer(_connid))
        {
            return _loop->timerRefresh(_connid);
        }
        std::weak_ptr<Connection> weak_self = shared_from_this();
        _loop->timerAdd(_connid, sec, [weak_self]()
                        {
        if (auto self = weak_self.lock()) {
            self->release();
        } });
    }
    void Connection::cancelInactiveReleaseInLoop(){
        _enableInactiveRelease = false;
        if (_loop->hasTimer(_connid)){
            _loop->timerCancel(_connid);
        }
    }
    void Connection::upgradeInLoop(const Any &context,const ConnectedCallback &conn,
                       const MessageCallback &msg,
                       const ClosedCallback &closed,
                       const AnyEventCallback &event)
    {
        _context = context;
        _connectedCallback = conn;
        _messageCallback = msg;
        _closedCallback = closed;
        _eventCallback = event;
    }


    // 获取sockfd描述符
    int Connection::getfd() { 
        return _sockfd;
    }
    // 获取连接ID
    uint64_t Connection::getConnectid() {
        return _connid; 
    }
    // 是否处于CONNECTED状态
    bool Connection::isConnected() {
        return (_statu == CONNECTED); 
    }
    // 设置上下⽂--连接建⽴完成时进⾏调⽤
    void Connection::setContext(const Any &context) {
        _context = context; 
    }
    // 获取上下⽂，返回的是指针
    Any* Connection::getContext() {
        return &_context;
    }

    //业务函数的回调设置
    void Connection::setConnectedCallback(const ConnectedCallback &cb){
        _connectedCallback = cb;
    }
    void Connection::setMessageCallback(const MessageCallback &cb) {
        _messageCallback = cb; 
    }
    void Connection::setClosedCallback(const ClosedCallback &cb) { 
        _closedCallback =cb; 
    }
    void Connection::setAnyEventCallback(const AnyEventCallback &cb) { 
        _eventCallback = cb; 
    }
    void Connection::setSrvClosedCallback(const ClosedCallback &cb){
        _serverClosedCallback = cb;
    }

    // 连接建⽴就绪后，进⾏channel回调设置，启动读监控，调⽤_connectedCallback
    void Connection::established(){
        // _loop->runInLoop(std::bind(&Connection::establishedInLoop, this));
        _loop->runInLoop([weak_self = weak_from_this()]()
                         {
        auto self = weak_self.lock();
        if (self) self->establishedInLoop(); });
    }

    // 发送数据，将数据放到发送缓冲区，启动写事件监控
    void Connection::send(const char *data, size_t len){
        // 外界传⼊的data，可能是个临时的空间，我们现在只是把发送操作压⼊了任务池，有可能并没有被⽴即执⾏
        // 因此有可能执⾏的时候，data指向的空间有可能已经被释放了。
        // Buffer buf;
        // buf.writeAndPush(data, len);
        // _loop->runInLoop(std::bind(&Connection::sendInLoop, this,std::move(buf)));
        Buffer buf;
        buf.writeAndPush(data, len);
        _loop->runInLoop([weak_self = weak_from_this(), buf = std::move(buf)]() mutable
                         {
        auto self = weak_self.lock();
        if (self) self->sendInLoop(buf); });
    }


    // 提供给组件使⽤者的关闭接⼝--并不实际关闭，需要判断有没有数据待处理
    void Connection::shutdown(){
        // _loop->runInLoop(std::bind(&Connection::shutdownInLoop, this));
        _loop->runInLoop([weak_self = weak_from_this()]()
                         {
        auto self = weak_self.lock();
        if (self) self->shutdownInLoop(); });
    }
    void Connection::release(){
        // _loop->queueInLoop(std::bind(&Connection::releaseInLoop, this));
        // 防止重复投递
        if (_statu == DISCONNECTED)
            return;
        std::weak_ptr<Connection> weak_self = shared_from_this();
        _loop->queueInLoop([weak_self]()
                           {
        auto self = weak_self.lock();
        if (self) self->releaseInLoop(); });
    }
    
    // 启动⾮活跃销毁，并定义多⻓时间⽆通信就是⾮活跃，添加定时任务
    void Connection::enableInactiveRelease(int sec){
        // _loop -> runInLoop(std::bind(&Connection::enableInactiveReleaseInLoop, this, sec));
         _loop->runInLoop([weak_self = weak_from_this(), sec]() {
        auto self = weak_self.lock();
        if (self) self->enableInactiveReleaseInLoop(sec);
    });
    }

    // 取消⾮活跃销毁
    void Connection::cancelInactiveRelease(){
        // _loop -> runInLoop(std::bind(&Connection::cancelInactiveReleaseInLoop, this));
        _loop->runInLoop([weak_self = weak_from_this()]() {
        auto self = weak_self.lock();
        if (self) self->cancelInactiveReleaseInLoop();
    });
    }

    // 切换协议---重置上下⽂以及阶段性回调处理函数 -- ⽽是这个接⼝必须在EventLoop线程中⽴即执⾏
    // 防备新的事件触发后，处理的时候，切换任务还没有被执⾏--会导致数据使⽤原协议处理了。
    void Connection::upgrade(const Any &context, const ConnectedCallback &conn, const MessageCallback &msg,
                 const ClosedCallback &closed, const AnyEventCallback &event){
        // _loop->assertInLoop();
        // _loop->runInLoop(std::bind(&Connection::upgradeInLoop, this,context, conn, msg, closed, event));
        _loop->assertInLoop();
        _loop->runInLoop([weak_self = weak_from_this(), context, conn, msg, closed, event]()
                         {
        auto self = weak_self.lock();
        if (self) self->upgradeInLoop(context, conn, msg, closed, event); });
    }