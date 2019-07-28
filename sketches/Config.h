#pragma once
#include "Arduino.h"

#define WEB_TERMINAL2			1
#define WEB_TERMINAL_MINI		2
#define WEB_CRANE				3
#define SCALE_SERVER			4

#define MASTER_USER				"sa"
#define MASTER_PASS				"343434"

//#define BOARD WEB_TERMINAL2
#define BOARD WEB_TERMINAL_MINI
//#define BOARD WEB_CRANE
//#define BOARD SCALE_SERVER

//#define POWER_DEBUG
//#define POWER_PLAN				EXTERNAL_POWER
#define HTML_PROGMEM          //Использовать веб страницы из flash памяти
#define MULTI_POINTS_CONNECT	/* Использовать для использования с несколькими точками доступа */

#ifdef HTML_PROGMEM
	#include "Page.h"
#endif

#if BOARD == WEB_TERMINAL2
	#include "web_terminal2.h"
#elif BOARD == WEB_TERMINAL_MINI
	#include "web_terminal_mini.h"
#elif BOARD == WEB_CRANE
	#include "web_crane.h"
#elif SCALE_SERVER
	#include "scale_server.h"
#endif // BOARD == WEB_TERMINAL2


struct MyEEPROMStruct {
#ifndef  MULTI_POINTS_CONNECT
	bool dnip;	
	char lanIp[16];
	char gate[16];
	char mask[16];
	char wSSID[33];
	char wKey[33];
#endif // MULTI_POINTS_CONNECT
	char hostName[16];	
	char hostUrl[0xff];
	int hostPin;
	char user[16];
	char password[16];
	unsigned int timeScan;		//Время период сканирования в секундах
	unsigned char deltaRSSI;	//Дельта мощности сигнала при проверке в процентах
};

/*
 {"cmd":"wt","w":0.212,"a":3,"s":true,"c":56}
 {"cmd":"snet","ssid":"KONST","key":"3fal-rshc-nuo3","dnip":"true","lan_ip":"192.168.1.100","gateway":"192.168.1.1","subnet":"255.255.255.0"}
 {"cmd":"swt","d":"","v":0.222,"a":3}
 {"cmd":"swt","d":"","v":0.22,"a":3}
 */
