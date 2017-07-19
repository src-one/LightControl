#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <vector>
#include "hueESP.h"

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

void printRequestDetails(AsyncWebServerRequest *request) {
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
  Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

  if (request->contentLength()) {
    Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
  }

  int headers = request->headers();
  int i;

  for (i = 0; i < headers; i++) {
    AsyncWebHeader *h = request->getHeader(i);
    Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }

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
    if (_devices[_current].hit == false)
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
        _devices[i].hit = false;
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

  char _response[strlen(HUE_2015_SETUP_TEMPLATE) + 50];
  sprintf_P(_response, HUE_2015_SETUP_TEMPLATE, buffer, _base_port, "LightControl Bridge", _uuid);

  DEBUG_MSG_HUE("[HUE] Setup Response #%d\n", _response);

  AsyncWebServerResponse *response = request->beginResponse(200, "application/xml", _response);
  response->addHeader( "Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
  request->send(response);
}
/*
void hueESP::_handleContent(AsyncWebServerRequest *request, unsigned int
device_id, char * content) {
    if (!_enabled) return;

    DEBUG_MSG_HUE("[HUE] Device #%d /upnp/control/basicevent1\n", device_id);
    hueesp_device_t device = _devices[device_id];

    if (strstr(content, "<BinaryState>0</BinaryState>") != NULL) {
        if (_callback) _callback(device_id, device.name, false);
    }

    if (strstr(content, "<BinaryState>1</BinaryState>") != NULL) {
        if (_callback) _callback(device_id, device.name, true);
    }
}
*/

String hueESP::_getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void hueESP::_attachApi(unsigned int port) {
  // TCP Server
  AsyncWebServer * server = new AsyncWebServer(port);

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
          [this, device_id](AsyncWebServerRequest *request, uint8_t *data,
     size_t len, size_t index, size_t total) {
              data[len] = 0;
              _handleContent(request, device_id, (char *) data);
          }
      );
  */
  server->on("index.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] index.html\n");
    request->send(SPIFFS, "/index.html");
  });

  server->on("/favicon.ico", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] favicon.ico\n");
    request->send(SPIFFS, "/images/favicon.ico");
  });

  // /api/*/lights/*/state
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /lights/*/state endpoint\n");

    hueesp_device_t device = _devices[0];

    if (device.state == true) {
      DEBUG_MSG_HUE("[HUE] serving /lights/* ON State\n");
      request->send(SPIFFS, "/hueLightsResponseOn.json", "application/json");
    } else {
      DEBUG_MSG_HUE("[HUE] serving /lights/* OFF State\n");
      request->send(SPIFFS, "/hueLightsResponseOff.json", "application/json");
    }
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

    hueesp_device_t device = _devices[0];
    String command = String(request->url().c_str());

    Serial.println(String("CHANNEL:\t") + _getValue(command, '/', 4));

    if (device.state == true) {
      DEBUG_MSG_HUE("[HUE] serving /lights/* ON State\n");
      request->send(SPIFFS, "/hueLightsResponseOn.json", "application/json");
    } else {
      DEBUG_MSG_HUE("[HUE] serving /lights/* OFF State\n");
      request->send(SPIFFS, "/hueLightsResponseOff.json", "application/json");
    }
  }).setFilter(matchLight);

  // /api/*/lights
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /lights endpoint\n");
    request->send(SPIFFS, "/hueLightsConfig.min.json", "application/json");
  }).setFilter(matchLights);

    // /api/config /api/*/config
  server->on("/api", HTTP_GET, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] serving /config endpoint\n");

    AsyncJsonResponse * response = new AsyncJsonResponse();
    //response->addHeader("Server","ESP Async Web Server");
    JsonObject& root = response->getRoot();
/*
    {
    	"name": "Philips hue",
    	"datastoreversion": "61",
    	"swversion": "1705121051",
    	"apiversion": "1.19.0",
    	"mac": "00:17:88:4e:bc:ea",
    	"bridgeid": "001788FFFE4EBCEA",
    	"factorynew": false,
    	"replacesbridgeid": null,
    	"modelid": "BSB002",
    	"starterkitid": ""
    }
*/
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
    DEBUG_MSG_HUE("[HUE] serving /api endpoint\n");

    request->send(SPIFFS, "/hueBridgeFullConfig.min.json", "application/json");
  }).setFilter(matchApi);

  // /api/*/lights/*/state
  server->on("/api", HTTP_PUT, [this](AsyncWebServerRequest *request) {
    DEBUG_MSG_HUE("[HUE] /api PUT request:\n");

    printRequestDetails(request);

    hueesp_device_t device = _devices[0];

    if (request->hasParam("body", true)) {
      Serial.println("\n#### COMMAND:");
      AsyncWebParameter *body = request->getParam("body", true);

      String message = body->value().c_str();
      Serial.println("MESSAGE:");
      Serial.println(message);

      StaticJsonBuffer<1500> jsonBuffer;
      JsonObject &root = jsonBuffer.parseObject(message);

      String command = String(request->url().c_str());
      Serial.println(String("CHANNEL:\t") + _getValue(command, '/', 4));

      if (root.containsKey("on")) {
        boolean state = root["on"].as<boolean>();
        Serial.println("STATE:\t\t" + String(state ? "true" : "false"));
        device.state = state;
      }

      if (root.containsKey("sat")) {
        int sat = root["sat"].as<int>();
        Serial.println("SAT:\t\t" + String(sat));
        // device.saturation = sat;
      }

      if (root.containsKey("hue")) {
        int hue = root["hue"].as<int>();
        Serial.println("HUE:\t\t" + String(hue));
        // device.hue = hue;
      }

      if (root.containsKey("bri")) {
        int brightness = root["bri"].as<int>();
        Serial.println("BRIGHTNESS:\t\t" + String(brightness));
        device.brightness = brightness;
      }

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\": " + String(message) + "}]");
      response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
    }
  },
  NULL,
  [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len == total) {
      DEBUG_MSG_HUE("[HUE] Body Data: %s\n", data);

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

  // new_device.server->serveStatic("/", SPIFFS,
  // "/").setDefaultFile("index.html");

  server->onNotFound([this](AsyncWebServerRequest *request) {
    hueesp_device_t device = _devices[0];

    printRequestDetails(request);

    String command = String(request->url().c_str());

    Serial.println(_getValue(command, '/', 2));
    Serial.println(_getValue(command, '/', 3));
    Serial.println(_getValue(command, '/', 4));
    Serial.println(_getValue(command, '/', 5));
    Serial.println(_getValue(command, '/', 6));

    String message = "";

    if (request->hasParam("body", true)) {
      Serial.println("\n#### COMMAND:");
      AsyncWebParameter *body = request->getParam("body", true);

      message = body->value().c_str();
      Serial.println("MESSAGE:");
      Serial.println(message);
    }

    String _command = "";

    if (request->method() == HTTP_PUT && request->hasParam("body", true)) {
      DEBUG_MSG_HUE("\n[HUE] serving request handling endpoint\n");

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\": " + String(message) + "}]");
      response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);

      Serial.println("\n#### END COMMAND ####\n\n");
    } else {
      DEBUG_MSG_HUE("[HUE] serving fallback endpoint\n");

      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{}");
      response->addHeader("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
      request->send(response);
      // request->send(404);
    }
  });

  server->begin();
}

void hueESP::addDevice(const char *device_name) {
  hueesp_device_t new_device;
  unsigned int device_id = _devices.size();

  // Copy name
  new_device.name = strdup(device_name);

  // Create UUID
  char uuid[15];
  sprintf(uuid, "444556%06X%02X\0", ESP.getChipId(), device_id); // "DEV" + CHIPID + DEV_ID
  new_device.uuid = strdup(uuid);

  // Attach
  _devices.push_back(new_device);

  DEBUG_MSG_HUE("[HUE] Device '%s' added (#%d)\n", device_name, device_id);
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

hueESP::hueESP(unsigned int port) {
  _base_port = port;

  // Create Bridge UUID
  char uuid[15];
  sprintf(uuid, "444556%06X\0", ESP.getChipId()); // "DEV" + CHIPID
  _uuid = strdup(uuid);

  _attachApi(_base_port);

  // UDP Server
  _udp.beginMulticast(WiFi.localIP(), HUE_UDP_MULTICAST_IP, HUE_UDP_MULTICAST_PORT);
}
