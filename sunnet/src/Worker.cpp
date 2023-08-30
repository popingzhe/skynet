#include <iostream>
#include <unistd.h>
#include "Worker.h"
#include "Service.h"
#include "Sunnet.h"
using namespace std;

void Worker::operator() (){
    while (true)
    {
        shared_ptr<Service> srv = Sunnet::inst->PopGlobalQueue();
        if(!srv){
          Sunnet::inst->WorkerWait();
          cout<<id<<" sleep"<<endl;
        }else{
            srv->ProcessMsgs(eachNum);
            CheckAndPutGlobal(srv);
        }
    }
}

void Worker::CheckAndPutGlobal(shared_ptr<Service> srv){
    if(srv->isExiting){
        return;
    }

    pthread_spin_lock(&srv->queueLock);
    {
        if(!srv->msgQueue.empty()){
            Sunnet::inst->PushGlobalQueue(srv);
        }
        else{
            srv->SetInGlobal(false);
        }
    }
    pthread_spin_unlock(&srv->queueLock);
}