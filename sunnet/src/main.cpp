#include <iostream>
#include "Sunnet.h"
#include "Msg.h"
#include<unistd.h>
using namespace std;

int test(){
    auto pingType = make_shared<string>("ping");
    uint32_t ping1 = Sunnet::inst->NewService(pingType);
    uint32_t ping2 = Sunnet::inst->NewService(pingType);
    uint32_t pong = Sunnet::inst->NewService(pingType);
    auto msg1 = Sunnet::inst->MakeMsg(ping1,new char[3]{'h','i','\0'},3);
    auto msg2 = Sunnet::inst->MakeMsg(ping2,new char[4]{'h','i','i','\0'},3);

    Sunnet::inst->Send(pong,msg1);
    Sunnet::inst->Send(pong,msg2);
}

int TsetSocket(){
    int fd = Sunnet::inst->Listen(8001,1);
    usleep(15*1000000);
    Sunnet::inst->CloseConn(fd);
}

int TestEcho(){
    auto t =make_shared<string>("gateway");
    uint32_t gateway = Sunnet::inst->NewService(t);
}

int main (){
    cout << "holle world!" <<endl;
    new Sunnet();
    Sunnet::inst->Start();
    //开启系统后执行一些前置逻辑
    auto t =make_shared<string>("main");
    Sunnet::inst->NewService(t);
    Sunnet::inst->Wait();
    return 0;
}
