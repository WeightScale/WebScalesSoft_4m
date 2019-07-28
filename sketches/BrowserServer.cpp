#include "BrowserServer.h"
#include "SettingsPage.h"
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include "Board.h"
#include "HttpUpdater.h"

BrowserServerClass * server;
AsyncWebSocket webSocket("/ws");
AsyncDNSServer dnsServer;

BrowserServerClass::BrowserServerClass(uint16_t port, char * username, char * password)	: AsyncWebServer(port) {
	_httpAuth.wwwUsername = username;
	_httpAuth.wwwPassword = password;
}

void BrowserServerClass::begin() {
	dnsServer.setTTL(300);
	dnsServer.setErrorReplyCode(AsyncDNSReplyCode::ServerFailure);
	dnsServer.start(53, "*", apIP);
	webSocket.onEvent(onWsEvent);
	addHandler(&webSocket);
#ifdef MULTI_POINTS_CONNECT
	addHandler(SettingsPage);
	on("/settings.json", HTTP_ANY, std::bind(&SettingsPageClass::handleValue, SettingsPage, std::placeholders::_1));
	on("/wtr", HTTP_GET, [this](AsyncWebServerRequest * request) {
			/* Получить вес и заряд. */
			AsyncResponseStream *response = request->beginResponseStream("text/json");
			DynamicJsonBuffer jsonBuffer;
			JsonObject &json = jsonBuffer.createObject();
			scales.doDataRandom(json);
			json.printTo(*response);
			request->send(response);
		});
#endif // MULTI_POINTS_CONNECT
	addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
	addHandler(new SPIFFSEditor(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str()));	
	addHandler(new HttpUpdaterClass("sa", "654321"));
	on("/wt",HTTP_GET,	[](AsyncWebServerRequest * request) {
			/* Получить вес и заряд. */
			AsyncResponseStream *response = request->beginResponseStream(F("text/json"));
			response->println(scales.dataValue());
			request->send(response);			
		});
	on("/tp",[this](AsyncWebServerRequest * request) {
			/* Установить тару. */
			#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
				Serial.println("{\"cmd\":\"tp\"}");
			#endif 								/* Установить тару. */
			return request->send(204, F("text/html"), "");
		});
	on("/rc",[](AsyncWebServerRequest * reguest) {		
		if (!server->checkAdminAuth(reguest)) {
			return reguest->requestAuthentication();
		}
		
		AsyncWebServerResponse *response = reguest->beginResponse_P(200, "text/html", "<meta http-equiv='refresh' content='10;URL=/'>RECONNECT...");
		response->addHeader("Connection", "close");
		reguest->send(response);
		reguest->onDisconnect([]() {
			Board->restart();
		});
	});
#ifdef HTML_PROGMEM	
	on("/", [](AsyncWebServerRequest * reguest){	reguest->send_P(200, F("text/html"), index_html); }); /* Главная страница. */
	on("/index-l.html", [](AsyncWebServerRequest * reguest){	reguest->send_P(200, F("text/html"), index_l_html); }); /* Легкая страница. */
	on("/global.css", [](AsyncWebServerRequest * reguest){	reguest->send_P(200, F("text/css"), global_css); }); /* Стили */
	on("/bat.png",[](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), bat_png, bat_png_len);
			request->send(response);
		});
	on("/und.png",[](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), und_png, und_png_len);
			request->send(response);
		});
	on("/set.png",[](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), set_png, set_png_len);
			request->send(response);
		});
	on("/zerow.png",[](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), zerow_png, zerow_png_len);
			request->send(response);
		});
	on("/scales.png",[](AsyncWebServerRequest * request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), scales_png, scales_png_len);
			request->send(response);
		});
	/*rewrite("/", "/settings.html").setFilter([](AsyncWebServerRequest *request) {
		
		return request->host() == "host1";	
	});*/
	//rewrite("/", "/settings.html").setFilter(ON_AP_FILTER);
	serveStatic("/", SPIFFS, "/");
#else
	//rewrite("/sn", "/settings.html");
	serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");	
#endif
	AsyncWebServer::begin();  // Web server start
};

bool BrowserServerClass::checkAdminAuth(AsyncWebServerRequest * r) {	
	return r->authenticate(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
	if (type == WS_EVT_CONNECT) {	
		if (server->count() > MAX_WEBSOCKET_CLIENT) {
			client->text("{\"cmd\":\"cls\",\"code\":1111}");
		}
	}else if (type == WS_EVT_DISCONNECT) {
		//client->close(true);
	}else if (type == WS_EVT_ERROR) {
		client->close(true);
	}else if (type == WS_EVT_DATA) {
		DynamicJsonBuffer jsonBuffer(len);
		JsonObject &root = jsonBuffer.parseObject(data);
		if (!root.success()) {
			return;
		}
		const char *command = root["cmd"]; /* Получить комманду */
		JsonObject& json = jsonBuffer.createObject();
		json["cmd"] = command;
#ifdef MULTI_POINTS_CONNECT
		if (strcmp(command, "gpoint") == 0) {
			JsonArray& points = json.createNestedArray("points");
			for (auto point : Board->wifi()->points()) {
				JsonObject& p = jsonBuffer.createObject();
				p["ssid"] = point.ssid;
				p["pass"] = point.passphrase;
				p["dnip"] = point.dnip;
				p["ip"] = point.ip;
				p["gate"] = point.gate;
				p["mask"] = point.mask;
				points.add(p);
			}				
		}else 
#endif //MULTI_POINTS_CONNECT
		if (strcmp(command, "tp") == 0) {
			String str = "";
			json.printTo(str);
			Serial.println(str);
			return;
		}else if (strcmp(command, "scan") == 0) {
			WiFi.scanNetworksAsync(std::bind(&WiFiModuleClass::printScanResult, Board->wifi(), std::placeholders::_1), true);
			return;
		}else if (strcmp(command, "binfo") == 0) {
		}else {
			return;
		}
		size_t lengh = json.measureLength();
		AsyncWebSocketMessageBuffer * buffer = webSocket.makeBuffer(lengh);
		if (buffer) {
			json.printTo((char *)buffer->get(), lengh + 1);
			if (client) {
				client->text(buffer);
			}else {
				delete buffer;
			}
		}
	}	
};