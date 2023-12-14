#ifndef LUA_PROC_H
#define LUA_PROC_H

#include "typedefs.h"
#include "lua/lua.hpp"
#include "main.h"

namespace LuaProc
{
    extern lua_State* g_Lua;
    extern String* code;

    void Init();
    void Proc(MCUPacket_t* data, MotorState* motorPtr, s32* delayPtr);
    void AddSystemErrorMessage(String error);
    void AddSystemErrorMessage(s32 error);
}

#endif