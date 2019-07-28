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
	String _dataValue;
public:
	ScalesClass() {};
	~ScalesClass() {};	
	
	size_t doData(JsonObject& json);
	size_t doDataRandom(JsonObject& json);
	String& dataValue() {return _dataValue;}; 
};

extern ScalesClass scales;