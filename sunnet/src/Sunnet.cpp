#include <iostream>
#include "Sunnet.h"
#include "Service.h"
#include<assert.h>
#include<unistd.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<signal.h>
using namespace std;

//单例
Sunnet* Sunnet::inst;
Sunnet::Sunnet(){
    inst = this;
}

void Sunnet::Start(){
    cout<< "holle sunnet" << endl;
    //忽略SIGPEIPE信号，防止客户端掉线导致的rst复位信号导致服务端退出
    signal(SIGPIPE,SIG_IGN);
    //服务列表初始化读写锁
    pthread_rwlock_init(&servicesLock,NULL);
    pthread_spin_init(&globalLock,PTHREAD_PROCESS_PRIVATE);
    pthread_cond_init(&sleepCond,NULL);
    pthread_mutex_init(&sleepMtx,NULL);
    assert(pthread_rwlock_init(&this->connsLock,NULL)==0);
    //开启worker()
    startWorker();
    //开启网络线程  
    StartSocket();
}

void Sunnet::startWorker(){
    for(int i = 0;i < WORKER_NUM;i++){
        cout << "start worker thread:" << i << endl;
        //创建线程对象
        Worker* worker = new Worker();
        worker->id = i;
        worker->eachNum = 1 << i;
        thread* wt = new thread(*worker);
        //添加变成数值
        workers.push_back(worker);
        workerThreads.push_back(wt);
    }
}

void Sunnet::Wait(){
    if(workerThreads[0]){
        workerThreads[0]->join();
        //阻塞主线程,等待第一个线程执行完了，才能去执行其他逻辑
    }
}
/************管理服务***********************************/
uint32_t Sunnet::NewService(shared_ptr<string> type){
    auto srv = make_shared<Service>();
    srv->type = type;
    pthread_rwlock_wrlock(&servicesLock);
    {
        srv->id = maxId;
        maxId++;
        services.emplace(srv->id,srv);
    }
    pthread_rwlock_unlock(&servicesLock);
    srv->OnInit();//初始化
    return srv->id;
}

//删除服务  只有自己删除自己，可以不用加锁
void Sunnet::KillService(uint32_t id){
    shared_ptr<Service> srv = GetService(id);
    if(!srv){
        return;
    }
    srv->OnExit();
    srv->isExiting = true;
    //删除表中信息
    pthread_rwlock_wrlock(&servicesLock);
    {
        services.erase(id);
    }
    pthread_rwlock_unlock(&servicesLock);
}

shared_ptr<Service> Sunnet::GetService(uint32_t id){
    shared_ptr<Service> srv = nullptr;
    pthread_rwlock_rdlock(&servicesLock);
    {
        unordered_map<uint32_t, shared_ptr<Service>>::iterator iter = services.find (id);
        if (iter != services.end()){
            srv = iter->second;
        }
    }
    pthread_rwlock_unlock(&servicesLock);
    return srv;
}
/***************服务间通信********************************************/
shared_ptr<Service> Sunnet::PopGlobalQueue(){
    shared_ptr<Service> srv = nullptr;
    pthread_spin_lock(&globalLock);
    {
        if(!globalQueue.empty()){
            srv = globalQueue.front();
            globalQueue.pop();
            globalLen--;
        }
    }
    pthread_spin_unlock(&globalLock);
    return srv;
}

void Sunnet::PushGlobalQueue(shared_ptr<Service> srv){
    pthread_spin_lock(&globalLock);
    {
        globalQueue.push(srv);
        globalLen++;
    }
    pthread_spin_unlock(&globalLock);
}

void Sunnet::Send(uint32_t toId,shared_ptr<BaseMsg> msg){
    cout<<"Send msg to "<<toId<<" "<<msg->type<<endl;
    shared_ptr<Service> toSrv = GetService(toId);
    if(!toSrv){
        cout<<"Send fail,toSrv not exist toId: "<< toId <<endl;
        return;
    }
    toSrv->PushMsg(msg);
    bool hasPush = false;

    pthread_spin_lock(&toSrv->inGlobalLock);
    {
        if(!toSrv->inGlobal){
            PushGlobalQueue(toSrv);
            toSrv->inGlobal = true;
            hasPush = true;
        }
    }
    pthread_spin_unlock(&toSrv->inGlobalLock);

    //唤起进程,如果有新服务加入队列就唤起线程
    if(hasPush){
        CheckAndWeak();
    }
}
/****************工作线程*********************************/
void Sunnet::WorkerWait(){
    pthread_mutex_lock(&sleepMtx);
    sleepCount++;
    pthread_cond_wait(&sleepCond,&sleepMtx);
    sleepCount--;
    pthread_mutex_unlock(&sleepMtx);
}

void Sunnet::CheckAndWeak(){
    if(sleepCount == 0){
        return;
    }
    if(WORKER_NUM - sleepCount <= globalLen){
        cout<<":wake up"<<endl;
        pthread_cond_signal(&sleepCond);
    }
}
/**************socket网络**************************/
//创建socket线程
void Sunnet::StartSocket(){
    socketWorker = new SocketWorker();
    socketWorker->Init();
    socketThread = new thread(*socketWorker);
         // 检查线程是否成功创建
    if (socketThread->joinable()) {
        std::cout << "socket线程创建成功" << std::endl;
        // 可以继续操作或等待线程结束
    } else {
        std::cout << "socket线程创建失败" << std::endl;
        // 可以进行适当的错误处理
    }
}

int Sunnet::AddConn(int fd,uint32_t id,Conn::TYPE type){
    auto conn = make_shared<Conn>();
    conn->fd = fd;
    conn->serviceId = id;
    conn->type = type;
    pthread_rwlock_wrlock(&connsLock);
    {
        conns.emplace(fd,conn);
    }
    pthread_rwlock_unlock(&connsLock);

}

shared_ptr<Conn> Sunnet::GetConn(int fd){
    shared_ptr<Conn> conn = nullptr;
    pthread_rwlock_rdlock(&connsLock);
    {
        unordered_map<uint32_t,shared_ptr<Conn>>::iterator iter = conns.find(fd);
        if(iter != conns.end()){
            conn = iter->second;
        }
    }
    pthread_rwlock_unlock(&connsLock);
    return  conn;
}

bool Sunnet::RemoveConn(int fd){
    int result;
    pthread_rwlock_wrlock(&connsLock);
    {
        result = conns.erase(fd);
    }
    pthread_rwlock_unlock(&connsLock);
    return result == 1;
}

int Sunnet::Listen(uint32_t port,uint32_t serviceId){
    //创建socket
    int listenFd = socket(AF_INET,SOCK_STREAM,0);
    if(listen<=0){
        cout<<"listen fail error,listenFd <= 0"<<endl;
        return -1;
    }
    //设置非阻塞
    fcntl(listenFd,F_SETFL,O_NONBLOCK);
    //bind htonl将32位（4字节）整数从主机字节序转换为网络字节序。
    //统一数据大小端
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int r = bind(listenFd,(struct  sockaddr*)&addr,sizeof(addr));
    if(r==-1){
        cout<<"listen error,bind fail"<<endl;
        return -1;
    }
    //listen
    r = listen(listenFd,64);
    if(r < 0){
        return -1;
    }
    //添加到管理结构
    AddConn(listenFd,serviceId,Conn::TYPE::LISTEN);
    socketWorker->AddEvent(listenFd);
    return listenFd;
}

void Sunnet::CloseConn(uint32_t fd){
    bool succ = RemoveConn(fd);
    close(fd);
    if(succ){
        socketWorker->RemoveEvent(fd);
    }
}
























//测试使用
shared_ptr<BaseMsg> Sunnet::MakeMsg(uint32_t source,char* buff,int len){
    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;
    msg->buff = shared_ptr<char>(buff);
    msg->size = len;
    return msg;
}