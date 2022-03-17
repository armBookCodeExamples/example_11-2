//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "event_log.h"

#include "siren.h"
#include "fire_alarm.h"
#include "user_interface.h"
#include "date_and_time.h"
#include "pc_serial_com.h"
#include "motion_sensor.h"
#include "system_event.h"
#include "sd_card.h"
#include "ble_com.h"

//=====[Declaration of private defines]========================================

#define EVENT_LOG_NAME_SHORT_MAX_LENGTH    22

//=====[Declaration of private data types]=====================================

typedef struct storedEvent {
    time_t seconds;
    char typeOfEvent[EVENT_LOG_NAME_MAX_LENGTH];
    bool storedInSd;
} storedEvent_t;

//=====[Declaration and initialization of public global objects]===============

systemEvent alarmEvent("ALARM");
systemEvent gasEvent("GAS_DET");
systemEvent overTempEvent("OVER_TEMP");
systemEvent ledICEvent("LED_IC");
systemEvent ledSBEvent("LED_SB");
systemEvent motionEvent("MOTION");

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

static int eventsIndex     = 0;
static storedEvent_t arrayOfStoredEvents[EVENT_LOG_MAX_STORAGE];
static bool eventAndStateStrSent;

//=====[Declarations (prototypes) of private functions]========================

static void eventLabelReduce(char * eventLogReportStr, systemEvent * event);

//=====[Implementations of public functions]===================================

void eventLogUpdate()
{
    eventAndStateStrSent = false;
    
    if ( !eventAndStateStrSent ) alarmEvent.stateUpdate( sirenStateRead() );
    if ( !eventAndStateStrSent ) gasEvent.stateUpdate(  gasDetectorStateRead() );
    if ( !eventAndStateStrSent ) 
        overTempEvent.stateUpdate(  overTemperatureDetectorStateRead() );
    if ( !eventAndStateStrSent ) 
        ledICEvent.stateUpdate(  incorrectCodeStateRead() );
    if ( !eventAndStateStrSent ) 
        ledSBEvent.stateUpdate( systemBlockedStateRead() );
    if ( !eventAndStateStrSent ) motionEvent.stateUpdate( motionSensorRead() );
    
}

int eventLogNumberOfStoredEvents()
{
    return eventsIndex;
}

void eventLogRead( int index, char* str )
{
    str[0] = '\0';
    strcat( str, "Event = " );
    strcat( str, arrayOfStoredEvents[index].typeOfEvent );
    strcat( str, "\r\nDate and Time = " );
    strcat( str, ctime(&arrayOfStoredEvents[index].seconds) );
    strcat( str, "\r\n" );
}

void eventLogWrite( bool currentState, const char* elementName )
{
    char eventAndStateStr[EVENT_LOG_NAME_MAX_LENGTH] = "";

    strcat( eventAndStateStr, elementName );
    if ( currentState ) {
        strcat( eventAndStateStr, "_ON" );
    } else {
        strcat( eventAndStateStr, "_OFF" );
    }

    arrayOfStoredEvents[eventsIndex].seconds = time(NULL);
    strcpy( arrayOfStoredEvents[eventsIndex].typeOfEvent, eventAndStateStr );
    arrayOfStoredEvents[eventsIndex].storedInSd = false;
    if ( eventsIndex < EVENT_LOG_MAX_STORAGE - 1 ) {
        eventsIndex++;
    } else {
        eventsIndex = 0;
    }

    pcSerialComStringWrite(eventAndStateStr);
    pcSerialComStringWrite("\r\n");
    
    bleComStringWrite(eventAndStateStr);
    bleComStringWrite("\r\n");
    
    eventAndStateStrSent = true;
}

bool eventLogSaveToSdCard()
{
    char fileName[SD_CARD_FILENAME_MAX_LENGTH];
    char eventStr[EVENT_STR_LENGTH] = "";
    bool eventsStored = false;

    time_t seconds;
    int i;

    seconds = time(NULL);
    fileName[0] = '\0';

    strftime( fileName, SD_CARD_FILENAME_MAX_LENGTH, 
              "%Y_%m_%d_%H_%M_%S", localtime(&seconds) );
    strcat( fileName, ".txt" );

    for (i = 0; i < eventLogNumberOfStoredEvents(); i++) {
        if ( !arrayOfStoredEvents[i].storedInSd ) {
            eventLogRead( i, eventStr );
            if ( sdCardWriteFile( fileName, eventStr ) ){
                arrayOfStoredEvents[i].storedInSd = true;
                pcSerialComStringWrite("Storing event ");
                pcSerialComIntWrite(i+1);
                pcSerialComStringWrite(" in file ");
                pcSerialComStringWrite(fileName);
                pcSerialComStringWrite("\r\n");
                eventsStored = true;
            }
        }
    }

    if ( eventsStored ) {
        pcSerialComStringWrite("New events successfully stored in the SD card\r\n\r\n");
    } else {
        pcSerialComStringWrite("No new events to store in the SD card\r\n\r\n");
    }

    return true;
}

void eventLogReport()
{
    char eventLogReportStr[EVENT_LOG_NAME_SHORT_MAX_LENGTH] = "";
    
    eventLabelReduce( eventLogReportStr, &alarmEvent );
    strcat( eventLogReportStr, "," );

    eventLabelReduce( eventLogReportStr, &gasEvent );
    strcat( eventLogReportStr, "," );

    eventLabelReduce( eventLogReportStr, &overTempEvent );
    strcat( eventLogReportStr, "," );

    eventLabelReduce( eventLogReportStr, &ledICEvent );
    strcat( eventLogReportStr, "," );

    eventLabelReduce( eventLogReportStr, &ledSBEvent );
    strcat( eventLogReportStr, "," );

    eventLabelReduce( eventLogReportStr, &motionEvent );
    
    bleComStringWrite(eventLogReportStr);
    bleComStringWrite("\r\n");
}

//=====[Implementations of private functions]==================================

static void eventLabelReduce(char * eventLogReportStr, systemEvent * event)
{ 
    if (strcmp(event->getLabel(), "ALARM") == 0) {
        strcat(eventLogReportStr,"A");
    } else if (strcmp(event->getLabel(), "GAS_DET") == 0) {
        strcat(eventLogReportStr,"G");
    } else if(strcmp(event->getLabel(), "OVER_TEMP") == 0) {
        strcat(eventLogReportStr,"T");
    } else if(strcmp(event->getLabel(), "LED_IC") == 0) {
        strcat(eventLogReportStr,"I");
    } else if(strcmp(event->getLabel(), "LED_SB") == 0) {
        strcat(eventLogReportStr,"S");
    } else if(strcmp(event->getLabel(), "MOTION") == 0) {
        strcat(eventLogReportStr,"M");
    }
    
    if ( event->lastStateRead() ) {
        strcat( eventLogReportStr, "N" );
    } else {
        strcat( eventLogReportStr, "F" );
    }
}
