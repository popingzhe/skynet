#pragma once
#include <cstdint> //uint_8的来源
#include <memory> //智能指针的来源
using namespace std;


//消息基类
class BaseMsg {
public:
    enum TYPE{
        SERVICE = 1,//服务间消息
        SOCKET_ACCEPT = 2,//处理链接消息
        SOCKET_RW = 3,//处理客户端读写消息
    };
    uint8_t type; //消息类型
//    char load[99999]{};
    virtual ~BaseMsg(){};
};

//服务间消息
class ServiceMsg : public BaseMsg {
public:
    uint32_t source;      //消息发送方
    shared_ptr<char> buff;//消息内容
    size_t size;          //消息大小  
};

class SocketAcceptMsg :public BaseMsg{
public:
    int listenFd;
    int cliendFd;
};

class SocketRWMsg :public BaseMsg{
public:
    int fd;
    bool isRead = false;
    bool isWrite = false;
};