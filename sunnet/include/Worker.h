#pragma once
#include <thread>
#include "Sunnet.h"
#include "Service.h"

class Sunnet;//前向声明，解决循环引用问题
using namespace std;

class Worker
{
public:
    int id;       //编号  
    int eachNum;  //每次处理消息数
    void operator() ();//线程函数
    void CheckAndPutGlobal(shared_ptr<Service> srv);
};


