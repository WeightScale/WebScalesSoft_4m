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

class CaptiveRequestHandler : public AsyncWebHandler {
public:
	CaptiveRequestHandler() {}
	virtual ~CaptiveRequestHandler() {}
	
	bool canHandle(AsyncWebServerRequest *request) {
		if (!request->host().equalsIgnoreCase(WiFi.softAPIP().toString())) {
			return true;
		}
		return false;
	}

	void handleRequest(AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
		response->addHeader("Location", String("http://") + WiFi.softAPIP().toString());
		request->send(response);
	}
};

extern AsyncDNSServer dnsServer;
extern BrowserServerClass * server;
extern AsyncWebSocket webSocket;
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);