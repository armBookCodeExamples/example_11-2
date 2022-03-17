//=====[#include guards - begin]===============================================

#ifndef _WIFI_COM_
#define _WIFI_COM_

//=====[Declaration of public defines]=========================================

#define AP_SSID_MAX_LENGTH     (32 + 1)
#define AP_PASSWORD_MAX_LENGTH (63 + 1)

//=====[Declaration of public data types]======================================

//=====[Declarations (prototypes) of public functions]=========================

void wifiComSetWiFiComApSsid( char * ApSsid );
void wifiComSetWiFiComApPassword( char * ApPassword );
char * wifiComGetIpAddress();

void wifiComRestart();
void wifiComInit();
void wifiComUpdate();

//=====[#include guards - end]=================================================

#endif /* _WIFI_COM_ */