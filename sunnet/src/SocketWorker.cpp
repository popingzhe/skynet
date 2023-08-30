#include "SocketWorker.h"
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <sys/epoll.h>
#include <string.h>
#include "Sunnet.h"
#include <fcntl.h>
#include <sys/socket.h>
#include "Msg.h"

void SocketWorker::Init(){
    cout<< "【socket】: init" <<endl;
    epoll = epoll_create(1024);//非负表示创建成功
    cout<< "【epoll】:"<<epoll<<endl;
    assert(epoll > 0);
}

void SocketWorker::operator()(){
    while (true)
    {   
        const int EVENT_SIZE = 64;
        struct epoll_event events[EVENT_SIZE];
        int eventCount = epoll_wait(epoll,events,EVENT_SIZE,-1);

        for(int i=0; i<eventCount; i++){
            epoll_event ev = events[i];
            OnEvent(ev);
        }
    }
    
}

void SocketWorker::AddEvent(int fd){
    cout<< "AddEvent fd:"<<fd<<endl;
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLET;
    ev.data.fd = fd;
    if(epoll_ctl(epoll,EPOLL_CTL_ADD,fd,&ev) == -1){
        cout<<"AddEvent epoll_ctl Fail:"<<strerror(errno)<<endl;
    }
}

void SocketWorker::RemoveEvent(int fd){
    cout<< "RemoveEvent fd:"<<fd<<endl;
    epoll_ctl(epoll,EPOLL_CTL_DEL,fd,NULL);

}

void SocketWorker::ModifyEvent(int fd,bool epollOut){
    cout<<"ModifyEvent fd "<<fd<<" "<<epollOut<<endl;
    struct epoll_event ev;
    ev.data.fd = fd;
    if(epollOut){
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    }else{
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epoll,EPOLL_CTL_MOD,fd,&ev);
}

void SocketWorker::OnEvent(epoll_event ev){
    cout<<"OnEVent"<<endl;
    int fd = ev.data.fd;
    auto conn = Sunnet::inst->GetConn(fd);
    cout<<"Onevent conn: "<<conn->type<<" "<<conn->serviceId<<endl;
    if(conn == NULL){
        cout<<"OnEvent error,conn == NULL"<<endl;
        return;
    }
    bool isRead = ev.events & EPOLLIN;
    bool isWrite = ev.events & EPOLLOUT;
    bool isError = ev.events & EPOLLERR;
    //监听socket
    if(conn->type == Conn::TYPE::LISTEN){
        if(isRead){
            OnAccept(conn);
        }      
    }else{
        if(isRead||isWrite){
            OnRW(conn,isRead,isWrite);
        }
        if(isError){
            cout<<"OnError fd"<<conn->fd<<endl;
        }
    }
}

void SocketWorker::OnAccept(shared_ptr<Conn> conn){
    cout<<"OnAccept fd "<<conn->fd<<endl;
    int clientfd = accept(conn->fd,NULL,NULL);
    if(clientfd < 0){
        cout<<"accept error"<<endl;
    }
    //设置非阻塞
    fcntl(clientfd,F_SETFL,O_NONBLOCK);
    //写缓冲区大小
    unsigned long buffSize = 4294967295;
    if(setsockopt(clientfd,SOL_SOCKET,SO_SNDBUFFORCE,&buffSize,sizeof(buffSize))<0){
        cout<<"OnAccept setsocket Fail "<<strerror(errno)<<endl;
    }
    //添加连接对象
    Sunnet::inst->AddConn(clientfd,conn->serviceId,Conn::TYPE::CLIENT);
    struct epoll_event ev;
    ev.data.fd = clientfd;
    ev.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll,EPOLL_CTL_ADD,clientfd,&ev)==-1){
        cout << "OnAccept epoll_ctl_add error "<<strerror(errno)<<endl;
    }
    auto msg = make_shared<SocketAcceptMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_ACCEPT;
    msg->listenFd = conn->fd;
    msg->cliendFd = clientfd;
    Sunnet::inst->Send(conn->serviceId,msg);
}

void SocketWorker::OnRW(shared_ptr<Conn> conn,bool r,bool w){
    cout<<"OnRW fd "<<conn->fd<<endl;
    auto msg = make_shared<SocketRWMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_RW;
    msg->fd = conn->fd;
    msg->isRead = r;
    msg->isWrite = w;
    Sunnet::inst->Send(conn->serviceId,msg);
    
}




