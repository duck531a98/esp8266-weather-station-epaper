#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "heweather.h"

heweatherclient::heweatherclient(const char* Serverurl,const char* langstring)
{
  server=Serverurl;
  lang=langstring;
  }
byte heweatherclient::getMeteoconIcon(int weathercodeindex) {
  if(weathercodeindex==0) return 12;
  if(weathercodeindex==1) return 58;
  if(weathercodeindex==2) return 58;
  if(weathercodeindex==3) return 58;
  if(weathercodeindex==4) return 54;
  if(weathercodeindex>=5&&weathercodeindex<=18) return 0;
  if(weathercodeindex>=19&&weathercodeindex<=32) return 19;
  if(weathercodeindex>=33&&weathercodeindex<=36)   return 5;
  if(weathercodeindex>=37&&weathercodeindex<=40)   return 16;
  if(weathercodeindex==41) return 28;
  if(weathercodeindex==42) return 28;
  if(weathercodeindex==43) return 28;
  /*if (iconText == "chanceflurries") return "F";
  if (iconText == "chancerain") return "Q";
  if (iconText == "chancesleet") return "W";
  if (iconText == "chancesnow") return "V";
  if (iconText == "chancetstorms") return "S";
  if (iconText == "clear") return "B";
  if (iconText == "cloudy") return "Y";
  if (iconText == "flurries") return "F";
  if (iconText == "fog") return "M";
  if (iconText == "hazy") return "E";
  if (iconText == "mostlycloudy") return "Y";
  if (iconText == "mostlysunny") return "H";
  if (iconText == "partlycloudy") return "H";
  if (iconText == "partlysunny") return "J";
  if (iconText == "sleet") return "W";
  if (iconText == "rain") return "R";
  if (iconText == "snow") return "W";
  if (iconText == "sunny") return "B";
  if (iconText == "tstorms") return "0";*/

  /*if (iconText == "nt_chanceflurries") return "F";
  if (iconText == "nt_chancerain") return "7";
  if (iconText == "nt_chancesleet") return "#";
  if (iconText == "nt_chancesnow") return "#";
  if (iconText == "nt_chancetstorms") return "&";
  if (iconText == "nt_clear") return "2";
  if (iconText == "nt_cloudy") return "Y";
  if (iconText == "nt_flurries") return "9";
  if (iconText == "nt_fog") return "M";
  if (iconText == "nt_hazy") return "E";
  if (iconText == "nt_mostlycloudy") return "5";
  if (iconText == "nt_mostlysunny") return "3";
  if (iconText == "nt_partlycloudy") return "4";
  if (iconText == "nt_partlysunny") return "4";
  if (iconText == "nt_sleet") return "9";
  if (iconText == "nt_rain") return "7";
  if (iconText == "nt_snow") return "#";
  if (iconText == "nt_sunny") return "4";
  if (iconText == "nt_tstorms") return "&";*/
 
  return 17;
}

void heweatherclient::whitespace(char c) {
  Serial.println("whitespace");
}

void heweatherclient::startDocument() {
  Serial.println("start document");
}

void heweatherclient::key(String key) {
   currentKey=key;
  
  Serial.println("key: " + key);
}

void heweatherclient::value(String value) {
if(currentParent=="now")
{
  if(currentKey=="cond") now_cond=value;
  if(currentKey=="cond_index") 
  {
    now_cond_index=value;
   if(value.toInt()>=19&&value.toInt()<=40)  rain=1;
  }
  if(currentKey=="hum") now_hum=value;
  if(currentKey=="tmp") now_tmp=value;
  if(currentKey=="dir") now_dir=value;
  if(currentKey=="sc") now_sc=value;
  }
if(currentParent=="today")
{
  if(currentKey=="cond_d") today_cond_d=value;
  if(currentKey=="cond_d_index") {today_cond_d_index=value;if(value.toInt()>=19&&value.toInt()<=40)  rain=1;}
  if(currentKey=="cond_n") today_cond_n=value;
  if(currentKey=="cond_n_index") {today_cond_n_index=value;if(value.toInt()>=19&&value.toInt()<=40)  rain=1;}
  if(currentKey=="tmpmax") today_tmp_max=value;
  if(currentKey=="tmpmin") today_tmp_min=value;
  if(currentKey=="txt_d") today_txt_d=value;
  if(currentKey=="txt_n") today_txt_n=value;
  }

if(currentParent=="tomorrow")
{
  if(currentKey=="cond_d") tomorrow_cond_d=value;
  if(currentKey=="cond_d_index") {tomorrow_cond_d_index=value;if(value.toInt()>=19&&value.toInt()<=40)  rain=1;}
    
  if(currentKey=="cond_n") tomorrow_cond_n=value;
  if(currentKey=="cond_n_index") tomorrow_cond_n_index=value;
  if(currentKey=="tmpmax") tomorrow_tmp_max=value;
  if(currentKey=="tmpmin") tomorrow_tmp_min=value;
  if(currentKey=="txt_d") tomorrow_txt_d=value;
  if(currentKey=="txt_n") tomorrow_txt_n=value;
  }

if(currentParent=="thedayaftertomorrow")
{
  if(currentKey=="cond_d") thedayaftertomorrow_cond_d=value;
  if(currentKey=="cond_d_index") thedayaftertomorrow_cond_d_index=value;
   
  if(currentKey=="cond_n") thedayaftertomorrow_cond_n=value;
  if(currentKey=="cond_n_index") thedayaftertomorrow_cond_n_index=value;
  if(currentKey=="tmpmax") thedayaftertomorrow_tmp_max=value;
  if(currentKey=="tmpmin") thedayaftertomorrow_tmp_min=value;
  }

  
 if(currentKey=="aqi") aqi=value;
 long aqiint = aqi.toInt();
 if(aqiint<=50) {aqitext="Excellent";airconditionbits_index=0;}
 else if(aqiint>50&&aqiint<=100){ aqitext="Good";airconditionbits_index=1;}
 else if(aqiint>100&&aqiint<=150) {aqitext="Lightly Polluted";airconditionbits_index=2;}
 else if(aqiint>150&&aqiint<=200) {aqitext="Moderately Polluted";airconditionbits_index=3;}
 else if(aqiint>250&&aqiint<=300) {aqitext="Heavily Polluted";airconditionbits_index=4;}
 else {aqitext="Severely Poluuted";airconditionbits_index=5;}
 if(currentKey=="co") co=value;
 if(currentKey=="no2") no2=value;
 if(currentKey=="o3") o3=value;
 if(currentKey=="pm10") pm10=value;
 if(currentKey=="pm25") pm25=value;
 if(currentKey=="so2") so2=value;
 if(currentKey=="city") citystr=value;
 if(currentKey=="date") date=value;
   if(currentKey=="qlty") qlty=value;
   if(currentKey="message") message=value;
  Serial.println("value: " + value);
}

void heweatherclient::endArray() {
  Serial.println("end array. ");
}

void heweatherclient::endObject() {
  Serial.println("end object. ");
  currentParent="";
}

void heweatherclient::endDocument() {
  Serial.println("end document. ");
}

void heweatherclient::startArray() {
   Serial.println("start array. ");
}

void heweatherclient::startObject() {
   Serial.println("start object. ");
     currentParent = currentKey;
}
void heweatherclient::update()
{
  rain=0;
 JsonStreamingParser parser;
  parser.setListener(this);
  WiFiClient client;
  const int httpPort =80;
 if (!client.connect(server, 80)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Requesting URL: ");

  // This will send the request to the server
  client.print(String("GET /weather.php?city=")+city +"&lang="+lang+"&client_name="+client_name+" HTTP/1.1\r\n" +
             "Host: " + server + "\r\n" +
             "Connection: close\r\n" +
             "\r\n" );
   

  int pos = 0;
  boolean isBody = false;
  char c;

  int size = 0;
  client.setNoDelay(false);
  while(client.connected()) {
    while((size = client.available()) > 0) {
      c = client.read();
      Serial.print(c);
      if (c == '{' || c == '[') {
        isBody = true;
      }
      if (isBody) {
        parser.parse(c);
      }
    }
  } }
