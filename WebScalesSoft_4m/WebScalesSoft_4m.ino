#include "Board.h"
#include "BrowserServer.h"
//Comment out the definition below if you don't want to use the ESP8266 gdb stub.
//#define ESP8266_USE_GDB_STUB

#ifdef ESP8266_USE_GDB_STUB
	#include <GDBStub.h>
	extern "C" int gdbstub_init();
	extern "C" int gdbstub_do_break();
#endif

void setup(){    
	#ifdef ESP8266_USE_GDB_STUB
		Serial.begin(921600);
		gdbstub_init();
		gdbstub_do_break();
	#else
		Serial.begin(921600);
		//Serial.setTimeout(100);
	#endif
		Board = new BoardClass();
		Board->init();
		server->begin();
}

void loop(){
	Board->handle();
#ifdef MULTI_POINTS_CONNECT
	Board->wifi()->connect();
	delay(1);
#endif // MULTI_POINTS_CONNECT					
	if (Serial.available())	{
		String str = Board->readSerial();
		DynamicJsonBuffer jsonBuffer(str.length());
		JsonObject &root = jsonBuffer.parseObject(str);
		if (!root.success() || !root.containsKey("cmd")){
			Serial.flush();
			return;
		}				
		Board->parceCmd(root);		
	}else{
		Serial.flush();
	}
	delay(1);	
}
