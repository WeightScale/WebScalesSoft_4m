#include "Board.h"
#include "BrowserServer.h"
#include "SettingsPage.h"
#include <ESP8266NetBIOS.h>

BoardClass * Board;

BoardClass::BoardClass() {
	_blink = new BlinkClass();
	//BlinkClass _blink;
	_memory = new MemoryClass<MyEEPROMStruct>(&_eeprom);
	if (!_memory->init()) {
		doDefault();
	}
	_wifi = new WiFiModuleClass(&_eeprom);
	server = new BrowserServerClass(80, MASTER_USER, MASTER_PASS);
	STAGotIP = WiFi.onStationModeGotIP(std::bind(&BoardClass::onSTAGotIP, this, std::placeholders::_1));	
	stationDisconnected = WiFi.onStationModeDisconnected(std::bind(&BoardClass::onStationModeDisconnected, this, std::placeholders::_1));
	stationConnected = WiFi.onStationModeConnected(std::bind(&BoardClass::onStationModeConnected, this, std::placeholders::_1));	
#ifdef MULTI_POINTS_CONNECT
	_wifi->loadPoints();
	SettingsPage = new SettingsPageClass(&_eeprom);
#endif // MULTI_POINTS_CONNECT
};

bool BoardClass::doDefault() {
	String host = F("MASTERSOFT");
	String url = F("sdb.net.ua");
	String u = "admin";
	host.toCharArray(_eeprom.hostName, host.length() + 1);
	url.toCharArray(_eeprom.hostUrl, url.length() + 1);
	u.toCharArray(_eeprom.user, u.length() + 1);
	u.toCharArray(_eeprom.password, u.length() + 1);
	_eeprom.hostPin = 0;
	_eeprom.timeScan = 20;
	_eeprom.deltaRSSI = 20;
#ifndef MULTI_POINTS_CONNECT
	u.toCharArray(_eeprom.wSSID, u.length() + 1);
	u.toCharArray(_eeprom.wKey, u.length() + 1);
#endif // !MULTI_POINTS_CONNECT
	return _memory->save();
};

void /*ICACHE_RAM_ATTR*/ BoardClass::onSTAGotIP(const WiFiEventStationModeGotIP& evt) {
#ifdef MULTI_POINTS_CONNECT
	WiFi.enableAP(false);
#else
	_wifi->pause();
#endif // MULTI_POINTS_CONNECT
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	NBNS.begin(WiFi.hostname().c_str());	
	sendConnectCmd(true);	
	onSTA();
};

void BoardClass::onStationModeConnected(const WiFiEventStationModeConnected& evt) {	
	Serial.println("{\"cmd\":\"resume\"}");
};

void BoardClass::onStationModeDisconnected(const WiFiEventStationModeDisconnected& evt) {	
#ifdef MULTI_POINTS_CONNECT
	WiFi.enableAP(true);
#else
	_wifi->resume();
#endif // MULTI_POINTS_CONNECT
	offSTA();
	sendConnectCmd(false);
	NBNS.end();
	webSocket.textAll("{\"cmd\":\"error\",\"ssid\":\"" + evt.ssid + "\",\"status\":\"" + String(evt.reason) + "\"}");
};

void BoardClass::sendConnectCmd(bool connect) {
	DynamicJsonBuffer jsonBuffer;
	JsonObject &json = jsonBuffer.createObject();
	json["cmd"] = "sta";
	json["con"] = connect;
	json["ip"] = WiFi.localIP().toString();
	json["ssid"] = WiFi.SSID();
	String str = "";
	json.printTo(str);
	Serial.println(str);	
};

String /*ICACHE_RAM_ATTR*/ BoardClass::readSerial(uint32_t timeout) {	
	String tempData = "";
	uint64_t timeOld = millis();
	while (Serial.available() || (millis() < (timeOld + timeout))) {
		if (Serial.available()) {	
			tempData += (char) Serial.read();
			timeOld = millis();
		}
		yield();
	}
	return tempData;
}

void BoardClass::parceCmd(JsonObject& cmd) {
	const char *command = cmd["cmd"];
	String strCmd = String();
#ifdef MULTI_POINTS_CONNECT
	if (strcmp(command, "point") == 0) {
		/* добавить точку доступа */		
		EntryWiFi p;
		p.ssid = cmd["ssid"].as<String>();
		p.passphrase = cmd["key"].as<String>();
		p.dnip = cmd["dnip"];
		p.ip = cmd["lan_ip"].as<String>();
		p.gate = cmd["gateway"].as<String>();
		p.mask = cmd["subnet"].as<String>();
		_wifi->savePoint(p);
		if (_wifi->savePoint(p)) {
			_wifi->loadPoints();
			_wifi->isUpdate(true);
			String req;
			cmd.printTo(req);
			Serial.println(req);
		}		
		return;
	}else if (strcmp(command, "gpoint") == 0) {
		/* получить список точк */		
		JsonArray& points = cmd.createNestedArray("points");
		for (auto point : Board->wifi()->points()) {
			JsonObject& p = points.createNestedObject();
			p["ssid"] = point.ssid;
			p["pass"] = point.passphrase;
			p["dnip"] = point.dnip;
			p["ip"] = point.ip;
			p["gate"] = point.gate;
			p["mask"] = point.mask;
		}	
		String str = "";
		cmd.printTo(str);
		Serial.println(str);
		return;
	}else if (strcmp(command, "delpoint") == 0) {
		/* удалить точку доступа */
		String point = cmd["ssid"].as<String>();
		if (_wifi->removePoint(point)) {
			if (WiFi.SSID().equals(point)) {
				_wifi->isUpdate(true);				
			}
			String req;
			cmd.printTo(req);
			Serial.println(req);
		}
		;
		return;
	}else 
#else
	if (strcmp(command, "snet") == 0) {
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		json["cmd"] = command;
		bool flag = savePointValue(cmd);
		if (flag) {
			json["code"] = 200;	
		}
		else {
			json["code"] = 400;
		}
		;		
		json.printTo(strCmd);
		Serial.println(strCmd);
		Serial.flush();
		if (flag)
			Board->restart();
		return;
	}
	else if (strcmp(command, "gnet") == 0) {
		cmd["id_host"] = _eeprom.hostName;
		cmd["id_dnip"] = _eeprom.dnip;
		cmd["id_lanip"] = _eeprom.lanIp;
		cmd["id_gate"] = _eeprom.gate;
		cmd["id_mask"] = _eeprom.mask;
		cmd["id_ssid"] = _eeprom.wSSID;
		cmd["id_key"] = _eeprom.wKey;
		cmd["id_server"] = _eeprom.hostUrl;
		cmd["id_pin"] = _eeprom.hostPin;
		cmd.printTo(strCmd);
		Serial.println(strCmd);
		return;
	}
	else
#endif // MULTI_POINTS_CONNECT
	if (strcmp(command, "wt") == 0) {
		cmd.printTo(strCmd);
		scales.doData(cmd);
	}else if (strcmp(command, "rst") == 0) {
		Board->restart();
		return;
	}else {
		cmd.printTo(strCmd);
	}
	webSocket.textAll(strCmd);
};

#ifndef MULTI_POINTS_CONNECT
bool BoardClass::savePointValue(JsonObject& value) {
	return false;
};	
#endif // MULTI_POINTS_CONNECT