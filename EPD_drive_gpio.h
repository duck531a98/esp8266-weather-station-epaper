#ifndef _EPD_DRIVE_GPIO_H_
#define _EPD_DRIVE_GPIO_H_
#include <SPI.h>

/********************------------------------------------------------------------

Hardware interface

------------------------------------------------------------------------------*/
//GPIO config

#define CS D8
#define EPD_CS_0	digitalWrite(CS,LOW)
#define EPD_CS_1	digitalWrite(CS,HIGH)
#define isEPD_CS  digitalRead(CS)

#define RST D4
#define EPD_RST_0	digitalWrite(RST,LOW)
#define EPD_RST_1	digitalWrite(RST,HIGH)
#define isEPD_RST digitalRead(RST)

#define DC D1
#define EPD_DC_0	digitalWrite (DC,LOW)
#define EPD_DC_1	digitalWrite (DC,HIGH)

#define BUSY D2
#define isEPD_BUSY digitalRead(BUSY)
#define EPD_BUSY_LEVEL 0

extern void SPI_Write(unsigned char value);
extern void driver_delay_xms(unsigned long xms);
#endif
