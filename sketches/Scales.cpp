#include "Scales.h"
#include "BrowserServer.h"

ScalesClass scales;

size_t ScalesClass::doData(JsonObject& json) {	
	_dataValue = String();
	json.printTo(_dataValue);
	return json.measureLength();
};

size_t ScalesClass::doDataRandom(JsonObject& json) {
	long sum = random(5000, 5050);	
	json["w"] = sum;
	json["t"] = millis();
	json["m"] = ESP.getFreeHeap();
	json["c"] = webSocket.count();	
	return json.measureLength();
}
