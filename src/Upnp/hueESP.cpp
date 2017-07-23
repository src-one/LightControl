#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include "hueESP.h"

struct rgbcolor {
  rgbcolor(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {};
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

int wildcmp(const char *wild, const char *string) {
  const char *cp = NULL, *mp = NULL;

  while ((*string) && (*wild != '*')) {
    if ((*wild != *string) && (*wild != '?')) {
      return 0;
    }
    wild++;
    string++;
  }

  while (*string) {
    if (*wild == '*') {
      if (!*++wild) {
        return 1;
      }
      mp = wild;
      cp = string + 1;
    } else if ((*wild == *string) || (*wild == '?')) {
      wild++;
      string++;
    } else {
      wild = mp;
      string = cp++;
    }
  }

  while (*wild == '*') {
    wild++;
  }

  return !*wild;
}

String extractValue(const char *data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = strlen(data) - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data[i] == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? String(data).substring(strIndex[0], strIndex[1]) : "";
}

void printRequestDetails(AsyncWebServerRequest *request) {
  return;
/*
  if (request->method() == HTTP_GET)
    Serial.printf("GET");
  else if (request->method() == HTTP_POST)
    Serial.printf("POST");
  else if (request->method() == HTTP_DELETE)
    Serial.printf("DELETE");
  else if (request->method() == HTTP_PUT)
    Serial.printf("PUT");
  else if (request->method() == HTTP_PATCH)
    Serial.printf("PATCH");
  else if (request->method() == HTTP_HEAD)
    Serial.printf("HEAD");
  else if (request->method() == HTTP_OPTIONS)
    Serial.printf("OPTIONS");
  else
    Serial.printf("UNKNOWN");
*/
  Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
/*
  if (request->contentLength()) {
    Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
  }
*/
/*
  int headers = request->headers();
  int i;

  for (i = 0; i < headers; i++) {
    AsyncWebHeader *h = request->getHeader(i);
    Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }
*/
/*
  int params = request->params();

  for (i = 0; i < params; i++) {
    AsyncWebParameter *p = request->getParam(i);
    if (p->isFile()) {
      Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    } else if (p->isPost()) {
      Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }
*/
  if (request->hasParam("body", true)) {
    AsyncWebParameter *body = request->getParam("body", true);

    String message = body->value().c_str();
    Serial.print("MESSAGE: ");
    Serial.println(message);
  }
}

bool matchLightState(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/lights/*/state", request->url().c_str());
}

bool matchNewLights(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/lights/new", request->url().c_str());
}

bool matchLight(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/lights/*", request->url().c_str());
}

bool matchLights(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/lights", request->url().c_str());
}

bool matchScenes(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/scenes", request->url().c_str());
}

bool matchGroups(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/groups", request->url().c_str());
}

bool matchSensors(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/sensors", request->url().c_str());
}

bool matchRules(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/rules", request->url().c_str());
}

bool matchSchedules(AsyncWebServerRequest *request) {
  return wildcmp("/api/*/schedules", request->url().c_str());
}

bool matchConfig(AsyncWebServerRequest *request) {
  return wildcmp("/api/config", request->url().c_str()) ||
         wildcmp("/api/*/config", request->url().c_str());
}

bool matchApi(AsyncWebServerRequest *request) {
  return wildcmp("/api", request->url().c_str()) ||
         wildcmp("/api/*", request->url().c_str());
}

void hueESP::_sendUDPResponse(unsigned int device_id) {
  hueesp_device_t device = _devices[device_id];
  DEBUG_MSG_HUE("[HUE] UDP response for device #%d (%s)\n", _current, "LightControl Bridge");

  char buffer[16];
  IPAddress ip = WiFi.localIP();
  sprintf(buffer, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  char response[strlen(HUE_UDP_TEMPLATE) + 40];
  /*
      sprintf_P(response, UDP_TEMPLATE,
          buffer,
          _base_port + _current,
          device.uuid,
          device.uuid
      );
  */
  sprintf_P(response, HUE_UDP_TEMPLATE, buffer, _base_port + _current);

  _udp.beginPacket(_remoteIP, _remotePort);
  _udp.write(response);
  _udp.endPacket();
}

void hueESP::_nextUDPResponse() {
  while (_roundsLeft) {
    if (_hit == false)
      break;


    if (++_current == _devices.size()) {
      --_roundsLeft;
      _current = 0;
    }
  }

  if (_roundsLeft > 0) {
    _sendUDPResponse(_current);
    if (++_current == _devices.size()) {
      --_roundsLeft;
      _current = 0;
    }
  }
}

void hueESP::_handleUDPPacket(IPAddress remoteIP, unsigned int remotePort, uint8_t *data, size_t len) {
  if (!_enabled)
    return;

  data[len] = 0;

  if (strstr((char *)data, HUE_UDP_SEARCH_PATTERN) == (char *)data) {
    // DEBUG_MSG_HUE("[HUE] Search request from %s\n", (char *) data);

    if (strstr((char *)data, HUE_UDP_DEVICE_PATTERN) != NULL) {
      #ifdef DEBUG_HUE
        char buffer[16];
        sprintf(buffer, "%d.%d.%d.%d", remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3]);
        DEBUG_MSG_HUE("[HUE] Search request from %s\n", buffer);
      #endif

      // Set hits to false
      for (unsigned int i = 0; i < _devices.size(); i++) {
        _hit = false;
      }

      // Send responses
      _remoteIP = remoteIP;
      _remotePort = remotePort;
      _current = random(0, _devices.size());
      _roundsLeft = HUE_UDP_RESPONSES_TRIES;
    }
  }
}

void hueESP::_handleSetup(AsyncWebServerRequest *request) {
  if (!_enabled)
    return;

  DEBUG_MSG_HUE("[HUE] Device /description.xml\n");
  char buffer[16];

  IPAddress ip = WiFi.localIP();
  sprintf(buffer, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  char _response[strlen(HUE_SETUP_TEMPLATE) + 50];
  sprintf_P(_response, HUE_SETUP_TEMPLATE, buffer, _base_port, "LightControl Bridge", buffer, _uuid, _uuid);

  DEBUG_MSG_HUE("[HUE] Setup Response #%d\n", _response);

  AsyncWebServerResponse *response = request->beginResponse(200, "application/xml", _response);
  response->addHeader( "Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
  request->send(response);
}

void hueESP::_setDevice(int channel, JsonObject &payload) {
  if (!payload.success()) {
    Serial.println("JSON parsing failed!");
    return;
  }

  hueesp_device_t & device = _devices[channel - 1];

  if (payload.containsKey("on")) {
    device.state= payload["on"].as<boolean>();
  }

  JsonVariant _sat = payload["sat"];
  if(_sat) {
    device.sat = _sat.as<int>();
    device.color_mode = COLOR_MODE_HUE;
    device.color = convert.hue(device.bri, device.hue, device.sat);
  }

  JsonVariant _hue = payload["hue"];
  if(_hue) {
    device.hue = _hue.as<int>();
    device.color_mode = COLOR_MODE_HUE;
    device.color = convert.hue(device.bri, device.hue, device.sat);
  }

  JsonVariant _bri = payload["bri"];
  if(_bri) {
    device.bri = _bri.as<int>();

    switch(device.color_mode){
      case COLOR_MODE_HUE:
        device.color = convert.hue(device.bri, device.hue, device.sat);
        break;

      case COLOR_MODE_XY:
        device.color = convert.xy(device.bri, device.x, device.y);
        break;

      case COLOR_MODE_CT:
        device.color = convert.ct(device.bri, device.ct);
        break;
    }
  }

  //JsonVariant _hue = payload["hue"];
  //if(_xy){
  if (payload.containsKey("xy")) {
    device.x = payload["xy"][0].as<float>();
    device.y = payload["xy"][1].as<float>();
    device.color_mode = COLOR_MODE_XY;
    device.color = convert.xy(device.bri, device.x, device.y);
  }

  JsonVariant _ct = payload["ct"];
  if(_ct && _ct != 0){
    device.ct = _ct.as<int>();
    device.color_mode = COLOR_MODE_CT;
    device.color = convert.ct(device.bri, device.ct);
  }
/*
  DEBUG_MSG_HUE("[DEV] CHANNEL:\t %d\n", channel);
  DEBUG_MSG_HUE("[DEV] UUID:\t %d\n", device.uuid);
  DEBUG_MSG_HUE("[DEV] COLOR:\t %d %d %d %d\n", device.color.r, device.color.g, device.color.b, device.color.w);
  DEBUG_MSG_HUE("[DEV] SAT:\t %d\n", device.sat);
  DEBUG_MSG_HUE("[DEV] HUE:\t %d\n", device.hue);
  DEBUG_MSG_HUE("[DEV] BRI:\t %d\n", device.bri);
  DEBUG_MSG_HUE("[DEV] X:\t %d.%06d\n", (int)device.x, (int)(device.x * 1000000) % 1000000);
  DEBUG_MSG_HUE("[DEV] Y:\t %d.%06d\n", (int)device.y, (int)(device.y * 1000000) % 1000000);
  DEBUG_MSG_HUE("[DEV] CT:\t %d\n", device.ct);
  DEBUG_MSG_HUE("[DEV] STATE:\t ");
  DEBUG_MSG_HUE(device.state ? "true" : "false");
  DEBUG_MSG_HUE("\n");
*/
  _devices[channel - 1] = device;

  if (_changeDeviceCallback) {
    _changeDeviceCallback(channel, device.state, device.bri, device.color);
  }
}

void hueESP::_attachApi(AsyncWebServer * server) {
  // TCP Server
  //AsyncWebServer * server = new AsyncWebServer(port);

  server->on("/description.xml", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving device description XML\n");
    _handleSetup(request);
  });
  /*
      new_device.server->on("/upnp/control/basicevent1", HTTP_POST,
          [](AsyncWebServerRequest *request) {
              request->send(200);
          },
          NULL,
          [this, device_id](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              data[len] = 0;
              _handleContent(request, device_id, (char *) data);
          }
      );
  */

  // /api/*/lights/*/state
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /lights/*/state endpoint\n");

    const char* command = request->url().c_str();

    Serial.println(String("CHANNEL:\t") + extractValue(command, '/', 4));
    int channel = atoi(extractValue(command, '/', 4).c_str());

    hueesp_device_t device = _devices[channel - 1];

    char _response[strlen(HUE_LIGHT_RESPONSE) + 100];

    String _colormode = "";
    switch(device.color_mode) {
      case COLOR_MODE_CT:
        _colormode = "ct";
        break;
      case COLOR_MODE_HUE:
        _colormode = "hs";
        break;
      case COLOR_MODE_XY:
        _colormode = "xy";
        break;
    }

    sprintf_P(_response, HUE_LIGHT_RESPONSE,
      (device.state ? "true" : "false"),
      device.bri,
      device.hue,
      device.sat,
      String(device.x, 6).c_str(),
      String(device.y, 6).c_str(),
      device.ct,
      _colormode.c_str(),
      device.name,
      device.uuid);

    //sprintf_P(_response, HUE_LIGHT_RESPONSE, (device.state ? "true" : "false"), device.uuid);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", _response);
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);
  }).setFilter(matchLightState);

  // /api/*/lights/new
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /lights/new endpoint\n");

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{}");
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);
  }).setFilter(matchNewLights);

  // /api/*/lights/*
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /lights/* endpoint\n");

    const char* command = request->url().c_str();

    Serial.println(String("CHANNEL:\t") + extractValue(command, '/', 4));
    int channel = atoi(extractValue(command, '/', 4).c_str());

    hueesp_device_t device = _devices[channel - 1];

    /*
      {
        "state": {
          "on": false,
          "bri": 0,
          "hue": 0,
          "sat": 0,
          "effect": "none",
          "xy": [
            0,
            0
          ],
          "ct": 0,
          "alert": "select",
          "colormode": "hs",
          "reachable": false
        },
        "type": "Extended color light",
        "name": "Hue color lamp 1",
        "modelid": "LCT010",
        "manufacturername": "Philips",
        "uniqueid": "00:17:88:01:02:3c:04:fa-0b",
        "swversion": "6601820"
      }
    */
/*
    AsyncJsonResponse * response = new AsyncJsonResponse();
    JsonObject& root = response->getRoot();
*/
/*
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    JsonObject& state = root.createNestedObject("state");

    state["on"] = device.state;
    state["bri"] = device.bri;
    state["hue"] = device.hue;
    state["sat"] = device.sat;
    state["effect"] = "none";

    JsonArray& xy = root.createNestedArray("xy");
    xy.add(device.x);
    xy.add(device.y);

    state["ct"] = device.ct;
    state["alert"] = "select";

    switch(device.color_mode) {
      case COLOR_MODE_CT:
        state["colormode"] = "ct";
        break;
      case COLOR_MODE_HUE:
        state["colormode"] = "hue";
        break;
      case COLOR_MODE_XY:
        state["colormode"] = "xy";
        break;
    }
    state["reachable"] = true;

    root["type"] = "Extended color light";
    root["name"] = device.name;
    root["modelid"] = "LCT010";
    root["manufacturername"] = "Philips";
    root["uniqueid"] = device.uuid;
    root["swversion"] = "6601820";


    //response->addHeader("Content-Type", "application/json");
    //response->setLength();
    root.printTo(*response);
*/
    //request->send(response);
    //request->send(200, "application/json", response);
/*
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{}");
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);
*/
    char _response[strlen(HUE_LIGHT_RESPONSE) + 100];

    String _colormode = "";
    switch(device.color_mode) {
      case COLOR_MODE_CT:
        _colormode = "ct";
        break;
      case COLOR_MODE_HUE:
        _colormode = "hs";
        break;
      case COLOR_MODE_XY:
        _colormode = "xy";
        break;
    }

    sprintf_P(_response, HUE_LIGHT_RESPONSE,
      (device.state ? "true" : "false"),
      device.bri,
      device.hue,
      device.sat,
      String(device.x, 6).c_str(),
      String(device.y, 6).c_str(),
      device.ct,
      _colormode.c_str(),
      device.name,
      device.uuid);

    //char _response[strlen(HUE_LIGHT_RESPONSE) + 50];
    //sprintf_P(_response, HUE_LIGHT_RESPONSE, (device.state ? "true" : "false"), device.uuid);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", _response);
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);

  }).setFilter(matchLight);

  // /api/*/lights
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /lights endpoint\n");

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    int i = 1;

    for (auto & device : _devices) {
      JsonObject& light = root.createNestedObject(String(i));

      JsonObject& state = light.createNestedObject("state");

      state["on"] = device.state;
      state["bri"] = device.bri;
      state["hue"] = device.hue;
      state["sat"] = device.sat;
      state["effect"] = "none";

      JsonArray& xy = light.createNestedArray("xy");
      xy.add(device.x);
      xy.add(device.y);

      state["ct"] = device.ct;
      state["alert"] = "select";

      switch(device.color_mode) {
        case COLOR_MODE_CT:
          state["colormode"] = "ct";
          break;
        case COLOR_MODE_HUE:
          state["colormode"] = "hs";
          break;
        case COLOR_MODE_XY:
          state["colormode"] = "xy";
          break;
      }
      state["reachable"] = true;

      light["type"] = "Extended color light";
      light["name"] = device.name;
      light["modelid"] = "LCT010";
      light["manufacturername"] = "Philips";
      light["uniqueid"] = device.uuid;
      light["swversion"] = "6601820";

      i++;
    }

    root.printTo(*response);
    request->send(response);
  }).setFilter(matchLights);

    // /api/config /api/*/config
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /config endpoint\n");

    //AsyncJsonResponse * response = new AsyncJsonResponse();
    //response->addHeader("Server","ESP Async Web Server");
    //JsonObject& root = response->getRoot();

    request->send(SPIFFS, "/hueBridgeConfig.min.json", "application/json");
  }).setFilter(matchConfig);

  // /api/*/scenes
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /scenes endpoint\n");

    request->send(SPIFFS, "/hueBridgeScenes.min.json", "application/json");
  }).setFilter(matchScenes);

  // /api/*/groups
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /groups endpoint\n");

    request->send(SPIFFS, "/hueBridgeGroups.min.json", "application/json");
  }).setFilter(matchGroups);

  // /api/*/sensors
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /sensors endpoint\n");

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{}");
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);
  }).setFilter(matchSensors);

  // /api/*/rules
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /rules endpoint\n");

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{}");
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    }).setFilter(matchRules);

  // /api/*/schedules
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /schedules endpoint\n");

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{}");
      response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    }).setFilter(matchSchedules);

  // /api/*
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //DEBUG_MSG_HUE("[HUE] serving /api endpoint\n");

    /*
        root["name"] = "Philips hue";
        root["datastoreversion"] = "61";
        root["swversion"] = "1705121051";
        root["apiversion"] = "1.19.0";
        root["mac"] = String(WiFi.macAddress());
        root["bridgeid"] = "001788FFFE4EBCEB";
        root["factorynew"] = false;
        root["replacesbridgeid"] = "";
        root["modelid"] = "BSB002";
        root["starterkitid"] = "";
        response->setLength();
        request->send(response);
    */

    request->send(SPIFFS, "/hueBridgeFullConfig.min.json", "application/json");
  }).setFilter(matchApi);

  // /api/*/lights/*/state
  server->on("/api", HTTP_PUT, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] /api PUT request:\n");

    printRequestDetails(request);

    if (request->hasParam("body", true)) {
      //Serial.println("\n#### COMMAND:");
      AsyncWebParameter *body = request->getParam("body", true);

      String message = body->value();
      //Serial.println("MESSAGE:");
      //Serial.println(message);

      StaticJsonBuffer<1500> jsonBuffer;
      JsonObject &root = jsonBuffer.parseObject(message);

      if (!root.success()) {
        Serial.println("JSON parsing failed!");
        return false;
      }

      const char* command = request->url().c_str();
      int channel = atoi(extractValue(command, '/', 4).c_str());

      //Serial.println(String("CHANNEL:\t") + channel);

      _setDevice(channel, root);

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\": true}]");
      response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    }
  },
  NULL,
  [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len == total && total != 0) {
      //DEBUG_MSG_HUE("[HUE] Body Data: %s\n", data);

      StaticJsonBuffer<1500> jsonBuffer;
      JsonObject &root = jsonBuffer.parseObject(data);

      const char* command = request->url().c_str();
      int channel = atoi(extractValue(command, '/', 4).c_str());

      _setDevice(channel, root);

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\": true}]");
      response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    }
  }).setFilter(matchLightState);

  // api/*/groups
  server->on("/api", HTTP_POST, [](AsyncWebServerRequest *request) {
      DEBUG_MSG_HUE("[HUE] /api/*/groups POST request:\n");

      printRequestDetails(request);

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\":{\"id\":\"1\"}}]");
      response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    },
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (index + len == total) {
       DEBUG_MSG_HUE("[HUE] JSON request: %s\n", data);

       AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\":{\"id\":\"1\"}}]");
       response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
       request->send(response);
      }
    }).setFilter(matchGroups);

  // api/*/scenes
  server->on("/api", HTTP_POST, [](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] /api/*/scenes POST request:\n");

    printRequestDetails(request);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\":{\"id\":\"1\"}}]");
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);
  },
  NULL,
  [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len == total) {
      DEBUG_MSG_HUE("[HUE] request: %s\n", data);

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\":{\"id\":\"1\"}}]");
      response->addHeader("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    }
  }).setFilter(matchScenes);

  // /api
  server->on("/api", HTTP_POST, [](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] /api POST request:\n");

    printRequestDetails(request);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\":{\"username\": \"all\"}}]");
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);
  },
  NULL,
  [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len == total) {
      DEBUG_MSG_HUE("[HUE] request: %s\n", data);
      DEBUG_MSG_HUE("[HUE] sending: %s\n", "[{\"success\":{\"username\": \"api\"}}]");

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\":{\"username\": \"all\"}}]");
      response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    }
  });
}

void hueESP::setBridgeName(const char *bridgeName) {
  _name = strdup(bridgeName);

  DEBUG_MSG_HUE("[HUE] set Bridge Name to '%s'\n", _name);
}

void hueESP::addDevice(const char *deviceName) {
  hueesp_device_t newDevice;
  unsigned int deviceId = _devices.size();

  // Copy name
  newDevice.name = strdup(deviceName);

  // Create UUID
  char uuid[15];
  sprintf(uuid, "444556%06X%02X\0", ESP.getChipId(), deviceId); // "DEV" + CHIPID + DEV_ID
  newDevice.uuid = strdup(uuid);

  // Attach
  _devices.push_back(newDevice);

  DEBUG_MSG_HUE("[HUE] Device '%s' added (%s)\n", newDevice.name, newDevice.uuid);
}

void hueESP::handle() {
  int len = _udp.parsePacket();
  if (len > 0) {
    IPAddress remoteIP = _udp.remoteIP();
    unsigned int remotePort = _udp.remotePort();
    uint8_t data[len];
    _udp.read(data, len);
    _handleUDPPacket(remoteIP, remotePort, data, len);
  }

  if (_roundsLeft > 0) {
    if (millis() - _lastTick > HUE_UDP_RESPONSES_INTERVAL) {
      _lastTick = millis();
      _nextUDPResponse();
    }
  }
}

//hueESP::hueESP(unsigned int port) {
hueESP::hueESP(AsyncWebServer * server) {
  //_base_port = port;

  // Create Bridge UUID
  char uuid[15];
  sprintf(uuid, "444556%06X\0", ESP.getChipId()); // "DEV" + CHIPID
  _uuid = strdup(uuid);

  //_attachApi(_base_port);
  _attachApi(server);


  // UDP Server
  _udp.beginMulticast(WiFi.localIP(), HUE_UDP_MULTICAST_IP, HUE_UDP_MULTICAST_PORT);
}
