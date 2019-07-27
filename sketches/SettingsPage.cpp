#include "SettingsPage.h"
#include "Board.h"
#include "BrowserServer.h"
#include "Version.h"

SettingsPageClass * SettingsPage;

bool SettingsPageClass::canHandle(AsyncWebServerRequest *request) {	
	if (request->url().equalsIgnoreCase(F("/settings.html"))) {
		goto auth;
	}
#ifndef HTML_PROGMEM
	else if (request->url().equalsIgnoreCase("/sn")) {
		goto auth;
	}
#endif
	else
		return false;
auth:
	if (!request->authenticate(_value->user, _value->password)) {
		if (!server->checkAdminAuth(request)) {
			request->requestAuthentication();
			return false;
		}
	}
	return true;
}

void SettingsPageClass::handleRequest(AsyncWebServerRequest *request) {
	if (request->args() > 0) {
		String message = " ";
#ifdef MULTI_POINTS_CONNECT	
		if (request->hasArg("delete")){
			if (Board->wifi()->removePoint(request->arg("ssid"))){
				//Board->wifi()->loadPoints();
				if(WiFi.SSID().equals(request->arg("ssid"))){
					Board->wifi()->isUpdate(true);
				}								
				goto url;
			}
				
		}else if (request->hasArg("ssid")) {
			EntryWiFi p ;
			p.ssid =  request->arg("ssid");
			p.passphrase = request->arg("key");
			/**/
			if (request->hasArg("dnip"))
				p.dnip = true;
			else
				p.dnip = false;
			p.ip = request->arg("lan_ip");
			p.gate = request->arg("gateway");
			p.mask = request->arg("subnet");						

			if (Board->wifi()->savePoint(p)) {
				Board->wifi()->loadPoints();
				Board->wifi()->isUpdate(true);
				goto url;
			}
			goto err;
		}
#endif // MULTI_POINTS_CONNECT		
		if (request->hasArg("host")) {			
			request->arg("host").toCharArray(_value->hostName, request->arg("host").length() + 1);
			_value->timeScan = request->arg("t_scan").toInt();
			Board->wifi()->setInterval(_value->timeScan * 1000);
			_value->deltaRSSI = request->arg("d_rssi").toInt();
			request->arg("nadmin").toCharArray(_value->user, request->arg("nadmin").length() + 1);
			request->arg("padmin").toCharArray(_value->password, request->arg("padmin").length() + 1);
			goto save;
		}
save:
		if (Board->memory()->save()) {
			goto url;
		}
err:
		return request->send(400);
	}
url:
#ifdef HTML_PROGMEM
	request->send_P(200, F("text/html"), settings_html);
#else
	if (request->url().equalsIgnoreCase("/sn"))
		request->send_P(200, F("text/html"), netIndex);
	else
		request->send(SPIFFS, request->url());
#endif
}

void SettingsPageClass::handleValue(AsyncWebServerRequest * request) {
	if (!request->authenticate(_value->user, _value->password)) {
		if (!server->checkAdminAuth(request)) {
			return request->requestAuthentication();
		}
	}
	
	AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	doSettings(root);	
	root.printTo(*response);
	request->send(response);
}

size_t SettingsPageClass::doSettings(JsonObject& root) {	
	root["id_host"] = _value->hostName;
	root["id_t_scan"] = _value->timeScan;
	root["id_d_rssi"] = _value->deltaRSSI;
	root["id_nadmin"] = _value->user;
	root["id_padmin"] = _value->password;
	
	JsonObject& info = root.createNestedObject("info");
	
	info["id_net"] = WiFi.SSID();
	info["id_sta_ip"] = WiFi.localIP().toString();
	info["id_rssi"] = String(min(max(2 * (WiFi.RSSI() + 100), 0), 100)) + "%"; 
	info["id_vr"] = SKETCH_VERSION;
	return root.measureLength();
};