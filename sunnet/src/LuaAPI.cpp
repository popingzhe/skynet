#include"LuaAPI.h"
#include<stdint.h>
#include"Sunnet.h"
#include<unistd.h>
#include<string.h>
#include<iostream>
using namespace std;
//注册lua模块
//注册Lua模块
void LuaAPI::Register(lua_State *luaState) {
    
    static luaL_Reg lualibs[] = {
        { "NewService", NewService },
        { "KillService", KillService },
        { "Send", Send },

        { "Listen", Listen },
        // { "CloseConn", CloseConn },
        { "Write", Write },
        { NULL, NULL }
    };

    luaL_newlib (luaState, lualibs);
    lua_setglobal(luaState, "sunnet");
}

//开启新服务
int LuaAPI::NewService(lua_State *luaState){
    //参数个数
    int num = lua_gettop(luaState);
    //参数1：服务类型
    if(lua_isstring(luaState,1)==0){
        lua_pushinteger(luaState,-1);
        return 1;
    }
    //将栈上索引为 1 的值转换为 C 字符串类型,len为长度
    size_t len = 0;
    const char *type = lua_tolstring(luaState,1,&len);
    char * newstr = new char[len+1];
    newstr[len] = '\0';
    memcpy(newstr,type,len);
    auto t = make_shared<string>(newstr);
    //处理
    uint32_t id = Sunnet::inst->NewService(t);
    //返回id
    
    lua_pushinteger(luaState,id);
    return 1;
}

int LuaAPI::KillService(lua_State *luaState){
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState,1) == 0){
        return 0;
    }
    int id = lua_tointeger(luaState,1);
    Sunnet::inst->KillService(id);
    return 0;
}

int LuaAPI::Send(lua_State *luaState){
    int num = lua_gettop(luaState);
    if(num != 3){
        cout<<"Send fail,num err"<<endl;
        return 0;
    }

    if(lua_isinteger(luaState,1) == 0){
        cout<<"Send fail,arg1 err"<<endl;
        return 0;
    }
    int source = lua_tointeger(luaState,1);

    if(lua_isinteger(luaState,2) == 0){
        cout<<"Send fail,arg2 err"<<endl;
        return 0;
    }
    int toId = lua_tointeger(luaState,2);

    if(lua_isstring(luaState,3) == 0){
        cout<<"Send fail,arg3 err"<<endl;
        return 0;
    }
    size_t len = 0;
    const char *type = lua_tolstring(luaState,3,&len);
    char * newstr = new char[len];
    memcpy(newstr,type,len);

    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;
    msg->buff = shared_ptr<char>(newstr);
    msg->size = len;
    Sunnet::inst->Send(toId,msg);
    return 0;
}

int LuaAPI::Write(lua_State *luaState){
    int num = lua_gettop(luaState);
    //参数fd
    if(lua_isinteger(luaState,1) == 0){
        lua_pushinteger(luaState,-1);
        return 1;
    }   
    int fd = lua_tointeger(luaState,1);
    //参数data
    if(lua_isstring(luaState,2)==0){
        lua_pushinteger(luaState,-1);
        return 1;
    }
    size_t len = 0;
    const char* buff = lua_tolstring(luaState,2,&len);
    int r = write(fd,buff,len);
    lua_pushinteger(luaState,r);
    return 1;
}

int LuaAPI::Listen(lua_State *luaState){
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState,1) == 0){
        lua_pushinteger(luaState,-1);
        return 1;
    }
    int port =  lua_tointeger(luaState,1); 

    if(lua_isinteger(luaState,1)==0){
        lua_pushinteger(luaState,-1);
        return 1;
    }
    int serviceId = lua_tointeger(luaState,2);
    int r =Sunnet::inst->Listen(port,serviceId);
    lua_pushinteger(luaState,r);
    return 1;
}