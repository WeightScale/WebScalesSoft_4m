#include "WiFiModule.h"
#include <FS.h>
#include <ArduinoJson.h>
#include <limits.h>
#include <string.h>
#include "BrowserServer.h"
#include "Board.h"

IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

WiFiModuleClass::WiFiModuleClass(MyEEPROMStruct * value) : _value(value), Task(value->timeScan * 1000) {
#ifdef MULTI_POINTS_CONNECT
	onRun(std::bind(&WiFiModuleClass::scan, this));
#else
	onRun(std::bind(&WiFiModuleClass::connect, this));
#endif // MULTI_POINTS_CONNECT	
	_hostName = String(_value->hostName);
	WiFi.persistent(false);
	WiFi.setAutoConnect(false);
	WiFi.setAutoReconnect(false);
	WiFi.setPhyMode(WIFI_PHY_MODE_11B);
	//WiFi.mode(WIFI_STA);
#ifdef MULTI_POINTS_CONNECT
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAPConfig(apIP, apIP, netMsk);
	WiFi.softAP(_hostName.c_str(), "12345678");
#else
	WiFi.mode(WIFI_STA);
	if (!_value->dnip) {
		if (_lanIp.fromString(_value->lanIp) && _gate.fromString(_value->gate)&&netMsk.fromString(_value->mask)) {
			WiFi.config(_lanIp, _gate, netMsk);    									// Надо сделать настройки ip адреса
		}
	}
#endif // MULTI_POINTS_CONNECT
	_hostName.toLowerCase();
	WiFi.hostname(_hostName);
};

/**/void /*ICACHE_RAM_ATTR*/ WiFiModuleClass::connect() {	
#ifdef MULTI_POINTS_CONNECT
	if (WiFi.scanComplete() == WIFI_SCAN_RUNNING)
		return;	
	wl_status_t _status = WiFi.status();
	if (!_isUdate && !_Scan)
	{		
		if (_status != WL_DISCONNECTED && _status != WL_NO_SSID_AVAIL && _status != WL_IDLE_STATUS && _status != WL_CONNECT_FAILED)	{
			//_connecting = false;
			return;
		}else{
			if (_time_connect > millis())
				return;
		}
		if (_EnableAP){
			return;	
		}
	}	
	int scanResult = WiFi.scanComplete();
	if (scanResult == WIFI_SCAN_RUNNING) {
		// scan is running, do nothing yet
		_status = WL_NO_SSID_AVAIL;
		return;
	} 
		
	if (scanResult == 0) {
		WiFi.scanDelete();
		delay(0);
		WiFi.disconnect();
		// scan wifi async mode
		WiFi.scanNetworks(true);
	}else if (scanResult == WIFI_SCAN_FAILED) {
		//WiFi.disconnect();		
		WiFi.scanNetworks(true);
	}else if (scanResult > 0) {
		// scan done, analyze
		EntryWiFi bestNetwork;
		int bestNetworkDb = INT_MIN;
		unsigned int bestRSSI;
		unsigned int currRSSI;
		uint8 bestBSSID[6];
		int32_t bestChannel;

		delay(0);
	
		for (int8_t i = 0; i < scanResult; ++i) {

			String ssid_scan;
			int32_t rssi_scan;
			uint8_t sec_scan;
			uint8_t* BSSID_scan;
			int32_t chan_scan;
			bool hidden_scan;

			WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);			
			if(WiFi.SSID().equals(ssid_scan))
				currRSSI = min(max(2 * (rssi_scan + 100), 0), 100);
			for(auto &entry : _accessPoints) {
				if (ssid_scan == entry.ssid) {
					if (rssi_scan > bestNetworkDb) {
						// best network
					   if(sec_scan == ENC_TYPE_NONE || entry.passphrase) {
							// check for passphrase if not open wlan
							bestNetworkDb = rssi_scan;
						    bestRSSI = min(max(2 * (rssi_scan + 100), 0), 100);
							bestChannel = chan_scan;
							bestNetwork = entry;
							memcpy((void*) &bestBSSID, (void*) BSSID_scan, sizeof(bestBSSID));
						}
					}
					break;
				}
			}		
			delay(0);
		}
		// clean up ram
		WiFi.scanDelete();

		delay(0);
		//wl_status_t _status;
		if(bestNetwork.ssid.length() > 0) {
			if (_isUdate) {
				if (_dnip != bestNetwork.dnip) {
					if (bestNetwork.dnip) {
						WiFi.disconnect();
					}
				}
				_isUdate = false;
				goto connect;
			}			
			
			if (_Scan){
				_Scan = false;
				_status = WiFi.status();	
				if (_status == WL_CONNECTED /*|| _status == WL_IDLE_STATUS*/)	{
					if (WiFi.SSID().equals(bestNetwork.ssid)){				
						goto resume;
					}else{
						if (currRSSI > (bestRSSI - _value->deltaRSSI)) {				
							goto resume;
						}
					}
				}
			}	
connect:							
			if (!bestNetwork.dnip){					
				if (_lanIp.fromString(bestNetwork.ip) && _gate.fromString(bestNetwork.gate) && netMsk.fromString(bestNetwork.mask))	{
					WiFi.config(_lanIp, _gate, netMsk);              									// Надо сделать настройки ip адреса
				}else{
					bestNetwork.dnip = true;	
				}
			}else{
				WiFi.config(0U, 0U, 0U);
			}
			Serial.println("{\"cmd\":\"pause\"}");		
			WiFi.begin(bestNetwork.ssid.c_str(), bestNetwork.passphrase.c_str(), bestChannel, bestBSSID);
			_status = WiFi.status();
			static const uint32_t connectTimeout = 5000;      //5s timeout
                
			auto startTime = millis();
			// wait for connection, fail, or timeout
			while(_status != WL_CONNECTED && _status != WL_NO_SSID_AVAIL && _status != WL_CONNECT_FAILED && (millis() - startTime) <= connectTimeout) {
				delay(10);
				_status = WiFi.status();
			}
			if (_status == WL_CONNECTED){
				WiFi.enableAP(false);
				//NBNS.begin(WiFi.hostname().c_str());
				_EnableAP = false;
			}else{
				//NBNS.end();
				_time_connect = millis()+5000;
				//webSocket.textAll("{\"cmd\":\"error\",\"ssid\":\"" + bestNetwork.ssid + "\",\"status\":\"" + String(_status) + "\"}");
				//Serial.println("{\"cmd\":\"error\",\"ssid\":\""+bestNetwork.ssid+"\",\"status\":\""+String(_status)+"\"}");
			}				
		}else{
			WiFi.enableAP(true);
			_EnableAP = true;
			WiFi.disconnect();
		}
		if (_Scan)
			_Scan = false;	
		if (_isUdate)
			_isUdate = false;			
	}
resume:	
	Serial.println("{\"cmd\":\"resume\"}");
	resume();
#else
	wl_status_t _status = WiFi.status();
	pause();
	if (String(_value->wSSID).length() == 0) {
		WiFi.setAutoConnect(false);
		WiFi.setAutoReconnect(false);
		return;
	}
	if (WiFi.SSID().equals(_value->wSSID)) {
		WiFi.begin();		
		_status = WiFi.status();
		static const uint32_t connectTimeout = 5000;       //5s timeout                
		auto startTime = millis();
		// wait for connection, fail, or timeout
		while(_status != WL_CONNECTED && _status != WL_NO_SSID_AVAIL && _status != WL_CONNECT_FAILED && (millis() - startTime) <= connectTimeout) {
			delay(10);
			_status = WiFi.status();
		}
		return;
	}
	WiFi.disconnect(false);
	/* !*/
	int n = WiFi.scanComplete();
	if (n == -2) {
		WiFi.scanNetworksAsync(std::bind(&WiFiModuleClass::scanResultForConnect, this, std::placeholders::_1), true);
	}
	else if (n > 0) {
		scanResultForConnect(n);
	}
#endif // MULTI_POINTS_CONNECT

}

#ifndef MULTI_POINTS_CONNECT
	void ICACHE_FLASH_ATTR WiFiModuleClass::scanResultForConnect(int networksFound) {
	
	}		  
#endif // !MULTI_POINTS_CONNECT


#ifdef MULTI_POINTS_CONNECT
	void /*ICACHE_RAM_ATTR*/ WiFiModuleClass::scan() {	
		pause();
		_Scan = true;
		Serial.println("{\"cmd\":\"pause\"}");
	};			  
#endif // MULTI_POINTS_CONNECT

void ICACHE_FLASH_ATTR WiFiModuleClass::printScanResult(int networksFound) {
	// sort by RSSI
	int n = networksFound;
	int indices[n];
	int skip[n];
	for (int i = 0; i < networksFound; i++) {
		indices[i] = i;
	}
	for (int i = 0; i < networksFound; i++) {
		for (int j = i + 1; j < networksFound; j++) {
			if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
				std::swap(indices[i], indices[j]);
				std::swap(skip[i], skip[j]);
			}
		}
	}
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	root["cmd"] = "ssl";
	JsonArray &scan = root.createNestedArray("list");
	for (int i = 0; i < 5 && i < networksFound; ++i) {
		JsonObject &item = scan.createNestedObject();
		item["ssid"] = WiFi.SSID(indices[i]);
		item["rssi"] = WiFi.RSSI(indices[i]);
	}
	size_t len = root.measureLength();
	AsyncWebSocketMessageBuffer *buffer = webSocket.makeBuffer(len);    //  creates a buffer (len + 1) for you.
	if(buffer) {
		root.printTo((char *)buffer->get(), len + 1);
		webSocket.textAll(buffer);
	}
	WiFi.scanDelete();
}

#ifdef MULTI_POINTS_CONNECT
	bool WiFiModuleClass::_downloadValue() {
	Dir dir = SPIFFS.openDir("/P/");
	while (dir.next()) {
		File f = SPIFFS.open(dir.fileName(), "r");
		size_t size = f.size();
		std::unique_ptr<char[]> buf(new char[size]);
		f.readBytes(buf.get(), size);
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.parseObject(buf.get());
		if (json.success()) {
			EntryWiFi ap;
			ap.ssid = json["ssid"].as<String>();
			ap.passphrase = json["pass"].as<String>();
			ap.dnip = json["dnip"].as<bool>();  
			ap.ip = json["ip"].as<String>();
			ap.gate = json["gate"].as<String>();
			ap.mask = json["mask"].as<String>();
			addPoint(ap);				
		}
	}
	return true;
}	

bool WiFiModuleClass::savePoint(EntryWiFi point) {
	Dir dir = SPIFFS.openDir("/P/");
	String filename = "/P/" + String(point.ssid) + ".json";
	//int i = 0;
	while (dir.next()) {
		if (dir.fileName().equals(filename)){
			if (!SPIFFS.remove(filename)){
				return false;
			}	
		}
		//i++;
	}
	File f = SPIFFS.open(filename, "w+");
	// Check if we created the file
	if(f) {
		DynamicJsonBuffer jsonBuffer;
		JsonObject &root = jsonBuffer.createObject();
		root["ssid"] = point.ssid;
		root["pass"] = point.passphrase;
		root["dnip"] = point.dnip;
		root["ip"] = point.ip;
		root["gate"] = point.gate;
		root["mask"] = point.mask;
		root.printTo(f);
		f.close();
		return addPoint(point);
	}
	f.close();
	return false;
};

bool /*ICACHE_RAM_ATTR*/ WiFiModuleClass::removePoint(const String &name) {
	String filename = "/P/" + name + ".json";
	if (SPIFFS.remove(filename)){
		loadPoints();
		return true;
	}
	return false;
}
#endif // MULTI_POINTS_CONNECT



