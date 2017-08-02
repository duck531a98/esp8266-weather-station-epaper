

/*
 heweather.h
*/
#pragma once
#include "JsonStreamingParser.h"
#include "JsonListener.h"


//JsonStreamingParser parser;
//heweatherListener listener;


class heweatherclient: public JsonListener{
private:
 String currentKey ;
 String currentParent;
 const char* server;
 const char* lang;
 public: 
   heweatherclient(const char * Serverurl,const char* langstring);
   String aqi; String co; String no2; String o3;
   String pm10; String pm25; String so2; String aqitext;
   byte airconditionbits_index;
   bool rain=0;
   const char* client_name;

   String now_cond;
   String now_hum;
   String now_tmp;
   String now_cond_index;
   String now_dir;
   String now_sc;

   String today_cond_d;
    String today_cond_d_index;
   String today_cond_n;
    String today_cond_n_index;
   String today_tmp_max;
   String today_tmp_min;
   String today_txt_d;
   String today_txt_n;

   String tomorrow_cond_d;
   String tomorrow_cond_d_index;
   String tomorrow_cond_n;
    String tomorrow_cond_n_index;
   String tomorrow_tmp_max;
   String tomorrow_tmp_min;
 String tomorrow_txt_d;
 String tomorrow_txt_n;
 String message;
 
   String thedayaftertomorrow_cond_d;
   String thedayaftertomorrow_cond_d_index;
   String thedayaftertomorrow_cond_n;
    String thedayaftertomorrow_cond_n_index;
   String thedayaftertomorrow_tmp_max;
   String thedayaftertomorrow_tmp_min;
  // virtual void keys(String key);
    void update();
    String citystr;String date;String qlty;
    String city;
    byte getMeteoconIcon(int weathercodeindex) ;
    
virtual void whitespace(char c);

    virtual void startDocument();

    virtual void key(String key);

    virtual void value(String value);

    virtual void endArray();

    virtual void endObject();

    virtual void endDocument();

    virtual void startArray();

    virtual void startObject();

    
  
   };
 

 
  
 
  
    

