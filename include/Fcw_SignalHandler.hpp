#pragma once 
#include <signal.h>
//信号处理类
class SignalHandler{
    public:
        //构造函数直接忽略掉SIGPIPE信号
        SignalHandler(){
            signal(SIGPIPE, SIG_IGN);
        }
};
//静态对象，程序启动时直接调用构造函数忽略信号
static SignalHandler sh;