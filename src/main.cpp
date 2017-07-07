#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <ProcessScheduler.h>
#include <Ticker.h>
#include <FS.h>
#include <Hash.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "AsyncJson.h"
#include "ArduinoJson.h"

#include "Hardware/PinConfig.h"

#include "Hardware/Hardware.h"
#include "Webserver/Webserver.h"

//#include "Tasks.h"

Hardware hardware;
Webserver webserver;

#define ENABLE_DEBUG_OUTPUT true

const char * hostName = "LightControl";

bool debug = false;
bool APMode = false;

const char* ssid = "FuckingAwesomeNet";
const char* password = "5bier10schnaps";

//const char* ssid = "FRITZ!Box 7490";
//const char* password = "10schnaps1bier";

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);


String onGetChannels(char data[]) {
  String status = "";

  for (uint8_t roomNum = 0; roomNum <  NUM_ROOMS; roomNum++) {
      for (uint8_t channelNum = 0; channelNum < NUM_CHANNELS; channelNum++ ) {
        status += String(roomNum) + "," + channelNum + "," + hardware.channel[roomNum][channelNum] + ";";
      }
   }

   //Serial.println("{\"data\":\"" + status + "\"}");
   return "{\"data\":\"" + status + "\"}";
}

void updateChannels(char payload[]) {
  StaticJsonBuffer<2048> jsonBuffer;

  JsonArray& root = jsonBuffer.parseArray(payload);

  for(JsonArray::iterator it = root.begin(); it != root.end(); ++it) {
    JsonObject& data = it->asObject();

    int roomNum = data["room"];
    int channelNum = data["channel"];
    unsigned long value = data["value"];

    hardware.setChannel(roomNum, channelNum, value);
  }
}

void channelsTask() {
   hardware.updateChannels();
   hardware.updateRelay();

   webserver.ws.textAll(onGetChannels(""));
}


// ------------------------ Tasks ------------------------

class WebserverProcess : public Process {
public:
    WebserverProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}

protected:
    virtual void service() {
        ArduinoOTA.handle();
    }
};

class HardwareProcess : public Process {
public:
    HardwareProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}

protected:
    virtual void service() {
      hardware.tick();
    }
};

class TimedEventProcess : public Process {
public:
    TimedEventProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}

protected:
    virtual void service() {

    }
};

class ChannelsEventProcess : public Process {
public:
    ChannelsEventProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}

protected:
    virtual void service() {
      channelsTask();
    }
};

Scheduler sched;

WebserverProcess webserverProc(sched, HIGH_PRIORITY, SERVICE_CONSTANTLY, RUNTIME_FOREVER);
HardwareProcess hardwareProc(sched, HIGH_PRIORITY, SERVICE_CONSTANTLY, RUNTIME_FOREVER);
TimedEventProcess timedEventProc(sched, LOW_PRIORITY, 1000, RUNTIME_FOREVER);
ChannelsEventProcess channelsEventProc(sched, HIGH_PRIORITY, SERVICE_CONSTANTLY, 1);

void attachTasks() {
  hardwareProc.add();
  hardwareProc.enable();

  webserverProc.add();
  webserverProc.enable();

  timedEventProc.add();
  timedEventProc.enable();

  channelsEventProc.add();
  channelsEventProc.enable();
}

void updateChannelsTask() {
  channelsEventProc.restart();
  channelsEventProc.setIterations(1);
  channelsEventProc.enable();
}

// ------------------------ END Tasks ------------------------

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    //client->textAll(status);
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String payload = "";

    if(info->final && info->index == 0 && info->len == len) {
      if(info->opcode == WS_TEXT) {
        for(size_t i=0; i < info->len; i++) {
          payload += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          payload += buff;
        }
      }
      Serial.printf("%s\n", payload.c_str());

      if(info->opcode == WS_TEXT) {
        client->text("received message: " + String(payload));
        //websocketTextOperation(payload);
      }
    }
  }
}


void attachOta() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setPassword("admin1337");

  ArduinoOTA.onStart([]() {
    webserver.events.send("Update Start", "ota");

    hardware.setBlinkLed(0.2);
  });

  ArduinoOTA.onEnd([]() {
    webserver.events.send("Update End", "ota");

    hardware.setLed(true);

    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    webserver.events.send(p, "ota");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    hardware.setBlinkLed(0.8);

    if(error == OTA_AUTH_ERROR) webserver.events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) webserver.events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) webserver.events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) webserver.events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) webserver.events.send("End Failed", "ota");
  });

  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();
}

void connectWiFi() {
  Serial.println("Press button for two seconds to clear WiFi settings!");

  delay(2000);

  if(hardware.isButtonPressed()) {
    Serial.println("Begin WiFi Smart Config");

    hardware.setBlinkLed(0.2);

    WiFi.mode(WIFI_STA);
    delay(500);

    WiFi.beginSmartConfig();

    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");

      if(WiFi.smartConfigDone()) {
        Serial.println("WiFi Smart Config Done.");
      }
    }

    Serial.println("");
    Serial.println("");

    WiFi.printDiag(Serial);

    Serial.println("");
    Serial.print("Connected to: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); // disable for ESP connect co   nfig mode

  if (WiFi.SSID()) {
    Serial.println("Using last saved values, should be faster");

    WiFi.begin();

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No saved credentials");
  }
}

void interruptGateway() {
  hardware.updateOnInterrupt();
}

void onSetChannel(char data[]) {
  updateChannels(data);
  updateChannelsTask();
}

void setup() {
    hardware.init();
    hardware.setYellowBlinkLed(0.5);

    attachInterrupt(BTN, interruptGateway, CHANGE);

    delay(10);

    connectWiFi();
    attachOta();
    updateChannelsTask();

    webserver.ws.onEvent(onWsEvent);

    webserver.onSetWebsocketText([](String payload) {
      Serial.println(payload);
    });

    webserver.onSetChannels(onSetChannel);
    webserver.onGetChannels(onGetChannels);

    webserver.init();

    if (MDNS.begin(hostName)) {
      Serial.println("MDNS responder started");

      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println("Error setting up MDNS responder!");
    }

    Serial.println("Web-Server started");

    attachTasks();
    sched.run();

    hardware.setBtnCallback([](boolean state) {
      Serial.println(String("Button ") + String(state ? "pressed!" : "released!"));
      Serial.println(hardware.isButtonPressed() ? "_pressed!" : "_released!");
    });

    hardware.setYellowLed(true);
}

void loop() {
  sched.run();
}
