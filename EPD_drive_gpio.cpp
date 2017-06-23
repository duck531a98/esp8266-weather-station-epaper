#include "EPD_drive_gpio.h"
#include <SPI.h>

/*********************************************

*********************************************/	
 void SPI_Write(unsigned char value)                                    
{     		
	SPI.transfer(value);
}

/*********************************************

*********************************************/	
void driver_delay_xms(unsigned long xms)	
{	
	delay(xms);
}


