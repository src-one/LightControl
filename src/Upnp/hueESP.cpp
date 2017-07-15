#include <vector>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "hueESP.h"

void hueESP::_sendUDPResponse(unsigned int device_id) {
    hueesp_device_t device = _devices[device_id];
    DEBUG_MSG_HUE("[HUE] UDP response for device #%d (%s)\n", _current, device.name);

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
    sprintf_P(response, HUE_UDP_TEMPLATE,
        buffer,
        _base_port + _current
    );

    _udp.beginPacket(_remoteIP, _remotePort);
    _udp.write(response);
    _udp.endPacket();

}

void hueESP::_nextUDPResponse() {
    while (_roundsLeft) {
        if (_devices[_current].hit == false) break;
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
    if (!_enabled) return;

    data[len] = 0;

    if (strstr((char *) data, HUE_UDP_SEARCH_PATTERN) == (char *) data) {
        //DEBUG_MSG_HUE("[HUE] Search request from %s\n", (char *) data);

        if (strstr((char *) data, HUE_UDP_DEVICE_PATTERN) != NULL) {
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

void hueESP::_handleSetup(AsyncWebServerRequest *request, unsigned int device_id) {
    if (!_enabled) return;

    DEBUG_MSG_HUE("[HUE] Device #%d /description.xml\n", device_id);
    _devices[device_id].hit = true;

    hueesp_device_t device = _devices[device_id];

    char buffer[16];
    IPAddress ip = WiFi.localIP();
    sprintf(buffer, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    char _response[strlen(HUE_2015_SETUP_TEMPLATE) + 50];
    sprintf_P(_response, HUE_2015_SETUP_TEMPLATE,
            buffer,
            _base_port + _current,
            device.name,
            device.uuid);

    DEBUG_MSG_HUE("[HUE] Setup Response #%d\n", _response);

    AsyncWebServerResponse *response = request->beginResponse(200, "application/xml", _response);
    response->addHeader("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    request->send(response);
}

void hueESP::_handleContent(AsyncWebServerRequest *request, unsigned int device_id, char * content) {
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

String hueESP::_getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for(int i=0; i <= maxIndex && found <= index; i++){
    if(data.charAt(i) == separator || i == maxIndex){
        found++;
        strIndex[0] = strIndex[1] + 1;
        strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void hueESP::addDevice(const char * device_name) {
    hueesp_device_t new_device;
    unsigned int device_id = _devices.size();

    // Copy name
    new_device.name = strdup(device_name);

    // Create UUID
    char uuid[15];
    sprintf(uuid, "444556%06X%02X\0", ESP.getChipId(), device_id); // "DEV" + CHIPID + DEV_ID
    new_device.uuid = strdup(uuid);

    // TCP Server
    new_device.server = new AsyncWebServer(_base_port + device_id);
    new_device.server->on("/setup.xml", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving device description XML\n");
        _handleSetup(request, device_id);
    });
    new_device.server->on("/description.xml", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving device description XML\n");
        _handleSetup(request, device_id);
    });
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

    new_device.server->on("index.html", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving index.html endpoint\n");
        request->send(SPIFFS, "/index.html");
    });

    new_device.server->on("/api/h3oQSBivbQZk9XRG4IlIvx1Rb9kdy54qBqpFIwUw/config", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving /scenes endpoint\n");

        request->send(SPIFFS, "/hueBridgeScenes.min.json", "application/json");
    });

    new_device.server->on("/api/h3oQSBivbQZk9XRG4IlIvx1Rb9kdy54qBqpFIwUw/scenes/3T2SvsxvwteNNys", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving /scenes/3T2SvsxvwteNNys endpoint\n");

        request->send(SPIFFS, "/hueBridgeScene1.min.json", "application/json");
    });

    new_device.server->on("/api/h3oQSBivbQZk9XRG4IlIvx1Rb9kdy54qBqpFIwUw/scenes", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving /scenes endpoint\n");

        request->send(SPIFFS, "/hueBridgeScenes.min.json", "application/json");
    });

    new_device.server->on("/api/h3oQSBivbQZk9XRG4IlIvx1Rb9kdy54qBqpFIwUw/lights/2/state", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving /lights/2/state endpoint\n");

        hueesp_device_t device = _devices[device_id];

        if(device.state == true) {
          DEBUG_MSG_HUE("[HUE] serving /lights/2 ON State\n");
          request->send(SPIFFS, "/hueLightsResponseOn.json", "application/json");
        } else {
          DEBUG_MSG_HUE("[HUE] serving /lights/2 OFF State\n");
          request->send(SPIFFS, "/hueLightsResponseOff.json", "application/json");
        }
    });

    new_device.server->on("/api/h3oQSBivbQZk9XRG4IlIvx1Rb9kdy54qBqpFIwUw/lights/2", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving /lights/2 endpoint\n");

        hueesp_device_t device = _devices[device_id];

        if(device.state == true) {
          DEBUG_MSG_HUE("[HUE] serving /lights/2 ON State\n");
          request->send(SPIFFS, "/hueLightsResponseOn.json", "application/json");
        } else {
          DEBUG_MSG_HUE("[HUE] serving /lights/2 OFF State\n");
          request->send(SPIFFS, "/hueLightsResponseOff.json", "application/json");
        }
    });

    new_device.server->on("/api/h3oQSBivbQZk9XRG4IlIvx1Rb9kdy54qBqpFIwUw/lights", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving /lights endpoint\n");

        request->send(SPIFFS, "/hueLightsConfig.min.json", "application/json");
    });

    new_device.server->on("/api/h3oQSBivbQZk9XRG4IlIvx1Rb9kdy54qBqpFIwUw", HTTP_GET, [this, device_id](AsyncWebServerRequest *request){
        DEBUG_MSG_HUE("[HUE] serving /api endpoint\n");

        request->send(SPIFFS, "/hueBridgeFullConfig.min.json", "application/json");
    });

    //new_device.server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    new_device.server->onNotFound([this, device_id](AsyncWebServerRequest *request){
      Serial.printf("NOT_FOUND: ");

      if(request->method() == HTTP_GET)
        Serial.printf("GET");
      else if(request->method() == HTTP_POST)
        Serial.printf("POST");
      else if(request->method() == HTTP_DELETE)
        Serial.printf("DELETE");
      else if(request->method() == HTTP_PUT)
        Serial.printf("PUT");
      else if(request->method() == HTTP_PATCH)
        Serial.printf("PATCH");
      else if(request->method() == HTTP_HEAD)
        Serial.printf("HEAD");
      else if(request->method() == HTTP_OPTIONS)
        Serial.printf("OPTIONS");
      else
        Serial.printf("UNKNOWN");
      Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

      if(request->contentLength()){
        Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
        Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
      }

      int headers = request->headers();
      int i;
      for(i=0;i<headers;i++){
        AsyncWebHeader* h = request->getHeader(i);
        Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
      }

      int params = request->params();
      for(i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){
          Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }

      //request->send(404);

      if(request->method() == HTTP_PUT && request->hasParam("body", true)) {
        hueesp_device_t device = _devices[device_id];

        Serial.println("\n#### COMMAND:");

        String command = String(request->url().c_str());
        AsyncWebParameter* body = request->getParam("body", true);
        /*
        char sz[100];
        strcpy(sz, request->url().c_str());
        char *_command = sz;

        char *str;
        while ((str = strtok_r(_command, "/", &_command)) != NULL) {
           Serial.println(String("part: ") + str);
        }
        */
        Serial.println(_getValue(command, '/', 2));
        Serial.println(_getValue(command, '/', 3));
        Serial.println(_getValue(command, '/', 4));
        Serial.println(_getValue(command, '/', 5));
        Serial.println(_getValue(command, '/', 6));

        if(_getValue(command, '/', 1) == "api")
          Serial.println(String("API: ") + _getValue(command, '/', 2));

        if(_getValue(command, '/', 3) == "lights")
          Serial.println(String("ID: ") + _getValue(command, '/', 4));

        //Serial.println(_p->value().c_str());

        const char* message = body->value().c_str();
        Serial.println("MESSAGE:");
        Serial.println(message);

        if (strcmp(message, "{\"on\": true}") == 0) {
          Serial.println("SWITCH ON");
          device.state = true;
        }

        if (strcmp(message, "{\"on\": false}") == 0) {
          Serial.println("SWITCH OFF");
          device.state = false;
        }

        if (strcmp(message, "{\"on\": true,\"bri\":127}") == 0) {
          Serial.println("DIM 50%");
          device.brightness = 127;
        }

        DEBUG_MSG_HUE("\n[HUE] serving request handling endpoint\n");
        ///AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{"success": {"on": false}}, {"success": {"bri": 0}}]");
        //AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\": {\"on\": true}}]");
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "[{\"success\": " + String(message) + "}]");
        response->addHeader("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        request->send(response);

        Serial.println("\n#### END COMMAND ####\n\n");
      } else {
        DEBUG_MSG_HUE("[HUE] serving fallback endpoint\n");

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{}");
        response->addHeader("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        request->send(response);
        //request->send(404);
      }
    });

    new_device.server->begin();

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

void hueESP::enable(bool enable) {
    DEBUG_MSG_HUE("[HUE] %s\n", enable ? "Enabled" : "Disabled");
    _enabled = enable;
}

hueESP::hueESP(unsigned int port) {
    _base_port = port;

    // UDP Server
    _udp.beginMulticast(WiFi.localIP(), HUE_UDP_MULTICAST_IP, HUE_UDP_MULTICAST_PORT);

}
