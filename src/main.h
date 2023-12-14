#ifndef MAIN_H
#define MAIN_H

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Vector.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"

#include "Arduino.h"
#include "lua_proc.h"
#include "typedefs.h"

namespace MAIN
{
    extern BOOL useSoftAPMode;
    extern String station_ssid;
    extern String station_password;
    extern String softap_ssid;
    extern String softap_password;
    extern Vector<String> SystemMessages;
    extern String LuaProgram;
    
    void ReserveSpaceForSystemMessages();
    void AddSystemMessage(String message);
    String AllSystemMessagesJSON();
    s32 ScanI2CBus();
    BOOL LoadLuaCodeFromFlash();
    BOOL LoadNetworkDataFromFlash();
}

#endif