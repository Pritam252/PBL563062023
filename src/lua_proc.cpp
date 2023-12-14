#include "lua_proc.h"

lua_State* LuaProc::g_Lua;

void LuaProc::Init() {
    //Actually create the state.
    g_Lua = luaL_newstate();
    //WARNING: should add error handling.
}

//Uncomment this for LuaDebug.
//#define SERIAL_DBG
#ifdef SERIAL_DBG
#define PRINT(MSG) Serial.println(MSG)
#else
#define PRINT(MSG)
#endif

void LuaProc::Proc(MCUPacket_t* data, MotorState* motorPtr, s32* delayPtr) {
    //Dust data.
    lua_pushnumber(g_Lua, data->dust);
    lua_setglobal(g_Lua, "dust_in");
    PRINT("dust pass");
    //Temp data.
    lua_pushnumber(g_Lua, data->temp);
    lua_setglobal(g_Lua, "temp_in");
    PRINT("temp pass");
    //Moist data.
    lua_pushnumber(g_Lua, data->moist);
    lua_setglobal(g_Lua, "moist_in");
    PRINT("moist pass");
    //Motion data.
    lua_pushboolean(g_Lua, data->motion);
    lua_setglobal(g_Lua, "motion_in");
    PRINT("motion pass");

    //Run processing.
    luaL_loadstring(g_Lua, MAIN::LuaProgram.c_str());
    PRINT("execute pass");
    s32 result = lua_pcall(g_Lua, 0, LUA_MULTRET, 0);
    if (LUA_OK != result)
    {
        //If pcall returns an error.
        AddSystemErrorMessage(result);
        return;
    }

    //Following code is called only if there were no errors during execution of VM code.

    //Read output values.
    s32 delay_out = 2000;
    lua_getglobal(g_Lua, "delay_out");
    //didnt use luaL_checkinteger because we needed custom error handling.
    //SECTION: read delay data from VM.
    s32 isDelayNum;
    s32 delay_got = lua_tointegerx(g_Lua, -1, &isDelayNum);
    if (isDelayNum) {
        //set it.
        delay_out = delay_got;
    } else {
        AddSystemErrorMessage("delay_out must be set.");
    }
    //ENDSECTION
    //SECTION: read motorstate data from VM.
    MotorState mState = false;
    lua_getglobal(g_Lua, "motor_out");
    s32 isMotorBool = lua_isboolean(g_Lua, -1);
    if (isMotorBool) {
        //cast the value on stack.
        mState = lua_toboolean(g_Lua, -1);
    } else {
        AddSystemErrorMessage("motor_out must be a boolean");
    }
    //ENDSECTION
    *motorPtr = mState;
    *delayPtr = delay_out;
    return;
}

void LuaProc::AddSystemErrorMessage(String error)
{
    //Call the function to add the Message.
    MAIN::AddSystemMessage("LUA ERROR: " + error);
}

void LuaProc::AddSystemErrorMessage(s32 error)
{
    //Call the function to add the Message.
    MAIN::AddSystemMessage("LUA PCALL ERROR, code: " + String(error));
}