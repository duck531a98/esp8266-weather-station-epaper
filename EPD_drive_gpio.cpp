#include "EPD_drive_gpio.h"
#include <SPI.h>

/*********************************************

*********************************************/	
byte HARDWARE_SPI=0;

void SPI_Write(unsigned char value)
{     		
	//
  if(HARDWARE_SPI==0)
  {
  EPD_CLK_0;
  //delayMicroseconds(1);
  for(byte i=0;i<8;i++)
    {
     //高位在前发送方式    根据升级器件特性定
    if((value&0x80)==0x80) EPD_DIN_1
    else EPD_DIN_0;
    //delayMicroseconds(1);      //等待数据稳定  根据实际时钟调整
    EPD_CLK_1;//上升沿发送数据
    //delayMicroseconds(1);//CLK高电平保持一段时间 这个可以不需要 根据具体的spi时钟来确定
    EPD_CLK_0;; //把时钟拉低实现为下一次上升沿发送数据做准备
    value = value<<1;//发送数据的位向前移动一位
    }
  }
  else
  {
    SPI.transfer(value);
    }
}

void SPI_WriteBuffer(const unsigned char *data, size_t length)
{
  if(data==nullptr||length==0) return;
  if(HARDWARE_SPI==0)
  {
    for(size_t i=0;i<length;i++) SPI_Write(data[i]);
    return;
  }

  // EPD command payloads commonly start at buffer+1. ESP8266 writeBytes()
  // requires 32-bit alignment and raises Exception (9) otherwise, while
  // transferBytes() detects unaligned pointers and stages them safely.
  SPI.transferBytes(data,nullptr,length);
}

void SPI_WriteBufferInverted(const unsigned char *data, size_t length)
{
  if(data==nullptr||length==0) return;
  if(HARDWARE_SPI==0)
  {
    for(size_t i=0;i<length;i++) SPI_Write((unsigned char)~data[i]);
    return;
  }

  unsigned char inverted[64];
  while(length>0)
  {
    size_t chunk=length<sizeof(inverted)?length:sizeof(inverted);
    for(size_t i=0;i<chunk;i++) inverted[i]=(unsigned char)~data[i];
    SPI.transferBytes(inverted,nullptr,chunk);
    data+=chunk;
    length-=chunk;
  }
}

void SPI_WriteRepeat(unsigned char value, size_t length)
{
  if(length==0) return;
  if(HARDWARE_SPI==0)
  {
    for(size_t i=0;i<length;i++) SPI_Write(value);
    return;
  }
  SPI.writePattern(&value,1,length);
}

/*********************************************

*********************************************/	
void driver_delay_xms(unsigned long xms)	
{	
	delay(xms);
}
