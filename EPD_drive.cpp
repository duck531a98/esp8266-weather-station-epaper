#include "FS.h"
#include "EPD_drive.h"
#include "EPD_drive_gpio.h"
#include "EPD_raster.h"
#include <LittleFS.h>

unsigned char UNICODEbuffer[200];
const char* fontname="";
static const size_t GLYPH_BUFFER_SIZE=1300;
static unsigned char glyphBuffer[GLYPH_BUFFER_SIZE];

namespace {

uint8_t ReadRamXbmByte(const uint8_t* address)
{
  return *address;
}

uint8_t ReadProgmemXbmByte(const uint8_t* address)
{
  return pgm_read_byte(address);
}

void YieldDuringRasterDraw()
{
  optimistic_yield(1000UL);
}

epd_raster::BufferLayout RasterLayoutFor(epd_type type)
{
  if(type==OPM42||type==DKE42_3COLOR||type==WF42)
    return epd_raster::ROTATED_1BPP;
  if(type==WF29BZ03||type==WF32)
    return epd_raster::INTERLEAVED_2BPP;
  return epd_raster::DIRECT_1BPP;
}

} // namespace



 Duck_EPD::Duck_EPD(){
 
}


void Duck_EPD::SetFont(FONT fontindex)
{
     FontIndex=fontindex;
     switch (fontindex)
     {
     case 0:
     fontname="/font16";fontwidth=16;fontheight=16;break;
     case 1:
     fontname="/font32";fontwidth=32;fontheight=32;break;
     case 2:
     fontname="/font10";fontwidth=10;fontheight=10;break;
     case 3:
     fontname="/font12";fontwidth=12;fontheight=12;break;
     case 5:
     fontname="/font70";fontwidth=70;fontheight=70;break;
     case 6:
     fontname="/font12num";fontwidth=12;fontheight=12;break;
     case 7:
     fontname="/font24";fontwidth=24;fontheight=24;break;
     case 8:
     fontname="/font8";fontwidth=8;fontheight=8;break;
     case 9:
     fontname="/font100num";fontwidth=100;fontheight=100;break;
     case ICON_WEATHER:
     fontname="/weathericon";fontwidth=32;fontheight=32;break;
     case 12:
     fontname="/weathericon80";fontwidth=80;fontheight=80;break;
      case 13:
     fontname="/weathericon32";fontwidth=32;fontheight=32;break;
     case 14:
     fontname="/weathericon50";fontwidth=50;fontheight=50;break;
     }
  }
void Duck_EPD::DrawCircle(int x,int y,int r,bool fill)
{
  if(r<0) return;
  if(r==0)
  {
    SetPixel(x,y);
    return;
  }

  int dx=0;
  int dy=r;
  int decision=1-r;
  while(dx<=dy)
  {
    if(fill)
    {
      for(int px=x-dy;px<=x+dy;px++)
      {
        SetPixel(px,y+dx);
        if(dx!=0) SetPixel(px,y-dx);
      }
      if(dx!=dy)
      {
        for(int px=x-dx;px<=x+dx;px++)
        {
          SetPixel(px,y+dy);
          SetPixel(px,y-dy);
        }
      }
    }
    else
    {
      SetPixel(x+dx,y+dy);
      SetPixel(x-dx,y+dy);
      SetPixel(x+dx,y-dy);
      SetPixel(x-dx,y-dy);
      SetPixel(x+dy,y+dx);
      SetPixel(x-dy,y+dx);
      SetPixel(x+dy,y-dx);
      SetPixel(x-dy,y-dx);
    }

    dx++;
    if(decision<0) decision+=2*dx+1;
    else
    {
      dy--;
      decision+=2*(dx-dy)+1;
    }
  }
}

 void Duck_EPD::DrawBox(byte x,int y, int w, int h)
 {

    for(int i=x;i<x+h;i++)
  {
    DrawXline( y, y+ w-1,  i);
  }
  
 }
 void Duck_EPD::DrawEmptyBox(int x,int y,int w,int h)
{
   DrawXline(y,y+w, x);
   DrawXline(y,y+w, x+h);
   DrawYline(x,x+w, y);
   DrawYline(x,x+w, y+w);
  
}
 
 void Duck_EPD::DrawChart(int x,int y,int w, int c1, int c2, int c3, int c4, int c5,int c6)
  {
    
    int percent,sum,max_c=0;
    sum=c1+c2+c3+c4+c5+c6;
    if(sum==0) sum=1;
    max_c=max(c1,max_c);max_c=max(c2,max_c);max_c=max(c3,max_c);
    max_c=max(c4,max_c);max_c=max(c5,max_c);max_c=max(c6,max_c);
    if(max_c==0) max_c=1;
    w=w*sum/(2*max_c);
    
    SetFont(FONT12);
    DrawUTF(x,y,">0.3um");
    percent=w*c1/sum;
    DrawBox(x+1,y+41,percent,10);
    SetFont(FONT8);
    DrawUTF(x+2,y+41+percent,String(c1));
   
    SetFont(FONT12);
    DrawUTF(x+12,y,">0.5um");
    percent=w*c2/sum;
    DrawBox(x+13,y+41,percent,10);
    SetFont(FONT8);
    DrawUTF(x+14,y+41+percent,String(c2));

    SetFont(FONT12);
    DrawUTF(x+24,y,">1.0um");
    percent=w*c3/sum;
    DrawBox(x+25,y+41,percent,10);
    SetFont(FONT8);
    DrawUTF(x+26,y+41+percent,String(c3));

    SetFont(FONT12);
    DrawUTF(x+36,y,">2.5um");
    percent=w*c4/sum;
    DrawBox(x+37,y+41,percent,10);
    SetFont(FONT8);
    DrawUTF(x+38,y+41+percent,String(c4));

    SetFont(FONT12);
    DrawUTF(x+48,y,">5.0um");
    percent=w*c5/sum;
    DrawBox(x+49,y+41,percent,10);
    SetFont(FONT8);
    DrawUTF(x+50,y+41+percent,String(c5));

    SetFont(FONT12);
    DrawUTF(x+60,y,">10um");
    percent=w*c6/sum;
    DrawBox(x+61,y+41,percent,10);
    SetFont(FONT8);
    DrawUTF(x+62,y+41+percent,String(c6));
    
    
    }
 void Duck_EPD::DrawCircleChart(int x,int y,int r,int w,int c1,int c2,int c3)
 {
     int sum=c1+c2+c3;
     if(sum==0) sum=1;
        for(int i=0;i<360;i++)
      {
        SetPixel(-round(cos(M_PI*i/180)*r)+x,round(sin(M_PI*i/180)*r)+y);
        
      }
      
      for(int i=0;i<360;i++)
      {
        SetPixel(-round(cos(M_PI*i/180)*(r-w))+x,round(sin(M_PI*i/180)*(r-w))+y);
        
      }
      
      for(int i=0;i<c1*360/sum;i++)
      {
        for(int j=0;j<w;j++)
        { 
        SetPixel(-round(cos(M_PI*i/180)*(r-j))+x,round(sin(M_PI*i/180)*(r-j))+y);
        }
      }
      
      for(int i=(c1+c2)*360/sum-1;i<(c1+c2)*360/sum;i++)
      {
        
        for(int j=0;j<w;j++)
        { 
        SetPixel(-round(cos(M_PI*i/180)*(r-j))+x,round(sin(M_PI*i/180)*(r-j))+y);
        }
      }
      
      for(int i=(c1+c2)*360/sum;i<360;i+=10)
      {
        
        for(int j=0;j<w;j++)
        { 
        SetPixel(-round(cos(M_PI*i/180)*(r-j))+x,round(sin(M_PI*i/180)*(r-j))+y);
        }
      }
      
      y+=2;
      DrawBox(x-r,y+r+2,8,8);
      SetFont(FONT12);
      DrawUTF(x-r-2,y+r+12,"PM1.0");
      SetFont(FONT12);
      DrawUTF(x-r-2,y+r+12+30,String(c1));
      
      
      SetFont(FONT12);
      DrawEmptyBox(x-r+14,y+r+2,8,8);
      DrawUTF(x-r+12,y+r+12,"PM2.5");
      
      SetFont(FONT12);
      DrawUTF(x-r+12,y+r+12+30,String(c2));
      DrawEmptyBox(x-r+28,y+r+2,8,8);
      for(int i=y+r+2;i<y+r+2+8;i+=2)
        {
          DrawYline(x-r+28,x-r+28+7,i);
          }
      SetFont(FONT12);
      DrawUTF(x-r+26,y+r+12,"PM10");
      SetFont(FONT12);
      DrawUTF(x-r+26,y+r+12+30,String(c3));
  
  
  }
int Duck_EPD::getIcon(int weathercodeindex) {
  if(weathercodeindex==0) return 12;
  if(weathercodeindex==1) return 58;
  if(weathercodeindex==2) return 58;
  if(weathercodeindex==3) return 58;
  if(weathercodeindex==4) return 54;
  if(weathercodeindex>=5&&weathercodeindex<=18) return 0;
  if(weathercodeindex>=19&&weathercodeindex<=32) return 19;
  if(weathercodeindex>=33&&weathercodeindex<=36)   return 16;
  if(weathercodeindex>=37&&weathercodeindex<=40)   return 16;
  if(weathercodeindex==41) return 37;
  if(weathercodeindex==42) return 37;
  if(weathercodeindex==43) return 37;
  return 17;
 }
 void Duck_EPD::DrawWeatherChart(int xmin,int xmax,int ymin,int ymax,int point_n,int show_n,
                                 String tmax,String tmin,String code_d,String code_n,
                                 String text_d,String text_n,String date,String week)//绘制天气温度变化曲线
 {
  if(tmax==",,,,,")
  {
    tmax="5,5,5,5,5,5";
    tmin="2,2,2,2,2,2";
    code_d="0,0,0,0,0,0";
    code_n="0,0,0,0,0,0";
    text_n="晴,晴,晴,晴,晴,晴";
    text_d="晴,晴,晴,晴,晴,晴";
    date="1-1,1-2,1-3,1-4,1-5,1-6";
    week="1,1,1,1,1,1";
    }
  const int maxPointCount=6;
  if(point_n<=0||point_n>maxPointCount||
     show_n<=1||show_n>point_n) return;
  String code_d_a[maxPointCount],code_n_a[maxPointCount],
         text_d_a[maxPointCount],text_n_a[maxPointCount],
         date_a[maxPointCount],week_a[maxPointCount];
  int tmax_a[maxPointCount],tmin_a[maxPointCount];
  int min_data=999,max_data=-999;  
  int tmin_x_cord[maxPointCount],tmax_x_cord[maxPointCount];//将数值转成屏幕坐标
  
  String temp_min[maxPointCount];
  String temp_max[maxPointCount];
  int j=0,k=0;
  //分割tmax和tmin
 // Serial.println(tmax); Serial.println(tmin);
  //Serial.println(code_d);
  for(size_t i=0;i<tmin.length();i++)
  {
    temp_min[j]+=tmin[i];
   
    if(tmin.charAt(i)==char(',')&&j<point_n-1) j++;
    
    }
  for(size_t i=0;i<tmax.length();i++)
  {
   
    temp_max[k]+=tmax[i];    
   
    if(tmax.charAt(i)==char(',')&&k<point_n-1) k++;
    } 
  j=0;
  //分割code_d
  for(size_t i=0;i<code_d.length();i++)
  {
    code_d_a[j]+=code_d[i];    
    if(code_d.charAt(i)==char(',')&&j<point_n-1) j++;
    }
  //分割code_n
  j=0;
  for(size_t i=0;i<code_n.length();i++)
  {
    code_n_a[j]+=code_n[i];    
    if(code_n.charAt(i)==char(',')&&j<point_n-1) j++;
    }
   //分割text_d
   j=0;
   for(size_t i=0;i<text_d.length();i++)
  {
    if(text_d.charAt(i)==char(',')&&j<point_n-1) j++;
    else text_d_a[j]+=text_d[i];    
    }
   //分割text_n
   j=0;
   for(size_t i=0;i<text_n.length();i++)
  {
    if(text_n.charAt(i)==char(',')&&j<point_n-1) j++;
    else text_n_a[j]+=text_n[i];   
    }
  //分割week_n
   j=0;
   for(size_t i=0;i<week.length();i++)
  {
    if(week.charAt(i)==char(',')&&j<point_n-1) j++;
    else week_a[j]+=week[i];   
    }
  j=0;
   for(size_t i=0;i<date.length();i++)
  {
    if(date.charAt(i)==char(',')&&j<point_n-1) j++;
    else date_a[j]+=date[i];   
    }
  for(int i=0;i<point_n;i++)
  {
  tmax_a[i]=temp_max[i].toInt();//Serial.printf("max:%d\n",tmax_a[i]);
  tmin_a[i]=temp_min[i].toInt();//Serial.printf("min:%d\n",tmin_a[i]);
  }
  //找出计算最大最小值
  for(int i=0;i<show_n;i++)
  {
    if(tmax_a[i]>max_data) max_data=tmax_a[i];
    if(tmax_a[i]<min_data) min_data=tmax_a[i];
    if(tmin_a[i]>max_data) max_data=tmin_a[i];
    if(tmin_a[i]<min_data) min_data=tmin_a[i];
    }
 //转换坐标
  if((max_data-min_data)!=0)
  {
  for(int i=0;i<show_n;i++)
  {
    tmin_x_cord[i]=xmax-((xmax-xmin)*(tmin_a[i]-min_data)/(max_data-min_data));
    tmax_x_cord[i]=xmax-((xmax-xmin)*(tmax_a[i]-min_data)/(max_data-min_data));   
    }
  }
  else
  {
    int middle=(xmin+xmax)/2;
    for(int i=0;i<show_n;i++)
    {
      tmin_x_cord[i]=middle;
      tmax_x_cord[i]=middle;
    }
  }
  int dy=(ymax-ymin)/(show_n-1);
  
  /*
  Spline line;
  float x[point_n];float y[point_n];
  for(int i=0;i<point_n;i++)
  {
    y[i]=*data;x[i]=ymin+dy*i;
    data++;
    }
  data=o;
 line.setPoints(x,y,point_n);
 line.setDegree(11);
 for(int i=ymin;i<ymax;i++)
  {
    SetPixel(line.value(i),i);
   
  }
 */
//画折线
  for(int i=0;i<show_n-1;i++)
  {          
      DrawLine(tmin_x_cord[i],ymin+dy*i,tmin_x_cord[i+1],ymin+dy*(i+1));   
      DrawLine(tmax_x_cord[i],ymin+dy*i,tmax_x_cord[i+1],ymin+dy*(i+1));   

     // DrawLine(tmin_x_cord[i]-1,ymin+dy*i,tmin_x_cord[i+1]-1,ymin+dy*(i+1));   
     // DrawLine(tmax_x_cord[i]-1,ymin+dy*i,tmax_x_cord[i+1]-1,ymin+dy*(i+1));   
  }
 //画圆圈，添加标注
 for(int i=0;i<show_n;i++)
  {    
      DrawCircle(tmin_x_cord[i],ymin+dy*i,3,1);
      DrawCircle(tmax_x_cord[i],ymin+dy*i,3,1);
      SetFont(FONT12);fontscale=1;
    
      DrawUTF(tmax_x_cord[i]-14,ymin+dy*i-3*(String(tmax_a[i]).length()+1),String(tmax_a[i])+"°");
      DrawUTF(tmin_x_cord[i]+2,ymin+dy*i-3*(String(tmax_a[i]).length()+1),String(tmin_a[i])+"°");
      
     if(EPD_Type!=WF32){
      SetFont(ICON32);DrawUTF(xmin-44,ymin+dy*i-16,String(char(getIcon(code_d_a[i].toInt()))));
      SetFont(FONT12);DrawUTF(xmin-56,ymin+dy*i-2*text_d_a[i].length(),text_d_a[i]);
      DrawUTF(xmin-82,ymin+dy*i-2*week_a[i].length(),week_a[i]);
      
      SetFont(ICON32);DrawUTF(xmax+14,ymin+dy*i-16,String(char(getIcon(code_n_a[i].toInt()))));
      SetFont(FONT12);DrawUTF(xmax+46,ymin+dy*i-2*text_n_a[i].length(),text_n_a[i]);
     }
     else
     {
      //SetFont(ICON32);DrawUTF(xmin-35,ymin+dy*i-16,String(char(getIcon(code_d_a[i].toInt()))));
      SetFont(FONT12);DrawUTF(xmax+14,ymin+dy*i-2*text_d_a[i].length(),text_d_a[i]);
      SetFont(ICON32);DrawUTF(xmax+26,ymin+dy*i-16,String(char(getIcon(code_d_a[i].toInt()))));
      SetFont(FONT12);DrawUTF(xmin-28,ymin+dy*i-2*week_a[i].length(),week_a[i]);
      
      }
      
      //Serial.print("code_d:");
      //Serial.println(code_d_a[i]);
      //Serial.print("txt_n:");
      //Serial.println(text_n_a[i]);
  } 
    if(EPD_Type!=WF32)Inverse(xmin-83,xmin-69,0,400);
  }
 void Duck_EPD::DrawXline(int start,int end, int x)
  {
    for(int i=start;i<=end;i++)
    {
      SetPixel(x,i);
      }
  }
void Duck_EPD::DrawYline(int start,int end,  int y)
  {
    for(int i=start;i<=end;i++)
    {
      SetPixel(i,y);
      }
  }
void Duck_EPD::DrawLine(int xstart,int ystart, int xend,int yend)
{
  int dx=abs(xend-xstart);
  int sx=xstart<xend?1:-1;
  int dy=-abs(yend-ystart);
  int sy=ystart<yend?1:-1;
  int error=dx+dy;
  while(true)
  {
    SetPixel(xstart,ystart);
    if(xstart==xend&&ystart==yend) break;
    int doubledError=2*error;
    if(doubledError>=dy)
    {
      error+=dy;
      xstart+=sx;
    }
    if(doubledError<=dx)
    {
      error+=dx;
      ystart+=sy;
    }
  }
}

void Duck_EPD::Inverse(int xStart,int xEnd,int yStart,int yEnd)
{
  if(EPD_Type==WF32&&xStart==0&&xEnd==xDot&&
     yStart>=0&&yStart<yEnd&&yEnd<=yDot)
  {
    // WF32 stores four pixels per byte in two interleaved bit planes.
    // A full-width rectangle can therefore be inverted byte-for-byte without
    // changing the other frame plane or calling InversePixel 27,000 times.
    const unsigned char planeMask=frame==0?0xaa:0x55;
    const size_t rowBytes=(size_t)xDot/4;
    unsigned char* row=EPDbuffer+(size_t)yStart*rowBytes;
    for(int y=yStart;y<yEnd;y++)
    {
      for(size_t column=0;column<rowBytes;column++) row[column]^=planeMask;
      row+=rowBytes;
      if(((y-yStart)&0x07)==0) optimistic_yield(1000UL);
    }
    return;
  }

  for(int i=0;i<(xEnd-xStart);i++)
  {
    for(int j=0;j<(yEnd-yStart);j++)
    {
      InversePixel(xStart+i,yStart+j);
      }
    if((i&0x07)==0) optimistic_yield(1000UL);
    }
  
  }
void Duck_EPD::DrawUTF(int16_t x,int16_t y,const String& code)
{
  char buffer[200];
  code.toCharArray(buffer,200);
  DrawUTF(x,y,fontwidth,fontheight,(unsigned char *)buffer);
  }
void Duck_EPD::DrawUTF(int16_t x,int16_t y,const char* code)
{
  if(code==nullptr) return;
  char buffer[200];
  size_t length=0;
  while(length<sizeof(buffer)-1&&code[length]!='\0') length++;
  memcpy(buffer,code,length);
  buffer[length]='\0';
  DrawUTF(x,y,fontwidth,fontheight,(unsigned char *)buffer);
}
void Duck_EPD::DrawUTF(int16_t x,int16_t y,byte width,byte height,unsigned char *code)
{
  int charcount;
  charcount=UTFtoUNICODE((unsigned char*)code);
  DrawUnicodeStr(x,y,width,height,charcount,(unsigned char *)UNICODEbuffer);
  
  }
int Duck_EPD::UTFtoUNICODE(unsigned char *code)
{ 
  int i=0;int charcount=0;
  const int maxCharCount=sizeof(UNICODEbuffer)/2-1;
  while(code[i]!='\0'&&charcount<maxCharCount)
  { 
    //Serial.println("current codei");
      //Serial.println(code[i],HEX);
      //  Serial.println(code[i]&0xf0,HEX);
    if(code[i]<=0x7f)  //ascii
    {
      
      UNICODEbuffer[charcount*2]=0x00;
      UNICODEbuffer[charcount*2+1]=code[i];
        // Serial.println("english or number");
      // Serial.println(UNICODEbuffer[charcount*2],HEX);
       //  Serial.println(UNICODEbuffer[charcount*2+1],HEX);
      i++;charcount++;
   
      }
    else if((code[i]&0xe0)==0xc0&&code[i+1]!='\0'&&(code[i+1]&0xc0)==0x80)
    {
    
      UNICODEbuffer[charcount*2+1]=(code[i]<<6)+(code[i+1]&0x3f);
      UNICODEbuffer[charcount*2]=(code[i]>>2)&0x07;
      i+=2;charcount++;
     // Serial.println("two bits utf-8");
      }
     else if((code[i]&0xf0)==0xe0&&code[i+1]!='\0'&&code[i+2]!='\0'&&(code[i+1]&0xc0)==0x80&&(code[i+2]&0xc0)==0x80)
    {
      
      UNICODEbuffer[charcount*2+1]=(code[i+1]<<6)+(code[i+2]&0x7f);
      UNICODEbuffer[charcount*2]=(code[i]<<4)+((code[i+1]>>2)&0x0f);
      
       //Serial.println("three bits utf-8");
       // Serial.println(UNICODEbuffer[charcount*2],HEX);
        // Serial.println(UNICODEbuffer[charcount*2+1],HEX);
        i+=3;charcount++;
      }
     else
      {
       i++;
      }
  }
  UNICODEbuffer[charcount*2]='\0';
  return charcount;
  }
void Duck_EPD::DrawUnicodeChar(int16_t x,int16_t y,byte width,byte height,unsigned char *code)
 { 
 
  int offset;
  int sizeofsinglechar;
  if (height%8==0) sizeofsinglechar=(height/8)*width;
  else sizeofsinglechar=(height/8+1)*width;
 offset=(code[0]*0x100+code[1])*sizeofsinglechar;
  // Serial.println("code[1]");
  // Serial.println(code[1]);
   // Serial.println("sizeofsinglechar");
  // Serial.println(sizeofsinglechar);
   if(sizeofsinglechar<=0||sizeofsinglechar>(int)GLYPH_BUFFER_SIZE) return;
   File f=LittleFS.open(fontname,"r");
   if(!f||!f.seek(offset,SeekSet)) return;
   if(f.readBytes((char *)glyphBuffer,sizeofsinglechar)!=(size_t)sizeofsinglechar)
     return;
 /*for(int i=0;i<32;i++)
  {
     
   Serial.println(zi[i],HEX);
    }*/
  // Serial.println("offset");
   //Serial.println(offset);
   if (offset<0xff*sizeofsinglechar && FontIndex<10) 
   {drawXbm(x,y,width,height,glyphBuffer); }
   else  {drawXbm(x,y,width,height,glyphBuffer);}
  
//SPIFFS.end();
}

void Duck_EPD::DrawUnicodeStr(int16_t x,int16_t y,byte width,byte height,byte strlength,unsigned char *code)
{
  int ymax=yDot;
   if(EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==WF42)
   {
  ymax=400;
   }
  
  CurrentCursor=0;
  int sizeofsinglechar;
  if (height%8==0) sizeofsinglechar=(height/8)*width;
  else sizeofsinglechar=(height/8+1)*width;
  if(sizeofsinglechar<=0||sizeofsinglechar>(int)GLYPH_BUFFER_SIZE) return;
  File fontFile=LittleFS.open(fontname,"r");
  if(!fontFile) return;
  int ymove=0;
  int xmove=0;
  strlength*=2;
  int i=0;
  while(i<strlength)
  {
      int offset;
      offset=(code[i]*0x100+code[i+1])*sizeofsinglechar;
      if(!fontFile.seek(offset,SeekSet)||
         fontFile.readBytes((char *)glyphBuffer,sizeofsinglechar)!=(size_t)sizeofsinglechar)
      {
        i+=2;
        optimistic_yield(1000UL);
        continue;
      }
      drawXbm(x+xmove,y+ymove,width,height,glyphBuffer);
      if (offset<0xff*sizeofsinglechar&&fontscale==1) 
      {
      ymove+=CurrentCursor+1;
      if((y+ymove+width/2)>=ymax-1) {xmove+=height+1;ymove=0;CurrentCursor=0;}
      }
     else if(offset<0xff*sizeofsinglechar&&fontscale==2) 
     {
      ymove+=CurrentCursor+2;
      if((y+ymove+width)>=ymax-1) {xmove+=height+1;ymove=0;CurrentCursor=0;}
      }
    else if(fontscale==2)
    {
      ymove+=width*2;
      if((y+ymove+width*2)>=ymax-1) {xmove+=height*2+2;ymove=0;CurrentCursor=0;}
      }
    else
    {
      ymove+=width;
      if((y+ymove+width)>=ymax-1) {xmove+=height+1;ymove=0;CurrentCursor=0;}
      }
    i++;i++;
    // Font rendering can involve many LittleFS reads. Let the ESP8266 service
    // its watchdog and the global wake deadline between glyphs.
    optimistic_yield(1000UL);
    }
  
  }
void Duck_EPD::drawXbm(int16_t xMove, int16_t yMove, int16_t width, int16_t height,unsigned char *xbm) {
  const epd_raster::Target target={
    EPDbuffer,sizeof(EPDbuffer),xDot,yDot,RasterLayoutFor(EPD_Type),frame
  };
  const epd_raster::DrawResult result=epd_raster::DrawXbm(
    target,xMove,yMove,width,height,fontscale,xbm,ReadRamXbmByte,
    YieldDuringRasterDraw);
  if(result.hasInk) CurrentCursor=result.cursor;
}

void Duck_EPD::DrawXbm_P(int16_t xMove, int16_t yMove, int16_t width, int16_t height,const unsigned char *xbm) {
  const epd_raster::Target target={
    EPDbuffer,sizeof(EPDbuffer),xDot,yDot,RasterLayoutFor(EPD_Type),frame
  };
  const epd_raster::DrawResult result=epd_raster::DrawXbm(
    target,xMove,yMove,width,height,fontscale,xbm,ReadProgmemXbmByte,
    YieldDuringRasterDraw);
  // Preserve the existing API detail: PROGMEM images at 2x scale do not move
  // the text cursor, while 1x glyphs do.
  if(fontscale==1&&result.hasInk) CurrentCursor=result.cursor;
}

void Duck_EPD::DrawXbm_p_gray(int16_t xMove, int16_t yMove, int16_t width, int16_t height,const unsigned char *xbm,byte level)
{
  int16_t heightInXbm = (height+1)/2;
  uint8_t Data=0;

  
  for(int16_t x = 0; x < width; x++) {
    for(int16_t y = 0; y < height; y++ ) {
      if (y%2!=0) {
        Data <<= 4; // Move a bit
      } else {  // Read new Data every 8 bit
        Data = pgm_read_byte(xbm+(y / 2) + x * heightInXbm);
      }
      // if there is a bit draw it
      if (((Data & 0xf0)>>4==level)) {SetPixel(xMove + y, yMove + x);CurrentCursor=x;}
    }
  }
  
  }
void Duck_EPD::DrawXbm_spiff_gray(int16_t xMove, int16_t yMove, int16_t width, int16_t height,byte level)
{
  File f = LittleFS.open("/pic.xbm", "r"); 
   
  uint8_t Data=0;

  
  for(int16_t x = 0; x < width; x++) {
    for(int16_t y = 0; y < height; y++ ) {
      if (y%2!=0) {
        Data <<= 4; // Move a bit
      } else {  // Read new Data every 8 bit
        Data = ~f.read();
      }
      // if there is a bit draw it
      if (((Data & 0xf0)>>4==level)) {SetPixel(xMove + y, yMove + x);CurrentCursor=x;}
    }
  }
  f.close();
  }
void Duck_EPD::SetPixel(int16_t x, int16_t y)
{ 
    
    if(EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==WF42)
   {
    int16_t temp=x;
    x=y;y=yDot-1-temp;
    if(x>=0&&y>=0&&x<xDot&&y<yDot)
    {
      size_t index=(size_t)x/8+(size_t)y*xDot/8;
      if(index<sizeof(EPDbuffer)) EPDbuffer[index]&=~(0x80>>x%8);
    }
   }
   else if(EPD_Type==WF29BZ03||EPD_Type==WF32)
   {
      if(frame==0)
      {
      if(x>=0&&y>=0&&x<xDot&&y<yDot)
      {
        size_t index=(size_t)x/4+(size_t)y*xDot/4;
        if(index<sizeof(EPDbuffer)) EPDbuffer[index]&=~(0x80>>((x%4)*2));
      }
      }
      else
      {
      if(x>=0&&y>=0&&x<xDot&&y<yDot)
      {
        size_t index=(size_t)x/4+(size_t)y*xDot/4;
        if(index<sizeof(EPDbuffer)) EPDbuffer[index]&=~(0x40>>((x%4)*2));
      }
      }
    }
   else
    {
      if(x>=0&&y>=0&&x<xDot&&y<yDot)
      {
        size_t index=(size_t)x/8+(size_t)y*xDot/8;
        if(index<sizeof(EPDbuffer)) EPDbuffer[index]&=~(0x80>>x%8);
      }
      }
   
  }
void Duck_EPD::InversePixel(int16_t x, int16_t y)
{ 
    
    if(EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==WF42)
   {
    int16_t temp=x;
    x=y;y=yDot-1-temp;
    if(x>=0&&y>=0&&x<xDot&&y<yDot)
    {
      size_t index=(size_t)x/8+(size_t)y*xDot/8;
      if(index<sizeof(EPDbuffer)) EPDbuffer[index]^=(0x80>>x%8);
    }
   }
   else if(EPD_Type==WF29BZ03||EPD_Type==WF32)
   {
    if(frame==0)
      {
      if(x>=0&&y>=0&&x<xDot&&y<yDot)
      {
        size_t index=(size_t)x/4+(size_t)y*xDot/4;
        if(index<sizeof(EPDbuffer)) EPDbuffer[index]^=(0x80>>((x%4)*2));
      }
      }
      else
      {
      if(x>=0&&y>=0&&x<xDot&&y<yDot)
      {
        size_t index=(size_t)x/4+(size_t)y*xDot/4;
        if(index<sizeof(EPDbuffer)) EPDbuffer[index]^=(0x40>>((x%4)*2));
      }
        }
    }
   else
   {
    if(x>=0&&y>=0&&x<xDot&&y<yDot)
    {
      size_t index=(size_t)x/8+(size_t)y*xDot/8;
      if(index<sizeof(EPDbuffer)) EPDbuffer[index]^=(0x80>>x%8);
    }
    //Serial.printf("(%d,%d)\n",x,y);
   }
  }
void Duck_EPD::clearbuffer()
{
  size_t bufferSize;
  if(EPD_Type==WF29BZ03||EPD_Type==WF32) bufferSize=(size_t)xDot*yDot/4;
  else bufferSize=(size_t)xDot*yDot/8;
  if(bufferSize>sizeof(EPDbuffer)) bufferSize=sizeof(EPDbuffer);
  memset(EPDbuffer,0xff,bufferSize);
  }
void Duck_EPD::EPD_Set_Model(byte model)
{
  EPD_Type=epd_type(model);
  
  switch (model)
     {
     case 0:
     xDot=128;yDot=296;break;//WX29
     case 1:
     xDot=128;yDot=296;break;//WF29
     case 2:
     xDot=400;yDot=300;break;//OPM4_2
     case 3:
     xDot=648;yDot=480;break;//WF58
     case 4:
     xDot=128;yDot=296;break;//WF29BZ03
     case 5:
     xDot=152;yDot=152;break;//C154
     case 6:
     xDot=400;yDot=300;break;//DKE42_3COLOR
     case 7:
     xDot=128;yDot=296;break;//DKE29_3COLOR
     case 8:
     xDot=400;yDot=300;break;//WF42
     case 9:
     xDot=200;yDot=300;break;//WF32
     default:
     EPD_Type=WX29;xDot=128;yDot=296;break;
     }
  
  
  }
unsigned char Duck_EPD::ReadBusy(void)
{
  return WaitBusy(200UL,false);
}
unsigned char Duck_EPD::ReadBusy_long(void)
{
  const unsigned long timeoutMs=
    (EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR||EPD_Type==WF29BZ03)
      ?16000UL:8000UL;
  return WaitBusy(timeoutMs,false);
}

unsigned char Duck_EPD::WaitBusy(unsigned long timeoutMs,bool ignorePreviousTimeout)
{
  if(busyTimedOut&&!ignorePreviousTimeout) return 0;
  if(ignorePreviousTimeout) busyTimedOut=false;
  const unsigned long waitStarted=millis();
  while(millis()-waitStarted<timeoutMs){
  //  println("isEPD_BUSY = %d\r\n",isEPD_CS);
if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
{
      if(READ_EPD_BUSY==0) {
        //Serial.println("Busy is Low \r\n");
        return 1;
      }
}
if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF29BZ03||EPD_Type==C154||EPD_Type==WF42||EPD_Type==WF32)
  {
if(READ_EPD_BUSY!=0) {
        //Serial.println("Busy is H \r\n");
        return 1;
      }
  }
    delay(2);
   //Serial.println("epd is Busy");
  }
  busyTimedOut=true;
  return 0;
}
void Duck_EPD::EPD_WriteCMD(unsigned char command)
{ 
  /*if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
  {
  ReadBusy();
  }   */  
	//ReadBusy();
	EPD_CS_0;	
	EPD_DC_0;    // command write
	SPI_Write(command);
	EPD_CS_1;
}
void Duck_EPD::EPD_WriteData (unsigned char Data)
{
 
  EPD_CS_0; 
  EPD_DC_1;     
  SPI_Write(Data);  
  EPD_CS_1;
}

void Duck_EPD::EPD_WriteCMD_p1(unsigned char command,unsigned char para)
{
	/*if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
 {
  ReadBusy();
 }*/
  //ReadBusy();
  EPD_CS_0; 
  EPD_DC_0;    // command write		
	SPI_Write(command);
	EPD_DC_1;		// command write
	SPI_Write(para);
  EPD_CS_1;	
}

bool Duck_EPD::deepsleep(void)
{
  return deepsleepImpl(false);
}

bool Duck_EPD::deepsleepAfterTimedRefresh(void)
{
  return deepsleepImpl(true);
}

bool Duck_EPD::deepsleepImpl(bool refreshAlreadySettled)
{
  const bool operationTimedOut=busyTimedOut;
  const bool timedRefreshPanel=
    EPD_Type==WF32||EPD_Type==DKE42_3COLOR;
  bool shutdownOk=true;

  // A prior BUSY timeout must never suppress the power-off sequence. Reset the
  // controller first so a stalled refresh cannot leave its charge pumps on
  // throughout the ESP deep-sleep interval.
  if(operationTimedOut)
  {
    EPD_RST_0;
    driver_delay_xms(10);
    EPD_RST_1;
    driver_delay_xms(10);
  }

  if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
  {
    if(timedRefreshPanel)
    {
      // The dedicated second wake has already allowed epd_time for the refresh.
      // DKE42 can keep BUSY asserted across that reset, so probing it again only
      // burns active time. Keep the old bounded probe for all other callers.
      if(!refreshAlreadySettled) WaitBusy(200UL,true);
      busyTimedOut=false;
    }
    else
    {
      const unsigned long readyTimeoutMs=operationTimedOut?500UL:
        (EPD_Type==DKE29_3COLOR?16000UL:8000UL);
      if(!WaitBusy(readyTimeoutMs,true)) shutdownOk=false;
    }
	  EPD_WriteCMD_p1(0x10,0x01);
	  //EPD_WriteCMD_p1(0x22,0xc0);//power off
	  //EPD_WriteCMD(0x20);
  }
  if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF29BZ03||EPD_Type==C154||EPD_Type==WF42||EPD_Type==WF32)
  {
    if(timedRefreshPanel)
    {
      // WF32 BUSY is not reliable across the timed deep-sleep reset used for
      // partial refreshes. The second-wake caller already waited epd_time, so
      // it can issue POWER_OFF immediately; retain the old probe elsewhere.
      if(!refreshAlreadySettled) WaitBusy(200UL,true);
      busyTimedOut=false;
    }
    else
    {
      const unsigned long readyTimeoutMs=operationTimedOut?500UL:
        (EPD_Type==WF29BZ03?16000UL:8000UL);
	    if(!WaitBusy(readyTimeoutMs,true)) shutdownOk=false;
    }
	  EPD_WriteCMD(0x50); 
	  EPD_WriteData (0xf7);//border floating
	  EPD_WriteCMD(0x02);//power off
	  if(timedRefreshPanel)
	  {
	    // These controllers can leave BUSY low after POWER_OFF because that
	    // output is no longer driven. Allow a short rail-settle window, then
	    // send DEEP_SLEEP anyway instead of reporting a false shutdown fault.
	    // This is deliberately best-effort; the command above is what disables
	    // the high-current analog section.
	    WaitBusy(refreshAlreadySettled?100UL:500UL,true);
	    busyTimedOut=false;
	  }
	  else if(!WaitBusy(2000UL,true)) shutdownOk=false;
    // Send the sleep command even if BUSY is faulty. It is a best-effort
    // hardware shutdown and the caller will apply a long retry backoff.
	  EPD_WriteCMD(0x07);//sleep
	  EPD_WriteData(0xa5);
  }
  busyTimedOut=operationTimedOut||!shutdownOk;
  return !busyTimedOut;
}


void Duck_EPD::EPD_Write(unsigned char *value, unsigned char Datalen)
{
  if(value==nullptr||Datalen==0) return;
	unsigned char *ptemp;
	ptemp = value;
	
  ReadBusy();
       
	EPD_CS_0;
	EPD_DC_0;		// When DC is 0, write command 
	SPI_Write(*ptemp);	//The first byte is written with the command value
	ptemp++;
	EPD_DC_1;		// When DC is 1, write Data
	SPI_WriteBuffer(ptemp,Datalen-1);
	EPD_CS_1;
}

void Duck_EPD::EPD_WriteDispRam(unsigned int XSize,unsigned int YSize,unsigned char *Dispbuff,unsigned int offset,byte label)
{
  if(EPD_Type==WF58&&label==1) return;
	
	unsigned int i = 0;
	if(EPD_Type==WF29BZ03||EPD_Type==WF32)
  {
      EPD_WriteCMD(0x10);
      EPD_CS_0; 
      EPD_DC_1;   
 /* for(i=0;i<(YSize*2);i++){
    for(j=0;j<(XSize*2);j++){
      SPI_Write(*Dispbuff);
      Dispbuff++;
    }  
  } */
  if(label!=1)
  {
    SPI_WriteRepeat(label,(size_t)YSize*(XSize*2));
    }
  else
  {
    Dispbuff+=offset;
    const unsigned int rowBytes=XSize*2;
    const unsigned int rowStride=xDot/4;
    const bool contiguousFullFrame=EPD_Type==WF32&&offset==0&&
      rowBytes==rowStride&&YSize==(unsigned int)yDot;
    if(contiguousFullFrame)
    {
      // A full WF32 frame is contiguous. transferBytes() still
      // handles the ESP8266 FIFO in 64-byte chunks, without 300 row calls.
      SPI_WriteBuffer(Dispbuff,(size_t)rowBytes*YSize);
    }
    else
    {
      // Partial windows contain a gap between rows and must remain strided.
      for(i=0;i<YSize;i++){
        SPI_WriteBuffer(Dispbuff,rowBytes);
        Dispbuff+=rowBytes;
        if(rowBytes<rowStride) Dispbuff+=rowStride-rowBytes;
      }
    }
  }
    EPD_CS_1; 
    return;
  }
	
  
  if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==C154||EPD_Type==WF42)
  {
      /*EPD_WriteCMD(0x10);
      EPD_CS_0; 
      EPD_DC_1;
     for(i=0;i<YSize;i++){
    for(j=0;j<XSize;j++){
      SPI_Write(0xFF);      
       }
     }    
    EPD_CS_1; */
  }
  if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
  {
  ReadBusy();  
	EPD_DC_0;    //command write
	EPD_CS_0;	
	SPI_Write(0x24);
  }
 
 if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==C154||EPD_Type==WF42)
  {
  EPD_CS_0;  
  EPD_DC_0;   //command write
  SPI_Write(0x13);
  }
	EPD_DC_1;		//Data write
  //Serial.printf("Xsize=%dYsize=%d\r\n",XSize,YSize);
 
  if(label!=1)
  {
    SPI_WriteRepeat(label,(size_t)YSize*XSize);
    }
  else
  {
    Dispbuff+=offset;
	for(i=0;i<YSize;i++){
    SPI_WriteBuffer(Dispbuff,XSize);
    Dispbuff+=XSize;
   Dispbuff+=xDot/8-XSize;
	}
  }

  
	EPD_CS_1;

}
void Duck_EPD::EPD_WriteDispRam_RED(unsigned int XSize,unsigned int YSize,unsigned char *Dispbuff,unsigned int offset,byte label)
{
  if(EPD_Type==WF58&&label==1) return;
  
  unsigned int i = 0;
  if(EPD_Type==WF29BZ03||EPD_Type==WF32)
  {
      EPD_WriteCMD(0x10);
      EPD_CS_0; 
      EPD_DC_1;
      const unsigned int rowBytes=XSize*2;
      const unsigned int rowStride=xDot/4;
      if(label!=1)
      {
        SPI_WriteRepeat(label,(size_t)YSize*rowBytes);
      }
      else
      {
        Dispbuff+=offset;
        for(i=0;i<YSize;i++){
          SPI_WriteBuffer(Dispbuff,rowBytes);
          Dispbuff+=rowBytes;
          if(rowBytes<rowStride) Dispbuff+=rowStride-rowBytes;
        }
      }
    EPD_CS_1; 
    return;
  }
  
  
  if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF42)
  {
      EPD_WriteCMD(0x10);
      EPD_CS_0; 
      EPD_DC_1;
    SPI_WriteRepeat(0x00,(size_t)YSize*XSize);
    EPD_CS_1; 
  }
  if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
  {
  ReadBusy();  
  EPD_DC_0;    //command write
  EPD_CS_0; 
  SPI_Write(0x26);
  }
 
 if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF42)
  {
  EPD_CS_0;  
  EPD_DC_0;   //command write
  SPI_Write(0x13);
  }
  EPD_DC_1;   //Data write
  //Serial.printf("Xsize=%dYsize=%d\r\n",XSize,YSize);
 
  if(label!=1)
  {
    SPI_WriteRepeat(label,(size_t)YSize*XSize);
    }
  else
  {
    Dispbuff+=offset;
  for(i=0;i<YSize;i++){
    SPI_WriteBufferInverted(Dispbuff,XSize);
    Dispbuff+=XSize;
   Dispbuff+=xDot/8-XSize;
  }
  }

  
  EPD_CS_1;

}
void Duck_EPD::EPD_WriteDispRam_Old(unsigned int XSize,unsigned int YSize,unsigned char *Dispbuff,unsigned int offset,byte label)
{
  if((EPD_Type==WF58&&label==1)||EPD_Type==C154) return;
  
  unsigned int i = 0;
  if(EPD_Type==WF29BZ03||EPD_Type==WF32)
  {
      EPD_WriteCMD(0x10);
      EPD_CS_0; 
      EPD_DC_1;
      const unsigned int rowBytes=XSize*2;
      const unsigned int rowStride=xDot/4;
      if(label!=1)
      {
        SPI_WriteRepeat(label,(size_t)YSize*rowBytes);
      }
      else
      {
        Dispbuff+=offset;
        const bool contiguousFullFrame=EPD_Type==WF32&&offset==0&&
          rowBytes==rowStride&&YSize==(unsigned int)yDot;
        if(contiguousFullFrame)
        {
          SPI_WriteBuffer(Dispbuff,(size_t)rowBytes*YSize);
        }
        else
        {
          for(i=0;i<YSize;i++){
            SPI_WriteBuffer(Dispbuff,rowBytes);
            Dispbuff+=rowBytes;
            if(rowBytes<rowStride) Dispbuff+=rowStride-rowBytes;
          }
        }
      }
    EPD_CS_1; 
    return;
  }
  
  
  /*if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF42)
  {
      EPD_WriteCMD(0x10);
      EPD_CS_0; 
      EPD_DC_1;
     for(i=0;i<YSize;i++){
    for(j=0;j<XSize;j++){
      SPI_Write(0x00);      
       }
     }    
    EPD_CS_1; 
  }*/
  if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
  {
  ReadBusy();  
  EPD_DC_0;    //command write
  EPD_CS_0; 
  SPI_Write(0x26);
  }
 
 if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF42)
  {
  ReadBusy();  
  EPD_CS_0;  
  EPD_DC_0;   //command write
  SPI_Write(0x10);
  }
  EPD_DC_1;   //Data write
  //Serial.printf("Xsize=%dYsize=%d\r\n",XSize,YSize);
 
  if(label!=1)
  {
    SPI_WriteRepeat(label,(size_t)YSize*XSize);
    }
  else
  {
    Dispbuff+=offset;
  for(i=0;i<YSize;i++){
    SPI_WriteBuffer(Dispbuff,XSize);
    Dispbuff+=XSize;
   Dispbuff+=xDot/8-XSize;
  }
  }

  
  EPD_CS_1;

}



void Duck_EPD::EPD_SetRamArea(uint16_t Xstart,uint16_t Xend,
						unsigned char Ystart,unsigned char Ystart1,unsigned char Yend,unsigned char Yend1)
{
  if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
  {
  unsigned char RamAreaX[3];	// X start and end
	unsigned char RamAreaY[5]; 	// Y start and end
	RamAreaX[0] = 0x44;	// command
	RamAreaX[1] = Xstart/8;
	RamAreaX[2] = Xend/8;
	RamAreaY[0] = 0x45;	// command
	RamAreaY[1] = Ystart;
	RamAreaY[2] = Ystart1;
	RamAreaY[3] = Yend;
  RamAreaY[4] = Yend1;
	EPD_Write(RamAreaX, sizeof(RamAreaX));
	EPD_Write(RamAreaY, sizeof(RamAreaY));
  //Serial.printf("set ram area%d %d %d %d %d %d %d %d %d\n",RamAreaX[0],RamAreaX[1],RamAreaX[2],RamAreaY[0],RamAreaY[1],RamAreaY[2],RamAreaY[3],RamAreaY[4]);

  }
  if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF42||EPD_Type==WF29BZ03||EPD_Type==WF32)
  {
      EPD_WriteCMD(0x91); //enter partial refresh mode
      EPD_WriteCMD(0x90); 
      if(EPD_Type==WF42||EPD_Type==WF32)
      {
      EPD_WriteData (Xstart/256);
      EPD_WriteData (Xstart%256);
      EPD_WriteData (Xend/256);
      EPD_WriteData (Xend%256);
      EPD_WriteData (Ystart);
      EPD_WriteData (Ystart1);
      EPD_WriteData (Yend);
      EPD_WriteData (Yend1);
      EPD_WriteData (0x0);      
        }
      else
      {
      EPD_WriteData (Xstart);
      EPD_WriteData (Xend);
      EPD_WriteData (Ystart);
      EPD_WriteData (Ystart1);
      EPD_WriteData (Yend);
      EPD_WriteData (Yend1);
      EPD_WriteData (0x0);
      }
  }
}

void Duck_EPD::EPD_SetRamPointer(uint16_t addrX,unsigned char addrY,unsigned char addrY1)
{
  unsigned char RamPointerX[2];  // default (0,0)
  unsigned char RamPointerY[3];
  //Set RAM X address counter
  RamPointerX[0] = 0x4e;
  RamPointerX[1] = addrX;
  //RamPointerX[1] = 0xff;
  //Set RAM Y address counter
  RamPointerY[0] = 0x4f;
  RamPointerY[1] = addrY;
  RamPointerY[2] = addrY1;
  //RamPointerY[1] = 0x2b;
  //RamPointerY[2] = 0x02;
  
  EPD_Write(RamPointerX, sizeof(RamPointerX));
  EPD_Write(RamPointerY, sizeof(RamPointerY));
}


void Duck_EPD::EPD_Init(void)
{
  busyTimedOut=false;
  EPD_RST_0;   
  driver_delay_xms(10);  
  EPD_RST_1;
  //driver_delay_xms(100);
	if(EPD_Type==WX29)
  {
    EPD_Write(GateVol, sizeof(GateVol));
  	EPD_Write(GDOControl, sizeof(GDOControl));	// Pannel configuration, Gate selection
  	EPD_Write(softstart, sizeof(softstart));	// X decrease, Y decrease
  	EPD_Write(VCOMVol, sizeof(VCOMVol));		// VCOM setting
  	EPD_Write(DummyLine, sizeof(DummyLine));	// dummy line per gate
  	EPD_Write(Gatetime, sizeof(Gatetime));		// Gage time setting
  	EPD_Write(RamDataEntryMode, sizeof(RamDataEntryMode));	// X increase, Y decrease
    EPD_WriteCMD_p1(0x22,0xc0);//poweron
    EPD_WriteCMD(0x20); 
  }

  if(EPD_Type==WF29BZ03)
  {
    EPD_WriteCMD(0x01); 
     EPD_WriteData(0x07);      
     EPD_WriteData(0x00);
     EPD_WriteData(0x0B);      
     EPD_WriteData(0x00);
    
     EPD_WriteCMD(0x06);         
     EPD_WriteData(0x07);
     EPD_WriteData(0x07);
     EPD_WriteData(0x07);
    
     EPD_WriteCMD(0x04);         
     ReadBusy();          
    
     EPD_WriteCMD(0X00);
     EPD_WriteData(0xb7);    //b7
    
     EPD_WriteCMD(0X50);
     EPD_WriteData(0x37);
    
        EPD_WriteCMD(0x30);     
        EPD_WriteData(0x39);    
        
        EPD_WriteCMD(0x61);     
        EPD_WriteData(0x80);    
        EPD_WriteData(0x01);    
        EPD_WriteData(0x28);
        
        EPD_WriteCMD(0x82);     
        EPD_WriteData(0x00);  
    
    }
    if(EPD_Type==WF32)
  {
    EPD_WriteCMD(0x01); 
     EPD_WriteData(0x07);      
     EPD_WriteData(0x00);
     EPD_WriteData(0x0B);      
     EPD_WriteData(0x00);
    
     EPD_WriteCMD(0x06);         
     EPD_WriteData(0x17);
     EPD_WriteData(0x17);
     EPD_WriteData(0x17);
    
     EPD_WriteCMD(0x04);         
     //delay(8);
     // WF32 power-on can exceed the generic 200 ms poll at low temperature or
     // voltage. A bounded 1.5 s wait avoids a false sticky timeout while still
     // keeping the wake path deterministic.
     WaitBusy(1500UL,false);
    
     EPD_WriteCMD(0X00);
     EPD_WriteData(0xf7);    //b7
    
     EPD_WriteCMD(0X50);
     EPD_WriteData(0x37); //37
    
        EPD_WriteCMD(0x30);     
        EPD_WriteData(0x39);    
        
        //EPD_WriteCMD(0x61);     
        //EPD_WriteData(0x80);    
        //EPD_WriteData(0x01);    
        //EPD_WriteData(0x28);
        
        EPD_WriteCMD(0x82);     
        EPD_WriteData(0x00);  
    
    }
	if(EPD_Type==WF29)
	{
     
     EPD_WriteCMD(0x01);     
     EPD_WriteData (0x03);
     EPD_WriteData (0x00);
     EPD_WriteData (0x26);//default26 max2b
     EPD_WriteData (0x26);//default26 max2b
     EPD_WriteData (0x03);
     
     EPD_WriteCMD(0x06);
     EPD_WriteData (0x17);
     EPD_WriteData (0x17);
     
     EPD_WriteCMD(0x04);
     //
     
     EPD_WriteCMD(0x00); 
     EPD_WriteData (0xb7);
      
     EPD_WriteCMD(0x30);  //PLL
     EPD_WriteData (0x3a);
     
     EPD_WriteCMD(0x61); //resolution
     EPD_WriteData (0x80);
     EPD_WriteData (0x01);
     EPD_WriteData (0x28);
     
     EPD_WriteCMD(0X82);
     EPD_WriteData (0x20);
     
     EPD_WriteCMD(0X50);
     EPD_WriteData (0x97);
	}
  if(EPD_Type==WF42)
  {
     
     EPD_WriteCMD(0x01);     
     EPD_WriteData (0x03);
     EPD_WriteData (0x00);
     EPD_WriteData (0x2f);//default26 max2b
     EPD_WriteData (0x2f);//default26 max2b
     EPD_WriteData (0xff);
     
     EPD_WriteCMD(0x06);
     EPD_WriteData (0x17);
     EPD_WriteData (0x17);
     EPD_WriteData (0x17);
     
     EPD_WriteCMD(0x04);
     ReadBusy();          
    
     
     EPD_WriteCMD(0x00); 
     EPD_WriteData (0xb7);//b7 a7
     EPD_WriteData (0x0b);
      
     EPD_WriteCMD(0x30);  //PLL
     EPD_WriteData (0x3c);
     
     //EPD_WriteCMD(0x61); //resolution
     //EPD_WriteData (0x80);
     //EPD_WriteData (0x01);
     //EPD_WriteData (0x28);
     
     EPD_WriteCMD(0X82);
     EPD_WriteData (0x12);
     
     EPD_WriteCMD(0X50);
     EPD_WriteData (0x97);
  }
  if(EPD_Type==WF58)
  {
    EPD_WriteCMD(0x01); 
    EPD_WriteData (0x37);     //POWER SETTING
    EPD_WriteData (0x00);

    EPD_WriteCMD(0X00);     //PANNEL SETTING
    EPD_WriteData(0xb7);
    //EPD_WriteData(0x08);
    
    EPD_WriteCMD(0x30);     //PLL setting
    EPD_WriteData(0x3a);   //PLL:    0-15¦:0x3C, 15+:0x3A  
    EPD_WriteCMD(0X82);     //VCOM VOLTAGE SETTING
    EPD_WriteData(0x28);    //all temperature  range
    
    EPD_WriteCMD(0x06);         //boost
    EPD_WriteData (0xc7);     
    EPD_WriteData (0xcc);
    EPD_WriteData (0x28);

    EPD_WriteCMD(0X50);     //VCOM AND Data INTERVAL SETTING
    EPD_WriteData(0x77);

    EPD_WriteCMD(0X60);     //TCON SETTING
    EPD_WriteData(0x22);

    EPD_WriteCMD(0X65);     //FLASH CONTROL
    EPD_WriteData(0x00);

    EPD_WriteCMD(0x61);         //tres      
    EPD_WriteData (0x02);   //source 600
    EPD_WriteData (0x88);
    EPD_WriteData (0x01);   //gate 448
    EPD_WriteData (0xe0);

    EPD_WriteCMD(0xe5);     //FLASH MODE        
    EPD_WriteData(0x03);  

    EPD_WriteCMD(0X00);     //PANNEL SETTING
    EPD_WriteData(0x17);
    
    EPD_WriteCMD(0x04);       //POWER ON
    ReadBusy();
  }
  /*if(EPD_Type==99)//dke
  {
    ReadBusy();   
    EPD_WriteCMD(0x12);     //SWRESET
    ReadBusy();   
  
    EPD_WriteCMD(0x74);
    EPD_WriteData(0x54);
    EPD_WriteCMD(0x7E);
    EPD_WriteData(0x3B);
    EPD_WriteCMD(0x2B);  // Reduce glitch under ACVCOM  
    EPD_WriteData(0x04);           
    EPD_WriteData(0x63);

    EPD_WriteCMD(0x0C);  // Soft start setting
    EPD_WriteData(0x8B);           
    EPD_WriteData(0x9C);
    EPD_WriteData(0x96);
    EPD_WriteData(0x0F);

    EPD_WriteCMD(0x01);  // Set MUX as 300
    EPD_WriteData(0x2B);           
    EPD_WriteData(0x01);
    EPD_WriteData(0x00);     

    EPD_WriteCMD(0x11);  // Data entry mode
    EPD_WriteData(0x01);         
    EPD_WriteCMD(0x44); 
    EPD_WriteData(0x00); // RAM x address start at 0
    EPD_WriteData(0x31); // RAM x address end at 31h(49+1)*8->400
    EPD_WriteCMD(0x45); 
    EPD_WriteData(0x2B);   // RAM y address start at 12Bh     
    EPD_WriteData(0x01);
    EPD_WriteData(0x00); // RAM y address end at 00h     
    EPD_WriteData(0x00);
    EPD_WriteCMD(0x3C); // board
    EPD_WriteData(0x01); // HIZ

    EPD_WriteCMD(0x18);
    EPD_WriteData(0X80);
    EPD_WriteCMD(0x22);
    EPD_WriteData(0XB1);  //Load Temperature and waveform setting.
    EPD_WriteCMD(0x20);
    ReadBusy();   
    

    EPD_WriteCMD(0x4E); 
    EPD_WriteData(0x00);
    EPD_WriteCMD(0x4F); 
    EPD_WriteData(0x2B);
    EPD_WriteData(0x01);
    
    }*/
  if(EPD_Type==OPM42||EPD_Type==DKE42_3COLOR)
  {
    EPD_WriteCMD(0x74);       // 
    EPD_WriteData(0x54);    // 
    EPD_WriteCMD(0x7E);       // 
    EPD_WriteData(0x3B);    // 
    EPD_WriteCMD(0x01);       // 
    EPD_WriteData(0x2B);    // 
    EPD_WriteData(0x01);
    EPD_WriteData(0x00);    // 

    EPD_WriteCMD(0x0C);       // 
    EPD_WriteData(0x8B);    // 
    EPD_WriteData(0x9C);    // 
    EPD_WriteData(0xD6);    //     
    EPD_WriteData(0x0F);    // 

    EPD_WriteCMD(0x3A);       // 
    EPD_WriteData(0x21);    // 
    EPD_WriteCMD(0x3B);       // 
    EPD_WriteData(0x06);    // 
    EPD_WriteCMD(0x3C);       // 
    EPD_WriteData(0x03);    // 

    EPD_WriteCMD(0x11);       // data enter mode
    EPD_WriteData(0x01);    // 01 –Y decrement, X increment,

    EPD_WriteCMD(0x2C);       // 
    EPD_WriteData(0x00);    //fff


    EPD_WriteCMD(0x37);       // 
    EPD_WriteData(0x00);    // 
    EPD_WriteData(0x00);    // 
    EPD_WriteData(0x00);    // 
    EPD_WriteData(0x00);    // 
    EPD_WriteData(0x80);    // 
  
    EPD_WriteCMD(0x21);       // 
    EPD_WriteData(0x40);    // 
    EPD_WriteCMD(0x22);
    EPD_WriteData(0xc7);    //c5forgraymode// 
  }
  if(EPD_Type==C154)
  {
  EPD_WriteCMD(0x01);
  EPD_WriteData(0x03);
  EPD_WriteData(0x00);
  EPD_WriteData(0x2b);
  EPD_WriteData(0x26);
  EPD_WriteData(0x03);
  
  EPD_WriteCMD(0x06); //boost soft start
  EPD_WriteData(0x17);
  EPD_WriteData(0x17);
  EPD_WriteData(0x17);
  EPD_WriteCMD(0x04);

  ReadBusy();

  EPD_WriteCMD(0x00); //panel setting
  EPD_WriteData(0xff); //LUT from OTP，160x296
  EPD_WriteData(0x0d); //VCOM to 0V fast

  EPD_WriteCMD(0x61); //resolution setting
  EPD_WriteData(0x98); //152
  EPD_WriteData(0x00); //152
  EPD_WriteData(0x98);

  EPD_WriteCMD(0X30);
  EPD_WriteData(0x29);

  EPD_WriteCMD(0X50); //VCOM AND DATA INTERVAL SETTING     
  EPD_WriteData(0x97); //WBmode:VBDF 17|D7 VBDW 97 VBDB 57   WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
  ReadBusy();  
    }
  if(EPD_Type==DKE29_3COLOR)
  {
  ReadBusy();  
  EPD_WriteCMD(0x12);  //SWRESET
  ReadBusy();   
    
  EPD_WriteCMD(0x01); //Driver output control      
  EPD_WriteData(0x27);
  EPD_WriteData(0x01);
  EPD_WriteData(0x00);

  EPD_WriteCMD(0x11); //data entry mode       
  EPD_WriteData(0x01);

  EPD_WriteCMD(0x44); //set Ram-X address start/end position   
  EPD_WriteData(0x00);
  EPD_WriteData(0x0F);    //0x0F-->(15+1)*8=128

  EPD_WriteCMD(0x45); //set Ram-Y address start/end position          
  EPD_WriteData(0x27);   //0x0127-->(295+1)=296
  EPD_WriteData(0x01);
  EPD_WriteData(0x00);
  EPD_WriteData(0x00); 

  EPD_WriteCMD(0x3C); //BorderWavefrom
  EPD_WriteData(0x05);  
      
  EPD_WriteCMD(0x18); //Read built-in temperature sensor
  EPD_WriteData(0x80);  
  
  EPD_WriteCMD(0x21); //  Display update control
  EPD_WriteData(0x00);  
  EPD_WriteData(0x80);  

  EPD_WriteCMD(0x4E);   // set RAM x address count to 0;
  EPD_WriteData(0x00);
  EPD_WriteCMD(0x4F);   // set RAM y address count to 0X199;    
  EPD_WriteData(0x27);
  EPD_WriteData(0x01);
  ReadBusy();    
  }
  
}
void Duck_EPD::EPD_Set_Contrast(byte vcom)
{   if(EPD_Type==OPM42)
  {
    EPD_WriteCMD(0x2C);       // 
    EPD_WriteData(vcom);    //fff
  }
  }

void Duck_EPD::EPD_Update(void)
{
   if(EPD_Type==OPM42)
   {
    EPD_WriteCMD(0x20);
   }
   if(EPD_Type==DKE42_3COLOR)
   {
    EPD_WriteCMD(0x22); //Display Update Control
    EPD_WriteData(0xC7);   
    EPD_WriteCMD(0x20);  //Activate Display Update Sequence
    }
  	if(EPD_Type==WX29)
    {
  	EPD_WriteCMD_p1(0x22,0x04);
  	EPD_WriteCMD(0x20);
    }
    if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF29BZ03||EPD_Type==C154||EPD_Type==WF42||EPD_Type==WF32)
  	{
  	EPD_WriteCMD(0x12);
  	}
    if(EPD_Type==DKE29_3COLOR)
    {
    EPD_WriteCMD(0x22); //Display Update Control
    EPD_WriteData(0xc7);   
    EPD_WriteCMD(0x20);  //Activate Display Update Sequence
    ReadBusy();
      }
}
void Duck_EPD::EPD_Update_Part(void)
{
  if(EPD_Type==DKE29_3COLOR)
  {
    EPD_WriteCMD(0x22); //Display Update Control
    EPD_WriteData(0xc7);   
    EPD_WriteCMD(0x20);  //Activate Display Update Sequence
    }
  if(EPD_Type==OPM42||EPD_Type==DKE42_3COLOR)
   {
  EPD_WriteCMD(0x20);
   }
 if(EPD_Type==WX29)
  {
	EPD_WriteCMD_p1(0x22,0x04);
	EPD_WriteCMD(0x20);
  }
 if(EPD_Type==WF29||EPD_Type==WF42)
 {
  EPD_WriteCMD(0x92);
  EPD_WriteCMD(0x12); 
  
 }
}

void Duck_EPD::EPD_init_Full(void)
{	
  EPD_Init();

	if(EPD_Type==WX29)
   {	
	EPD_Write((unsigned char *)LUTDefault_full,sizeof(LUTDefault_full));
   }
	if(EPD_Type==OPM42)
 {
  EPD_Write((unsigned char *)LUTDefault_full_opm42,sizeof(LUTDefault_full_opm42));
  }

  if(EPD_Type==DKE42_3COLOR)
 {
  EPD_Write((unsigned char *)LUTDefault_full_dke42,sizeof(LUTDefault_full_dke42));
  }
	if(EPD_Type==WF29BZ03)
  {
  EPD_Write((unsigned char *)lut_vcomDC_bz03,sizeof(lut_vcomDC_bz03));
  EPD_Write((unsigned char *)lut_ww_bz03,sizeof(lut_ww_bz03));
  EPD_Write((unsigned char *)lut_bw_bz03,sizeof(lut_bw_bz03));
  EPD_Write((unsigned char *)lut_wb_bz03,sizeof(lut_wb_bz03));
  EPD_Write((unsigned char *)lut_bb_bz03,sizeof(lut_bb_bz03));
  }
	if(EPD_Type==WF32)
  {
  EPD_Write((unsigned char *)lut_vcomDC_wf32,sizeof(lut_vcomDC_wf32));
  EPD_Write((unsigned char *)lut_ww_wf32,sizeof(lut_ww_wf32));
  EPD_Write((unsigned char *)lut_bw_wf32,sizeof(lut_bw_wf32));
  EPD_Write((unsigned char *)lut_wb_wf32,sizeof(lut_wb_wf32));
  EPD_Write((unsigned char *)lut_bb_wf32,sizeof(lut_bb_wf32));
  }
  if(EPD_Type==WF42)
  {  
  EPD_Write((unsigned char *)lut_wf42_vcomDC,sizeof(lut_wf42_vcomDC));
  EPD_Write((unsigned char *)lut_wf42_ww,sizeof(lut_wf42_ww));
  EPD_Write((unsigned char *)lut_wf42_bw,sizeof(lut_wf42_bw));
  EPD_Write((unsigned char *)lut_wf42_wb,sizeof(lut_wf42_wb));
  EPD_Write((unsigned char *)lut_wf42_bb,sizeof(lut_wf42_bb));
  }
  if(EPD_Type==C154)
  {
  EPD_Write((unsigned char *)lut_vcomDC_154,sizeof(lut_vcomDC_154));
  EPD_Write((unsigned char *)lut_ww_154,sizeof(lut_ww_154));
  EPD_Write((unsigned char *)lut_bw_154,sizeof(lut_bw_154));
  EPD_Write((unsigned char *)lut_wb_154,sizeof(lut_wb_154));
  EPD_Write((unsigned char *)lut_bb_154,sizeof(lut_bb_154));
    
    }
  if(EPD_Type==DKE29_3COLOR)
  {
   EPD_Write((unsigned char *)LUTDefault_full_dke29,sizeof(LUTDefault_full_dke29));
  }
}


void Duck_EPD::EPD_init_Part(void)
{	
  
  if(EPD_Type==WX29)
   {
	EPD_Init();			// display
	EPD_Write((unsigned char *)LUTDefault_part,sizeof(LUTDefault_part));
	EPD_WriteCMD_p1(0x22,0xc0);//poweron
  EPD_WriteCMD(0x20);   		
   }
	if(EPD_Type==OPM42)
 {
  EPD_Init();
  EPD_WriteCMD(0x21); 
  EPD_WriteData(0x00);
  EPD_Write((unsigned char *)LUTDefault_part_opm42,sizeof(LUTDefault_part_opm42));
  }
  if(EPD_Type==DKE42_3COLOR)
 {
  EPD_Init();
  EPD_WriteCMD(0x21); 
  EPD_WriteData(0x00);
  EPD_Write((unsigned char *)LUTDefault_part_dke42,sizeof(LUTDefault_part_dke42));
  }
	if(EPD_Type==WF29)
  {  
	  EPD_Init();
	  EPD_WriteCMD(0x50); 
    EPD_WriteData(0xb7);
    EPD_Write((unsigned char *)lut_vcomDC1,sizeof(lut_vcomDC1));
    EPD_Write((unsigned char *)lut_ww1,sizeof(lut_ww1));
    EPD_Write((unsigned char *)lut_bw1,sizeof(lut_bw1));
    EPD_Write((unsigned char *)lut_wb1,sizeof(lut_wb1));
    EPD_Write((unsigned char *)lut_bb1,sizeof(lut_bb1));
  }
  if(EPD_Type==WF32)
  {
    EPD_Init();
    EPD_Write((unsigned char *)lut_vcomDC_part_wf32,sizeof(lut_vcomDC_part_wf32));
    EPD_Write((unsigned char *)lut_ww_part_wf32,sizeof(lut_ww_part_wf32));
    EPD_Write((unsigned char *)lut_bw_part_wf32,sizeof(lut_bw_part_wf32));
    EPD_Write((unsigned char *)lut_wb_part_wf32,sizeof(lut_wb_part_wf32));
    EPD_Write((unsigned char *)lut_bb_part_wf32,sizeof(lut_bb_part_wf32));
    }
  if(EPD_Type==WF42)
  {  
    EPD_Init();
    //EPD_WriteCMD(0x50); 
    //EPD_WriteData(0xb7);
    EPD_Write((unsigned char *)lut_part_wf42_vcomDC,sizeof(lut_part_wf42_vcomDC));
    EPD_Write((unsigned char *)lut_part_wf42_ww,sizeof(lut_part_wf42_ww));
    EPD_Write((unsigned char *)lut_part_wf42_bw,sizeof(lut_part_wf42_bw));
    EPD_Write((unsigned char *)lut_part_wf42_wb,sizeof(lut_part_wf42_wb));
    EPD_Write((unsigned char *)lut_part_wf42_bb,sizeof(lut_part_wf42_bb));
  }
  if(EPD_Type==DKE29_3COLOR)
  {
  EPD_Init();
  EPD_WriteCMD(0x21); 
  EPD_WriteData(0x00);
  EPD_Write((unsigned char *)LUTDefault_part_dke29,sizeof(LUTDefault_part_dke29));
    }
  if(EPD_Type==WF29BZ03)
  {
   EPD_Init();
    EPD_Write((unsigned char *)lut_vcomDC_part_wf32,sizeof(lut_vcomDC_part_wf32));
    EPD_Write((unsigned char *)lut_ww_part_wf32,sizeof(lut_ww_part_wf32));
    EPD_Write((unsigned char *)lut_bw_part_wf32,sizeof(lut_bw_part_wf32));
    EPD_Write((unsigned char *)lut_wb_part_wf32,sizeof(lut_wb_part_wf32));
    EPD_Write((unsigned char *)lut_bb_part_wf32,sizeof(lut_bb_part_wf32));
  }
  
}
void Duck_EPD::EPD_Transfer_Full_BW(unsigned char *DisBuffer,unsigned char Label)
{if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
    {
  unsigned int yStart=0;
  unsigned int yEnd=yDot-1;
  unsigned int xStart=0;
  unsigned int xEnd=xDot-1;
  unsigned long temp=yStart;
 
  yStart=yDot-1-yEnd;yEnd=yDot-1-temp;
  EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
  EPD_SetRamArea(xStart,xEnd,yEnd%256,yEnd/256,yStart%256,yStart/256);
 
  if(Label == 2){
    EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer, 0,0x00); // white  
  }
  else if(Label==3)
  {
    EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,0xff);  // black
    }
  else if(Label==4)
  {
    EPD_WriteDispRam_Old(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);  // black
    }
  else{
    EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,1); 
  } 
  //EPD_Update();
  //ReadBusy_long();
  //EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,1); 
    }
  
  if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF29BZ03||EPD_Type==C154||EPD_Type==WF42||EPD_Type==WF32)
  {
  if(Label == 2){
    EPD_WriteDispRam(xDot/8, yDot,(unsigned char *)DisBuffer,0, 0xff); // white  
  }
  else if(Label==3)
  {
    EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,0x00);  // black
   }
  else if(Label==4)
  {
    EPD_WriteDispRam_Old(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);  // black
    }
  else{
    EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,1); // white
  } 
  //EPD_Update(); 
  }
    
}
void Duck_EPD::EPD_Transfer_Full_RED(unsigned char *DisBuffer,unsigned char Label)
{
  if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
    {
  unsigned int yStart=0;
  unsigned int yEnd=yDot-1;
  unsigned int xStart=0;
  unsigned int xEnd=xDot-1;
  unsigned long temp=yStart;
 
  yStart=yDot-1-yEnd;yEnd=yDot-1-temp;
  EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
  EPD_SetRamArea(xStart,xEnd,yEnd%256,yEnd/256,yStart%256,yStart/256);
 
  if(Label == 2){
    EPD_WriteDispRam_RED(xDot/8, yDot, (unsigned char *)DisBuffer, 0,0x00); // white  
  }
  else if(Label==3)
  {
    EPD_WriteDispRam_RED(xDot/8, yDot, (unsigned char *)DisBuffer,0,0xff);  // black
    }
  else{
    EPD_WriteDispRam_RED(xDot/8, yDot, (unsigned char *)DisBuffer,0,1); 
  } 
  //EPD_Update();
  //ReadBusy_long();
  //EPD_WriteDispRam_Old(xDot/8, yDot, (unsigned char *)DisBuffer,0,1); 
    }
  
  if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF29BZ03||EPD_Type==C154||EPD_Type==WF42||EPD_Type==WF32)
  {
  if(Label == 2){
    EPD_WriteDispRam_RED(xDot/8, yDot,(unsigned char *)DisBuffer,0, 0xff); // white  
  }
  else if(Label==3)
  {
    EPD_WriteDispRam_RED(xDot/8, yDot, (unsigned char *)DisBuffer,0,0x00);  // black
    }else{
    EPD_WriteDispRam_RED(xDot/8, yDot, (unsigned char *)DisBuffer,0,1); // white
  } 
  //EPD_Update(); 
  }
    
}
void Duck_EPD::EPD_Dis_Full(unsigned char *DisBuffer,unsigned char Label)
{
	if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
    {
  unsigned int yStart=0;
  unsigned int yEnd=yDot-1;
  unsigned int xStart=0;
  unsigned int xEnd=xDot-1;
  unsigned long temp=yStart;
 
  yStart=yDot-1-yEnd;yEnd=yDot-1-temp;
  EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
  EPD_SetRamArea(xStart,xEnd,yEnd%256,yEnd/256,yStart%256,yStart/256);
 
	if(Label == 2){
		EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer, 0,0x00);	// white	
	}
	else if(Label==3)
	{
	  EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,0xff);  // black
	  }
	else{
		EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);	
	}	
	EPD_Update();
  if(!ReadBusy_long()) return;
  if(EPD_Type==DKE29_3COLOR)
  {
    //EPD_Transfer_Full_RED((unsigned char *)EPDbuffer,1);
    EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
    EPD_WriteDispRam_Old(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);
  }
  else
  {
  EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
  EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);
  }
    }
	
	if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF29BZ03||EPD_Type==C154||EPD_Type==WF42||EPD_Type==WF32)
  {
	if(Label == 2){
    EPD_WriteDispRam(xDot/8, yDot,(unsigned char *)DisBuffer,0, 0xff); // white  
  }
  else if(Label==3)
  {
    EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,0x00);  // black
    }
  else{
		EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);	
	}	
  if(!ReadBusy()) return;
  //EPD_WriteDispRam_Old(xDot/8, yDot, (unsigned char *)DisBuffer,0,1); 
  EPD_Update();
  /*if(EPD_Type==WF32)
    {
    WiFi.mode(WIFI_OFF);      
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    gpio_pin_wakeup_enable(GPIO_ID_PIN(4), GPIO_PIN_INTR_HILEVEL);
    wifi_fpm_open();
    wifi_fpm_do_sleep(0xFFFFFFF);  // only 0xFFFFFFF, any other value and it won't disconnect the RTC timer
    delay(10);
    }	*/
  if(!ReadBusy_long()) return;
  EPD_WriteDispRam_Old(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);  
  }
	  
}


void Duck_EPD::EPD_Dis_Part(int xStart,int xEnd,int yStart,int yEnd,unsigned char *DisBuffer,unsigned char Label)
{
  //EPD.EPD_Dis_Part(16,87,237,399,(unsigned char *)EPD.EPDbuffer,1);
  if(EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==WF42)
  {
  int temp1=xStart,temp2=xEnd;
  xStart=yStart;xEnd=yEnd;
  yEnd=yDot-temp1-2;yStart=yDot-temp2-3;
    }
   unsigned int Xsize=xEnd-xStart;
      unsigned int Ysize=yEnd-yStart+1;
     if(Xsize%8!= 0){
      Xsize = Xsize+(8-Xsize%8);
      }
      Xsize = Xsize/8;
  unsigned int offset;
  if(EPD_Type==WF29BZ03||EPD_Type==WF32) offset=yStart*xDot/4+xStart/4;
  else offset=yStart*xDot/8+xStart/8;
	if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
 {  
  unsigned long temp=yStart;
  yStart=yDot-1-yEnd;yEnd=yDot-1-temp;
  
  EPD_SetRamArea(xStart,xEnd,yEnd%256,yEnd/256,yStart%256,yStart/256);
  EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
	if(Label==2)  EPD_WriteDispRam(xEnd-xStart, yEnd-yStart+1,(unsigned char *)DisBuffer,offset,0x00);
  else if(Label==3) EPD_WriteDispRam(xEnd-xStart, yEnd-yStart+1,(unsigned char *)DisBuffer,offset,0xff);
  else  EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);  
  EPD_Update_Part();  
  //ReadBusy();
  if(EPD_Type==DKE29_3COLOR) 
  {
    ReadBusy();ReadBusy();ReadBusy();ReadBusy();
    EPD_WriteDispRam_Old(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);     
  }  
  //EPD_WriteDispRam_Old(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1); 
  
 }
	
	if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF42||EPD_Type==WF29BZ03||EPD_Type==WF32)
  { 
  
	EPD_SetRamArea(xStart,xEnd,yStart/256,yStart%256,yEnd/256,yEnd%256);
  ReadBusy_long();
  //EPD_WriteDispRam_Old(Xsize, Ysize, (unsigned char *)DisBuffer,offset,0x00); 
  if(Label==2)  EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,0xff);
  else if(Label==3) EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,0x00);
  else  EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);  
  
  EPD_Update_Part();
  ReadBusy();ReadBusy();ReadBusy();
  EPD_WriteDispRam_Old(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);  
  }
}
void Duck_EPD::EPD_Transfer_Part(int xStart,int xEnd,int yStart,int yEnd,unsigned char *DisBuffer,unsigned char Label)
{
  if(EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
  {
  int temp1=xStart,temp2=xEnd;
  xStart=yStart;xEnd=yEnd;
  yEnd=yDot-temp1-2;yStart=yDot-temp2-3;
    }
   unsigned int Xsize=xEnd-xStart;
      unsigned int Ysize=yEnd-yStart+1;
     if(Xsize%8!= 0){
      Xsize = Xsize+(8-Xsize%8);
      }
      Xsize = Xsize/8;
  unsigned int offset;
  if(EPD_Type==WF29BZ03||EPD_Type==WF32) offset=yStart*xDot/4+xStart/4;
  else offset=yStart*xDot/8+xStart/8;
  if(EPD_Type==WX29||EPD_Type==OPM42||EPD_Type==DKE42_3COLOR||EPD_Type==DKE29_3COLOR)
 {
  
  unsigned long temp=yStart;
  yStart=yDot-1-yEnd;yEnd=yDot-1-temp;
  
  EPD_SetRamArea(xStart,xEnd,yEnd%256,yEnd/256,yStart%256,yStart/256);
  EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
  if(Label==2)  EPD_WriteDispRam(xEnd-xStart, yEnd-yStart+1,(unsigned char *)DisBuffer,offset,0x00);
  else if(Label==3) EPD_WriteDispRam(xEnd-xStart, yEnd-yStart+1,(unsigned char *)DisBuffer,offset,0xff);
  else  EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);  
  //EPD_Update_Part();
  ReadBusy();
  //EPD_WriteDispRam_Old(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);  
  //ReadBusy();
  //EPD_SetRamArea(xStart,xEnd,yEnd%256,yEnd/256,yStart%256,yStart/256);
  //EPD_SetRamPointer(xStart/8,yEnd%256,yEnd/256);
  //EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);  
 }
  
  if(EPD_Type==WF29||EPD_Type==WF58||EPD_Type==WF42||EPD_Type==WF29BZ03||EPD_Type==WF32)
  { 
  
  EPD_SetRamArea(xStart,xEnd,yStart/256,yStart%256,yEnd/256,yEnd%256);
  if(Label==2)  EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,0xff);
  else if(Label==3) EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,0x00);
  else  EPD_WriteDispRam(Xsize, Ysize,(unsigned char *)DisBuffer,offset,1);  
  //EPD_Update_Part();
  }
}
