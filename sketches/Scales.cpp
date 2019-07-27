#include "Scales.h"
#include "BrowserServer.h"

ScalesClass scales;

size_t ScalesClass::doData(JsonObject& json) {
	float m = json["ms"]["w"] ;
	float s = json["sl"]["w"];	
	json["w"] = json.containsKey("sl") ? String(m+s) : String("slave???");
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
