#pragma once
#include <queue>
#include <thread>
#include "Msg.h"
#include <pthread.h>

using namespace std;
//通过使用 extern "C"，可以确保 C++ 编译器按照 C 语言的约定来处理这些 Lua 的 C 函数和数据结构。
extern "C"{
    #include"lua.h"
    #include"lauxlib.h"
    #include"lualib.h"
}

class Service{
public:
    //唯一id
    uint32_t id;
    //服务类型
    shared_ptr<string> type;
    //是否是在退出
    bool isExiting = false;
    //消息列表和锁
    queue<shared_ptr<BaseMsg>> msgQueue;
    pthread_spinlock_t queueLock;
    //全局队列标记
    bool inGlobal = false;
    pthread_spinlock_t inGlobalLock;
    void SetInGlobal(bool siIn);
public:
    Service();
    ~Service();
    //服务逻辑
    //回调函数
    void OnInit();
    void OnMsg (shared_ptr<BaseMsg> msg);
    void OnExit();
    //插入消息
    void PushMsg(shared_ptr<BaseMsg> msg);
    //执行消息
    bool ProcessMsg();
    void ProcessMsgs(int max);

    
private:
    //取消息
    shared_ptr<BaseMsg> PopMsg();
    //消息处理
    void OnServiceMsg(shared_ptr<ServiceMsg> msg);
    void OnAcceptMsg(shared_ptr<SocketAcceptMsg> msg);
    void OnRWMsg(shared_ptr<SocketRWMsg> msg);
    void OnSocketData(int fd,const char* buff,int len);
    void OnSocketWritable(int fd);
    void OnSocketClose(int fd);

private:
    //lua虚拟机
    lua_State *luaState;
};