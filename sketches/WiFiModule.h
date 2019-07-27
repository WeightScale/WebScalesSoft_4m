#pragma once
#include <ESP8266WiFi.h>
#include "Task.h"
#include <functional>
#include <StringArray.h>
#include <ArduinoJson.h>
#include "Config.h"

#define MAX_POINTS				10
//#define AP_FILE_LIST			"listap.json"

struct EntryWiFi {
	String ssid;
	String passphrase;
	bool dnip;
	String ip;
	String gate;
	String mask;
};

typedef std::vector<EntryWiFi> Wifilist;

class WiFiModuleClass : public Task{
private:	
	IPAddress _lanIp;
	IPAddress _gate;
#ifdef MULTI_POINTS_CONNECT
	Wifilist _accessPoints;
	int cached_size;
	bool _enableSSID = true;		/* если нет сети в списке*/
	bool _downloadValue();
	bool _dnip;
	bool _isUdate = false;			/*обновились настройки*/
	bool _Scan = false;				/*сканирование запущено*/
	bool _EnableAP = false;			/*только точка доступа*/
	unsigned long _time_connect;	/*время для следуещей попытки соединения*/
#else
	//EntryWiFi _entryWiFi;
#endif // MULTI_POINTS_CONNECT
	
	MyEEPROMStruct * _value;
	String _hostName;		
public:	
	WiFiModuleClass(MyEEPROMStruct * value);
	WiFiModuleClass(char *host);
#ifdef MULTI_POINTS_CONNECT
	~WiFiModuleClass() {_accessPoints.clear();};
	Wifilist points() {return _accessPoints;};
	bool EnableAP() {return _EnableAP;};
#else
	~WiFiModuleClass() {
		free(_value);
	};
#endif // MULTI_POINTS_CONNECT	
	void connect();
	void scanResultForConnect(int networksFound);	
	void printScanResult(int networksFound);
#ifdef MULTI_POINTS_CONNECT
	bool savePoint(EntryWiFi point);
	bool removePoint(const String &name);	
	void scan();	
	bool /*ICACHE_RAM_ATTR*/ addPoint(EntryWiFi _point) {		
		// Check if the Thread already exists on the array
		for( auto p : _accessPoints) {
			if (p.ssid.equals(_point.ssid))
				return true;					
		}
		if (_accessPoints.size() < MAX_POINTS) {
			_accessPoints.push_back(_point);
			cached_size++;
			return true;
		}		
		// Array is full
		return false;
	} 
	void loadPoints() {		
		_accessPoints.clear();
		_downloadValue();
	};	
	bool isUpdate() {return _isUdate;};
	void isUpdate(bool u) {_isUdate = u;};
#endif // MULTI_POINTS_CONNECT		
};
extern IPAddress apIP;
extern IPAddress netMsk;