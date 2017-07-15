#ifndef HUEESP_h
#define HUEESP_h

#define HUE_DEFAULT_TCP_BASE_PORT   8178
#define HUE_UDP_MULTICAST_IP        IPAddress(239,255,255,250)
#define HUE_UDP_MULTICAST_PORT      1900
#define HUE_TCP_MAX_CLIENTS         10

#define HUE_UDP_SEARCH_PATTERN      "M-SEARCH"
#define HUE_UDP_DEVICE_PATTERN      "urn:schemas-upnp-org:device:basic:1"

#define HUE_UDP_RESPONSES_INTERVAL  250
#define HUE_UDP_RESPONSES_TRIES     5

#define DEBUG_HUE              true

const char HUE_UDP_TEMPLATE[] PROGMEM =
    "HTTP/1.1 200 OK\r\n"
    "CACHE-CONTROL: max-age=100\r\n"
    "EXT:\r\n"
    "LOCATION: http://%s:%d/description.xml\r\n"
    "SERVER: FreeRTOS/7.4.2, UPnP/1.0, IpBridge/1.7.0\r\n"
    "ST: urn:schemas-upnp-org:device:basic:1\r\n"
    "USN: uuid:b4eeb628-680e-11e7-9c2c-6c4008b0d85e\r\n\r\n";

const char _HUE_UDP_TEMPLATE[] PROGMEM =
    "HTTP/1.1 200 OK\r\n"
    "CACHE-CONTROL: max-age=86400\r\n"
    "DATE: Sun, 20 Nov 2016 00:00:00 GMT\r\n"
    "EXT:\r\n"
    "LOCATION: http://%s:%d/description.xml\r\n"
    "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
    "01-NLS: %s\r\n"
    "SERVER: FreeRTOS/7.4.2, UPnP/1.0, IpBridge/1.7.0\r\n"
    "ST: urn:schemas-upnp-org:device:basic:1\r\n"
    "USN: uuid:b4eeb628-680e-11e7-9c2c-6c4008b0d85e\r\n\r\n";

const char HUE_UDP_BROADCAST_TEMPLATE[] PROGMEM =
    "NOTIFY * HTTP/1.1"
    "HOST: 239.255.255.250:1900"
    "CACHE-CONTROL: max-age=100"
    "LOCATION: http://%s:%d/description.xml"
    "SERVER: FreeRTOS/7.4.2, UPnP/1.0, IpBridge/1.7.0"
    "NTS: ssdp:alive"
    "NT: uuid:2f402f80-da50-11e1-9b23-001788101fe2"
    "USN: uuid:2f402f80-da50-11e1-9b23-001788101fe2";


const char HUE_SETUP_TEMPLATE[] PROGMEM =
  "<?xml version=\"1.0\"?>"
  "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "<specVersion>"
      "<major>1</major>"
      "<minor>0</minor>"
    "</specVersion>"
    "<URLBase>http://%s:%d/</URLBase>"
    "<device>"
      "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
      "<friendlyName>Alexa Hue Bridge</friendlyName>"
      "<manufacturer>Royal Philips Electronics</manufacturer>"
      "<manufacturerURL>http://www.philips.com</manufacturerURL>"
      "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
      "<modelName>Philips hue bridge 2012</modelName>"
      "<modelNumber>929000226503</modelNumber>"
      "<modelURL>http://www.meethue.com</modelURL>"
      "<serialNumber>001788101fe2</serialNumber>"
      "<UDN>uuid:b4eeb628-680e-11e7-9c2c-6c4008b0d85e</UDN>"
      "<presentationURL>index.html</presentationURL>"
      "<iconList>"
        "<icon>"
          "<mimetype>image/png</mimetype>"
          "<height>48</height>"
          "<width>48</width>"
          "<depth>24</depth>"
          "<url>hue_logo_0.png</url>"
        "</icon>"
        "<icon>"
          "<mimetype>image/png</mimetype>"
          "<height>120</height>"
          "<width>120</width>"
          "<depth>24</depth>"
          "<url>hue_logo_3.png</url>"
        "</icon>"
      "</iconList>"
    "</device>"
  "</root>";

  const char HUE_2015_SETUP_TEMPLATE[] PROGMEM =
    "<?xml version=\"1.0\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
      "<specVersion>"
        "<major>1</major>"
        "<minor>0</minor>"
      "</specVersion>"
      "<URLBase>http://%s:%d/</URLBase>"
      "<device>"
        "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
        "<friendlyName>Alexa Hue Bridge</friendlyName>"
        "<manufacturer>Royal Philips Electronics</manufacturer>"
        "<manufacturerURL>http://www.philips.com</manufacturerURL>"
        "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
        "<modelName>Philips hue bridge 2015</modelName>"
        "<modelNumber>929000226503</modelNumber>"
        "<modelURL>http://www.meethue.com</modelURL>"
        "<serialNumber>001788101fe2</serialNumber>"
        "<UDN>uuid:b4eeb628-680e-11e7-9c2c-6c4008b0d85e</UDN>"
        "<presentationURL>index.html</presentationURL>"
        "<iconList>"
          "<icon>"
            "<mimetype>image/png</mimetype>"
            "<height>48</height>"
            "<width>48</width>"
            "<depth>24</depth>"
            "<url>hue_logo_0.png</url>"
          "</icon>"
          "<icon>"
            "<mimetype>image/png</mimetype>"
            "<height>120</height>"
            "<width>120</width>"
            "<depth>24</depth>"
            "<url>hue_logo_3.png</url>"
          "</icon>"
        "</iconList>"
      "</device>"
    "</root>";

const char HUE_HEADERS[] PROGMEM =
    "HTTP/1.1 200 OK\r\n"
    "CONTENT-LENGTH: %d\r\n"
    "CONTENT-TYPE: text/xml\r\n"
    "DATE: Sun, 01 Jan 2017 00:00:00 GMT\r\n"
    "LAST-MODIFIED: Sat, 01 Jan 2017 00:00:00 GMT\r\n"
    "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
    "X-USER-AGENT: redsonic\r\n"
    "CONNECTION: close\r\n\r\n"
    "%s\r\n";

#ifdef DEBUG_HUE
    #define DEBUG_MSG_HUE(...) Serial.printf( __VA_ARGS__ )
#else
    #define DEBUG_MSG_HUE(...)
#endif

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <WiFiUdp.h>
#include <functional>
#include <vector>

typedef std::function<void(unsigned char, const char *, bool)> TStateFunction;

typedef struct {
    char * name;
    char * uuid;
    bool hit;
    bool state;
    bool brightness;
    bool hue;
    AsyncWebServer * server;
} hueesp_device_t;

class hueESP {
    public:
        hueESP(unsigned int port = HUE_DEFAULT_TCP_BASE_PORT);
        void addDevice(const char * device_name);
        void onMessage(TStateFunction fn) { _callback = fn; }
        void enable(bool enable);
        void handle();

    private:
        bool _enabled = true;
        unsigned int _base_port = HUE_DEFAULT_TCP_BASE_PORT;
        std::vector<hueesp_device_t> _devices;
        WiFiUDP _udp;
        TStateFunction _callback = NULL;

        unsigned int _roundsLeft = 0;
        unsigned int _current = 0;
        unsigned long _lastTick;
        IPAddress _remoteIP;
        unsigned int _remotePort;

        String _getValue(String data, char separator, int index);
        void _sendUDPResponse(unsigned int device_id);
        void _nextUDPResponse();
        void _handleUDPPacket(const IPAddress remoteIP, unsigned int remotePort, uint8_t *data, size_t len);
        void _handleSetup(AsyncWebServerRequest *request, unsigned int device_id);
        void _handleContent(AsyncWebServerRequest *request, unsigned int device_id, char * content);

};

#endif
