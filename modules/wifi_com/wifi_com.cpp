//=====[Libraries]=============================================================

#include "arm_book_lib.h"

#include "wifi_com.h"

#include "non_blocking_delay.h"
#include "pc_serial_com.h"

//=====[Declaration of private defines]========================================

#define DELAY_10_SECONDS        10000
#define DELAY_5_SECONDS         5000

#define IP_MAX_LENGTH (15 + 1)

//=====[Declaration of private data types]=====================================

typedef enum {
   WIFI_STATE_INIT,
   WIFI_STATE_SEND_AT,
   WIFI_STATE_WAIT_AT,
   WIFI_STATE_SEND_CWMODE,
   WIFI_STATE_WAIT_CWMODE,
   WIFI_STATE_SEND_CWJAP_IS_SET,
   WIFI_STATE_WAIT_CWJAP_IS_SET,
   WIFI_STATE_SEND_CWJAP_SET,
   WIFI_STATE_WAIT_CWJAP_SET_1,
   WIFI_STATE_WAIT_CWJAP_SET_2,
   WIFI_STATE_SEND_CIFSR,
   WIFI_STATE_WAIT_CIFSR,
   WIFI_STATE_LOAD_IP,
   WIFI_STATE_IDLE,
   WIFI_STATE_ERROR
} wifiComState_t;

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartWifi( PE_8, PE_7, 115200 );

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

static const char responseOk[] = "OK";
static const char responseCwjapOk[] = "+CWJAP:";
static const char responseCwjap1[] = "WIFI CONNECTED";
static const char responseCwjap2[] = "WIFI GOT IP";
static const char responseCifsr[] = "+CIFSR:STAIP,\"";

static char wifiComApSsid[AP_SSID_MAX_LENGTH] = "";
static char wifiComApPassword[AP_PASSWORD_MAX_LENGTH] = "";
static char wifiComIpAddress[IP_MAX_LENGTH];

static const char* wifiComExpectedResponse;
static wifiComState_t wifiComState;

static nonBlockingDelay_t wifiComDelay;

//=====[Declarations (prototypes) of private functions]========================

static bool isExpectedResponse();
bool wifiComCharRead( char* receivedChar );
void wifiComStringWrite( const char* str );

//=====[Implementations of public functions]===================================

void wifiComSetWiFiComApSsid( char * ApSsid )
{
    strncpy(wifiComApSsid, ApSsid, AP_SSID_MAX_LENGTH);
}

void wifiComSetWiFiComApPassword( char * ApPassword )
{
    strncpy(wifiComApPassword, ApPassword, AP_PASSWORD_MAX_LENGTH );
}

char * wifiComGetIpAddress()
{
   return wifiComIpAddress;
}

void wifiComRestart()
{
    wifiComState = WIFI_STATE_INIT;
}

void wifiComInit()
{
    wifiComState = WIFI_STATE_INIT;
}

void wifiComUpdate()
{
   static char receivedCharWifiCom;
   static int IpStringPositionIndex;

   switch (wifiComState) {

      case WIFI_STATE_INIT:
         nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
         wifiComState = WIFI_STATE_SEND_AT;
      break;

      case WIFI_STATE_SEND_AT:
         if (nonBlockingDelayRead(&wifiComDelay)) {
            wifiComStringWrite( "AT\r\n" );
            wifiComExpectedResponse = responseOk;
            nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
            wifiComState = WIFI_STATE_WAIT_AT;
         }
      break;

      case WIFI_STATE_WAIT_AT:
         if (isExpectedResponse()) {
            nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
            wifiComState = WIFI_STATE_SEND_CWMODE;
         }
         if (nonBlockingDelayRead(&wifiComDelay)) {
            pcSerialComStringWrite("AT command not responded ");
            pcSerialComStringWrite("correctly\r\n");
            wifiComState = WIFI_STATE_ERROR;
         }
      break;

      case WIFI_STATE_SEND_CWMODE:
         if (nonBlockingDelayRead(&wifiComDelay)) {
            wifiComStringWrite( "AT+CWMODE=1\r\n" );
            wifiComExpectedResponse = responseOk;
            nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
            wifiComState = WIFI_STATE_WAIT_CWMODE;
         }
      break;

      case WIFI_STATE_WAIT_CWMODE:
         if (isExpectedResponse()) {
            nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
            wifiComState = WIFI_STATE_SEND_CWJAP_IS_SET;
         }
         if (nonBlockingDelayRead(&wifiComDelay)) {
            pcSerialComStringWrite("AT+CWMODE=1 command not ");
            pcSerialComStringWrite("responded correctly\r\n");
            wifiComState = WIFI_STATE_ERROR;
         }
      break;

      case WIFI_STATE_SEND_CWJAP_IS_SET:
         if (nonBlockingDelayRead(&wifiComDelay)) {
            wifiComStringWrite( "AT+CWJAP?\r\n" );
            wifiComExpectedResponse = responseCwjapOk;
            nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
            wifiComState = WIFI_STATE_WAIT_CWJAP_IS_SET;
         }
      break;

      case WIFI_STATE_WAIT_CWJAP_IS_SET:
         if (isExpectedResponse()) {
            wifiComExpectedResponse = responseOk;
            wifiComState = WIFI_STATE_SEND_CIFSR;
         }
         if (nonBlockingDelayRead(&wifiComDelay)) {
            nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
            wifiComState = WIFI_STATE_SEND_CWJAP_SET;
         }
      break;

      case WIFI_STATE_SEND_CWJAP_SET:
         if (nonBlockingDelayRead(&wifiComDelay)) {
            wifiComStringWrite( "AT+CWJAP=\"" );
            wifiComStringWrite( wifiComApSsid );
            wifiComStringWrite( "\",\"" );
            wifiComStringWrite( wifiComApPassword );
            wifiComStringWrite( "\"" );
            wifiComStringWrite( "\r\n" );
            wifiComExpectedResponse = responseCwjap1;
            nonBlockingDelayWrite(&wifiComDelay, DELAY_10_SECONDS);
            wifiComState = WIFI_STATE_WAIT_CWJAP_SET_1;
         }
      break;

      case WIFI_STATE_WAIT_CWJAP_SET_1:
         if (isExpectedResponse()) {
            wifiComExpectedResponse = responseCwjap2;
            wifiComState = WIFI_STATE_WAIT_CWJAP_SET_2;
         }
         if (nonBlockingDelayRead(&wifiComDelay)) {
            pcSerialComStringWrite("Error in state: ");
            pcSerialComStringWrite("WIFI_STATE_WAIT_CWJAP_SET_1\r\n");
            pcSerialComStringWrite("Check Wi-Fi AP credentials ");
            pcSerialComStringWrite("and restart\r\n");
            wifiComState = WIFI_STATE_ERROR;
         }
         break;

      case WIFI_STATE_WAIT_CWJAP_SET_2:
         if (isExpectedResponse()) {
            wifiComState = WIFI_STATE_SEND_CIFSR;
         }
         if (nonBlockingDelayRead(&wifiComDelay)) {
            pcSerialComStringWrite("Error in state: ");
            pcSerialComStringWrite("WIFI_STATE_WAIT_CWJAP_SET_2\r\n");
            pcSerialComStringWrite("Check Wi-Fi AP credentials ");
            pcSerialComStringWrite("and restart\r\n");
            wifiComState = WIFI_STATE_ERROR;
         }
      break;

      case WIFI_STATE_SEND_CIFSR:
         if (nonBlockingDelayRead(&wifiComDelay)) {
            wifiComStringWrite( "AT+CIFSR\r\n" );
            wifiComExpectedResponse = responseCifsr;
            nonBlockingDelayWrite(&wifiComDelay, DELAY_5_SECONDS);
            wifiComState = WIFI_STATE_WAIT_CIFSR;
         }
      break;

      case WIFI_STATE_WAIT_CIFSR:
         if (isExpectedResponse()) {
            wifiComState = WIFI_STATE_LOAD_IP;
            IpStringPositionIndex = 0;
         }
         if (nonBlockingDelayRead(&wifiComDelay)) {
            pcSerialComStringWrite("AT+CIFSR command not responded ");
            pcSerialComStringWrite("correctly\r\n");
            wifiComState = WIFI_STATE_ERROR;
         }
      break;

      case WIFI_STATE_LOAD_IP:
         if (wifiComCharRead(&receivedCharWifiCom)) {
            if ( (receivedCharWifiCom != '"') && 
               (IpStringPositionIndex < IP_MAX_LENGTH) ) {
               wifiComIpAddress[IpStringPositionIndex] = receivedCharWifiCom;
               IpStringPositionIndex++;
            } else {
               wifiComIpAddress[IpStringPositionIndex] = '\0';
               pcSerialComStringWrite("IP address assigned correctly\r\n\r\n");
               wifiComState = WIFI_STATE_IDLE;
            }
         }
      break;

      case WIFI_STATE_IDLE:
      case WIFI_STATE_ERROR:
      break;
   }
}

//=====[Implementations of private functions]==================================

bool wifiComCharRead( char* receivedChar )
{
    char receivedCharLocal = '\0';
    if( uartWifi.readable() ) {
        uartWifi.read(&receivedCharLocal,1);
        *receivedChar = receivedCharLocal;
        return true;
    }
    return false;
}

void wifiComStringWrite( const char* str )
{
    uartWifi.write( str, strlen(str) );
}

static bool isExpectedResponse()
{
   static int responseStringPositionIndex = 0;
   char charReceived;
   bool moduleResponse = false;

   if( wifiComCharRead(&charReceived) ){
      if (charReceived == wifiComExpectedResponse[responseStringPositionIndex]) {
         responseStringPositionIndex++;
         if (wifiComExpectedResponse[responseStringPositionIndex] == '\0') {
            responseStringPositionIndex = 0;
            moduleResponse = true;
         }
      } else {
         responseStringPositionIndex = 0;
      }
   }
   return moduleResponse;
}