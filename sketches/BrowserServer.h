#pragma once
#include <ESPAsyncWebServer.h>
#include <ESPAsyncDNSServer.h>

#define MAX_WEBSOCKET_CLIENT		4

typedef struct {
	String wwwUsername;
	String wwwPassword;
} strHTTPAuth_t;

class BrowserServerClass : public AsyncWebServer{
protected:
	strHTTPAuth_t _httpAuth;
	
public:
	BrowserServerClass(uint16_t port, char * username, char * password);
	~BrowserServerClass() {};
	void begin();
	bool checkAdminAuth(AsyncWebServerRequest * request);
};

extern BrowserServerClass * server;
extern AsyncWebSocket webSocket;
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);