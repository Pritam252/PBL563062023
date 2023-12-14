#include "main.h"
#include "packet_proc.h"
#include "typedefs.h"
#include "consts.h"

#define LCD_DISPLAY_SUPPORT

using namespace MAIN;

//Boolean to determine whether station or softap will be used.
//TODO: Read from switch.
BOOL MAIN::useSoftAPMode = true;

//For STATION Mode
String MAIN::station_ssid = "";
String MAIN::station_password = "";

//For SoftAP Mode
String MAIN::softap_ssid = "";
String MAIN::softap_password = "";

//Maximum Message Buffer size.
const int SYSTEM_MESSAGES_COUNT_MAX = 10;

BOOL ledState = LOW;
const f32 ledPin = LED_BUILTIN;

//LiquidCrystal Display.
LiquidCrystal_I2C lcd(LCD_ADDR,16,2);

//Define some variables.
String MAIN::LuaProgram = "";
f32 dust = 0.0;
f32 moist = 0.0;
f32 temp = 0.0;
BOOL motion = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//Use the Vector one below instead of the String array directly.
String SystemMessagesArray[SYSTEM_MESSAGES_COUNT_MAX];

//Use this one instead of the one above.
Vector<String> MAIN::SystemMessages(SystemMessagesArray);

//I2C Return packets.
MCUPacket_t lastPacketRecieved;
#define RPacket lastPacketRecieved

MotorState toSendPacket;
#define TPacket toSendPacket

//Defined functions.
void handleStationSSIDPostBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (request->_tempObject == NULL) {
    request->_tempObject = new char[total + 1];
  }
  char *tempObject = (char *)request->_tempObject;
  for (int i = 0; i < (int)len; i++) {
    tempObject[index + i] = (char)data[i];
  }
  tempObject[total] = '\0';
  if (index + len == total) {
    station_ssid = tempObject;
    //Save code to file.
    File file = LittleFS.open("/station_ssid.t","w");
    if (!file)
    {
      Serial.print("Couldn't open ");
      Serial.print("/station_ssid.t");
      Serial.println(", error condition triggered!");
      return;
    }
    if (file.print(station_ssid))
    {
      Serial.print("Code saved to ");
      Serial.println(station_ssid);
    }
    else
    {
      Serial.print("Code failed to save to ");
      Serial.println(station_ssid);
    }
    file.close();
    return;
  }
}

void handleStationPASSPostBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (request->_tempObject == NULL) {
        request->_tempObject = new char[total + 1];
      }
      char *tempObject = (char *)request->_tempObject;
      for (int i = 0; i < (int)len; i++) {
        tempObject[index + i] = (char)data[i];
      }
      tempObject[total] = '\0';
      if (index + len == total) {
        station_password = tempObject;

        //Save code to file.
        File file = LittleFS.open("/station_pass.t","w");
        if (!file)
        {
          Serial.print("Couldn't open ");
          Serial.print("/station_pass.t");
          Serial.println(", error condition triggered!");
          return;
        }
        if (file.print(station_password))
        {
          Serial.print("Code saved to ");
          Serial.println("/station_pass.t");
        }
        else
        {
          Serial.print("Code failed to save to ");
          Serial.println("/station_pass.t");
        }
        file.close();
        return;
      }
    }

void handleSoftAPSSIDPostBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (request->_tempObject == NULL) {
    request->_tempObject = new char[total + 1];
  }
  char *tempObject = (char *)request->_tempObject;
  for (int i = 0; i < (int)len; i++) {
    tempObject[index + i] = (char)data[i];
  }
  tempObject[total] = '\0';
  if (index + len == total) {
    softap_ssid = tempObject;
    //Save code to file.
    File file = LittleFS.open("/softap_ssid.t","w");
    if (!file)
    {
      Serial.print("Couldn't open ");
      Serial.print("/softap_ssid.t");
      Serial.println(", error condition triggered!");
      return;
    }
    if (file.print(softap_ssid))
    {
      Serial.print("Code saved to ");
      Serial.println("/softap_ssid.t");
    }
    else
    {
      Serial.print("Code failed to save to ");
      Serial.println("/softap_ssid.t");
    }
    file.close();
    return;
  }
}

void handleSoftAPPassPostBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (request->_tempObject == NULL) {
    request->_tempObject = new char[total + 1];
  }
  char *tempObject = (char *)request->_tempObject;
  for (int i = 0; i < (int)len; i++) {
    tempObject[index + i] = (char)data[i];
  }
  tempObject[total] = '\0';
  if (index + len == total) {
    softap_password = tempObject;
    //Save code to file.
    File file = LittleFS.open("/softap_pass.t","w");
    if (!file)
    {
      Serial.print("Couldn't open ");
      Serial.print("/softap_pass.t");
      Serial.println(", error condition triggered!");
      return;
    }
    if (file.print(softap_password))
    {
      Serial.print("Code saved to ");
      Serial.println("/softap_pass.t");
    }
    else
    {
      Serial.print("Code failed to save to ");
      Serial.println("/softap_pass.t");
    }
    file.close();
    return;
  }
}

/*
Top index for the Vector is SYSTEM_MESSAGES_COUNT_MAX-1,
At index 0 (delete)
So we use a for loop to repeatingly move everything down starting at index 1
To index SYSTEM_MESSAGES_COUNT_MAX-1 (just SYSTEM_MESSAGES_COUNT_MAX is sufficient because of
the for loop)
*/
void MAIN::ReserveSpaceForSystemMessages() {
  for (int index = 1; index < SYSTEM_MESSAGES_COUNT_MAX; index++) {
    MAIN::SystemMessages[index-1] = MAIN::SystemMessages[index]; 
  }
}

void MAIN::AddSystemMessage(String message) {
  //Reserve some space for new message
  MAIN::ReserveSpaceForSystemMessages();
  MAIN::SystemMessages[SYSTEM_MESSAGES_COUNT_MAX-1] = message;
}

const size_t SYSTEM_MESSAGE_JSON_CAPACITY = JSON_ARRAY_SIZE(SYSTEM_MESSAGES_COUNT_MAX);

//Function to return all the system messages in a json array format.
String MAIN::AllSystemMessagesJSON() {
  String returnString = "";
  StaticJsonDocument<SYSTEM_MESSAGE_JSON_CAPACITY> doc;
  JsonArray msgArray = doc.to<JsonArray>();
  for (int index = 0; index < SYSTEM_MESSAGES_COUNT_MAX; index++) {
    msgArray.add(MAIN::SystemMessages[index].c_str());
  }
  serializeJson(doc, returnString);
  return returnString;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      ledState = !ledState;
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

//WebSocket Init function:
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}



//Functions to handle All Post Requests
BOOL isBusyRecievingCode = false;
void handleLuaPostBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (request->_tempObject == NULL) {
    isBusyRecievingCode = true;
    request->_tempObject = new char[total + 1];
  }
  char *tempObject = (char *)request->_tempObject;
  for (int i = 0; i < (int)len; i++) {
    tempObject[index + i] = (char)data[i];
  }
  tempObject[total] = '\0';
  if (index + len == total) {
    MAIN::LuaProgram = tempObject;
    isBusyRecievingCode = false;

#ifdef DEBUG_FEATURE
    Serial.print("Recieved Code: ");
    Serial.println(LuaProgram);
#endif

    //Save code to file.
    File file = LittleFS.open("/code.lua","w");
    if (!file)
    {
      Serial.println("Couldn't open code.lua, error condition triggered!");
      return;
    }
    if (file.print(MAIN::LuaProgram))
    {
      Serial.println("Code saved to code.lua.");
    }
    else
    {
      Serial.println("Code failed to save to code.lua.");
    }
    file.close();
    return;
  }
}

s32 MAIN::ScanI2CBus()
{
  int nDevices = 0;

  Serial.println("Scanning...");

  for (byte address = 1; address < 127; ++address) {
    // The i2c_scanner uses the return value of
    // the Wire.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");

      ++nDevices;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
  return nDevices;
}

BOOL MAIN::LoadLuaCodeFromFlash()
{
  File file = LittleFS.open("/code.lua", "r");
  if (!file)
  {
    //delay_out, motor_out
    MAIN::LuaProgram = "delay_out = 2000\n motor_out = false";
    Serial.println("Lua code load from flash failed, default code loaded.");
    return false;
  }
  MAIN::LuaProgram = file.readString();
  file.close();
  return true;
}

u8 MAIN::LoadNetworkDataFromFlash()
{
  /*
  /station_ssid.t
  /station_pass.t
  /softap_ssid.t
  /softap_pass.t
  */

  result_flag_t result = 0x00;

  //StationSSID
  File station_ssid_file = LittleFS.open("/station_ssid.t", "r");
  if (!station_ssid_file) {
    result += 0x01;
  } else {
    station_ssid = station_ssid_file.readString();
    station_ssid_file.close();
  }

  //StationPass
  File station_pass_file = LittleFS.open("/station_pass.t", "r");
  if (!station_pass_file) {
    result += 0x02;
  } else  {
    station_password = station_pass_file.readString();
    station_pass_file.close();
  }

  //SoftAPSSID
  File softap_ssid_file = LittleFS.open("/softap_ssid.t", "r");
  if (!softap_ssid_file) {
    result += 0x04;
  } else {
    softap_ssid = softap_ssid_file.readString();
    softap_ssid_file.close();
  }

  //SoftAPPass
  File softap_pass_file = LittleFS.open("/softap_pass.t", "r");
  if (!softap_pass_file) {
    result += 0x08;
  } else {
    softap_password = softap_pass_file.readString();
    softap_pass_file.close();
  }

  //No error.
  return result;
}

void setup()
{
    Serial.begin(115200);

    Wire.begin();
    
#ifdef DEBUG_FEATURE
    //DEBUG:
    //Scan the entire I2C Bus for devices.
    ScanI2CBus();
#endif

    //Init variables
    toSendPacket = true;

    LittleFS.begin();

    //Pulldown arduino
    pinMode(D3, OUTPUT);
    //useSOFTAP_SW (external pulldown)
    pinMode(D6, INPUT);
    //MOTOR
    pinMode(D5, OUTPUT);

    //FOR DEBUG
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    //END DEBUG

    //SECTION WIFI READ
    //Determine whether to use SoftAP or Station
    MAIN::useSoftAPMode = false;//digitalRead(D6);
    //ENDSECTION

    //Make sure to tell arduino to keep motor off during bootup.
    Wire.beginTransmission(AVR_ADDR);
    Wire.write(false); //MotorState false;
    Wire.endTransmission();

    //Load lua code functions:
    LoadLuaCodeFromFlash();

    //Load network information.
    LoadNetworkDataFromFlash();

    //SECTION LCD
#ifdef LCD_DISPLAY_SUPPORT
    lcd.begin();
    lcd.backlight();
    //ENDSECTION LCD

    //SECTION WIFI
    lcd.print("Connecting WiFi:");
    lcd.setCursor(2,1);
    lcd.print(MAIN::useSoftAPMode ? MAIN::softap_ssid : MAIN::station_ssid);
#endif

    String connect_ssid;

    if (MAIN::useSoftAPMode) {
      connect_ssid = MAIN::softap_ssid;
      WiFi.softAP(MAIN::softap_ssid.c_str(), MAIN::softap_password.c_str());
    }
    else {
      connect_ssid = MAIN::station_ssid;
      WiFi.begin(MAIN::station_ssid.c_str(), MAIN::station_password.c_str());
      Serial.println(MAIN::station_ssid);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
      }
    }

    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());
    //As well as on the lcd.

  #ifdef LCD_DISPLAY_SUPPORT
    lcd.clear();
    lcd.setCursor(1,1);
    lcd.print(connect_ssid);
    lcd.setCursor(2,0);
    lcd.print(WiFi.localIP());
  #endif
    //ENDSECTION WIFI

    //Some debug code
    MAIN::AddSystemMessage("Dust & Heat detector & purifier booted up!");

    //Start WebSocket service.
    initWebSocket();

    //INDEX.html main page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/index.html");
    });

    //All POST Request handlers
    server.on("/code_post", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (isBusyRecievingCode) {
        request->send(200, "text/plain", "server is already recieving code");
      } else {
        request->send(200, "text/plain", "code recieved");
      }
    }, NULL, handleLuaPostBody);

    //GET "code"
    server.on("/code", HTTP_GET, [](AsyncWebServerRequest *request) {
      //Return the current stored lua code on this device
      //as the body of the response.
      request->send(200, "text/plain", MAIN::LuaProgram);
    });

    //GET "sys_msgs"
    server.on("/sys_msgs", HTTP_GET, [](AsyncWebServerRequest *request) {
      //Return all the messages in the system.
      request->send(200, "text/plain", MAIN::AllSystemMessagesJSON());
    });

    //GET "data"
    const size_t SENSOR_DATA_JSON_CAPACITY = 64; //Calculated using https://arduinojson.org/v6/assistant/
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
      String returnString;
      StaticJsonDocument<SENSOR_DATA_JSON_CAPACITY> returnDoc;
      returnDoc["dust"] = dust;
      returnDoc["temp"] = temp;
      returnDoc["moist"] = moist;
      returnDoc["motion"] = (bool)motion;
      serializeJson(returnDoc, returnString);
      request->send(200, "text/plain", returnString);
    });

    //Deal with the "gauge.js" resource file
    server.on("/gauge.js", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/gauge.js");
    });

    //Deal with jQuery library.
    server.on("/jquery.js", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/jquery.js");
    });

    //Change StationSSID
    server.on("/stationssid_change", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "StationSSID Recieved!");
    }, NULL, handleStationSSIDPostBody);

    //Get StationSSID
    server.on("/stationssid_get", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", station_ssid);
    });
    
    //Change StationPassword
    server.on("/stationpass_change", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "StationPass Recieved!");
    }, NULL, handleStationPASSPostBody);

    //Change SoftAPSSID
    server.on("/softapssid_change", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "SoftAPSSID Recieved!");
    }, NULL, handleSoftAPSSIDPostBody);

    //Get SoftAPSSID
    server.on("/softapssid_get", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", softap_ssid);
    });

    //Change SoftAPPassword
    server.on("/softappass_change", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "SoftAPPass Recieved!");
    }, NULL, handleSoftAPPassPostBody);

    //Actually start the server
    server.begin();

    //SECTION I2C
    //UPDATE: Pritam: Apparently NodeMCU doesnt even support slave for I2C.
    //Had to rewrite everything at 11:30PM.. (04d/12m/2023y)
    //Code to begin Arduino I2C (is it already connected by the lcd?).
    //Wire.begin(D1, D2);

    //Tell arduino that we're ready.
    digitalWrite(D3, HIGH);

    //ENDSECTION I2C

    //Lua
    LuaProc::Init();
}

f32 old_dust = 0.0;
f32 old_temp = 0.0;
f32 old_moist = 0.0;
BOOL old_motion = false;

void loop()
{
    //Added so that disconnected clients don't take up socket slots.
    ws.cleanupClients();

#ifdef SERIAL_DBG
    //Update I2C Data here:
    //Now request Arduino for sensor data.
    /*
    Use 13 explicitly because apparently gcc is padding the struct to 16
    (ESP8266 is 32 bit.) so it pads it by a multiple of 4.
    */
    Wire.requestFrom(AVR_ADDR, sizeof(MCUPacket_t));
    //Read the data and put it into RPacket
    Serial.println("f32 size: " + String(sizeof(f32)));
    Serial.println("uint8_t size: " + String(sizeof(uint8_t)));
    Serial.println("MCUPacket_t size: " + String(sizeof(MCUPacket_t)));
#endif

    //Read data from Arduino.
    Wire.requestFrom(AVR_ADDR, sizeof(MCUPacket_t));
    PacketProc::I2C_readAnything(RPacket);
    dust = RPacket.dust;
    temp = RPacket.temp;
    moist = RPacket.moist;
    motion = RPacket.motion;

    //Read system message list and if message list isn't empty then
    //execute WS "sys_msg"
    if (!MAIN::SystemMessages.empty()) {
      ws.textAll("sys_msg");
    }

    //We're going to compare the old variables with the new ones to check whether
    //to inform all clients about data being changed.
    //(this is done so that we don't waste unnecessary bandwidth)
    BOOL shouldUpdate = false;
    shouldUpdate = shouldUpdate || (old_dust != dust);
    shouldUpdate = shouldUpdate || (old_temp != temp);
    shouldUpdate = shouldUpdate || (old_moist != moist);
    shouldUpdate = shouldUpdate || (old_motion != motion);

    if (shouldUpdate) {
      ws.textAll("update");
    }

    //Lua execution.
    //Default values.
    MotorState motorState = false;
    s32 pollDelay = 2000;
    LuaProc::Proc(&RPacket, &motorState, &pollDelay);

    //Arduino write back.
    Wire.beginTransmission(AVR_ADDR);
    Wire.write(motorState);
    Wire.endTransmission();
    
    //WARNING, CODE BELOW SHOULD ALWAYS BE BELOW ANY LOGIC IMPORTANT CODE.
    //Update the old variables.
    old_dust = dust;
    old_temp = temp;
    old_moist = moist;
    old_motion = motion;

    //Uses data from Lua instead now.
    delay(pollDelay);
}