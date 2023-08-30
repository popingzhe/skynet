#pragma once

extern "C" {
    #include "lua.h"
}

using namespace std;

class LuaAPI{
public:
    static void Register(lua_State *luaState);

    static int NewService(lua_State *lua_State);
    static int KillService(lua_State *lua_State);
    static int Send(lua_State *lua_State);

    static int Listen(lua_State *lua_State);
    static int CloseConn(lua_State *lua_State);
    static int Write(lua_State *lua_State);
};