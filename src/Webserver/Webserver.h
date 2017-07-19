#ifndef __Webserver_h
#define __Webserver_h

#include <functional>

#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

class Webserver;

class Webserver
{
  public:
    Webserver();

    void init();

    void onGetChannels(String (*func)(char payload[]));
    void onSetChannels(void (*func)(char payload[]));
    void onSetWebsocketText(void (*func)(String payload));

    AsyncWebServer server = AsyncWebServer(8080);
    AsyncWebSocket ws = AsyncWebSocket("/ws");
    AsyncEventSource events = AsyncEventSource("/events");

  private:
    void _apiGetChannels(char payload[]);
    String (*_getChannelsCallback)(char payload[]) = NULL;

    void _apiSetChannels(char payload[]);
    void (*_setChannelsCallback)(char payload[]) = NULL;

    void _apiSetWebsocketText(String payload);
    void (*_setWebsocketTextCallback)(String payload) = NULL;

    void _attachFileServer();
    void _attachApiSetChannelsEndpoint();
    void _attachApiGetChannelsEndpoint();
    void _attachApiGetHeapEndpoint();
    void _attachWebsocketListener();

    //void _onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

    char channelResultBuffer[1024];
    long channelResultBufferPos = 0;
};

#endif
