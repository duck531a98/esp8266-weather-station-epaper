#ifndef _EPD_DRIVE_GPIO_H_
#define _EPD_DRIVE_GPIO_H_
#include <SPI.h>
#pragma once
/********************------------------------------------------------------------
extern byte HARDWARE_SPI=0;
Hardware interface

------------------------------------------------------------------------------*/
//GPIO config

#define CS 15
#define EPD_CS_0	digitalWrite(CS,LOW)
#define EPD_CS_1	digitalWrite(CS,HIGH)
#define isEPD_CS  digitalRead(CS)

#define RST 2
#define EPD_RST_0	digitalWrite(RST,LOW)
#define EPD_RST_1	digitalWrite(RST,HIGH)
#define isEPD_RST digitalRead(RST)

#define DC 0
#define EPD_DC_0	digitalWrite (DC,LOW)
#define EPD_DC_1	digitalWrite (DC,HIGH)

#define BUSY 4
#define READ_EPD_BUSY digitalRead(BUSY)
#define EPD_BUSY_LEVEL 0

#define CLK 14
#define EPD_CLK_0  GPOC = (1 << CLK);
#define EPD_CLK_1  GPOS = (1 << CLK);

#define DIN 13
#define EPD_DIN_0  GPOC = (1 << DIN);
#define EPD_DIN_1  GPOS = (1 << DIN);

extern void SPI_Write(unsigned char value);
extern void SPI_WriteBuffer(const unsigned char *data, size_t length);
extern void SPI_WriteBufferInverted(const unsigned char *data, size_t length);
extern void SPI_WriteRepeat(unsigned char value, size_t length);
extern void driver_delay_xms(unsigned long xms);
extern byte HARDWARE_SPI;
#endif
