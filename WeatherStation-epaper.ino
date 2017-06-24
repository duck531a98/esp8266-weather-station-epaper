/**The MIT License (MIT)

Copyright (c) 2016 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>
#include "Wire.h"
#include "TimeClient.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include "heweather.h"
#include <EEPROM.h>
#include <SPI.h>
#include "EPD_drive.h"
#include "EPD_drive_gpio.h"
/***************************
 * Begin Settings
 **************************/
// Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
// for setup instructions
WiFiManager wifiManager;
const char* WIFI_SSID = "duckgagaga";
const char* WIFI_PWD = "0114987395";
const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes
const int sleeptime=60;//min
const float UTC_OFFSET = 8;

String city;String lastUpdate = "--";
bool shouldsave=false;bool updating=false;
TimeClient timeClient(UTC_OFFSET);heweatherclient heweather;Ticker ticker;Ticker avoidstuck;WaveShare_EPD EPD = WaveShare_EPD();
// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

//declaring prototypes
void setReadyForWeatherUpdate();
void saveConfigCallback () {
   shouldsave=true;
}
void setup() {
  Serial.begin(115200);Serial.println();Serial.println();
  pinMode(D3,INPUT);
  pinMode(CS,OUTPUT);
  pinMode(DC,OUTPUT);
  pinMode(RST,OUTPUT);
  pinMode(BUSY,INPUT);
  EEPROM.begin(60);
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  SPI.begin();
  EPD.EPD_init_Part();driver_delay_xms(DELAYTIME);
  //WiFi.begin(WIFI_SSID, WIFI_PWD);
  //wifiManager.resetSettings();
  /*************************************************
   wifimanager
   *************************************************/
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter custom_city("city", "city", "qingdao", 40);
  wifiManager.addParameter(&custom_city); 
  wifiManager.autoConnect("Weather widget");
  city= custom_city.getValue();
   /*************************************************
   EPPROM
   *************************************************/
  if (city!="qingdao")
  {
    
     EEPROM.write(0,city.length());
         Serial.print("\n\n");
         Serial.println(String("eeprom_write_0:")+city.length());
         for(int i=0;i<city.length();i++)
         {
           EEPROM.write(i+1,city[i]);
        
         Serial.println(city[i]); Serial.print("\n\n");
          }
         EEPROM.commit(); 
    }
 byte city_length=EEPROM.read(0);
 Serial.println("EEPROM_CITY-LENGTH:");Serial.println(city_length);
 if (city_length>0)
 {
  String test="";
  for(int x=1;x<city_length+1;x++) 
  {test+=(char)EEPROM.read(x);
  Serial.print((char)EEPROM.read(x));
  }
  city=test;
  Serial.println("   test:");Serial.print(test);
  heweather.city=city;
  }
/*************************************************
   update weather
*************************************************/
heweather.city="huangdao";
avoidstuck.attach(10,check);
updating=true;
updateData();updating=false;
updatedisplay();

//ticker.attach(UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);
}
void check()
{
  if(updating==true)
  {EPD.deepsleep(); ESP.deepSleep(60 * sleeptime * 1000000);}
  avoidstuck.detach();
  return;
  }

void loop() {
 
 if(digitalRead(D3)==LOW)
 {
 wifiManager.resetSettings();
 }
 EPD.deepsleep();
 ESP.deepSleep(60 * sleeptime * 1000000);


}
void updatedisplay()
{
    
    EPD.clearshadows(); EPD.clearbuffer();EPD.fontscale=1; 
    
    EPD.SetFont(12);unsigned char code[]={0x00,heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};EPD.DrawUnicodeStr(0,16,80,80,1,code);
    EPD.SetFont(13);unsigned char code2[]={0x00,heweather.getMeteoconIcon(heweather.today_cond_d_index.toInt())};EPD.DrawUnicodeStr(0,113,32,32,1,code2);
    EPD.SetFont(13);unsigned char code3[]={0x00,heweather.getMeteoconIcon(heweather.tomorrow_cond_d_index.toInt())};EPD.DrawUnicodeStr(33,113,32,32,1,code3);
    EPD.DrawXline(114,295,33);EPD.DrawXline(114,295,66);
    
    EPD.SetFont(0x0);
    Serial.println("heweather.citystr");
    Serial.println(heweather.citystr);
    EPD.DrawUTF(96,60,16,16,heweather.citystr);//城市名
    EPD.DrawUTF(112,60,16,16,heweather.date.substring(5, 10));
    EPD.DrawUTF(0,145,16,16,(unsigned char *)"今天");EPD.DrawUTF(16,145,16,16,heweather.today_tmp_min+"°~"+heweather.today_tmp_max+"°"+heweather.today_txt_d);
    EPD.DrawUTF(33,145,16,16,(unsigned char *)"明天");EPD.DrawUTF(49,145,16,16,heweather.tomorrow_tmp_min+"°~"+heweather.tomorrow_tmp_max+"°"+heweather.tomorrow_txt_d);
    EPD.DrawUTF(70,116,16,16,"空气质量："+heweather.qlty);
    EPD.DrawUTF(86,116,16,16,"RH:"+heweather.now_hum+"%"+" "+heweather.now_dir+heweather.now_sc);
    EPD.DrawUTF(102,116,16,16,"今天夜间"+heweather.today_txt_n);
    EPD.SetFont(0x2);
    EPD.DrawUTF(0,270,10,10,lastUpdate);//updatetime
    float voltage=(float)(analogRead(A0))/1024;
    EPD.DrawUTF(10,270,10,10,(String)((voltage+0.083)*13/3));
    EPD.SetFont(0x1);
    EPD.DrawUTF(96,5,32,32,heweather.now_tmp+"°");//天气实况温度
    EPD.DrawYline(96,127,57);
    
    for(int i=0;i<1808;i++) EPD.EPDbuffer[i]=~EPD.EPDbuffer[i];
    EPD.EPD_Dis_Part(0,127,0,295,(unsigned char *)EPD.EPDbuffer,1);
    
    driver_delay_xms(DELAYTIME);
      
  
  }



void updateData() {
  
  timeClient.updateTime();
  heweather.update();
  lastUpdate = timeClient.getHours()+":"+timeClient.getMinutes();
  readyForWeatherUpdate = false;
   Serial.print("heweather.rain");Serial.print(heweather.rain);Serial.print("\n");
  delay(1000);
}







void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //进入配置模式
  
  EPD.clearshadows(); EPD.clearbuffer();EPD.fontscale=1;
  EPD.SetFont(0x0);
  EPD.DrawUTF(0,0,16,16,(unsigned char *)"WIFI未连接，请连接“Weather Widget”继续配置");
  EPD.EPD_Dis_Part(0,127,0,295,(unsigned char *)EPD.EPDbuffer,1);
  driver_delay_xms(DELAYTIME);
}
