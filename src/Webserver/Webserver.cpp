#include "Webserver.h"

Webserver::Webserver()
{
  Serial.println("webserver loaded");
}

void Webserver::init() {
  _attachFileServer();

  _attachApiSetChannelsEndpoint();
  _attachApiGetChannelsEndpoint();

  _attachApiGetHeapEndpoint();

  server->begin();
  server->addHandler(&ws);
  //ws.onEvent(_onWsEvent);
  Serial.println("HTTP server started");
}

// ####################### Channels ########################

void Webserver::onGetChannels(String (*func)(char payload[])) {
  _getChannelsCallback = func;
}

void Webserver::_apiGetChannels(char payload[]) {
  _getChannelsCallback(payload);
}


void Webserver::onSetChannels(void (*func)(char payload[])) {
  _setChannelsCallback = func;
}

void Webserver::_apiSetChannels(char payload[]) {
  _setChannelsCallback(payload);
}


void Webserver::onSetWebsocketText(void (*func)(String payload)) {
  _setWebsocketTextCallback = func;
}

void Webserver::_apiSetWebsocketText(String payload) {
  _setWebsocketTextCallback(payload);
}


// ####################### Fileserver ########################

void Webserver::_attachFileServer() {
  server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server->onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });
}

// ####################### Endpoints ########################

void Webserver::_attachApiSetChannelsEndpoint() {
  server->on("/_api/channel", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/json", "{}");
  },

  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  },

  [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if(!index) {
      memset(channelResultBuffer, 0, sizeof channelResultBuffer);
      channelResultBufferPos = 0;
    }

    for(size_t i = 0; i < len; i++) {
      channelResultBuffer[channelResultBufferPos] = data[i];
      channelResultBufferPos++;
    }

    if(index + len == total && _setChannelsCallback != NULL) {
      _setChannelsCallback(channelResultBuffer);
    }
  });
}

/*
void Webserver::_attachApiSetBarcodeEndpoint()
{
  server.on("/api/barcode", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/json", "{\"success\": true}");
  },

  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  },

  [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if(!index) {
      memset(channelResultBuffer, 0, sizeof channelResultBuffer);
      channelResultBufferPos = 0;
    }

    for(size_t i = 0; i < len; i++) {
      channelResultBuffer[channelResultBufferPos] = data[i];
      channelResultBufferPos++;
    }

    if(index + len == total && _setChannelsCallback != NULL) {
      Serial.println(channelResultBuffer);
    }
  });
}
*/

void Webserver::_attachApiGetChannelsEndpoint() {
  server->on("/_api/channels", HTTP_GET, [&](AsyncWebServerRequest *request) {
    String result = "";

    if(_getChannelsCallback != NULL) {
      result = _getChannelsCallback("");
    }

    request->send(200, "text/json", "{\"data\":\"" + result + "\"}");
  });
}

void Webserver::_attachApiGetHeapEndpoint()
{
  server->on("/_api/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
}

void Webserver::_attachWebsocketListener() {
  /*
  onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
    if(type == WS_EVT_CONNECT){
      Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
      //client->printf("Hello Client %u :)", client->id());
      //client->textAll(status);
      client->ping();
    } else if(type == WS_EVT_DISCONNECT){
      Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    } else if(type == WS_EVT_ERROR){
      Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    } else if(type == WS_EVT_PONG){
      Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data: "");
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
        //Serial.printf("%s\n", payload.c_str());

        if(info->opcode == WS_TEXT) {
          client->text("received message: " + String(payload));
          //websocketTextOperation(payload);

          if(_setWebsocketTextCallback != NULL) {
            _setWebsocketTextCallback(payload);
          }
        }
      }
    }
  }

  ws.onEvent(onWsEvent);
  */
}
