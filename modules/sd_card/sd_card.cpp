//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "sd_card.h"

#include "event_log.h"
#include "date_and_time.h"
#include "pc_serial_com.h"

#include "FATFileSystem.h"
#include "SDBlockDevice.h"

#include "platform/mbed_retarget.h"

//=====[Declaration of private defines]========================================

#define SPI3_MOSI   PC_12
#define SPI3_MISO   PC_11
#define SPI3_SCK    PC_10
#define SPI3_CS     PA_4_ALT0

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

SDBlockDevice sd( SPI3_MOSI, SPI3_MISO, SPI3_SCK, SPI3_CS );

FATFileSystem sdCardFileSystem("sd", &sd);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

//=====[Declarations (prototypes) of private functions]========================

//=====[Implementations of public functions]===================================

bool sdCardInit()
{
    pcSerialComStringWrite("Looking for a filesystem in the SD card... \r\n");
    sdCardFileSystem.mount(&sd);
    DIR *sdCardListOfDirectories = opendir("/sd/");
    if ( sdCardListOfDirectories != NULL ) {
        pcSerialComStringWrite("Filesystem found in the SD card. \r\n");
        closedir(sdCardListOfDirectories);
        return true;
    } else {
        pcSerialComStringWrite("Filesystem not mounted. \r\n");
        pcSerialComStringWrite("Insert an SD card and ");
        pcSerialComStringWrite("reset the NUCLEO board.\r\n");
        return false;
    }
}

bool sdCardWriteFile( const char* fileName, const char* writeBuffer )
{
    char fileNameSD[SD_CARD_FILENAME_MAX_LENGTH+4] = "";
    
    fileNameSD[0] = '\0';
    strcat( fileNameSD, "/sd/" );
    strcat( fileNameSD, fileName );

    FILE *sdCardFilePointer = fopen( fileNameSD, "a" );

    if ( sdCardFilePointer != NULL ) {
        fprintf( sdCardFilePointer, "%s", writeBuffer );                       
        fclose( sdCardFilePointer );
        return true;
    } else {
        return false;
    }
}

bool sdCardReadFile( const char* fileName, char * readBuffer, int readBufferSize )
{
    char fileNameSD[SD_CARD_FILENAME_MAX_LENGTH+4] = "";
    int i;
    
    fileNameSD[0] = '\0';
    strcat( fileNameSD, "/sd/" );
    strcat( fileNameSD, fileName );
    
    FILE *sdCardFilePointer = fopen( fileNameSD, "r" );
    
    if ( sdCardFilePointer != NULL ) {
        pcSerialComStringWrite( "Opening file: " );
        pcSerialComStringWrite( fileNameSD );
        pcSerialComStringWrite( "\r\n" );

        i = 0;
        while ( ( !feof(sdCardFilePointer) ) && ( i < readBufferSize - 1 ) ) {
           fread( &readBuffer[i], 1, 1, sdCardFilePointer );
           i++;
        }
        readBuffer[i-1] = '\0';
        fclose( sdCardFilePointer );
        return true;
    } else {
        pcSerialComStringWrite( "File not found\r\n" );
        return false;
    }
}

bool sdCardListFiles( char* fileNamesBuffer, int fileNamesBufferSize )
{
    int NumberOfUsedBytesInBuffer = 0;
    struct dirent *sdCardDirectoryEntryPointer;

    DIR *sdCardListOfDirectories = opendir("/sd/");

    if ( sdCardListOfDirectories != NULL ) {
        pcSerialComStringWrite("Printing all filenames:\r\n");
        sdCardDirectoryEntryPointer = readdir(sdCardListOfDirectories);
        
        while ( ( sdCardDirectoryEntryPointer != NULL ) && 
                ( NumberOfUsedBytesInBuffer + strlen(sdCardDirectoryEntryPointer->d_name) < 
                    fileNamesBufferSize) ) {
            strcat( fileNamesBuffer, sdCardDirectoryEntryPointer->d_name );
            strcat( fileNamesBuffer, "\r\n" );
            NumberOfUsedBytesInBuffer = NumberOfUsedBytesInBuffer +
                                        strlen(sdCardDirectoryEntryPointer->d_name);
            sdCardDirectoryEntryPointer = readdir(sdCardListOfDirectories);
        }
        
        closedir(sdCardListOfDirectories);
        
        return true;
    } else {
        pcSerialComStringWrite("Insert an SD card and ");
        pcSerialComStringWrite("reset the NUCLEO board.\r\n");
        return false;
    }
}
