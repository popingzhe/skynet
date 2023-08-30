#include "Service.h"
#include "Sunnet.h"
#include <iostream>
#include <unistd.h>
#include<string.h>
#include"LuaAPI.h"

Service::Service(){
    //初始化锁
    pthread_spin_init(&queueLock,PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&inGlobalLock,PTHREAD_PROCESS_PRIVATE);
}

Service::~Service(){
    pthread_spin_destroy(&queueLock);
    pthread_spin_destroy(&inGlobalLock);
}

//插入消息
void Service::PushMsg(shared_ptr<BaseMsg> msg){
    pthread_spin_lock(&queueLock);
    {
        msgQueue.push(msg);
    }
    pthread_spin_unlock(&queueLock);
}

shared_ptr<BaseMsg> Service::PopMsg(){
    shared_ptr<BaseMsg> msg = nullptr;
    pthread_spin_lock(&queueLock);
    {
        if(!msgQueue.empty()){
            msg = msgQueue.front();
            msgQueue.pop();
        }
    }
    pthread_spin_unlock(&queueLock);
    return msg;
}

//创建服务后触发
void Service::OnInit(){
    cout<<"【"<<id<<"】 OnInit"<<endl;
    //创建lua虚拟机
    luaState = luaL_newstate();
    luaL_openlibs(luaState);
    //注册Sunnet系统API
    LuaAPI::Register(luaState);
    //设置lua全局变量
    // lua_pushinteger(luaState, id);
    // lua_setglobal(luaState, "serviceId");
    //执行lua文件
    string filename = "../service/"+*type+"/init.lua";
    int isok = luaL_dofile(luaState,filename.data());
    if(isok == 1){//返回1表示失败
        cout<<"run lua fail:"<<lua_tostring(luaState,-1)<<endl;
    }


    //调用Lua函数
    lua_getglobal(luaState,"OnInit");//将全局变量 OnInit 压入 Lua 虚拟机栈顶
    lua_pushinteger(luaState,id);//将整数值 id 压入 Lua 虚拟机栈顶
    isok = lua_pcall(luaState,1,0,0);//调用压入栈顶的函数，传入一个参数，期望返回0个参数
    if(isok != 0){
        cout<<"call lua OnInit fail : "<<lua_tostring(luaState,-1)<<endl;
    }

}
//收到消息触发
void Service::OnMsg(shared_ptr<BaseMsg> msg){
   //SERVICE
   if(msg->type == BaseMsg::TYPE::SERVICE){
        auto m =dynamic_pointer_cast<ServiceMsg>(msg);
        OnServiceMsg(m);
   }
    //SOCKET_ACCEPT
    else if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT){
        auto m = dynamic_pointer_cast<SocketAcceptMsg>(msg);
        OnAcceptMsg(m);
    }
    //SOCKET_RW
    else if(msg->type == BaseMsg::TYPE::SOCKET_RW){
        auto m = dynamic_pointer_cast<SocketRWMsg>(msg);
        OnRWMsg(m);
    }  
}
//退出服务触发
void Service::OnExit(){
    cout<<"【"<<id<<"】 OnExit"<<endl;
    //关闭前调用OnExit
    lua_getglobal(luaState,"OnExit");
    int isok = lua_pcall(luaState,0,0,0);
    if(isok != 0){
        cout<<"call lua OnExit fail : "<<lua_tostring(luaState,-1)<<endl;
    }
    //关闭虚拟机
    lua_close(luaState);
}

//处理消息
bool Service::ProcessMsg(){
    shared_ptr<BaseMsg> msg = PopMsg();
    if(msg){
        OnMsg(msg);
        return true;
    }else{
        return false;//队列空
    }
}

void Service::ProcessMsgs(int max){
    for(int i=0; i<max; i++){
        bool succ = ProcessMsg();
        if(!succ){
            break;
        }
    }
}

void Service::SetInGlobal(bool isIn){
    pthread_spin_lock(&inGlobalLock);
    {
        inGlobal = isIn;
    }
    pthread_spin_unlock(&inGlobalLock);
}


//其他服务间消息
void Service::OnServiceMsg(shared_ptr<ServiceMsg> msg){
    cout<<"OnServiceMsg"<<endl;

    lua_getglobal(luaState,"OnServiceMsg");
    lua_pushinteger(luaState,msg->source);
    lua_pushlstring(luaState,msg->buff.get(),msg->size);//msg->buff.get()获取数据
    int isok = lua_pcall(luaState,2,0,0);
    if(isok != 0){
        cout<<"call lua OnServiceMsg fail "<<lua_tostring(luaState,-1)<<endl;
    }
}
//接受其他socket
void Service::OnAcceptMsg(shared_ptr<SocketAcceptMsg> msg){
    cout<<"OnAcceptMsg"<<endl;
    lua_getglobal(luaState,"OnAcceptMsg");
    lua_pushinteger(luaState,msg->listenFd);
    lua_pushinteger(luaState,msg->cliendFd);
    int isok = lua_pcall(luaState,2,0,0);
    if(isok != 0){
        cout<<"call lua OnServiceMsg fail "<<lua_tostring(luaState,-1)<<endl;
    }
}

//套接字读写
void Service::OnRWMsg(shared_ptr<SocketRWMsg> msg){
    int fd = msg->fd;
    if(msg->isRead){
        const int BUFFSIZE = 512;
        char buff[BUFFSIZE];
        int len = 0;
        do
        {
            len = read(fd,buff,BUFFSIZE);
            if(len>0){
                OnSocketData(fd,buff,len);
            }
        } while (len == BUFFSIZE);
        
        if( len<=0 && errno != EAGAIN){
            if(Sunnet::inst->GetConn(fd)){
                OnSocketClose(fd);
                Sunnet::inst->CloseConn(fd);
            }
        }
    }

    if(msg->isWrite){
        if(Sunnet::inst->GetConn(fd)){
            OnSocketWritable(fd);
        }
    }

}


void Service::OnSocketData(int fd,const char* buff,int len){
    cout<<"OnSocketData "<<fd<<"buff:"<<buff<<endl;
    lua_getglobal(luaState,"OnSocketData");
    lua_pushinteger(luaState,fd);
    lua_pushlstring(luaState,buff,len);
    int isok = lua_pcall(luaState,2,0,0);
    if(isok != 0){
        cout<< "call lua OnSocketData fail "<<lua_tostring(luaState,-1)<<endl;
    }
}

void Service::OnSocketWritable(int fd){
    cout<<"OnSocketWritable "<< fd <<endl;
}

void Service::OnSocketClose(int fd){
    cout<<"OnSocketClose "<<fd<<endl;
    lua_getglobal(luaState,"OnSocketClose");
    lua_pushinteger(luaState,fd);
    int isok = lua_pcall(luaState,1,0,0);
    if(isok != 0){
        cout<< "call lua OnSocketClose fail "<<lua_tostring(luaState,-1)<<endl;
    }
}
