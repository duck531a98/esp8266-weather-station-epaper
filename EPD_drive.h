
 extern unsigned char UNICODEbuffer[200];
 extern String fontname;
#ifndef _EPD_DRIVE_H_
#define _EPD_DRIVE_H_

/**************-------------------------------------------**************
              Register table
**************-------------------------------------------**************/
#define EPD2X9 1
//#define EPD1X54  1
#ifdef EPD2X9
  #define xDot 128
  #define yDot 296
  #define DELAYTIME 1500
  
  static const unsigned char GDVol[] = {0x03,0x00}; // Gate voltage +15V/-15V
#elif  EPD02X13
  #define xDot 128
  #define yDot 250
  #define DELAYTIME 4000
  static const  unsigned char GDVol[] = {0x03,0xea};  // Gate voltage +15V/-15V
#elif  EPD1X54
  #define xDot 200
  #define yDot 200
  #define DELAYTIME 1500
  static const  unsigned char GDVol[] = {0x03,0x00};  // Gate voltage +15V/-15V
#endif
  static  unsigned char GDOControl[]={0x01,(yDot-1)%256,(yDot-1)/256,0x00}; //for 1.54inch
  static  unsigned char softstart[]={0x0c,0xd7,0xd6,0x9d};
  static  unsigned char Rambypass[] = {0x21,0x8f};   // Display update
  static unsigned char MAsequency[] = {0x22,0xf0};    // clock 
  static  unsigned char SDVol[] = {0x04,0x0a}; // Source voltage +15V/-15V
  static  unsigned char VCOMVol[] = {0x2c,0xa8}; // VCOM 7c 0xa8
  static unsigned char GateVol[]={0x03,0xea};
  
  static  unsigned char BOOSTERFB[] = {0xf0,0x1f}; // Source voltage +15V/-15V
  static  unsigned char DummyLine[] = {0x3a,0x01}; // 4 dummy line per gate
  static  unsigned char Gatetime[] = {0x3b,B1000};  // 2us per line
  static  unsigned char BorderWavefrom[] = {0x3c,0x63};  // Border
  static  unsigned char RamDataEntryMode[] = {0x11,0x01};  // Ram data entry mode

#ifdef EPD02X13  
  static const unsigned char LUTDefault_full[]={
  0x32, // command
  0x22,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x01,0x00,0x00,0x00,0x00,
  };
  static const unsigned char LUTDefault_part[]={
  0x32, // command
  0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  };
#else
    //Write LUT register
  static const unsigned char LUTDefault_part[31] = {
    0x32, // command
    0x10,0x18,0x18,0x08,0x18,0x18,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x13,0x14,0x44,0x12,0x00,0x00,0x00,0x00,0x00,0x00
  };
  static const unsigned char LUTDefault_full[31] = {
    0x32, // command
    0x02,0x02,0x01,0x11,0x12,0x12,0x22,0x22,0x66,0x69,0x69,0x59,0x58,0x99,0x99,0x88,0x00,0x00,0x00,0x00,0xF8,0xB4,0x13,0x51,0x35,0x51,0x51,0x19,0x01,0x00 
  };  
#endif

/**************-------------------------------------------**************
							Show LIb
**************-------------------------------------------**************/


static const unsigned char 	progress_head[] = {
	0x00,0x00,0x00,0x00,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,
	0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,
};

static const unsigned char 	progress_zero[] = {
	0x00,0x00,0x00,0x00,0x3F,0xFC,0x3F,0xFC,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,
	0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,
	};
static const unsigned char 	progress_start[] = {
	0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,
	0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,
	};
static const unsigned char 	progress_Spare[] = {
	0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,
	0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC
	};
static const unsigned char 	progress_full[] = {
	0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,
0x30,0x0C,0x30,0x0C,0x30,0x0C,0x30,0x0C,0x3F,0xFC,0x3F,0xFC,0x00,0x00,0x00,0x00,
	};
static const unsigned char	progress_end[] = {
	0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,
	0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x3F,0xFC,0x00,0x00,0x00,0x00,
};


class WaveShare_EPD {
  
	unsigned char ReadBusy(void);
	void EPD_WriteCMD(unsigned char command);
	void EPD_WriteCMD_p1(unsigned char command,unsigned char para);
	void EPD_POWERON(void);
	void EPD_Write(unsigned char *value, unsigned char datalen);
	void EPD_WriteDispRam(unsigned char XSize,unsigned int YSize,unsigned char *Dispbuff);
	
	void EPD_SetRamArea(unsigned char Xstart,unsigned char Xend,unsigned char Ystart,unsigned char Ystart1,unsigned char Yend,unsigned char Yend1);
	void EPD_SetRamPointer(unsigned char addrX,unsigned char addrY,unsigned char addrY1);
	void EPD_part_display(unsigned char RAM_XST,unsigned char RAM_XEND,unsigned char RAM_YST,unsigned char RAM_YST1,unsigned char RAM_YEND,unsigned char RAM_YEND1);
	void EPD_Init(void);
	
	void EPD_Update_Part(void);
	void EPD_WirteLUT(unsigned char *LUTvalue,unsigned char Size);
	
	

	void Dis_Char(char acsii,char size,char mode,char next,unsigned char *buffer);
	
public:
void deepsleep(void);
byte fontscale;byte FontIndex;int16_t CurrentCursor;unsigned char EPDbuffer[xDot*yDot/8];
byte fontwidth;byte fontheight;
 void DrawYline(byte start,byte end, byte y);
 void DrawXline(int start,int end, byte x);
void SetFont(byte fontindex);
void DrawUTF(byte x,int16_t y,byte width,byte height,String code);
void DrawUTF(byte x,int16_t y,byte width,byte height,unsigned char *code);
int UTFtoUNICODE(unsigned char *code);
void DrawUnicodeChar(byte x,int16_t y,byte width,byte height,unsigned char *code);
void DrawUnicodeStr(byte x,int16_t y,byte width,byte height,byte strlength,unsigned char *code);
void drawXbm(int16_t xMove, int16_t yMove, int16_t width, int16_t height,unsigned char *xbm);
void DrawXbm_P(int16_t xMove, int16_t yMove, int16_t width, int16_t height,const unsigned char *xbm);
	WaveShare_EPD();
 void SetPixel(char x, int y);
  void clearbuffer();
	void EPD_Update(void);
	void clearshadows();
	void EPD_init_Full(void);
 void EPD_Dis_Full(unsigned char *DisBuffer,unsigned char Label);
 void Dis_Drawing2(unsigned char xStart,unsigned long yStart,unsigned char *DisBuffer,unsigned char xSize,unsigned char ySize);
void  Dis_String2(unsigned char x,unsigned char y, const char *pString,unsigned int  Size);
  void EPD_Dis_Part(unsigned char xStart,unsigned char xEnd,unsigned long yStart,unsigned long yEnd,unsigned char *DisBuffer,unsigned char Label);
	void EPD_WriteDispRamMono(unsigned char XSize,unsigned int YSize,unsigned char dispdata);
	void EPD_init_Part(void);
	void Dis_Clear_full(void);
	void Dis_Clear_part(void);
	void Dis_pic(unsigned char xStart,unsigned char xEnd,unsigned long yStart,unsigned long yEnd,unsigned char *DisBuffer);
	void Dis_String(unsigned char x,unsigned char y, const char *pString,unsigned int  Size);
	void Dis_Drawing(unsigned char xStart,unsigned long yStart,unsigned char *DisBuffer,unsigned char xSize,unsigned char ySize);
	void Dis_Progress(unsigned char progress_len);
	void Dis_showtime(unsigned int hour,unsigned int min,unsigned int sec);
};

#endif

