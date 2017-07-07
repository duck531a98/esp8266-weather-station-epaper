#include "FS.h"
#include "EPD_drive.h"
#include "EPD_drive_gpio.h"


unsigned char UNICODEbuffer[100];
String fontname;
/***********************************************************************************************************************
			------------------------------------------------------------------------
			|\\\																///|
			|\\\						Drive layer					///|
			------------------------------------------------------------------------
***********************************************************************************************************************/
/*******************************************************************************
Constructor
*******************************************************************************/
 WaveShare_EPD::WaveShare_EPD(){
 
}
/*******************************************************************************
function：
			read busy
*******************************************************************************/
void WaveShare_EPD::SetFont(byte fontindex)
{
     FontIndex=fontindex;
     switch (fontindex)
     {
     case 0:
     fontname="/font16";break;
     case 1:
     fontname="/font32";break;
     case 2:
     fontname="/font10";break;
     case 11:
     fontname="/weathericon";break;
     case 12:
     fontname="/weathericon80";break;
      case 13:
     fontname="/weathericon32";break;
     }
  }
 void WaveShare_EPD::DrawXline(int start,int end, byte x)
  {
    for(int i=start;i<=end;i++)
    {
      SetPixel(x,i);
      }
  }
   void WaveShare_EPD::DrawYline(byte start,byte end,  byte y)
  {
    for(int i=start;i<=end;i++)
    {
      SetPixel(i,y);
      }
  }
void WaveShare_EPD::DrawUTF(byte x,int16_t y,byte width,byte height,String code)
{
  char buffer[100];
  code.toCharArray(buffer,100);
  DrawUTF(x,y,width,height,(unsigned char *)buffer);
  }
void WaveShare_EPD::DrawUTF(byte x,int16_t y,byte width,byte height,unsigned char *code)
{
  int charcount;
  charcount=UTFtoUNICODE((unsigned char*)code);
  DrawUnicodeStr(x,y,width,height,charcount,(unsigned char *)UNICODEbuffer);
  
  }
int WaveShare_EPD::UTFtoUNICODE(unsigned char *code)
{ 
  int i=0;int charcount=0;
  while(code[i]!='\0')
  { 
    Serial.println("current codei");
      Serial.println(code[i],HEX);
        Serial.println(code[i]&0xf0,HEX);
    if(code[i]<=0x7f)  //ascii
    {
      
      UNICODEbuffer[charcount*2]=0x00;
      UNICODEbuffer[charcount*2+1]=code[i];
         Serial.println("english or number");
       Serial.println(UNICODEbuffer[charcount*2],HEX);
         Serial.println(UNICODEbuffer[charcount*2+1],HEX);
      i++;charcount++;
   
      }
    else if((code[i]&0xe0)==0xc0)
    {
    
      UNICODEbuffer[charcount*2+1]=(code[i]<<6)+(code[i+1]&0x3f);
      UNICODEbuffer[charcount*2]=(code[i]>>2)&0x07;
      i+=2;charcount++;
      Serial.println("two bits utf-8");
      }
     else if((code[i]&0xf0)==0xe0)
    {
      
      UNICODEbuffer[charcount*2+1]=(code[i+1]<<6)+(code[i+2]&0x7f);
      UNICODEbuffer[charcount*2]=(code[i]<<4)+((code[i+1]>>2)&0x0f);
      
       Serial.println("three bits utf-8");
        Serial.println(UNICODEbuffer[charcount*2],HEX);
         Serial.println(UNICODEbuffer[charcount*2+1],HEX);i+=3;charcount++;
      }
     else
     {
      return 0;}
  }
  UNICODEbuffer[charcount*2]='\0';
  return charcount;
  }
void WaveShare_EPD::DrawUnicodeChar(byte x,int16_t y,byte width,byte height,unsigned char *code)
 { 
  SPIFFS.begin();
  int offset;
  int sizeofsinglechar;
  if (height%8==0) sizeofsinglechar=(height/8)*width;
  else sizeofsinglechar=(height/8+1)*width;
 offset=(code[0]*0x100+code[1])*sizeofsinglechar;
   Serial.println("code[1]");
   Serial.println(code[1]);
    Serial.println("sizeofsinglechar");
   Serial.println(sizeofsinglechar);
  File f=SPIFFS.open(fontname,"r");
  f.seek(offset,SeekSet);
  char zi[sizeofsinglechar];
  f.readBytes(zi,sizeofsinglechar);
 /*for(int i=0;i<32;i++)
  {
     
   Serial.println(zi[i],HEX);
    }*/
   Serial.println("offset");
   Serial.println(offset);
   if (offset<0xff*sizeofsinglechar && FontIndex<10) 
   {drawXbm(x,y,width,height,(unsigned char *)zi); }
   else  {drawXbm(x,y,width,height,(unsigned char *)zi);}
  
SPIFFS.end();
}

void WaveShare_EPD::DrawUnicodeStr(byte x,int16_t y,byte width,byte height,byte strlength,unsigned char *code)
{
  CurrentCursor=0;
   byte sizeofsinglechar;
  if (height%8==0) sizeofsinglechar=(height/8)*width;
  else sizeofsinglechar=(height/8+1)*width;
  int ymove=0;
  strlength*=2;
  int i=0;
  while(i<strlength)
  {
      int offset;
      offset=(code[i]*0x100+code[i+1])*sizeofsinglechar;
      if (offset<0xff*sizeofsinglechar) 
      {
      DrawUnicodeChar(x,y+ymove,width,height,(unsigned char *)code+i);
      ymove+=CurrentCursor+1;
    }
    else if(fontscale==2)
    {
      DrawUnicodeChar(x,y+ymove,width,height,(unsigned char *)code+i);
      ymove+=width*2;
      }
    else
    {
      DrawUnicodeChar(x,y+ymove,width,height,(unsigned char *)code+i);
      ymove+=width;
      }
    i++;i++;
    }
  
  }
void WaveShare_EPD::drawXbm(int16_t xMove, int16_t yMove, int16_t width, int16_t height,unsigned char *xbm) {
  int16_t heightInXbm = (height + 7) / 8;
  uint8_t data;
  for(int16_t x = 0; x < width; x++) {
    for(int16_t y = 0; y < height; y++ ) {
      if (y & 7) {
        data <<= 1; // Move a bit
      } else {  // Read new data every 8 bit
        data = xbm[(y / 8) + x * heightInXbm];
      }
      // if there is a bit draw it
      if (((data & 0x80)>>7)) {
       if(fontscale==1) {SetPixel(xMove + y, yMove + x);CurrentCursor=x;}
       if(fontscale==2) {SetPixel(xMove + y*2, yMove + x*2);SetPixel(xMove + y*2+1, yMove + x*2);SetPixel(xMove + y*2, yMove + x*2+1);SetPixel(xMove + y*2+1, yMove + x*2+1);}
      }
    }
  }
}

void WaveShare_EPD::DrawXbm_P(int16_t xMove, int16_t yMove, int16_t width, int16_t height,unsigned char *xbm) {
 int16_t heightInXbm = (height + 7) / 8;
 uint8_t data;
 unsigned char temp[heightInXbm*width];
 memcpy_P(temp, xbm, heightInXbm*width);
  
  for(int16_t x = 0; x < width; x++) {
    for(int16_t y = 0; y < height; y++ ) {
      if (y & 7) {
        data <<= 1; // Move a bit
      } else {  // Read new data every 8 bit
        data = temp[(y / 8) + x * heightInXbm];
      }
      // if there is a bit draw it
      if (((data & 0x80)>>7)) {
       if(fontscale==1) {SetPixel(xMove + y, yMove + x);CurrentCursor=x;}
       if(fontscale==2) {SetPixel(xMove + y*2, yMove + x*2);SetPixel(xMove + y*2+1, yMove + x*2);SetPixel(xMove + y*2, yMove + x*2+1);SetPixel(xMove + y*2+1, yMove + x*2+1);}
      }
    }
  }
}
void WaveShare_EPD::SetPixel(char x, int y)
{ 
 
    EPDbuffer[x/8+y*xDot/8]&=~(0x80>>x%8) ;
 
  }
void WaveShare_EPD::clearbuffer()
{
  for(int i=0;i<(xDot*yDot/8);i++) EPDbuffer[i]=0xff;
  
  }
void WaveShare_EPD::clearshadows()
{
 unsigned char temp[]={0x00};
 EPD_Dis_Part(0,127,0,295,(unsigned char *)temp,0);
temp[0]=0xff;
 EPD_Dis_Part(0,127,0,295,(unsigned char *)temp,0);
  }
unsigned char WaveShare_EPD::ReadBusy(void)
{
  unsigned long i=0;
  for(i=0;i<400;i++){
	//	println("isEPD_BUSY = %d\r\n",isEPD_CS);
      if(isEPD_BUSY==EPD_BUSY_LEVEL) {
				Serial.println("Busy is Low \r\n");
      	return 1;
      }
	  driver_delay_xms(10);
  }
  return 0;
}
/*******************************************************************************
function：
		write command
*******************************************************************************/
void WaveShare_EPD::EPD_WriteCMD(unsigned char command)
{        
  EPD_CS_0;	
	EPD_DC_0;		// command write
	SPI_Write(command);
	EPD_CS_1;
}
/*******************************************************************************
function：
		write command and data
*******************************************************************************/
void WaveShare_EPD::EPD_WriteCMD_p1(unsigned char command,unsigned char para)
{
	ReadBusy();	     
  EPD_CS_0;		
	EPD_DC_0;		// command write
	SPI_Write(command);
	EPD_DC_1;		// command write
	SPI_Write(para);
  EPD_CS_1;	
}
/*******************************************************************************
function：
		Configure the power supply
*******************************************************************************/
void WaveShare_EPD::deepsleep(void)
{
  EPD_WriteCMD_p1(0x10,0x01);
 // EPD_WriteCMD(0x20);
  //EPD_WriteCMD(0xff);
}

void WaveShare_EPD::EPD_POWERON(void)
{
	EPD_WriteCMD_p1(0x22,0xc0);
	EPD_WriteCMD(0x20);
	//EPD_WriteCMD(0xff);
}

/*******************************************************************************
function：
		The first byte is written with the command value
		Remove the command value, 
		the address after a shift, 
		the length of less than one byte	
*******************************************************************************/
void WaveShare_EPD::EPD_Write(unsigned char *value, unsigned char datalen)
{
	unsigned char i = 0;
	unsigned char *ptemp;
	ptemp = value;
	
	EPD_CS_0;
	EPD_DC_0;		// When DC is 0, write command 
	SPI_Write(*ptemp);	//The first byte is written with the command value
	ptemp++;
	EPD_DC_1;		// When DC is 1, write data
	Serial.println("send data  :"); 
	for(i= 0;i<datalen-1;i++){	// sub the data
		SPI_Write(*ptemp);
		ptemp++;
	}
	EPD_CS_1;
}
/*******************************************************************************
Function: Write the display buffer
Parameters: 
		XSize x the direction of the direction of 128 points, adjusted to an 
		integer multiple of 8 times YSize y direction quantity Dispbuff displays 
		the data storage location. The data must be arranged in a correct manner
********************************************************************************/
void WaveShare_EPD::EPD_WriteDispRam(unsigned char XSize,unsigned int YSize,
							unsigned char *Dispbuff)
{
	int i = 0,j = 0;
	if(XSize%8 != 0){
		XSize = XSize+(8-XSize%8);
	}
	XSize = XSize/8;

	ReadBusy();	
	EPD_CS_0;	
	EPD_DC_0;		//command write
	SPI_Write(0x24);
	EPD_DC_1;		//data write
	for(i=0;i<YSize;i++){
		for(j=0;j<XSize;j++){
			SPI_Write(*Dispbuff);
			Dispbuff++;
		}
	}
	EPD_CS_1;
}

/*******************************************************************************
Function: Write the display buffer to write a certain area to the same display.
Parameters: XSize x the direction of the direction of 128 points,adjusted to 
			an integer multiple of 8 times YSize y direction quantity  Dispdata 
			display data.
********************************************************************************/
void WaveShare_EPD::EPD_WriteDispRamMono(unsigned char XSize,unsigned int YSize,
							unsigned char dispdata)
{
	int i = 0,j = 0;
	if(XSize%8 != 0){
		XSize = XSize+(8-XSize%8);
	}
	XSize = XSize/8;
	
	ReadBusy();	    
	EPD_CS_0;
	EPD_DC_0;		// command write
	SPI_Write(0x24);
	EPD_DC_1;		// data write
	for(i=0;i<YSize;i++){
		for(j=0;j<XSize;j++){
		 SPI_Write(dispdata);
		}
	}
	EPD_CS_1;
}

/********************************************************************************
Set RAM X and Y -address Start / End position
********************************************************************************/
void WaveShare_EPD::EPD_SetRamArea(unsigned char Xstart,unsigned char Xend,
						unsigned char Ystart,unsigned char Ystart1,unsigned char Yend,unsigned char Yend1)
{
  unsigned char RamAreaX[3];	// X start and end
	unsigned char RamAreaY[5]; 	// Y start and end
	RamAreaX[0] = 0x44;	// command
	RamAreaX[1] = Xstart;
	RamAreaX[2] = Xend;
	RamAreaY[0] = 0x45;	// command
	RamAreaY[1] = Ystart;
	RamAreaY[2] = Ystart1;
	RamAreaY[3] = Yend;
  RamAreaY[4] = Yend1;
	EPD_Write(RamAreaX, sizeof(RamAreaX));
	EPD_Write(RamAreaY, sizeof(RamAreaY));
}

/********************************************************************************
Set RAM X and Y -address counter
********************************************************************************/
void WaveShare_EPD::EPD_SetRamPointer(unsigned char addrX,unsigned char addrY,unsigned char addrY1)
{
  unsigned char RamPointerX[2];	// default (0,0)
	unsigned char RamPointerY[3];
	//Set RAM X address counter
	RamPointerX[0] = 0x4e;
	RamPointerX[1] = addrX;
	//Set RAM Y address counter
	RamPointerY[0] = 0x4f;
	RamPointerY[1] = addrY;
	RamPointerY[2] = addrY1;
	
	EPD_Write(RamPointerX, sizeof(RamPointerX));
	EPD_Write(RamPointerY, sizeof(RamPointerY));
}

/********************************************************************************
1.Set RAM X and Y -address Start / End position
2.Set RAM X and Y -address counter
********************************************************************************/
void WaveShare_EPD::EPD_part_display(unsigned char RAM_XST,unsigned char RAM_XEND,unsigned char RAM_YST,unsigned char RAM_YST1,unsigned char RAM_YEND,unsigned char RAM_YEND1)
{    
	EPD_SetRamArea(RAM_XST,RAM_XEND,RAM_YST,RAM_YST1,RAM_YEND,RAM_YEND1);  	/*set w h*/
    EPD_SetRamPointer (RAM_XST,RAM_YST,RAM_YST1);		    /*set orginal*/
}

//=========================functions============================
/*******************************************************************************
Initialize the register
********************************************************************************/
void WaveShare_EPD::EPD_Init(void)
{
	//1.reset driver
	EPD_RST_0;		// Module reset
	driver_delay_xms(100);
	EPD_RST_1;
	driver_delay_xms(100);
	
	//2. set register
	Serial.println("***********set register Start**********");
  EPD_Write(GateVol, sizeof(GateVol));
	EPD_Write(GDOControl, sizeof(GDOControl));	// Pannel configuration, Gate selection
	EPD_Write(softstart, sizeof(softstart));	// X decrease, Y decrease
	EPD_Write(VCOMVol, sizeof(VCOMVol));		// VCOM setting
	EPD_Write(DummyLine, sizeof(DummyLine));	// dummy line per gate
	EPD_Write(Gatetime, sizeof(Gatetime));		// Gage time setting
	EPD_Write(RamDataEntryMode, sizeof(RamDataEntryMode));	// X increase, Y decrease
	EPD_SetRamArea(0x00,xDot/8,0x00,0x00,(yDot-1)%256,(yDot-1)/256);	// X-source area,Y-gage area
	EPD_SetRamPointer(0x00,0,0);	// set ram
	Serial.println("***********set register  end**********");
}

/********************************************************************************
Display data updates
********************************************************************************/
void WaveShare_EPD::EPD_Update(void)
{
	EPD_WriteCMD_p1(0x22,0xc7);
	EPD_WriteCMD(0x20);
	EPD_WriteCMD(0xff);
}
void WaveShare_EPD::EPD_Update_Part(void)
{
	EPD_WriteCMD_p1(0x22,0x04);
	EPD_WriteCMD(0x20);
	EPD_WriteCMD(0xff);
}
/*******************************************************************************
write the waveform to the dirver's ram
********************************************************************************/
void WaveShare_EPD::EPD_WirteLUT(unsigned char *LUTvalue,unsigned char Size)
{	
	EPD_Write(LUTvalue, Size);
}

/*******************************************************************************
Full screen initialization
********************************************************************************/
void WaveShare_EPD::EPD_init_Full(void)
{		
	EPD_Init();			// Reset and set register 
  EPD_WirteLUT((unsigned char *)LUTDefault_full,sizeof(LUTDefault_full));
		
	EPD_POWERON();
    //driver_delay_xms(100000); 		
}

/*******************************************************************************
Part screen initialization
********************************************************************************/
void WaveShare_EPD::EPD_init_Part(void)
{		
	EPD_Init();			// display
	EPD_WirteLUT((unsigned char *)LUTDefault_part,sizeof(LUTDefault_part));
	EPD_POWERON();        		
}
/********************************************************************************
parameter:
	Label  :
       		=1 Displays the contents of the DisBuffer
	   		=0 Displays the contents of the first byte in DisBuffer,
********************************************************************************/
void WaveShare_EPD::EPD_Dis_Full(unsigned char *DisBuffer,unsigned char Label)
{
    EPD_SetRamPointer(0x00,0,0);	// set ram
	Serial.println(">>>>>>------start send display data!!---------<<<<<<<");
	if(Label == 0){
		EPD_WriteDispRamMono(xDot, yDot, 0x01);	// white	
	}else{
		EPD_WriteDispRam(xDot, yDot, (unsigned char *)DisBuffer);	// white
	}	
	EPD_Update();	
	  
}

/********************************************************************************
parameter: 
		xStart :   X direction Start coordinates
		xEnd   :   X direction end coordinates
		yStart :   Y direction Start coordinates
		yEnd   :   Y direction end coordinates
		DisBuffer : Display content
		Label  :
       		=1 Displays the contents of the DisBuffer
	   		=0 Displays the contents of the first byte in DisBuffer,
********************************************************************************/
void WaveShare_EPD::EPD_Dis_Part(unsigned char xStart,unsigned char xEnd,unsigned long yStart,unsigned long yEnd,unsigned char *DisBuffer,unsigned char Label)
{
	Serial.println(">>>>>>------start send display data!!---------<<<<<<<");
	if(Label==0){// black
		EPD_part_display(xStart/8,xEnd/8,yEnd%256,yEnd/256,yStart%256,yStart/256);
		EPD_WriteDispRamMono(xEnd-xStart, yEnd-yStart+1, DisBuffer[0]);
 		EPD_Update_Part();
		driver_delay_xms(500);
		EPD_part_display(xStart/8,xEnd/8,yEnd%256,yEnd/256,yStart%256,yStart/256);	
		EPD_WriteDispRamMono(xEnd-xStart, yEnd-yStart+1,DisBuffer[0]);	
	}else{// show 
		EPD_part_display(xStart/8,xEnd/8,yEnd%256,yEnd/256,yStart%256,yStart/256);		
		EPD_WriteDispRam(xEnd-xStart, yEnd-yStart+1,DisBuffer);
		EPD_Update_Part();
		driver_delay_xms(500);
		EPD_part_display(xStart/8,xEnd/8,yEnd%256,yEnd/256,yStart%256,yStart/256);
		EPD_WriteDispRam(xEnd-xStart, yEnd-yStart+1,DisBuffer);
	}
}

/***********************************************************************************************************************
			------------------------------------------------------------------------
			|\\\																///|
			|\\\						App layer								///|
			------------------------------------------------------------------------
***********************************************************************************************************************/
/********************************************************************************
		clear full screen
********************************************************************************/
void WaveShare_EPD::Dis_Clear_full(void)
{
	unsigned char m;
	//init
	Serial.println("full init");
	EPD_init_Full();
	driver_delay_xms(DELAYTIME);

	//Clear screen
	Serial.println("full clear\t\n");
 
	m=0xff;
	EPD_Dis_Full((unsigned char *)&m,0);  //all white
 clearbuffer();
	driver_delay_xms(DELAYTIME);
}
/********************************************************************************
		clear part screen
********************************************************************************/
void WaveShare_EPD::Dis_Clear_part(void)
{
	unsigned char m;
	//init
	EPD_init_Part();
	driver_delay_xms(DELAYTIME);

	//Clear screen
	m=0xff;
	EPD_Dis_Part(0,xDot-1,0,yDot-1,(unsigned char *)&m,0);	 //all white
	driver_delay_xms(DELAYTIME);
}

/********************************************************************************
parameter: 
		xStart :   X direction Start coordinates
		xEnd   :   X direction end coordinates
		yStart :   Y direction Start coordinates
		yEnd   :   Y direction end coordinates
		DisBuffer : Display content
********************************************************************************/
void WaveShare_EPD::Dis_pic(unsigned char xStart,unsigned char xEnd,unsigned long yStart,unsigned long yEnd,unsigned char *DisBuffer)
{
	EPD_Dis_Part(xStart,xEnd,yStart,yEnd,(unsigned char *)DisBuffer,1);
}

/********************************************************************************
funtion : Select the character size
parameter :
	acsii : char data 
	size : char len
	mode : char mode
	next : char len
Remarks:
********************************************************************************/

/********************************************************************************
funtion : write string
parameter :
	x : x start address
	y : y start address
	pString : Display data
	Size : char len
Remarks:
********************************************************************************/
 

/********************************************************************************
funtion : Drawing pic
parameter :
	xStart : x start address
	yStart : y start address
	DisBuffer : Display data
	xSize : Displays the x length of the image
	ySize : Displays the y length of the image
Remarks:
	The sample image is 32 * 32
********************************************************************************/
void WaveShare_EPD::Dis_Drawing(unsigned char xStart,unsigned long yStart,unsigned char *DisBuffer,unsigned char xSize,unsigned char ySize)
{
	unsigned char x_addr = xStart*8;
	unsigned char y_addr = yStart*8;
	EPD_Dis_Part(y_addr,y_addr+xSize-1,yDot-ySize-x_addr,yDot-x_addr-1,(unsigned char *)DisBuffer,1);
}
void WaveShare_EPD::Dis_Drawing2(unsigned char xStart,unsigned long yStart,unsigned char *DisBuffer,unsigned char xSize,unsigned char ySize)
{
  unsigned char x_addr = xStart*8;
  unsigned char y_addr = yStart;
  EPD_Dis_Part(x_addr,x_addr+xSize-1,yDot-y_addr-ySize,yDot-y_addr-1,(unsigned char *)DisBuffer,1);
}
/********************************************************************************
funtion : show Progress
parameter :
	progress_len : Progress bar length	
********************************************************************************/
void WaveShare_EPD::Dis_Progress(unsigned char progress_len)
{
	int x,y,z;
	int pheight_pix = 2;
	int pWidth_pix = 16;
  unsigned char buffer[5000];
	const unsigned char *phead ,*pzero,*pstart,*pspare,*pfull,*pend;
	//1.Initialize the progress bar length and place it in the center of the lower end of the display
	y = 0;
	for(z= 0;z < progress_len;z++){
		phead = progress_head;
		pspare = progress_Spare;
		pend = progress_end;
		for(x = 0;x <pWidth_pix*pheight_pix;x++){
			if(z == 0){
				buffer[y] = *phead;
				phead++;
				y++;
			}else if(z == progress_len -1){
				buffer[y] = *pend;
				pend++;
				y++;
			}else{
				buffer[y] = *pspare;
				pspare++;
				y++;
			}
		}
	}
	EPD_Dis_Part(xDot-xDot/10-1,xDot-xDot/10+8,(yDot-16*progress_len)/2-1,(yDot-16*progress_len)/2-1+16*progress_len,(unsigned char *)buffer,1);
	//2.Load progress bar
	y =0;
	for(z= 0;z < progress_len;z++){
		pstart = progress_start;
		pzero = progress_zero;
		pfull = progress_full;
		for(x = 0;x <pWidth_pix*pheight_pix;x++){
			if(z == 0){
				buffer[y] = *pzero;
				pzero++;
				y++;
			}else if(z == progress_len-1){
				buffer[y] = *pfull;
				pfull++;
				y++;
			}else{
				buffer[y] = *pstart;
				pstart++;
				y++;
			}
		}
		EPD_Dis_Part(xDot-xDot/10-1,xDot-xDot/10+8,(yDot-16*progress_len)/2-1,(yDot-16*progress_len)/2-1+16*progress_len,(unsigned char *)buffer,1);
	}
}
/********************************************************************************
funtion : show time 
parameter :
	hour : The number of hours
	min : The number of minute
	sec : The number of sec
Remarks:
********************************************************************************/




/***********************************************************
						end file
***********************************************************/

