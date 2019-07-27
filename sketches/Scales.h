#pragma once
#include "Task.h"
#include <ArduinoJson.h>

struct ScalesValue {
	float weight;
	int accuracy;
	int stableNum;
	unsigned int charge;
};

class ScalesClass {
private:
	ScalesValue _master;
	ScalesValue _slave;	
	String _dataValue;
public:
	ScalesClass() {};
	~ScalesClass() {};
	
	void master(ScalesValue value){	_master = value; }
	void slave(ScalesValue value) {	_slave = value;	};
	size_t doData(JsonObject& json);
	size_t doDataRandom(JsonObject& json);
	String& dataValue() {return _dataValue;}; 
};

extern ScalesClass scales;