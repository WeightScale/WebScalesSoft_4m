#pragma once
#include <FS.h>
#include <ESP_EEPROM.h>
//#include "Task.h"
//#include "Config.h"
#include "TaskController.h"
#include "WiFiModule.h"
#include "Scales.h"


template <typename T>class MemoryClass : protected EEPROMClass {
public:
	T * _value;

public:
	MemoryClass(T *mem) {
		_value = mem;
	};
	~MemoryClass() {
		close();
	};
	bool init() {
		SPIFFS.begin();	
		begin(sizeof(T));
		if (percentUsed() < 0)
			return false;
		get(0, *_value);
		return true;
	};
	bool save() {
		put(0, *_value);
		return commit();
	};
	void close() {
		end();
		SPIFFS.end();
	};
	bool doDefault();	
};

class BlinkClass : public Task {
public:
	unsigned int _flash = 500;
public:
	BlinkClass(): Task(500) {
		pinMode(LED, OUTPUT);
		ledOn();
		onRun(std::bind(&BlinkClass::blinkAP, this));
	};
	void blinkSTA() {
		static unsigned char clk;
		bool led = !digitalRead(LED);
		digitalWrite(LED, led);
		if (clk < 6) {
			led ? _flash = 70 : _flash = 40;
			clk++;
		}
		else {
			_flash = 2000;
			digitalWrite(LED, HIGH);
			clk = 0;
		}
		setInterval(_flash);
	}
	void blinkAP() {
		ledTogle();
		setInterval(500);
	}
	void ledOn() {digitalWrite(LED, LOW); };
	void ledOff() {digitalWrite(LED, HIGH); };
	void ledTogle() {digitalWrite(LED, !digitalRead(LED)); };
};

class BoardClass : public TaskController {
private:	
	struct MyEEPROMStruct _eeprom;
	BlinkClass * _blink;
	WiFiModuleClass * _wifi;
	WiFiEventHandler stationConnected;
	WiFiEventHandler stationDisconnected;
	WiFiEventHandler STAGotIP;
	MemoryClass<MyEEPROMStruct> *_memory;	
	//BrowserServerClass *_server;
public :
	BoardClass();
	~BoardClass() {
		delete _blink;
		delete _wifi;
		delete _memory;
	};
	void init() {
		add(_blink);
		add(_wifi);
		//add(&scales);
	};
	void handle() {
		run();		
	};
	MemoryClass<MyEEPROMStruct> *memory() {return _memory;};
	WiFiModuleClass *wifi() {return _wifi;};
	bool doDefault();
	void onSTA() {_blink->onRun(std::bind(&BlinkClass::blinkSTA, _blink)); };
	void offSTA() {_blink->onRun(std::bind(&BlinkClass::blinkAP, _blink)); };
	void onStationModeConnected(const WiFiEventStationModeConnected& evt);
	void onStationModeDisconnected(const WiFiEventStationModeDisconnected& evt);
	void onSTAGotIP(const WiFiEventStationModeGotIP& evt);
	void parceCmd(JsonObject& cmd);
	String readSerial(uint32_t timeout = 10);
#ifndef MULTI_POINTS_CONNECT
	bool savePointValue(JsonObject& value);	
#endif // MULTI_POINTS_CONNECT
	void sendConnectCmd(bool connect);
	void restart() {
		SPIFFS.end();
		ESP.restart();
	};

};

String toStringIp(IPAddress ip);
extern BoardClass * Board;