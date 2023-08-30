#pragma once
#include <vector>
#include "Worker.h"
#include "Service.h"
#include <unordered_map>
#include "SocketWorker.h"
#include "Conn.h"

class Worker;

class Sunnet{
public:
    //单例，全局唯一
    static Sunnet* inst;
public:
    Sunnet();
    //初始化
    void Start();
    //等待运行
    void Wait();
private:
    //工作线程
    int WORKER_NUM = 3;
    vector<Worker*> workers; //worker对象
    vector<thread*> workerThreads; //线程
private:
    void startWorker();

//管理服务
public:
    //服务列表
    unordered_map<u_int32_t,shared_ptr<Service>> services;
    uint32_t maxId = 0; 
    pthread_rwlock_t servicesLock; //读写锁
public:
    //增删服务
    uint32_t NewService(shared_ptr<string> type);
    void KillService(uint32_t id);
private:
    //获取服务
    shared_ptr<Service> GetService(uint32_t id);

private:
    //全局队列
    queue<shared_ptr<Service>> globalQueue;
    int globalLen = 0;
    pthread_spinlock_t globalLock;
public:
    //发送消息
    void Send(uint32_t toId,shared_ptr<BaseMsg> msg);
    //全局队列操作
    shared_ptr<Service> PopGlobalQueue();
    void PushGlobalQueue(shared_ptr<Service> srv);
    shared_ptr<BaseMsg> MakeMsg(uint32_t source,char* buff,int len);
private:
    //休眠唤醒
    pthread_mutex_t sleepMtx;
    pthread_cond_t sleepCond;
    int sleepCount = 0;
public:
    //唤醒工作线程
    void CheckAndWeak();
    //让工作线程等待.仅工作线程调用
    void WorkerWait();
private:
    //socket线程对象
    SocketWorker* socketWorker;
    thread* socketThread;
public:
    void StartSocket();
public:
    //增删查改Conn
    int AddConn(int fd,uint32_t id,Conn::TYPE type);
    shared_ptr<Conn> GetConn(int fd);
    bool RemoveConn(int fd);
private:
    //conns
    unordered_map<uint32_t,shared_ptr<Conn>> conns;
    pthread_rwlock_t connsLock;//读写锁
public:
    //网络接口
    int Listen(uint32_t port,uint32_t serviceId);
    void CloseConn(uint32_t fd);
};




