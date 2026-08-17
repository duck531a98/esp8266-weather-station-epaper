/**
GPL v3.0
Copyright (c) 2017 by Hui Lu
*/

#include <ESP8266WiFi.h>
#include "Wire.h"
#include "TimeClient.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "DFRobot_SHT20.h"
#include "heweather.h"
#include <EEPROM.h>
#include <SPI.h>
#include "webpage.h"
#include "EPD_drive.h"
#include "EPD_drive_gpio.h"
//#include "EPD_drive_gpio.cpp"
#include "bitmaps.h"
#include "gray_image.h"
#include "lang.h"
#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#define ADC_ON digitalWrite(12,1);
#define ADC_OFF digitalWrite(12,0);
//#define debug 1 ///< 调试模式，打开串口输出
#define SDA 13
#define SCL 14
//#define FAST_MODE
ADC_MODE(0);
/***************************
  全局设置  
 **************************/
bool first_run=false;
bool exit_portal=0;
String wifi_ssid;
String wifi_password;
int epd_time=1200;//局刷耗时ms
String epd_type;
byte epd_type_index;
byte contrast=0x15;
String qweather_host;
String qweather_api_key;
String qweather_location_id;
String qweather_location_name;
String qweather_air_coordinates;
int crc;
String update_time_range;
byte showtime=0;            ///< 是否显示时间，不支持局刷的屏不要开
int sleeptime=60;    ///< 更新天气的间隔 单位为分钟,间隔是下面更新时间的间隔的整数倍如更新时间间隔为30分钟，更新天气间隔须为30分钟的倍数，不能是31，43分钟这样
int timeupdateinterval=1*60;  ///< 更新时间的间隔，单位为秒（显示时间时不可更改，只显示天气可改为60*30）
const float UTC_OFFSET = 8; ///< 中国标准时间 UTC+8
byte end_time=23;           ///< 停止更新天气的时间 23：00
byte start_time=0;          ///< 开始更新天气的时间 7：00
 /***************************
  **************************/
String city;
String lastUpdate = "--";
bool shouldsave=false;
bool config_ready=false;
bool epdModelTrusted=false;
uint32_t eepromQWeatherConfigHash=0;
bool eepromQWeatherConfigHashValid=false;
TimeClient timeClient(UTC_OFFSET);
Duck_EPD EPD = Duck_EPD();
heweatherclient heweather(lang);
ESP8266WebServer web(80);
StaticJsonDocument<1536> doc;
DFRobot_SHT20 sht20;
File uploadFile;
const size_t PHOTO_UPLOAD_BYTES=400UL*300UL/2UL;
const size_t PHOTO_UPLOAD_REQUEST_MAX=PHOTO_UPLOAD_BYTES+4096UL;
const char PHOTO_UPLOAD_PATH[]="/pic.xbm";
const char PHOTO_UPLOAD_TEMP_PATH[]="/pic.tmp";
size_t uploadBytesReceived=0;
int uploadResponseStatus=400;
const char* uploadResponseMessage="未收到图片";
bool uploadSucceeded=false;
bool pictureDisplayPending=false;
bool epdSpiActive=false;
bool permanentSleepAfterDisplay=false;
#ifdef POWER_TEST_FORCE_WEATHER
bool powerTestSecondWeatherPending=false;
#endif
const uint32_t EPD_SPI_CLOCK_HZ=20000000UL;
const unsigned long PORTAL_TIMEOUT_MS=5UL*60UL*1000UL;
const unsigned long WIFI_CONNECT_BUDGET_MS=10000UL;
const unsigned long WIFI_FAST_CONNECT_MS=2500UL;
const unsigned long WF32_WAKE_BUDGET_MS=45000UL;
const unsigned long DEFAULT_WAKE_BUDGET_MS=75000UL;
const unsigned long FAILURE_BACKOFF_SECONDS=60UL*60UL;
const float CRITICAL_BATTERY_VOLTAGE=3.55f;
const uint32_t WIFI_CACHE_RTC_OFFSET=32;
const uint32_t WIFI_CACHE_MAGIC=0x57464331UL; // "WFC1"
const uint32_t BATTERY_BACKOFF_RTC_OFFSET=37;
const uint32_t BATTERY_BACKOFF_MAGIC=0x42415431UL; // "BAT1"
const uint32_t PORTAL_RESTART_RTC_OFFSET=38;
const uint32_t PORTAL_RESTART_MAGIC=0x41503131UL; // "AP11"
const uint32_t PORTAL_ATTEMPT_RTC_OFFSET=39;
const uint32_t PORTAL_ATTEMPT_MAGIC=0x41505431UL; // "APT1"
const uint32_t QWEATHER_BACKOFF_RTC_OFFSET=40;
const uint32_t QWEATHER_BACKOFF_MAGIC=0x51574231UL; // "QWB1"
const uint32_t AIR_BACKOFF_RTC_OFFSET=45;
const uint32_t AIR_BACKOFF_MAGIC=0x41514231UL; // "AQB1"
const uint32_t MAX_QWEATHER_BACKOFF_SECONDS=24UL*60UL*60UL;
const uint32_t AIR_RETRY_BACKOFF_SECONDS=6UL*60UL*60UL;
const byte MAX_QWEATHER_BACKOFF_FAILURES=10;
const uint32_t PERMANENT_SLEEP_RTC_OFFSET=4;
const uint32_t PERMANENT_SLEEP_MAGIC=0x534c5031UL; // "SLP1"
const uint32_t UNIX_TIME_RTC_OFFSET=16;
const uint32_t LEGACY_UNIX_TIME_RTC_MAGIC=0x554e4958UL; // "UNIX"
const uint32_t UNIX_TIME_RTC_MAGIC=0x554d5332UL; // "UMS2"
const byte EEPROM_CACHE_MAGIC=0x57;
// Bump this together with QWEATHER_CONFIG_HASH_SCHEMA. Version 2 adds the
// QWeather configuration hash at bytes 12..15 for the EEPROM-only wake path.
const byte EEPROM_CACHE_VERSION=2;
const byte QWEATHER_CONFIG_HASH_SCHEMA=2;
const byte EEPROM_QWEATHER_HASH_ADDRESS=12;
const byte RTC_SCHEDULE_MAGIC=126;
const byte RTC_SCHEDULE_CHECK_SEED=0xa7;
const byte BATTERY_STATE_OK=0;
const byte BATTERY_STATE_CRITICAL=1;
const byte BATTERY_STATE_IMPLAUSIBLE=2;
Ticker wakeDeadlineTimer;
volatile bool wakeDeadlineExpired=false;

struct WiFiQuickConnectCache
{
  uint32_t magic;
  uint32_t ssidHash;
  byte channel;
  byte bssid[6];
  byte reserved;
  uint32_t checksum;
};

struct UnixTimeRtcCache
{
  uint32_t magic;
  uint32_t epoch;
  uint32_t milliseconds;
  uint32_t checksum;
};

struct LegacyUnixTimeRtcCache
{
  uint32_t magic;
  uint32_t epoch;
  uint32_t checksum;
};

struct QWeatherBackoffCache
{
  uint32_t magic;
  uint32_t configHash;
  uint32_t retryNotBefore;
  uint16_t failureCount;
  byte failureClass;
  byte reserved;
  uint32_t checksum;
};

struct AirBackoffCache
{
  uint32_t magic;
  uint32_t configHash;
  uint32_t retryNotBefore;
  uint32_t checksum;
};

static_assert(sizeof(UnixTimeRtcCache)%4==0,
               "Unix RTC cache must remain 4-byte aligned");
static_assert(UNIX_TIME_RTC_OFFSET+sizeof(UnixTimeRtcCache)/4<=
                WIFI_CACHE_RTC_OFFSET,
              "Unix RTC cache overlaps the WiFi cache");
static_assert(sizeof(WiFiQuickConnectCache)==20,
              "WiFi RTC cache must remain 4-byte aligned");
static_assert(WIFI_CACHE_RTC_OFFSET+sizeof(WiFiQuickConnectCache)/4==
              BATTERY_BACKOFF_RTC_OFFSET,
              "WiFi RTC cache overlaps the backoff markers");
static_assert(PORTAL_RESTART_RTC_OFFSET+1==PORTAL_ATTEMPT_RTC_OFFSET,
              "Portal RTC markers must not overlap");
static_assert(PORTAL_ATTEMPT_RTC_OFFSET+1==QWEATHER_BACKOFF_RTC_OFFSET,
              "Weather backoff must follow portal markers");
static_assert(sizeof(QWeatherBackoffCache)==20,
              "Weather backoff cache must remain 4-byte aligned");
static_assert(QWEATHER_BACKOFF_RTC_OFFSET+
                sizeof(QWeatherBackoffCache)/sizeof(uint32_t)==
              AIR_BACKOFF_RTC_OFFSET,
              "Air backoff must follow weather backoff");
static_assert(sizeof(AirBackoffCache)==16,
              "Air backoff cache must remain 4-byte aligned");
static_assert(AIR_BACKOFF_RTC_OFFSET+
                sizeof(AirBackoffCache)/sizeof(uint32_t)<=128,
              "Backoff caches exceed RTC user memory");
bool loadWiFiQuickConnectCache(WiFiQuickConnectCache& cache);
void sleepAfterPowerFault();
uint32_t qweatherBackoffChecksum(const QWeatherBackoffCache& cache);
bool readQWeatherBackoff(QWeatherBackoffCache& cache);
uint32_t airBackoffChecksum(const AirBackoffCache& cache);
bool readAirBackoff(AirBackoffCache& cache);
bool qweatherBackoffWasLoaded=false;

void beginEpdSpi()
{
  if(epdSpiActive) return;
  SPI.begin();
  // HSPI 会把 GPIO12 配成 MISO；本板用它控制 ADC/传感器电源，
  // 屏幕只写不读，因此立即恢复为低电平输出。
  pinMode(12,OUTPUT);
  ADC_OFF;
  SPI.beginTransaction(SPISettings(EPD_SPI_CLOCK_HZ,MSBFIRST,SPI_MODE0));
  HARDWARE_SPI=1;
  epdSpiActive=true;
}

void endEpdSpi()
{
  if(!epdSpiActive) return;
  HARDWARE_SPI=0;
  SPI.endTransaction();
  SPI.end();
  epdSpiActive=false;
}

void stopWiFiRadio()
{
  WiFi.setAutoReconnect(false);
  WiFi.setAutoConnect(false);
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  delay(1);
}

uint32_t fnv1a(const byte* data,size_t length,uint32_t hash)
{
  for(size_t i=0;i<length;i++)
  {
    hash^=data[i];
    hash*=16777619UL;
  }
  return hash;
}

uint32_t hashString(const String& value)
{
  return fnv1a((const byte*)value.c_str(),value.length(),2166136261UL);
}

uint32_t currentQWeatherConfigHash()
{
  uint32_t hash=2166136261UL;
  const byte separator=0;
  hash=fnv1a(&QWEATHER_CONFIG_HASH_SCHEMA,1,hash);
  hash=fnv1a((const byte*)qweather_host.c_str(),qweather_host.length(),hash);
  hash=fnv1a(&separator,1,hash);
  hash=fnv1a((const byte*)qweather_api_key.c_str(),
             qweather_api_key.length(),hash);
  hash=fnv1a(&separator,1,hash);
  hash=fnv1a((const byte*)city.c_str(),city.length(),hash);
  return fnv1a(&separator,1,hash);
}

uint32_t readEepromQWeatherConfigHash()
{
  uint32_t hash=0;
  for(byte index=0;index<sizeof(hash);index++)
    hash|=(uint32_t)EEPROM.read(EEPROM_QWEATHER_HASH_ADDRESS+index)
          <<(index*8);
  return hash;
}

void writeEepromQWeatherConfigHash(uint32_t hash)
{
  for(byte index=0;index<sizeof(hash);index++)
    EEPROM.write(EEPROM_QWEATHER_HASH_ADDRESS+index,
                 (byte)(hash>>(index*8)));
}

bool availableQWeatherConfigHash(uint32_t& hash)
{
  if(config_ready)
  {
    hash=currentQWeatherConfigHash();
    return true;
  }
  if(eepromQWeatherConfigHashValid)
  {
    hash=eepromQWeatherConfigHash;
    return true;
  }
  return false;
}

uint32_t qweatherBackoffChecksum(const QWeatherBackoffCache& cache)
{
  return fnv1a((const byte*)&cache,
               offsetof(QWeatherBackoffCache,checksum),2166136261UL);
}

bool readQWeatherBackoff(QWeatherBackoffCache& cache)
{
  memset(&cache,0,sizeof(cache));
  uint32_t magic=0;
  if(!ESP.rtcUserMemoryRead(QWEATHER_BACKOFF_RTC_OFFSET,
                            &magic,sizeof(magic))||
     magic!=QWEATHER_BACKOFF_MAGIC)
    return false;
  if(!ESP.rtcUserMemoryRead(QWEATHER_BACKOFF_RTC_OFFSET,
                            (uint32_t*)&cache,sizeof(cache))||
     cache.magic!=QWEATHER_BACKOFF_MAGIC||
     cache.failureCount==0||
     cache.failureCount>MAX_QWEATHER_BACKOFF_FAILURES||
     cache.reserved!=0||
     cache.failureClass<
       (byte)heweatherclient::FailureClass::Transient||
     cache.failureClass>
       (byte)heweatherclient::FailureClass::Permanent||
     cache.checksum!=qweatherBackoffChecksum(cache))
    return false;
  return true;
}

void clearQWeatherBackoff()
{
  uint32_t clearedMagic=0;
  ESP.rtcUserMemoryWrite(QWEATHER_BACKOFF_RTC_OFFSET,
                         &clearedMagic,sizeof(clearedMagic));
  qweatherBackoffWasLoaded=false;
}

bool qweatherBackoffDefersRequest()
{
  qweatherBackoffWasLoaded=false;
  QWeatherBackoffCache cache;
  if(!readQWeatherBackoff(cache)) return false;
  uint32_t configHash=0;
  if(availableQWeatherConfigHash(configHash)&&
     cache.configHash!=configHash)
  {
    clearQWeatherBackoff();
    return false;
  }

  qweatherBackoffWasLoaded=true;
  if(cache.failureClass==
       (byte)heweatherclient::FailureClass::Configuration||
     cache.failureClass==
       (byte)heweatherclient::FailureClass::Permanent)
    return true;
  // If the clock cache was lost while a timed backoff survived, let the next
  // ordinary weather slot recover time through NTP. Keeping it deferred here
  // would leave the device RF-disabled forever because the deadline could
  // never become comparable again.
  if(!timeClient.hasValidUnixTime()) return false;
  return (int32_t)(cache.retryNotBefore-timeClient.getUnixEpoch())>0;
}

bool qweatherBackoffAllowsRadio()
{
  QWeatherBackoffCache cache;
  if(!readQWeatherBackoff(cache)) return true;
  // The ordinary time-only wake has not mounted LittleFS yet. Compare against
  // the hash already carried in its validated EEPROM cache, avoiding both a
  // filesystem mount and an unnecessary RF-disabled wake for a stale marker.
  uint32_t configHash=0;
  if(availableQWeatherConfigHash(configHash)&&
     cache.configHash!=configHash)
  {
    clearQWeatherBackoff();
    return true;
  }
  if(cache.failureClass==
       (byte)heweatherclient::FailureClass::Configuration||
     cache.failureClass==
       (byte)heweatherclient::FailureClass::Permanent)
    return false;
  // A timed marker without a trusted clock must allow one normal scheduled
  // RF wake so updateData() can restore Unix time through NTP.
  if(!timeClient.hasValidUnixTime()) return true;
  // Callers advance time to the next wake before asking, so this comparison
  // selects RF for that next boot, exactly matching deepSleepInstant semantics.
  return (int32_t)(cache.retryNotBefore-timeClient.getUnixEpoch())<=0;
}

void recordQWeatherResult(heweatherclient::FailureClass failureClass,
                          uint32_t retryAfterSeconds)
{
  if(failureClass==heweatherclient::FailureClass::None)
  {
    if(qweatherBackoffWasLoaded) clearQWeatherBackoff();
    return;
  }

  QWeatherBackoffCache previous;
  uint32_t configHash=currentQWeatherConfigHash();
  bool currentFailureIsTimed=
    failureClass==heweatherclient::FailureClass::Transient||
    failureClass==heweatherclient::FailureClass::RateLimited;
  bool sameFailure=readQWeatherBackoff(previous)&&
                   previous.configHash==configHash&&
                   (previous.failureClass==(byte)failureClass||
                    (currentFailureIsTimed&&
                     (previous.failureClass==
                        (byte)heweatherclient::FailureClass::Transient||
                      previous.failureClass==
                        (byte)heweatherclient::FailureClass::RateLimited)));
  uint16_t nextFailureCount=sameFailure
    ?(uint16_t)(previous.failureCount+1U)
    :1U;
  if(nextFailureCount>MAX_QWEATHER_BACKOFF_FAILURES)
    nextFailureCount=MAX_QWEATHER_BACKOFF_FAILURES;
  byte failureCount=(byte)nextFailureCount;

  QWeatherBackoffCache cache={};
  cache.magic=QWEATHER_BACKOFF_MAGIC;
  cache.configHash=configHash;
  cache.failureCount=failureCount;
  cache.failureClass=(byte)failureClass;
  if(failureClass==heweatherclient::FailureClass::Configuration||
     failureClass==heweatherclient::FailureClass::Permanent)
  {
    cache.retryNotBefore=UINT32_MAX;
  }
  else
  {
    uint32_t delaySeconds=(uint32_t)sleeptime*60U;
    if(delaySeconds<1800U) delaySeconds=1800U;
    byte doublings=(byte)(failureCount-1);
    if(doublings>5) doublings=5;
    while(doublings--&&delaySeconds<MAX_QWEATHER_BACKOFF_SECONDS)
    {
      delaySeconds*=2U;
      if(delaySeconds>MAX_QWEATHER_BACKOFF_SECONDS)
        delaySeconds=MAX_QWEATHER_BACKOFF_SECONDS;
    }
    if(currentFailureIsTimed)
    {
      uint32_t limitedRetryAfter=retryAfterSeconds;
      if(limitedRetryAfter>MAX_QWEATHER_BACKOFF_SECONDS)
        limitedRetryAfter=MAX_QWEATHER_BACKOFF_SECONDS;
      if(delaySeconds<limitedRetryAfter) delaySeconds=limitedRetryAfter;
    }
    uint32_t now=timeClient.getUnixEpoch();
    cache.retryNotBefore=
      delaySeconds>UINT32_MAX-now?UINT32_MAX:now+delaySeconds;
  }
  cache.checksum=qweatherBackoffChecksum(cache);
  ESP.rtcUserMemoryWrite(QWEATHER_BACKOFF_RTC_OFFSET,
                         (uint32_t*)&cache,sizeof(cache));
  qweatherBackoffWasLoaded=true;
}

uint32_t airBackoffChecksum(const AirBackoffCache& cache)
{
  return fnv1a((const byte*)&cache,
               offsetof(AirBackoffCache,checksum),2166136261UL);
}

bool readAirBackoff(AirBackoffCache& cache)
{
  memset(&cache,0,sizeof(cache));
  uint32_t magic=0;
  if(!ESP.rtcUserMemoryRead(AIR_BACKOFF_RTC_OFFSET,
                            &magic,sizeof(magic))||
     magic!=AIR_BACKOFF_MAGIC)
    return false;
  return ESP.rtcUserMemoryRead(AIR_BACKOFF_RTC_OFFSET,
                               (uint32_t*)&cache,sizeof(cache))&&
         cache.magic==AIR_BACKOFF_MAGIC&&
         cache.retryNotBefore!=0&&
         cache.checksum==airBackoffChecksum(cache);
}

void clearAirBackoff()
{
  uint32_t clearedMagic=0;
  ESP.rtcUserMemoryWrite(AIR_BACKOFF_RTC_OFFSET,
                         &clearedMagic,sizeof(clearedMagic));
}

bool airBackoffAllowsRequest()
{
  AirBackoffCache cache;
  if(!readAirBackoff(cache)) return true;
  if(cache.configHash!=currentQWeatherConfigHash())
  {
    clearAirBackoff();
    return true;
  }
  if(cache.retryNotBefore==UINT32_MAX) return false;
  if(!timeClient.hasValidUnixTime()) return true;
  return (int32_t)(cache.retryNotBefore-timeClient.getUnixEpoch())<=0;
}

void recordAirResult(heweatherclient::FailureClass failureClass,
                     uint32_t retryAfterSeconds)
{
  if(failureClass==heweatherclient::FailureClass::None)
  {
    AirBackoffCache existing;
    if(readAirBackoff(existing)) clearAirBackoff();
    return;
  }

  AirBackoffCache cache={};
  cache.magic=AIR_BACKOFF_MAGIC;
  cache.configHash=currentQWeatherConfigHash();
  if(failureClass==heweatherclient::FailureClass::Configuration||
     failureClass==heweatherclient::FailureClass::Permanent)
  {
    cache.retryNotBefore=UINT32_MAX;
  }
  else
  {
    uint32_t delaySeconds=(uint32_t)sleeptime*60U;
    if(delaySeconds<AIR_RETRY_BACKOFF_SECONDS)
      delaySeconds=AIR_RETRY_BACKOFF_SECONDS;
    if(retryAfterSeconds>MAX_QWEATHER_BACKOFF_SECONDS)
      retryAfterSeconds=MAX_QWEATHER_BACKOFF_SECONDS;
    if(delaySeconds<retryAfterSeconds) delaySeconds=retryAfterSeconds;
    uint32_t now=timeClient.getUnixEpoch();
    cache.retryNotBefore=
      delaySeconds>UINT32_MAX-now?UINT32_MAX:now+delaySeconds;
  }
  cache.checksum=airBackoffChecksum(cache);
  ESP.rtcUserMemoryWrite(AIR_BACKOFF_RTC_OFFSET,
                         (uint32_t*)&cache,sizeof(cache));
}

bool loadWiFiQuickConnectCache(WiFiQuickConnectCache& cache)
{
  if(!ESP.rtcUserMemoryRead(WIFI_CACHE_RTC_OFFSET,
                            (uint32_t*)&cache,sizeof(cache)))
  {
    #ifdef debug
    Serial.println("WiFi RTC cache read failed");
    #endif
    return false;
  }
  uint32_t currentSsidHash=hashString(wifi_ssid);
  if(cache.magic!=WIFI_CACHE_MAGIC||cache.ssidHash!=currentSsidHash||
     cache.channel<1||cache.channel>14)
  {
    #ifdef debug
    Serial.printf("WiFi RTC cache invalid: magic=%08lx channel=%u hash=%08lx/%08lx\n",
                  cache.magic,cache.channel,cache.ssidHash,currentSsidHash);
    #endif
    return false;
  }
  uint32_t expected=fnv1a((const byte*)&cache,
                          offsetof(WiFiQuickConnectCache,checksum),2166136261UL);
  if(cache.checksum!=expected)
  {
    #ifdef debug
    Serial.printf("WiFi RTC cache checksum mismatch: %08lx/%08lx\n",
                  cache.checksum,expected);
    #endif
    return false;
  }

  bool allZero=true;
  bool allOnes=true;
  for(byte i=0;i<sizeof(cache.bssid);i++)
  {
    if(cache.bssid[i]!=0) allZero=false;
    if(cache.bssid[i]!=0xff) allOnes=false;
  }
  bool valid=!allZero&&!allOnes;
  #ifdef debug
  if(!valid) Serial.println("WiFi RTC cache BSSID invalid");
  #endif
  return valid;
}

void saveWiFiQuickConnectCache()
{
  const byte* connectedBssid=WiFi.BSSID();
  byte connectedChannel=WiFi.channel();
  if(connectedBssid==nullptr||connectedChannel<1||connectedChannel>14) return;

  WiFiQuickConnectCache cache={};
  cache.magic=WIFI_CACHE_MAGIC;
  cache.ssidHash=hashString(wifi_ssid);
  cache.channel=connectedChannel;
  memcpy(cache.bssid,connectedBssid,sizeof(cache.bssid));
  cache.checksum=fnv1a((const byte*)&cache,
                       offsetof(WiFiQuickConnectCache,checksum),2166136261UL);
  bool saved=ESP.rtcUserMemoryWrite(WIFI_CACHE_RTC_OFFSET,
                                    (uint32_t*)&cache,sizeof(cache));
  #ifdef debug
  Serial.printf("WiFi RTC cache %s on channel %u\n",
                saved?"saved":"write failed",connectedChannel);
  #endif
}

void invalidateWiFiQuickConnectCache()
{
  uint32_t cleared[sizeof(WiFiQuickConnectCache)/sizeof(uint32_t)]={};
  ESP.rtcUserMemoryWrite(WIFI_CACHE_RTC_OFFSET,cleared,sizeof(cleared));
}

bool waitForWiFi(unsigned long connectStarted)
{
  while(WiFi.status()!=WL_CONNECTED&&
        millis()-connectStarted<WIFI_CONNECT_BUDGET_MS&&
        !wakeDeadlineExpired)
  {
    delay(25);
  }
  if(wakeDeadlineExpired) sleepAfterPowerFault();
  return WiFi.status()==WL_CONNECTED;
}

bool connectWiFiWithinBudget()
{
  const unsigned long connectStarted=millis();
  WiFiQuickConnectCache cache;
  bool usedQuickConnect=loadWiFiQuickConnectCache(cache);
  if(usedQuickConnect)
  {
    #ifdef debug
    Serial.printf("WiFi directed connect on channel %u\n",cache.channel);
    #endif
    WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str(),
               cache.channel,cache.bssid,true);
    while(WiFi.status()!=WL_CONNECTED&&
          millis()-connectStarted<WIFI_FAST_CONNECT_MS&&
          !wakeDeadlineExpired)
    {
      delay(25);
    }
    if(wakeDeadlineExpired) sleepAfterPowerFault();
    if(WiFi.status()!=WL_CONNECTED)
    {
      // A directed miss means that this BSSID/channel can no longer be
      // trusted. Do not spend the first 2.5 seconds on it again next wake.
      invalidateWiFiQuickConnectCache();
      WiFi.disconnect(false);
      delay(10);
    }
  }

  if(WiFi.status()!=WL_CONNECTED)
  {
    #ifdef debug
    if(usedQuickConnect) Serial.println("WiFi directed connect missed; scanning");
    #endif
    WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str());
  }

  bool connected=waitForWiFi(connectStarted);
  if(connected) saveWiFiQuickConnectCache();
  #ifdef debug
  Serial.printf("WiFi %s at %lums%s, RSSI=%ddBm\n",
                connected?"connected":"failed",millis(),
                usedQuickConnect?" (RTC cache)":"",
                connected?WiFi.RSSI():0);
  #endif
  return connected;
}

bool epdTypeToIndex(const String& type, byte& index)
{
  if(type=="wx29") index=WX29;
  else if(type=="wf29") index=WF29;
  else if(type=="opm42") index=OPM42;
  else if(type=="wf58") index=WF58;
  else if(type=="wft29bz03") index=WF29BZ03;
  else if(type=="dke42") index=DKE42_3COLOR;
  else if(type=="dke29") index=DKE29_3COLOR;
  else if(type=="wf42") index=WF42;
  else if(type=="wf32") index=WF32;
  else return false;
  return true;
}

bool stringLengthInRange(const String& value, size_t minimum, size_t maximum)
{
  return value.length()>=minimum&&value.length()<=maximum;
}

String normalizeQWeatherHost(String host)
{
  host.trim();
  host.toLowerCase();
  // The console and documentation often present the Host as part of a full
  // HTTPS URL. This field stores only the DNS name because the client always
  // supplies HTTPS itself.
  if(host.startsWith("https://")) host.remove(0,8);
  else if(host.startsWith("http://")) host.remove(0,7);
  while(host.endsWith("/")) host.remove(host.length()-1);
  return host;
}

bool jsonStringLengthInRange(JsonVariantConst value, size_t minimum, size_t maximum)
{
  if(!value.is<const char*>()) return false;
  const char* text=value.as<const char*>();
  if(text==nullptr) return false;
  size_t length=strlen(text);
  return length>=minimum&&length<=maximum;
}

bool isValidTimeText(const String& value, byte maximum)
{
  return value.length()==2&&
         value[0]>='0'&&value[0]<='9'&&
         value[1]>='0'&&value[1]<='9'&&
         value.toInt()<=maximum;
}

String formatMonthDayForDisplay(const String& date)
{
  if(date.length()!=10||date[4]!='-'||date[7]!='-') return date;
  for(byte i=0;i<10;i++)
  {
    if(i==4||i==7) continue;
    if(date[i]<'0'||date[i]>'9') return date;
  }
  int month=date.substring(5,7).toInt();
  int day=date.substring(8,10).toInt();
  if(month<1||month>12||day<1||day>31) return date;
  return String(month)+"月"+String(day)+"日";
}

void sanitizeStoredTime(byte rtc_mem[4],const byte currentTime[4])
{
  bool valid=true;
  for(byte i=0;i<4;i++)
  {
    if(rtc_mem[i]<'0'||rtc_mem[i]>'9') valid=false;
  }
  if(!valid)
  {
    memcpy(rtc_mem,currentTime,4);
  }
}

byte rtcScheduleChecksum(const uint32_t& rtcWord)
{
  const byte* rtcMem=(const byte*)&rtcWord;
  return RTC_SCHEDULE_CHECK_SEED^rtcMem[0]^rtcMem[2]^rtcMem[3];
}

bool rtcScheduleIsValid(const uint32_t& rtcWord)
{
  const byte* rtcMem=(const byte*)&rtcWord;
  return rtcMem[0]==RTC_SCHEDULE_MAGIC&&rtcMem[3]<=2&&
         rtcMem[1]==rtcScheduleChecksum(rtcWord);
}

bool readRtcSchedule(uint32_t& rtcWord)
{
  rtcWord=0;
  return ESP.rtcUserMemoryRead(0,&rtcWord,sizeof(rtcWord))&&
         rtcScheduleIsValid(rtcWord);
}

bool writeRtcSchedule(uint32_t& rtcWord)
{
  byte* rtcMem=(byte*)&rtcWord;
  rtcMem[0]=RTC_SCHEDULE_MAGIC;
  rtcMem[1]=rtcScheduleChecksum(rtcWord);
  return ESP.rtcUserMemoryWrite(0,&rtcWord,sizeof(rtcWord));
}

void forceWeatherUpdateNextBoot()
{
  uint32_t rtcWord=0;
  bool valid=readRtcSchedule(rtcWord);
  if(!valid) rtcWord=0;
  byte* rtc_mem=(byte*)&rtcWord;
  rtc_mem[2]=0xff;
  if(!valid) rtc_mem[3]=0;
  writeRtcSchedule(rtcWord);
}

void clearEpdSleepPending()
{
  uint32_t rtcWord=0;
  if(!readRtcSchedule(rtcWord)) return;
  byte* rtcMem=(byte*)&rtcWord;
  rtcMem[3]=0;
  writeRtcSchedule(rtcWord);
}

int weatherUpdateCountTarget(int intervalSeconds)
{
  if(intervalSeconds<=0) return 1;
  int target=(int)(((long)sleeptime*60L)/intervalSeconds);
  if(target<1) target=1;
  if(target>255) target=255;
  return target;
}

bool weatherUpdateWindowOpenAfterSeconds(uint32_t secondsFromNow)
{
  // An unknown clock must be allowed to reach NTP; otherwise a stale night
  // window could keep the device permanently offline.
  if(!timeClient.hasValidUnixTime()) return true;
  uint32_t secondsOfDay=(uint32_t)timeClient.getHours_byte()*3600UL+
                        (uint32_t)timeClient.getMinutes_byte()*60UL+
                        timeClient.getSeconds_byte();
  byte futureHour=(byte)(((secondsOfDay+secondsFromNow)%86400UL)/3600UL);
  return futureHour>=start_time&&futureHour<end_time;
}

bool weatherUpdateWindowOpen()
{
  return weatherUpdateWindowOpenAfterSeconds(0);
}

bool nextWakeNeedsRadio()
{
  // timeupdateinterval is temporarily changed to "seconds until next minute"
  // before sleeping. The next boot reloads the configured cadence instead.
  const int configuredInterval=showtime==1?60:1800;
  int updateCountTarget=weatherUpdateCountTarget(configuredInterval);
  uint32_t rtcWord=0;
  if(!readRtcSchedule(rtcWord)) return true;
  byte* rtcMem=(byte*)&rtcWord;
  // This mirrors the first decision in update_time() on the next boot.
  bool scheduleDue=(int)rtcMem[2]>=updateCountTarget-1;
  return scheduleDue&&weatherUpdateWindowOpen()&&
         qweatherBackoffAllowsRadio();
}

bool trustedEpdSleepIsPending()
{
  uint32_t rtcWord=0;
  if(!readRtcSchedule(rtcWord)) return false;
  byte* rtcMem=(byte*)&rtcWord;
  return rtcMem[3]==1||rtcMem[3]==2;
}

unsigned long currentWakeBudgetMs()
{
  return epd_type_index==WF32?WF32_WAKE_BUDGET_MS:DEFAULT_WAKE_BUDGET_MS;
}

void parkHighPowerHardware()
{
  pinMode(12,OUTPUT);
  ADC_OFF;

  // Resetting the panel cancels a stuck refresh and turns off its charge-pump
  // sequence. Restore the ESP8266 boot-strap levels before timed deep sleep.
  pinMode(RST,OUTPUT);
  EPD_RST_0;
  // This helper can run from Ticker's yield-scheduled CONT callback. Avoid a
  // nested yield while forcing the panel controller out of an active cycle.
  delayMicroseconds(10000);
  EPD_RST_1;
  pinMode(CS,OUTPUT);
  EPD_CS_0;
}

void sleepAfterPowerFault()
{
  wakeDeadlineTimer.detach();
  wakeDeadlineExpired=true;
  forceWeatherUpdateNextBoot();
  timeClient.advanceMilliseconds(
    (uint32_t)FAILURE_BACKOFF_SECONDS*1000UL);
  write_time_to_rtc_mem();
  parkHighPowerHardware();
  // forceWeatherUpdateNextBoot() makes the schedule due, but a persisted
  // weather backoff may still prohibit RF on that next boot. The helper reads
  // the already-advanced next-wake time, matching deepSleepInstant semantics.
  RFMode nextWakeMode=nextWakeNeedsRadio()?WAKE_RF_DEFAULT:
                                             WAKE_RF_DISABLED;
  ESP.deepSleepInstant((uint64_t)FAILURE_BACKOFF_SECONDS*1000000ULL,
                       nextWakeMode);
}

void wakeDeadlineExpiredCallback()
{
  wakeDeadlineExpired=true;
  sleepAfterPowerFault();
}

void armPowerGuard(unsigned long timeoutMs)
{
  wakeDeadlineTimer.detach();
  wakeDeadlineExpired=false;
  // The ordinary scheduled variant only runs after loop(). This sketch does
  // its work in setup(), so use the accurate variant that runs at next yield.
  // It is repeating by API design; sleepAfterPowerFault() detaches it first,
  // making this effectively one-shot.
  wakeDeadlineTimer.attach_ms_scheduled_accurate(
    timeoutMs,wakeDeadlineExpiredCallback);
}

void armNormalPowerGuard()
{
  armPowerGuard(currentWakeBudgetMs());
}

void sleepEpdOrBackoff()
{
  if(!EPD.deepsleep())
  {
    #ifdef debug
    Serial.printf("EPD shutdown failed at %lums; backing off\n",millis());
    #endif
    sleepAfterPowerFault();
  }
}

void sleepEpdAfterTimedRefreshOrBackoff()
{
  if(!EPD.deepsleepAfterTimedRefresh())
  {
    #ifdef debug
    Serial.printf("Timed-refresh EPD shutdown failed at %lums; backing off\n",
                  millis());
    #endif
    sleepAfterPowerFault();
  }
}

void enterPermanentDeepSleep()
{
  wakeDeadlineTimer.detach();
  wakeDeadlineExpired=false;
  parkHighPowerHardware();
  ESP.deepSleep(0,WAKE_RF_DISABLED);
}

float readBatteryVoltage(byte sampleCount)
{
  if(sampleCount==0) return NAN;
  pinMode(12,OUTPUT);
  ADC_ON;
  delay(2);
  unsigned long sum=0;
  for(byte i=0;i<sampleCount;i++) sum+=analogRead(A0);
  ADC_OFF;
  return (float)sum*5.7f/(1024.0f*sampleCount);
}

byte batteryPowerState()
{
  float voltage=readBatteryVoltage(12);
  #ifdef debug
  Serial.printf("Pre-network battery: %.3fV at %lums\n",voltage,millis());
  #endif
  if(isnan(voltage)||voltage<2.5f||voltage>5.2f)
    return BATTERY_STATE_IMPLAUSIBLE;
  if(voltage<=CRITICAL_BATTERY_VOLTAGE) return BATTERY_STATE_CRITICAL;
  return BATTERY_STATE_OK;
}

bool batteryProbeBackoffWasActive()
{
  uint32_t marker=0;
  return ESP.rtcUserMemoryRead(BATTERY_BACKOFF_RTC_OFFSET,&marker,
                               sizeof(marker))&&
         marker==BATTERY_BACKOFF_MAGIC;
}

void setBatteryProbeBackoff(bool active)
{
  uint32_t marker=active?BATTERY_BACKOFF_MAGIC:0;
  ESP.rtcUserMemoryWrite(BATTERY_BACKOFF_RTC_OFFSET,&marker,sizeof(marker));
}

bool portalRadioRestartWasRequested()
{
  uint32_t marker=0;
  return ESP.rtcUserMemoryRead(PORTAL_RESTART_RTC_OFFSET,&marker,
                               sizeof(marker))&&
         marker==PORTAL_RESTART_MAGIC;
}

void setPortalRadioRestart(bool active)
{
  uint32_t marker=active?PORTAL_RESTART_MAGIC:0;
  ESP.rtcUserMemoryWrite(PORTAL_RESTART_RTC_OFFSET,&marker,sizeof(marker));
}

bool automaticConfigPortalWasAttempted()
{
  uint32_t marker=0;
  return ESP.rtcUserMemoryRead(PORTAL_ATTEMPT_RTC_OFFSET,&marker,
                               sizeof(marker))&&
         marker==PORTAL_ATTEMPT_MAGIC;
}

void setAutomaticConfigPortalAttempted(bool attempted)
{
  uint32_t marker=attempted?PORTAL_ATTEMPT_MAGIC:0;
  ESP.rtcUserMemoryWrite(PORTAL_ATTEMPT_RTC_OFFSET,&marker,sizeof(marker));
}

void clearAutomaticConfigPortalAttempt()
{
  if(automaticConfigPortalWasAttempted())
    setAutomaticConfigPortalAttempted(false);
}

void restartForPortalRadio()
{
  setPortalRadioRestart(true);
  wakeDeadlineTimer.detach();
  web.stop();
  WiFi.softAPdisconnect(true);
  stopWiFiRadio();
  timeClient.advanceMilliseconds(1);
  write_time_to_rtc_mem();
  parkHighPowerHardware();
  ESP.deepSleepInstant(1000ULL,WAKE_RF_DEFAULT);
}

void sleepAfterConfigPortalBackoff(
  RFMode nextWakeMode,bool reevaluateWeatherRadioAfterAdvance)
{
  wakeDeadlineTimer.detach();
  web.stop();
  WiFi.softAPdisconnect(true);
  stopWiFiRadio();
  timeClient.advanceMilliseconds(
    (uint32_t)FAILURE_BACKOFF_SECONDS*1000UL);
  write_time_to_rtc_mem();
  if(reevaluateWeatherRadioAfterAdvance)
    nextWakeMode=nextWakeNeedsRadio()?WAKE_RF_DEFAULT:
                                        WAKE_RF_DISABLED;
  parkHighPowerHardware();
  ESP.deepSleepInstant((uint64_t)FAILURE_BACKOFF_SECONDS*1000000ULL,
                       nextWakeMode);
}

void sleepAfterImplausibleBatteryReading()
{
  setBatteryProbeBackoff(true);
  wakeDeadlineTimer.detach();
  timeClient.advanceMilliseconds(
    (uint32_t)FAILURE_BACKOFF_SECONDS*1000UL);
  write_time_to_rtc_mem();
  parkHighPowerHardware();
  // Probe again in one hour without paying the RF startup current.
  ESP.deepSleepInstant((uint64_t)FAILURE_BACKOFF_SECONDS*1000000ULL,
                       WAKE_RF_DISABLED);
}

void clearAlwaysSleepFlag()
{
  uint32_t marker=0;
  ESP.rtcUserMemoryWrite(PERMANENT_SLEEP_RTC_OFFSET,&marker,sizeof(marker));
}

bool eepromConfigMatchesRuntime()
{
  return EEPROM.read(9)==EEPROM_CACHE_MAGIC&&
         EEPROM.read(10)==EEPROM_CACHE_VERSION&&
         EEPROM.read(11)==eepromConfigCrc()&&
         readEepromQWeatherConfigHash()==currentQWeatherConfigHash()&&
         EEPROM.read(0)==epd_type_index&&
         EEPROM.read(1)==showtime&&
         EEPROM.read(2)==byte(sleeptime>>8)&&
         EEPROM.read(3)==byte(sleeptime)&&
         EEPROM.read(4)==byte(timeupdateinterval>>8)&&
         EEPROM.read(5)==byte(timeupdateinterval)&&
         EEPROM.read(6)==end_time&&
         EEPROM.read(7)==start_time&&
         EEPROM.read(8)==contrast;
}

uint8_t eepromConfigCrc()
{
  uint8_t checksum=0x5a;
  for(byte address=0;address<=15;address++)
  {
    if(address==11) continue;
    checksum^=EEPROM.read(address);
    for(byte bit=0;bit<8;bit++)
    {
      checksum=(checksum&0x80)?(uint8_t)((checksum<<1)^0x31):
                               (uint8_t)(checksum<<1);
    }
  }
  return checksum;
}

uint8_t calculateSht31Crc(const uint8_t data[2])
{
  uint8_t crc=0xff;
  for(byte dataIndex=0;dataIndex<2;dataIndex++)
  {
    crc^=data[dataIndex];
    for(byte bit=0;bit<8;bit++)
    {
      crc=(crc&0x80)?(uint8_t)((crc<<1)^0x31):(uint8_t)(crc<<1);
    }
  }
  return crc;
}

bool readSht31(float& temperature, float& humidity)
{
  Wire.beginTransmission(0x44);
  Wire.write(0x2c);
  Wire.write(0x10);
  if(Wire.endTransmission()!=0) return false;
  delay(10);

  byte received=Wire.requestFrom((uint8_t)0x44,(uint8_t)6);
  if(received!=6)
  {
    while(Wire.available()) Wire.read();
    return false;
  }
  uint8_t raw[6];
  for(byte i=0;i<6;i++) raw[i]=Wire.read();
  if(calculateSht31Crc(raw)!=raw[2]||
     calculateSht31Crc(raw+3)!=raw[5]) return false;

  uint16_t rawTemperature=((uint16_t)raw[0]<<8)|raw[1];
  uint16_t rawHumidity=((uint16_t)raw[3]<<8)|raw[4];
  temperature=175.0f*(float)rawTemperature/65535.0f-45.0f;
  humidity=100.0f*(float)rawHumidity/65535.0f;
  return !isnan(temperature)&&!isnan(humidity)&&
         temperature>=-45.0f&&temperature<=130.0f&&
         humidity>=0.0f&&humidity<=100.0f;
}

void normalizeRuntimeConfig()
{
  byte mappedIndex;
  if(epdTypeToIndex(epd_type,mappedIndex)) epd_type_index=mappedIndex;
  else if(epd_type_index>WF32) epd_type_index=WX29;
  if(showtime>1) showtime=0;
  if(sleeptime<=0||sleeptime>240) sleeptime=60;
  if(start_time>23) start_time=0;
  if(end_time==0||end_time>24) end_time=24;
  if(start_time>=end_time) {start_time=0;end_time=24;}
  update_time_range=(start_time==0&&end_time==24)?"1":"2";
  timeupdateinterval=showtime==1?60:1800;
}

bool replaceConfigWithTemp()
{
  const bool hadConfig=LittleFS.exists("/config.json");
  LittleFS.remove("/config.bak");
  if(hadConfig&&!LittleFS.rename("/config.json","/config.bak"))
    return false;
  if(LittleFS.rename("/config.tmp","/config.json"))
  {
    LittleFS.remove("/config.bak");
    return true;
  }
  if(hadConfig) LittleFS.rename("/config.bak","/config.json");
  return false;
}

bool persistRuntimeConfig()
{
  doc.clear();
  doc["config_version"]=3;
  doc["city"]=city;
  doc["epd_type"]=epd_type;
  doc["epd_type_index"]=epd_type_index;
  doc["showtime"]=showtime;
  doc["sleeptime"]=sleeptime;
  doc["start_time"]=start_time;
  doc["end_time"]=end_time;
  doc["update_time"]=update_time_range;
  doc["wifi_ssid"]=wifi_ssid;
  doc["wifi_password"]=wifi_password;
  doc["contrast"]=contrast;
  doc["qweather_host"]=qweather_host;
  doc["qweather_api_key"]=qweather_api_key;
  doc["qweather_location_id"]=qweather_location_id;
  doc["qweather_location_name"]=qweather_location_name;
  doc["qweather_air_coordinates"]=qweather_air_coordinates;
  if(doc.overflowed()) return false;

  size_t expectedLength=measureJson(doc);
  if(expectedLength==0||expectedLength>2048) return false;
  LittleFS.remove("/config.tmp");
  File configFile=LittleFS.open("/config.tmp","w");
  if(!configFile) return false;
  size_t writtenLength=serializeJson(doc,configFile);
  configFile.flush();
  bool failed=configFile.getWriteError()!=0||
              writtenLength!=expectedLength||
              configFile.size()!=expectedLength;
  configFile.close();
  if(failed)
  {
    LittleFS.remove("/config.tmp");
    return false;
  }

  File verifyFile=LittleFS.open("/config.tmp","r");
  if(!verifyFile)
  {
    LittleFS.remove("/config.tmp");
    return false;
  }
  doc.clear();
  DeserializationError verifyError=deserializeJson(doc,verifyFile);
  verifyFile.close();
  if(verifyError||!replaceConfigWithTemp())
  {
    LittleFS.remove("/config.tmp");
    return false;
  }
  return true;
}

bool epdModelIsSupported(byte index)
{
  return index<=WF32&&index!=WF58&&index!=C154;
}

bool loadConfigFromFilesystem()
{
  config_ready=false;
  auto loadValidatedConfig=[&](const char* path)->bool
  {
    if(!LittleFS.exists(path)) return false;
    File configFile=LittleFS.open(path,"r");
    if(!configFile) return false;
    size_t size=configFile.size();
    if(size==0||size>2048)
    {
      configFile.close();
      return false;
    }

    doc.clear();
    DeserializationError error=deserializeJson(doc,configFile);
    configFile.close();
    String parsedEpdType=doc["epd_type"].as<String>();
    byte parsedEpdIndex=WX29;
    int parsedShowtime=doc["showtime"]|-1;
    int parsedSleeptime=doc["sleeptime"]|-1;
    int parsedStartTime=doc["start_time"]|-1;
    int parsedEndTime=doc["end_time"]|-1;
    int parsedContrast=doc["contrast"]|-1;
    String parsedCity=doc["city"].as<String>();
    parsedCity.trim();
    int parsedConfigVersion=doc["config_version"]|1;
    bool passwordValid=jsonStringLengthInRange(doc["wifi_password"],0,0)||
                       jsonStringLengthInRange(doc["wifi_password"],8,64);
    bool baseComplete=!error&&
                       jsonStringLengthInRange(doc["wifi_ssid"],1,32)&&
                       passwordValid&&
                       heweatherclient::isValidLocation(parsedCity)&&
                       epdTypeToIndex(parsedEpdType,parsedEpdIndex)&&
                       epdModelIsSupported(parsedEpdIndex)&&
                       (parsedShowtime==0||parsedShowtime==1)&&
                       parsedSleeptime>=30&&parsedSleeptime<=240&&
                       parsedSleeptime%30==0&&
                       parsedStartTime>=0&&parsedStartTime<=23&&
                       parsedEndTime>parsedStartTime&&parsedEndTime<=24&&
                       parsedContrast>=0&&parsedContrast<=255;
    if(!baseComplete)
    {
      #ifdef debug
      Serial.printf("Config file %s is incomplete or invalid\n",path);
      if(error) Serial.println(error.c_str());
      #endif
      return false;
    }

    city=parsedCity;
    epd_type=parsedEpdType;
    showtime=parsedShowtime;
    sleeptime=parsedSleeptime;
    start_time=parsedStartTime;
    end_time=parsedEndTime;
    wifi_ssid=doc["wifi_ssid"].as<String>();
    wifi_password=doc["wifi_password"].as<String>();
    epd_type_index=parsedEpdIndex;
    contrast=parsedContrast;
    qweather_host=doc["qweather_host"].as<String>();
    qweather_host.trim();
    qweather_host.toLowerCase();
    qweather_api_key=doc["qweather_api_key"].as<String>();
    qweather_api_key.trim();
    qweather_location_id=doc["qweather_location_id"].as<String>();
    qweather_location_name=doc["qweather_location_name"].as<String>();
    qweather_air_coordinates=
      doc["qweather_air_coordinates"].as<String>();
    qweather_air_coordinates.trim();
    // Version 3 is the first direct-QWeather schema that records both a
    // language-correct Geo result and the coordinates required by Air v1.
    // Older caches are refreshed exactly once and then persisted as v3.
    bool locationIdValid=qweather_location_id.length()>0&&
                         qweather_location_id.length()<=32;
    bool locationMetadataValid=
      parsedConfigVersion>=3&&locationIdValid&&
      qweather_location_name.length()>0&&qweather_location_name.length()<=64&&
      heweatherclient::isValidAirCoordinates(qweather_air_coordinates);
    if(!locationMetadataValid)
    {
      // v2 already had a stable QWeather LocationID. Retain it so the one-time
      // zh/coordinate migration cannot jump to a different same-named city.
      if(parsedConfigVersion<2||!locationIdValid)
        qweather_location_id="";
      qweather_location_name="";
      qweather_air_coordinates="";
    }
    normalizeRuntimeConfig();
    config_ready=heweatherclient::isValidApiHost(qweather_host)&&
                 heweatherclient::isValidApiKey(qweather_api_key);
    return true;
  };

  if(loadValidatedConfig("/config.json")) return true;
  if(!loadValidatedConfig("/config.bak")) return false;
  if(!LittleFS.rename("/config.bak","/config.json"))
  {
    #ifdef debug
    Serial.println("Validated config backup could not replace primary config");
    #endif
  }
  return true;
}


void setup() { 
  
  #ifdef debug
  Serial.begin(2000000); 
  Serial.printf("Serial begins at %dms\n\n",millis()); 
  #endif
  const rst_info* resetInfo=ESP.getResetInfoPtr();
  const bool deepSleepWake=resetInfo!=nullptr&&
                           resetInfo->reason==REASON_DEEP_SLEEP_AWAKE;
  const bool externalReset=resetInfo==nullptr||
                           resetInfo->reason==REASON_DEFAULT_RST||
                           resetInfo->reason==REASON_EXT_SYS_RST;
  read_time_from_rtc_mem();  //读取时间
  #ifdef debug 
  Serial.printf("time read finish at %dms\n\n",millis()); 
  #endif
  EEPROM.begin(20);
  bool cachedConfigValid=read_config_from_eeprom();//从eeprom读取基本设置，节省时间
  epdModelTrusted=cachedConfigValid;
  // If the cache cannot prove the screen model, defer any model-specific
  // shutdown command until LittleFS has supplied a fully validated config.
  bool delayedEpdShutdownCheck=
    !cachedConfigValid&&trustedEpdSleepIsPending();
  pinMode(5,INPUT_PULLUP);
  pinMode(CS,OUTPUT);
  pinMode(DC,OUTPUT);
  pinMode(RST,OUTPUT);
  pinMode(BUSY,INPUT);
  pinMode(CLK,OUTPUT);
  pinMode(DIN,OUTPUT);
  pinMode(12,OUTPUT);
  ADC_OFF;
  if(externalReset)
  {
    clearAutomaticConfigPortalAttempt();
    clearQWeatherBackoff();
    clearAirBackoff();
  }

  const byte permanentSleepMarker=read_config();
  if(permanentSleepMarker==126&&!deepSleepWake)
  {
    // A physical reset / new upload is an explicit recovery attempt. Recheck
    // the battery below instead of requiring a full power disconnect.
    clearAlwaysSleepFlag();
    #ifdef debug
    Serial.println("External reset cleared the low-battery sleep latch");
    #endif
  }
  else if(permanentSleepMarker==126)
  {
    #ifdef debug
    Serial.println("always sleep flag=1, go to sleep");//什么也不做直接睡眠，电池没电或者长时间没连接WIFI
    #endif
    // 该 RTC 标志只能由外部复位清除；无需每小时唤醒并重复等待屏幕。
    enterPermanentDeepSleep();
    return;
  }

  const bool abnormalReset=resetInfo!=nullptr&&
    (resetInfo->reason==REASON_WDT_RST||
     resetInfo->reason==REASON_EXCEPTION_RST||
     resetInfo->reason==REASON_SOFT_WDT_RST||
     resetInfo->reason==REASON_SOFT_RESTART);
  if(abnormalReset)
  {
    #ifdef debug
    Serial.printf("Abnormal reset reason=%d; one-hour power backoff\n",
                  resetInfo->reason);
    #endif
    sleepAfterPowerFault();
    return;
  }

  #ifdef FAST_MODE
  crc=1;
  #else
  crc=deepSleepWake?1:ESP.checkFlashCRC();
  #endif
  
  if(crc==false) 
 {
  #ifdef debug
  Serial.printf("crc false!\n\n");
  #endif
  always_sleep();
  enterPermanentDeepSleep();
  return;
  }

  // EN / power-on is an explicit recovery attempt. Set this before a pending
  // EPD shutdown can deep-sleep so nextWakeNeedsRadio() selects RF_DEFAULT.
  if(!deepSleepWake) forceWeatherUpdateNextBoot();

  #ifdef debug
  Serial.printf("Change to sta mode\n\n");
  #endif
  bool portalButtonPressed=digitalRead(5)==0;
  if(portalButtonPressed)
  {
    delay(25);
    portalButtonPressed=digitalRead(5)==0;
  }
  bool resumePortalAfterRfRestart=
    deepSleepWake&&portalRadioRestartWasRequested();
  if(!deepSleepWake&&portalRadioRestartWasRequested())
    setPortalRadioRestart(false);
  if(deepSleepWake&&portalButtonPressed&&!resumePortalAfterRfRestart)
  {
    // A minute wake may have inherited RF_DISABLED. Persist the request and
    // reboot once with RF_DEFAULT so the AP can actually start.
    restartForPortalRadio();
    return;
  }
  bool portalRequested=portalButtonPressed||resumePortalAfterRfRestart;
  bool automaticPortalAttempted=automaticConfigPortalWasAttempted();
  if(cachedConfigValid) EPD.EPD_Set_Model(epd_type_index);
  #ifdef debug
  Serial.printf("EPD model index=%u, driver type=%u\n",
                epd_type_index,(unsigned)EPD.EPD_Type);
  #endif
  armNormalPowerGuard();
  // 时间局刷后的第二次唤醒只发送屏幕休眠命令，绝不启动 Wi-Fi。
  if(cachedConfigValid&&epd_type_index!=WF58&&!portalRequested) check_epd();
  #ifdef debug
  Serial.printf("check sleep command finish at %dms\n\n",millis());
  #endif

  check_rtc_mem(0);  //检测是不是第一次上电
  #ifdef debug
  Serial.printf("check_rtc at %dms\n\n",millis());
  #endif

  #ifdef POWER_TEST_FORCE_WEATHER
  if(!deepSleepWake)
  {
    forceWeatherUpdateNextBoot();
    powerTestSecondWeatherPending=true;
    #ifdef debug
    Serial.println("Power test: forcing weather update");
    #endif
  }
  #endif
  #ifdef POWER_TEST_FORCE_TIME
  if(!deepSleepWake)
  {
    timeClient.localEpoc=45240000L; // 12:34:00, test-only valid clock
    write_time_to_rtc_mem();
    uint32_t rtcWord=0;
    byte* rtcMem=(byte*)&rtcWord;
    rtcMem[2]=0;
    rtcMem[3]=0;
    writeRtcSchedule(rtcWord);
    first_run=false;
    #ifdef debug
    Serial.println("Power test: forcing time-only partial refresh");
    #endif
  }
  #endif

  // Check the supply before every ordinary minute refresh as well as before
  // Wi-Fi. A low battery must not spend another hour doing partial updates.
  bool recoveringFromBatteryProbe=deepSleepWake&&batteryProbeBackoffWasActive();
  if(!deepSleepWake&&batteryProbeBackoffWasActive())
  {
    setBatteryProbeBackoff(false);
  }
  if(!portalRequested)
  {
    byte batteryState=batteryPowerState();
    if(batteryState==BATTERY_STATE_IMPLAUSIBLE)
    {
      #ifdef debug
      Serial.println("Battery reading implausible; RF-disabled one-hour probe backoff");
      #endif
      sleepAfterImplausibleBatteryReading();
      return;
    }
    if(batteryState==BATTERY_STATE_CRITICAL)
    {
      #ifdef debug
      Serial.println("Battery critically low; sleeping until external reset");
      #endif
      setBatteryProbeBackoff(false);
      always_sleep();
      enterPermanentDeepSleep();
      return;
    }
    if(recoveringFromBatteryProbe)
    {
      // The current wake inherited WAKE_RF_DISABLED. The official ESP8266
      // core requires one short WAKE_RF_DEFAULT sleep before Wi-Fi can work.
      setBatteryProbeBackoff(false);
      forceWeatherUpdateNextBoot();
      wakeDeadlineTimer.detach();
      parkHighPowerHardware();
      ESP.deepSleepInstant(1000ULL,WAKE_RF_DEFAULT);
      return;
    }
  }
  else if(recoveringFromBatteryProbe)
  {
    // A held service button overrides the battery probe, but this wake still
    // inherited RF_DISABLED. Restart once with RF available for the portal.
    setBatteryProbeBackoff(false);
    restartForPortalRadio();
    return;
  }

  // Most WF32 minute wakes need only the RTC and the validated EEPROM cache.
  // Defer LittleFS mounting and JSON parsing until weather/config data is
  // actually required.
  bool weatherUpdateDue=false;
  if(cachedConfigValid&&deepSleepWake&&!portalRequested&&
     !automaticPortalAttempted&&
     epd_type_index!=WF58)
  {
    update_time();//仅天气到期时返回
    weatherUpdateDue=true;
  }

  // A transient mount failure must never erase fonts, images, or credentials.
  LittleFS.setConfig(LittleFSConfig(false));
  bool filesystemReady=LittleFS.begin();
  bool filesystemConfigValid=filesystemReady&&loadConfigFromFilesystem();
  if(filesystemConfigValid) epdModelTrusted=true;
  bool eepromCacheReady=
    filesystemConfigValid&&eepromConfigMatchesRuntime();
  if(filesystemConfigValid&&!eepromCacheReady)
  {
    eepromCacheReady=write_config_to_eeprom();
  }
  if(filesystemConfigValid&&!eepromCacheReady)
  {
    // Keep the fast EEPROM-only path disabled until the cache matches the
    // trusted file. Otherwise a failed commit could use an old screen model on
    // the next wake before LittleFS is mounted.
    setAutomaticConfigPortalAttempted(true);
    automaticPortalAttempted=true;
  }
  if(delayedEpdShutdownCheck&&!portalRequested)
  {
    if(filesystemConfigValid&&epd_type_index!=WF58)
    {
      EPD.EPD_Set_Model(epd_type_index);
      check_epd();
    }
    else
    {
      // With no trustworthy model, reset the panel rather than risk sending a
      // different controller's power-off sequence. Clear the stale phase and
      // retry from a bounded, RF-capable wake.
      clearEpdSleepPending();
      sleepAfterPowerFault();
      return;
    }
  }
  if(filesystemConfigValid&&!eepromCacheReady)
  {
    #ifdef debug
    Serial.println("EEPROM cache sync failed; RF-disabled safe backoff");
    #endif
    sleepAfterConfigPortalBackoff(WAKE_RF_DISABLED,false);
    return;
  }
  if(config_ready&&automaticPortalAttempted)
  {
    // The full file and early-boot cache now agree. Clear the slow-path marker
    // before update_time() can deep-sleep, otherwise every minute wake would
    // keep remounting LittleFS until the next weather request.
    setAutomaticConfigPortalAttempted(false);
    automaticPortalAttempted=false;
    if(deepSleepWake)
    {
      // This recovery wake inherited WAKE_RF_DISABLED from the EEPROM/config
      // backoff. ESP8266 cannot re-enable that radio in-place, so bridge once
      // through a short sleep before any due weather path can run.
      forceWeatherUpdateNextBoot();
      wakeDeadlineTimer.detach();
      parkHighPowerHardware();
      ESP.deepSleepInstant(1000ULL,WAKE_RF_DEFAULT);
      return;
    }
  }
  bool deferFastSleep=!config_ready||epd_type_index==WF58||portalRequested;
  if(!deferFastSleep&&!weatherUpdateDue)
  {
    update_time();//刷新时间，返回则继续向下运行
    weatherUpdateDue=true;
  }

  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);

   /*************************************************
   wifimanager
   *************************************************/
  // 损坏配置或旧版 WF58 配置必须先在门户中修复，避免深睡后延迟修复。
  if(epd_type_index==WF58) config_ready=false;
  if(!config_ready&&deepSleepWake&&!portalRequested&&
     automaticPortalAttempted)
  {
    #ifdef debug
    Serial.println("Automatic config portal already timed out; RF-disabled backoff");
    #endif
    setPortalRadioRestart(false);
    sleepAfterConfigPortalBackoff(WAKE_RF_DISABLED,false);
    return;
  }
  if(!config_ready&&deepSleepWake&&!resumePortalAfterRfRestart)
  {
    restartForPortalRadio();
    return;
  }
  if(!config_ready)
  {
    bool portalServed=StartPortal();
    if(!config_ready)
    {
      // Once an AP was actually served, do not spend another five minutes on
      // every hourly wake. A held physical button still bypasses this marker.
      if(portalServed)
      {
        setAutomaticConfigPortalAttempted(true);
        setPortalRadioRestart(false);
      }
      sleepAfterConfigPortalBackoff(portalServed?WAKE_RF_DISABLED:
                                                 WAKE_RF_DEFAULT,false);
      return;
    }
  }
  else if(portalRequested)
  {
    if(!StartPortal())
    {
      // The explicit RF bridge must not remain latched if AP startup failed.
      // Resume the ordinary due schedule after a bounded, backoff-aware sleep.
      setPortalRadioRestart(false);
      sleepAfterPowerFault();
      return;
    }
  }
  if(!config_ready||!epdModelTrusted)
  {
    // A save attempted from an otherwise valid portal can still fail while
    // syncing the early-boot cache. Never continue into a refresh with an
    // untrusted model after StartPortal() returns.
    setAutomaticConfigPortalAttempted(true);
    sleepAfterConfigPortalBackoff(WAKE_RF_DISABLED,false);
    return;
  }
  bool weatherRequestDeferred=qweatherBackoffDefersRequest();
  if(weatherRequestDeferred&&!portalRequested)
  {
    #ifdef debug
    Serial.println("QWeather request deferred; keeping local time updates");
    #endif
    // This wake was scheduled with RF because weather was due. Turn it off
    // before doing the ordinary local-time refresh and RTC bookkeeping.
    stopWiFiRadio();
    update_time();
    // update_time() normally deep-sleeps. With a one-slot schedule it returns
    // because every slot is immediately due; a corrupt RTC can also return.
    // Advance through the bounded fallback first, then choose the next boot's
    // RF mode against the updated backoff deadline.
    sleepAfterConfigPortalBackoff(WAKE_RF_DISABLED,true);
    return;
  }
  armNormalPowerGuard();
  EPD.EPD_Set_Model(epd_type_index);
  // 只有确认本次需要天气数据后才开启 STA；时间刷新路径已在上方休眠。
  if(!WiFi.mode(WIFI_STA))
  {
    #ifdef debug
    Serial.println("RF unavailable on this wake; retrying with WAKE_RF_DEFAULT");
    #endif
    sleepAfterPowerFault();
    return;
  }
  // 配置文件是凭据的唯一来源；不要先尝试 SDK 中可能过期的旧接入点。
  if(!connectWiFiWithinBudget())
  {
         #ifdef debug
      Serial.printf("Wifi connect failed at %dms \n\n",millis());
      #endif
      stopWiFiRadio();
      if(first_run)
      {
        beginEpdSpi();
        EPD.EPD_Set_Model(epd_type_index);
        EPD.EPD_init_Full();
        EPD.clearbuffer();
        EPD.fontscale=2;
        EPD.SetFont(FONT12);
        EPD.DrawUTF(0,0,"WIFI连接失败");
        EPD.DrawUTF(32,0,"请重新配置或尝试重新启动");
        EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
        sleepEpdOrBackoff();
      }
      const unsigned long retrySeconds=60UL*60UL;
      if(timeClient.localEpoc!=0)
      {
        timeClient.advanceMilliseconds(retrySeconds*1000UL);
        write_time_to_rtc_mem();
      }
      forceWeatherUpdateNextBoot();
      // WiFi is already off, so the SDK's deferred shutdown only extends this
      // failed wake. The RF mode below remains for the *next* boot.
      ESP.deepSleepInstant((uint64_t)retrySeconds*1000000ULL,
                           WAKE_RF_DEFAULT);
      return;
  }
  #ifdef debug
  Serial.printf("Wifi connected at %dms \n",millis());
  #endif
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
 
 
   
   /*************************************************
   EPPROM
   *************************************************/
heweather.city=city;
heweather.location=city;
heweather.apiHost=qweather_host;
heweather.apiKey=qweather_api_key;
heweather.locationId=qweather_location_id;
heweather.resolvedLocationName=qweather_location_name;
heweather.airCoordinates=qweather_air_coordinates;
  
/*************************************************
   update weather
*************************************************/
//heweather.city="huangdao";
heweather.EPDbuffer=&EPD.EPDbuffer[0];

updateData();
stopWiFiRadio();
updatedisplay();

}
void show_status()
{
  if(EPD.EPD_Type==OPM42||EPD.EPD_Type==DKE42_3COLOR||EPD.EPD_Type==WF42)
{ 
  if(first_run==true)
  {
      EPD.EPD_init_Full();
      EPD.clearbuffer();
      EPD.fontscale=2;
      EPD.SetFont(FONT12); 
      EPD.EPD_Set_Contrast(contrast);
      //EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
      EPD.DrawUTF(140,50,heweather.city+" 连接和风天气中....");
      //EPD.Inverse(0,16,0,400);
      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
      //EPD.EPD_Dis_Part(0,299,0,399,(unsigned char *)EPD.EPDbuffer,1);     
  }
  /*else
  {
      EPD.EPD_init_Part();
      EPD.clearbuffer();
      EPD.fontscale=1;
      EPD.SetFont(FONT12); 
      EPD.EPD_Set_Contrast(contrast);
      EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
      EPD.DrawUTF(2,14,heweather.city+"  连接和风天气中....");
      EPD.Inverse(0,16,0,400);
      EPD.EPD_Dis_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);     
    }*/
} 
 else if(EPD.EPD_Type==WX29||EPD.EPD_Type==WF29||EPD.EPD_Type==DKE29_3COLOR||EPD.EPD_Type==WF29BZ03||EPD.EPD_Type==WF32)
 {
  if(first_run==true)
  {
      EPD.EPD_init_Full();
      EPD.clearbuffer();
      EPD.fontscale=2;
      EPD.SetFont(FONT12); 
      EPD.EPD_Set_Contrast(contrast);
      //EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
      EPD.DrawUTF(64-16,20,heweather.city+" 连接和风天气中....");
      //EPD.Inverse(0,16,0,400);
      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
      //EPD.EPD_Dis_Part(0,127,0,295,(unsigned char *)EPD.EPDbuffer,1);     
  }
  } 
  }
void loop() {
 
    
    if(!heweather.timeout||first_run) sleepEpdOrBackoff();
    if(permanentSleepAfterDisplay)
    {
      #ifdef debug
      Serial.println("Low-battery page complete; entering permanent sleep");
      #endif
      enterPermanentDeepSleep();
      return;
    }
    #ifdef POWER_TEST_FORCE_WEATHER
    if(powerTestSecondWeatherPending)
    {
      powerTestSecondWeatherPending=false;
      forceWeatherUpdateNextBoot();
      timeClient.advanceMilliseconds(3000UL);
      write_time_to_rtc_mem();
      #ifdef debug
      Serial.println("Power test: deep-sleeping 3s for RTC WiFi cache check");
      #endif
      ESP.deepSleepInstant(3000000ULL,WAKE_RF_DEFAULT);
      return;
    }
    #endif
    if(showtime==1)
    {
      byte seconds=timeClient.getSeconds_byte();
      if(seconds>58) timeupdateinterval=60-seconds+60;
      else timeupdateinterval=60-seconds;     
      //timeupdateinterval=1;//暴力测试 
    }
    timeClient.advanceMilliseconds((uint32_t)timeupdateinterval*1000UL);
    //timeClient.localEpoc+=60*1000;//暴力测试
    write_time_to_rtc_mem();//save time before sleeping}
    #ifdef debug
     Serial.printf("Finish at %dms\n",millis());
    #endif
    
    // Match the RF option to what the *next* wake will do. In particular,
    // configurations whose weather interval equals the sleep interval need RF
    // immediately; WAKE_RF_DISABLED cannot be reversed after boot.
    RFMode nextWakeMode=nextWakeNeedsRadio()?WAKE_RF_DEFAULT:
                                               WAKE_RF_DISABLED;
    // All active hardware has been shut down above. Avoid the SDK's deferred
    // WiFi shutdown while preserving nextWakeMode for the following boot.
    ESP.deepSleepInstant((uint64_t)timeupdateinterval*1000000ULL,
                         nextWakeMode);
    //ESP.deepSleep(1 * 1 * 1000000,WAKE_RF_DISABLED); //暴力测试

}
void updatedisplay()
{
  // Keep the last valid e-paper image on routine network failures. This saves
  // an unnecessary refresh and guarantees that no error branch powers a panel
  // which is then left awake while the ESP sleeps.
  if(heweather.timeout&&!first_run)
  {
    #ifdef debug
    Serial.println("Weather failed; keeping previous display");
    #endif
    return;
  }

  bool sht20_found=false;
  bool sht30_found=false;
  float Hum=NAN,Temp=NAN;
  if(!heweather.timeout)
  {
    endEpdSpi();
    pinMode(12,OUTPUT);
    ADC_OFF;
    pinMode(SDA,OUTPUT);
    pinMode(SCL,OUTPUT);
    digitalWrite(SDA,LOW);
    digitalWrite(SCL,LOW);
    delay(20);
    ADC_ON;
    delay(50);
    Wire.begin(SDA,SCL);
    Wire.setClock(100000);

    // 优先读取更快的 SHT31；成功后不再重复访问 SHT20。
    for(byte attempt=0;attempt<2&&!sht30_found;attempt++)
    {
      Wire.beginTransmission(0x44);
      byte error=Wire.endTransmission();
      #ifdef debug
      Serial.printf("SHT31 i2c error=%d\n",error);
      #endif
      if(error==0)
      {
        float candidateHum=NAN;
        float candidateTemp=NAN;
        if(readSht31(candidateTemp,candidateHum))
        {
          Hum=candidateHum;
          Temp=candidateTemp;
          sht30_found=true;
        }
      }
    }

    if(!sht30_found)
    {
      for(byte attempt=0;attempt<2&&!sht20_found;attempt++)
      {
        Wire.beginTransmission(0x40);
        byte error=Wire.endTransmission();
        #ifdef debug
        Serial.printf("SHT20 i2c error=%d\n",error);
        #endif
        if(error!=0) continue;

        // DFRobot_SHT20 stores the TwoWire pointer only in initSHT20().
        // Without this call an attached 0x40 sensor dereferences nullptr and
        // can create a high-drain exception reboot loop.
        sht20.initSHT20(Wire);
        Wire.setClock(100000);
        float candidateHum=sht20.readHumidity();
        float candidateTemp=sht20.readTemperature();
        if(!isnan(candidateHum)&&!isnan(candidateTemp)&&
           candidateHum>=0.0f&&candidateHum<=100.0f&&
           candidateTemp>=-45.0f&&candidateTemp<=130.0f)
        {
          Hum=candidateHum;
          Temp=candidateTemp;
          sht20_found=true;
        }
      }
    }

    #ifdef debug
    Serial.printf("hum=%ftemp=%f\n",Hum,Temp);
    #endif
    ADC_OFF;
  }

  beginEpdSpi();
  EPD.clearbuffer();
  #ifdef debug
  Serial.printf("Weather frame render begin at %lums\n",millis());
  #endif
  if(heweather.citystr=="null") heweather.citystr="未知城市,请重新设置";
    if(showtime==1){
    if(EPD.EPD_Type==WX29||EPD.EPD_Type==WF29||EPD.EPD_Type==DKE29_3COLOR||EPD.EPD_Type==WF29BZ03)
    {
      if(heweather.timeout==false)
     {
          EPD.fontscale=1;    
          EPD.SetFont(ICON50); 
          EPD.DrawUTF(0,0,String(char(heweather.getMeteoconIcon(heweather.now_cond_index.toInt()))));
        
          EPD.SetFont(FONT12);//今天天气
          byte x1=68,y1=0;
          EPD.DrawUTF(x1,y1+32,(String)todaystr+" "+heweather.today_tmp_min+"°~"+heweather.today_tmp_max+"°");
          EPD.DrawUTF(x1+12,y1+32,heweather.today_txt_d+"/"+heweather.today_txt_n);
          EPD.SetFont(ICON32);
          unsigned char code2[]={0x00,heweather.getMeteoconIcon(heweather.today_cond_d_index.toInt())};
          EPD.DrawUnicodeChar(x1-3,y1,32,32,code2);
          EPD.DrawXline(0,125,63);
        
          EPD.SetFont(FONT12);//明天天气
          byte x2=98,y2=0;
          EPD.DrawUTF(x2,y2+32,(String)tomorrowstr+" "+heweather.tomorrow_tmp_min+"°~"+heweather.tomorrow_tmp_max+"°");
          EPD.DrawUTF(x2+12,y2+32,heweather.tomorrow_txt_d+"/"+heweather.tomorrow_txt_n);
          EPD.SetFont(ICON32);
          unsigned char code3[]={0x00,heweather.getMeteoconIcon(heweather.tomorrow_cond_d_index.toInt())};
          EPD.DrawUnicodeChar(x2-3,y2,32,32,code3);
          EPD.DrawXline(0,125,95);
         
        
          EPD.SetFont(FONT12);
          EPD.DrawXbm_P(48,0,12,12,(unsigned char *)city_icon);//位置图标
          EPD.DrawUTF(48,13,heweather.citystr);//城市名
          EPD.DrawXbm_P(48,60,12,12,(unsigned char *)aqi_icon);
          EPD.DrawUTF(48,74,heweather.qlty);//空气质量
      
         // EPD.DrawUTF(5,63,"RH:"+heweather.now_hum+"%");//湿度
          EPD.DrawUTF(0,130,heweather.year);//日期
          EPD.DrawUTF(12,130,heweather.nongli);//农历
          EPD.DrawXbm_P(96,128,12,12,(unsigned char *)message);
          EPD.DrawUTF(96,140,heweather.message);//消息
      
          EPD.DrawXbm_P(24,132,35,72,digi_num[timeClient.getHours()[0]-0x30]);
          EPD.DrawXbm_P(24,169,35,72,digi_num[timeClient.getHours()[1]-0x30]);
          EPD.DrawXbm_P(24,206,35,72,digi_num[10]);
          EPD.DrawXbm_P(24,220,35,72,digi_num[timeClient.getMinutes()[0]-0x30]);
          EPD.DrawXbm_P(24,255,35,72,digi_num[timeClient.getMinutes()[1]-0x30]);
          if(EPD.EPD_Type==WF29BZ03)
          {
          write_last_time(timeClient.getHours()[0],timeClient.getHours()[1],timeClient.getMinutes()[0],timeClient.getMinutes()[1]);
          }
             if(sht30_found==true||sht20_found==true)
            {
              EPD.DrawUTF(1,63,"室外");  EPD.DrawUTF(1,96,"室内");
              EPD.DrawUTF(16,63,heweather.now_tmp+"°");//天气实况温度
              EPD.DrawUTF(16,96,String(Temp,1)+"°");//室内温度
              EPD.DrawUTF(28,63,heweather.now_hum+"%");
              EPD.DrawUTF(28,96,String(Hum,0)+"%");//室内湿度
              EPD.DrawYline(5,40,58);EPD.DrawYline(5,40,90);
              }
            else
            {
              EPD.DrawUTF(5,63,"RH:"+heweather.now_hum+"%");
              EPD.SetFont(FONT32);
              EPD.DrawUTF(16,60,heweather.now_tmp+"°");//天气实况温度
              EPD.DrawYline(5,40,58);
              }
          
          EPD.Inverse(0,127,0,127);
          dis_batt(3,272);//电量显示
        }
        else
        {
                 if(first_run==true)
                     {
                      EPD.EPD_init_Full();
                      EPD.clearbuffer();
                      EPD.fontscale=2;
                      EPD.SetFont(FONT12); 
                      EPD.EPD_Set_Contrast(contrast);
                      EPD.DrawUTF(16,0,"和风天气请求超时");
                      EPD.DrawUTF(50,0,"检查网络连接或按左侧EN键重试");
                      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
                     }          
          
          }
    }
    else  if(EPD.EPD_Type==OPM42||EPD.EPD_Type==DKE42_3COLOR||EPD.EPD_Type==WF58||EPD.EPD_Type==WF42)
    {
      if(heweather.timeout==false)
      {
      EPD.fontscale=1;
      EPD.SetFont(FONT12); 
      EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
      EPD.DrawUTF(2,14,heweather.citystr+" 更新时间："+lastUpdate);

//日历部分
      EPD.fontscale=2;
      String displayDate=heweather.date;
      if(EPD.EPD_Type==OPM42||EPD.EPD_Type==DKE42_3COLOR||EPD.EPD_Type==WF42)
      {
        displayDate=formatMonthDayForDisplay(heweather.date);
      }
      EPD.DrawUTF(20,2,displayDate);//日期
      EPD.DrawUTF(47,2,heweather.weekday);//星期
      EPD.fontscale=1;
      EPD.DrawUTF(76,2,heweather.nongli.substring(12));//农历
      
//天气信息栏2
    int b=105;
    
    EPD.DrawXbm_P(24,132+b,35,72,digi_num[timeClient.getHours()[0]-0x30]);
    EPD.DrawXbm_P(24,169+b,35,72,digi_num[timeClient.getHours()[1]-0x30]);
    EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
    EPD.DrawXbm_P(24,220+b,35,72,digi_num[timeClient.getMinutes()[0]-0x30]);
    EPD.DrawXbm_P(24,255+b,35,72,digi_num[timeClient.getMinutes()[1]-0x30]);
    if(EPD.EPD_Type==WF42)
          {
          write_last_time(timeClient.getHours()[0],timeClient.getHours()[1],timeClient.getMinutes()[0],timeClient.getMinutes()[1]);
          }
      int x=20;int y=0; 
      EPD.SetFont(ICON80);unsigned char code[]={0x00,heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};EPD.DrawUnicodeStr(16,95,80,80,1,code);
//天气信息栏1
      x=20;y=180;
      EPD.SetFont(FONT12);EPD.fontscale=2; 
      EPD.DrawUTF(x,y,heweather.now_tmp+"°");
      EPD.DrawUTF(x+=27,y,heweather.now_cond); 
      EPD.fontscale=1; 
       if(sht30_found==true||sht20_found==true)
      {
      EPD.DrawUTF(x=76,y,"室温"+String(Temp,1)+"°");
      EPD.DrawUTF(x+=14,y,"湿度"+String(Hum,0)+"%");
      }   
      //EPD.DrawUTF(x+=30,205,heweather.now_hum+"%");
      //EPD.DrawUTF(70,120,"多云");
      EPD.fontscale=1;     
 
      EPD.DrawWeatherChart(190,216,25,375,6,6,heweather.tmax_array,heweather.tmin_array,heweather.code_d_array,heweather.code_n_array,heweather.text_d_array,heweather.text_n_array,heweather.date_array,heweather.week_array);
     
      EPD.DrawXbm_P(285,1,12,12,(unsigned char *)message);
      EPD.DrawUTF(285,14,heweather.message);       //EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,3);
      dis_batt(3,377);
      EPD.Inverse(0,16,0,400);
        
      }
      else 
    {
     EPD.EPD_Set_Model(epd_type_index);
     if(first_run==true)
     {
      EPD.EPD_init_Full();
      EPD.clearbuffer();
      EPD.fontscale=2;
      EPD.SetFont(FONT12); 
      EPD.EPD_Set_Contrast(contrast);
      EPD.DrawUTF(16,0,"和风天气请求超时");
      EPD.DrawUTF(50,0,"请检查网络连接或按左侧EN键重试");
      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
     }
     else
     {
      EPD.EPD_init_Part();
      EPD.clearbuffer();
      EPD.fontscale=1;
      EPD.SetFont(FONT12); 
      EPD.EPD_Set_Contrast(contrast);
      EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
      EPD.DrawUTF(2,14,heweather.citystr+" 和风天气超时于："+lastUpdate);
      int b=105;
      String timeoutHours=timeClient.getHours();
      String timeoutMinutes=timeClient.getMinutes();
      bool timeoutTimeValid=isValidTimeText(timeoutHours,23)&&isValidTimeText(timeoutMinutes,59);
      if(timeoutTimeValid)
      {
        EPD.DrawXbm_P(24,132+b,35,72,digi_num[timeoutHours[0]-0x30]);
        EPD.DrawXbm_P(24,169+b,35,72,digi_num[timeoutHours[1]-0x30]);
        EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
        EPD.DrawXbm_P(24,220+b,35,72,digi_num[timeoutMinutes[0]-0x30]);
        EPD.DrawXbm_P(24,255+b,35,72,digi_num[timeoutMinutes[1]-0x30]);
      }
      EPD.Inverse(0,16,0,400);
      EPD.EPD_Transfer_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);
      if(timeoutTimeValid) EPD.EPD_Transfer_Part(16,87,237,399,(unsigned char *)EPD.EPDbuffer,1);
      EPD.EPD_Update_Part();
      EPD.ReadBusy_long();
      //EPD.EPD_init_Part();
      EPD.EPD_Transfer_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);
      //EPD.EPD_Transfer_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);
      EPD.EPD_Transfer_Part(16,87,237,399,(unsigned char *)EPD.EPDbuffer,1);
      EPD.EPD_Update_Part();
      }
    
     
      }
    }
    else if (EPD.EPD_Type==WF32)
    {
            if(heweather.timeout==false)
            {
                EPD.fontscale=1;
                EPD.SetFont(FONT12); 
                EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
                EPD.DrawUTF(2,14,heweather.citystr+" 更新于"+lastUpdate);
               
                EPD.SetFont(ICON80);unsigned char code[]={0x00,heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};EPD.DrawUnicodeStr(16,16,80,80,1,code);
                
                EPD.SetFont(FONT12);EPD.fontscale=2; 
                EPD.DrawUTF(100,10,heweather.now_cond+"|"+heweather.now_tmp+"°");//天气实况温度
               // EPD.DrawUTF(130,10,heweather.now_tmp+"°|"+heweather.now_hum+"%");
               
               // EPD.DrawUTF(x+=27,y,heweather.now_cond); 
              //  EPD.DrawYline(110,122,57);
                EPD.SetFont(FONT12);
                EPD.fontscale=1; 
                
                
               /*
                EPD.SetFont(FONT12);//今天天气
                byte x1=135,y1=5;
                EPD.DrawXline(2,118,x1-5);
                EPD.DrawUTF(x1,y1+40,(String)todaystr+heweather.today_tmp_min+"°~"+heweather.today_tmp_max+"°");
                
                EPD.DrawUTF(x1+12,y1+40,heweather.today_txt_d+"/"+heweather.today_txt_n);
                EPD.SetFont(ICON32);
                unsigned char code2[]={0x00,heweather.getMeteoconIcon(heweather.today_cond_d_index.toInt())};
                EPD.DrawUnicodeChar(x1-3,y1,32,32,code2);


                EPD.SetFont(FONT12);//今天天气
                x1=170,y1=5;
                EPD.DrawXline(2,118,x1-5);
                EPD.DrawUTF(x1,y1+40,(String)tomorrowstr+heweather.tomorrow_tmp_min+"°~"+heweather.tomorrow_tmp_max+"°");
                
                EPD.DrawUTF(x1+12,y1+40,heweather.tomorrow_txt_d+"/"+heweather.tomorrow_txt_n);
                EPD.SetFont(ICON32);
                unsigned char code3[]={0x00,heweather.getMeteoconIcon(heweather.tomorrow_cond_d_index.toInt())};
                EPD.DrawUnicodeChar(x1-3,y1,32,32,code2);
            */
                
          //天气信息栏2
              int b=5;
              EPD.SetFont(FONT12);
              EPD.DrawXbm_P(15,132+b,35,72,digi_num[timeClient.getHours()[0]-0x30]);
              EPD.DrawXbm_P(15,169+b,35,72,digi_num[timeClient.getHours()[1]-0x30]);
              EPD.DrawXbm_P(15,206+b,35,72,digi_num[10]);
              EPD.DrawXbm_P(15,220+b,35,72,digi_num[timeClient.getMinutes()[0]-0x30]);
              EPD.DrawXbm_P(15,255+b,35,72,digi_num[timeClient.getMinutes()[1]-0x30]);
              write_last_time(timeClient.getHours()[0],timeClient.getHours()[1],timeClient.getMinutes()[0],timeClient.getMinutes()[1]);
             // EPD.DrawUTF(100,137,heweather.year);//日期
             // EPD.DrawUTF(114,137,heweather.nongli);//农历

              EPD.DrawWeatherChart(115,140,155,280,6,4,heweather.tmax_array,heweather.tmin_array,heweather.code_d_array,heweather.code_n_array,heweather.text_d_array,heweather.text_n_array,heweather.date_array,heweather.week_array);


              EPD.fontscale=1;
              EPD.DrawXline(2,118,130);
              EPD.DrawXbm_P(135,8,12,12,(unsigned char *)aqi_icon);
              EPD.DrawUTF(135,21,"空气质量"+heweather.qlty);//空气质量
              EPD.DrawUTF(135+17,8,heweather.date);//日期
              //EPD.DrawUTF(135+14,5,heweather.year.substring(18));//星期
              EPD.fontscale=1;
              String nongli=heweather.nongli;
              EPD.DrawUTF(135+34,8,heweather.nongli);//农历
               if(sht20_found==true||sht30_found==true)
                {
                 EPD.DrawUTF(135+51,8,"室温"+String(Temp,1)+"°"+"　湿度"+String(Hum,1)+"%");
                }
              EPD.fontscale=1;

                EPD.Inverse(0,200,0,135); dis_batt(3,277);

             }
            else
            {
              if(first_run==true)
                     {
                      EPD.EPD_init_Full();
                      EPD.clearbuffer();
                      EPD.fontscale=2;
                      EPD.SetFont(FONT12); 
                      EPD.EPD_Set_Contrast(contrast);
                      EPD.DrawUTF(16,0,"和风天气请求超时");
                      EPD.DrawUTF(50,0,"请检查网络连接或按左侧EN键重试");
                      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
                     }
              }
     }
      
    }
    else //showtime==0
    {
    if(EPD.EPD_Type==WX29||EPD.EPD_Type==WF29||EPD.EPD_Type==WF29BZ03||EPD.EPD_Type==DKE29_3COLOR)
    {
      if(heweather.timeout==false)
     {
          EPD.fontscale=1; 
        
          EPD.SetFont(ICON80);unsigned char code[]={0x00,heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};EPD.DrawUnicodeStr(0,16,80,80,1,code);
          EPD.SetFont(ICON32);unsigned char code2[]={0x00,heweather.getMeteoconIcon(heweather.today_cond_d_index.toInt())};EPD.DrawUnicodeStr(0,113,32,32,1,code2);
          EPD.SetFont(ICON32);unsigned char code3[]={0x00,heweather.getMeteoconIcon(heweather.tomorrow_cond_d_index.toInt())};EPD.DrawUnicodeStr(28,113,32,32,1,code3);
          EPD.DrawXline(114,295,30);EPD.DrawXline(114,295,57);
        
          EPD.SetFont(FONT12);
         
          EPD.DrawXbm_P(80,5,12,12,(unsigned char *)city_icon);
          EPD.DrawUTF(80,21,heweather.citystr);//城市名
          
          EPD.DrawUTF(112,70,heweather.date.substring(5, 10));
          EPD.DrawUTF(3,145,(String)todaystr+" "+heweather.today_tmp_min+"°~"+heweather.today_tmp_max+"°");
          EPD.DrawUTF(15,145,heweather.today_txt_d+"/"+heweather.today_txt_n);
          EPD.DrawUTF(32,145,(String)tomorrowstr+" "+heweather.tomorrow_tmp_min+"°~"+heweather.tomorrow_tmp_max+"°");
          EPD.DrawUTF(44,145,heweather.tomorrow_txt_d+"/"+heweather.tomorrow_txt_n);
          EPD.DrawXbm_P(61,116,12,12,(unsigned char *)aqi_icon);
          EPD.DrawUTF(61,131,airstr+heweather.qlty);
          
    
          
             if(sht30_found==true||sht20_found==true)
          {      
            EPD.DrawUTF(96,70,"RH:"+heweather.now_hum+"%");
            EPD.SetFont(FONT32);
            EPD.DrawUTF(96,5,heweather.now_tmp+"°");//天气实况温度
            EPD.DrawYline(96,127,67);
            EPD.SetFont(FONT12);
            EPD.DrawXbm_P(76,116,12,12,(unsigned char *)fl);
            EPD.DrawUTF(76,131,""+String(Temp,1)+"°"+"　"+String(Hum,1)+"%");
            EPD.DrawXbm_P(91,116,12,12,(unsigned char *)message);
            EPD.DrawUTF(91,131,heweather.message);
            }
          else
          {
            EPD.DrawUTF(96,70,"RH:"+heweather.now_hum+"%");
            EPD.SetFont(FONT32);
            EPD.DrawUTF(96,5,heweather.now_tmp+"°");//天气实况温度
            EPD.DrawYline(96,127,67);
            EPD.SetFont(FONT12);
            EPD.DrawXbm_P(76,116,12,12,(unsigned char *)message);
            EPD.DrawUTF(76,131,heweather.message);
            }
          
          
          EPD.Inverse(0,128,0,113);dis_batt(3,272);   
     }
     else
     {                 if(first_run==true)
                     {
                      EPD.EPD_init_Full();
                      EPD.clearbuffer();
                      EPD.fontscale=2;
                      EPD.SetFont(FONT12); 
                      EPD.EPD_Set_Contrast(contrast);
                      EPD.DrawUTF(16,0,"和风天气请求超时");
                      EPD.DrawUTF(50,0,"检查网络连接或按左侧EN键重试");
                      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
                     }      
      }
    }
    else if(EPD.EPD_Type==OPM42||EPD.EPD_Type==DKE42_3COLOR||EPD.EPD_Type==WF58||EPD.EPD_Type==WF42)
    {
      if(heweather.timeout==false)
      {
      EPD.fontscale=1;
      EPD.SetFont(FONT12); 
      EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
     
      EPD.DrawUTF(2,14,heweather.citystr+" 更新时间："+lastUpdate);

//日历部分
      EPD.fontscale=2;
      String displayDate=heweather.date;
      if(EPD.EPD_Type==OPM42||EPD.EPD_Type==DKE42_3COLOR||EPD.EPD_Type==WF42)
      {
        displayDate=formatMonthDayForDisplay(heweather.date);
      }
      EPD.DrawUTF(20,2,displayDate);//日期
      EPD.DrawUTF(47,2,heweather.weekday);//星期
      EPD.fontscale=1;
      EPD.DrawUTF(76,2,heweather.nongli.substring(12));//农历
      
//天气信息栏2
      int x=20;int y=0;     
      EPD.DrawXbm_P(x,280,12,12,(unsigned char *)fl);EPD.DrawUTF(x,294,"体感温度"+heweather.now_fl+"°");
      EPD.DrawXbm_P(x+=14,280,12,12,(unsigned char *)dir);EPD.DrawUTF(x,294,"风向"+heweather.now_dir);
      EPD.DrawXbm_P(x+=14,280,12,12,(unsigned char *)sc);EPD.DrawUTF(x,294,"风力"+heweather.now_sc+"级");
      EPD.DrawXbm_P(x+=14,280,12,12,(unsigned char *)pcpn);EPD.DrawUTF(x,294,"降水量"+heweather.now_pcpn);
      EPD.DrawXbm_P(x+=14,280,12,12,(unsigned char *)vis);EPD.DrawUTF(x,294,"能见度"+heweather.now_vis+"km");
      EPD.DrawXbm_P(x+=14,280,12,12,(unsigned char *)pres);EPD.DrawUTF(x,294,"气压"+heweather.now_pres+"hPa");
      

      EPD.SetFont(ICON80);unsigned char code[]={0x00,heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};EPD.DrawUnicodeStr(12,120,80,80,1,code);
//天气信息栏1
      x=20;y=210;
      EPD.SetFont(FONT12);EPD.fontscale=2; 
      EPD.DrawUTF(x,y,heweather.now_tmp+"°");
      EPD.DrawUTF(x+=27,y,heweather.now_cond); 
      EPD.fontscale=1; 
      if(sht30_found==true||sht20_found==true)
      {
      EPD.DrawUTF(x=76,y,"室温"+String(Temp,1)+"°");
      EPD.DrawUTF(x+=14,y,"湿度"+String(Hum,0)+"%");
      }   
      //EPD.DrawUTF(x+=30,205,heweather.now_hum+"%");
      //EPD.DrawUTF(70,120,"多云");
      EPD.fontscale=1;     
      
      //EPD.DrawXbm_P(x+=14,205,12,12,(unsigned char *)aqi_icon);
      
      
      //EPD.DrawXline(0,399,105);
      EPD.DrawYline(20,100,114);
      EPD.DrawYline(20,100,275);
      EPD.DrawWeatherChart(190,216,25,375,6,6,heweather.tmax_array,heweather.tmin_array,heweather.code_d_array,heweather.code_n_array,heweather.text_d_array,heweather.text_n_array,heweather.date_array,heweather.week_array);
     
      EPD.DrawXbm_P(285,1,12,12,(unsigned char *)message);
      EPD.DrawUTF(285,14,heweather.message); 
      dis_batt(3,377); 
      EPD.Inverse(0,16,0,400);   
    }
    else
    {
     EPD.EPD_Set_Model(epd_type_index);
     
     if(first_run==true)
     {
      EPD.EPD_init_Full();
      EPD.clearbuffer();
      EPD.fontscale=2;
      EPD.SetFont(FONT12); 
      EPD.EPD_Set_Contrast(contrast);
      EPD.DrawUTF(16,0,"和风天气请求超时");
      EPD.DrawUTF(50,0,"请检查网络连接或按左侧EN键重试");
      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
     }
     else
     {
      EPD.EPD_init_Part();
      EPD.clearbuffer();
      EPD.fontscale=1;
      EPD.SetFont(FONT12); 
      EPD.EPD_Set_Contrast(contrast);
      EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
      EPD.DrawUTF(2,14,heweather.citystr+" 和风天气超时于："+lastUpdate);
      /*int b=105;
    
      EPD.DrawXbm_P(24,132+b,35,72,digi_num[timeClient.getHours()[0]-0x30]);
      EPD.DrawXbm_P(24,169+b,35,72,digi_num[timeClient.getHours()[1]-0x30]);
      EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
      EPD.DrawXbm_P(24,220+b,35,72,digi_num[timeClient.getMinutes()[0]-0x30]);
      EPD.DrawXbm_P(24,255+b,35,72,digi_num[timeClient.getMinutes()[1]-0x30]);*/
      EPD.Inverse(0,16,0,400);
      EPD.EPD_Transfer_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);
      //EPD.EPD_Transfer_Part(16,87,237,399,(unsigned char *)EPD.EPDbuffer,1);
      EPD.EPD_Update_Part();
      EPD.ReadBusy_long();
      //EPD.EPD_init_Part();
      EPD.EPD_Transfer_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);
      //EPD.EPD_Transfer_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);
      //EPD.EPD_Transfer_Part(16,87,237,399,(unsigned char *)EPD.EPDbuffer,1);
      EPD.EPD_Update_Part();
      }
    
      }
    }
    else if (EPD.EPD_Type==WF32)
    {
            if(heweather.timeout==false)
            {
                EPD.fontscale=1;
                EPD.SetFont(FONT12); 
                EPD.DrawXbm_P(2,2,12,12,(unsigned char *)city_icon);
                EPD.DrawUTF(2,14,heweather.citystr+" 更新于"+lastUpdate);
               
                EPD.SetFont(ICON80);unsigned char code[]={0x00,heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};EPD.DrawUnicodeStr(16,16,80,80,1,code);
                EPD.SetFont(FONT12);EPD.fontscale=2; 
                EPD.DrawUTF(100,10,heweather.now_cond+"|"+heweather.now_tmp+"°");//天气实况温度
                EPD.SetFont(FONT12);
                EPD.fontscale=1;                
      
                EPD.SetFont(FONT12);//今天天气
                byte x1=135,y1=5;
                EPD.DrawXline(2,118,x1-5);
                EPD.DrawUTF(x1,y1+40,(String)todaystr+heweather.today_tmp_min+"°~"+heweather.today_tmp_max+"°");
                
                EPD.DrawUTF(x1+12,y1+40,heweather.today_txt_d+"/"+heweather.today_txt_n);
                EPD.SetFont(ICON32);
                unsigned char code2[]={0x00,heweather.getMeteoconIcon(heweather.today_cond_d_index.toInt())};
                EPD.DrawUnicodeChar(x1-3,y1,32,32,code2);


                EPD.SetFont(FONT12);//今天天气
                x1=170,y1=5;
                EPD.DrawXline(2,118,x1-5);
                EPD.DrawUTF(x1,y1+40,(String)tomorrowstr+heweather.tomorrow_tmp_min+"°~"+heweather.tomorrow_tmp_max+"°");
                
                EPD.DrawUTF(x1+12,y1+40,heweather.tomorrow_txt_d+"/"+heweather.tomorrow_txt_n);
                EPD.SetFont(ICON32);
                unsigned char code3[]={0x00,heweather.getMeteoconIcon(heweather.tomorrow_cond_d_index.toInt())};
                EPD.DrawUnicodeChar(x1-3,y1,32,32,code3);

              EPD.SetFont(FONT12);
              EPD.DrawXline(150,293,97);     
              EPD.DrawWeatherChart(130,140,155,280,6,4,heweather.tmax_array,heweather.tmin_array,heweather.code_d_array,heweather.code_n_array,heweather.text_d_array,heweather.text_n_array,heweather.date_array,heweather.week_array);
              //EPD.DrawXbm_P(172,140,12,12,(unsigned char *)message);
              //EPD.DrawUTF(172,154,heweather.message); 
              /*EPD.DrawXbm_P(x,y,12,12,(unsigned char *)fl);EPD.DrawUTF(x,y+14,"体感温度"+heweather.now_fl+"°");
              EPD.DrawXbm_P(x+=14,y,12,12,(unsigned char *)dir);EPD.DrawUTF(x,y+14,"风向"+heweather.now_dir);
              EPD.DrawXbm_P(x+=14,y,12,12,(unsigned char *)sc);EPD.DrawUTF(x,y+14,"风力"+heweather.now_sc+"级");
              EPD.DrawXbm_P(x+=14,y,12,12,(unsigned char *)pcpn);EPD.DrawUTF(x,y+14,"降水量"+heweather.now_pcpn);
              EPD.DrawXbm_P(x+=14,y,12,12,(unsigned char *)vis);EPD.DrawUTF(x,y+14,"能见度"+heweather.now_vis+"km");
              EPD.DrawXbm_P(x+=14,y,12,12,(unsigned char *)pres);EPD.DrawUTF(x,y+14,"气压"+heweather.now_pres+"hPa");*/
             
          //天气信息栏2
              int x=20;
              EPD.SetFont(FONT12);
              EPD.fontscale=2;
              EPD.DrawUTF(x,200,heweather.date);//日期
              EPD.DrawUTF(x+30,225,heweather.weekday);//星期
              EPD.fontscale=1;
              String nongli=heweather.nongli;
              EPD.DrawUTF(x+60,299-(nongli.length())*4-3,heweather.nongli);//农历
               if(sht20_found==true||sht30_found==true)
                {
                 EPD.DrawUTF(x+75,160,"室温"+String(Temp,1)+"°"+"　室内湿度"+String(Hum,1)+"%");
                
                }   
         
                EPD.fontscale=1;     
                           
              
                EPD.Inverse(0,200,0,135);dis_batt(3,277);   
             }
            else
            {
              if(first_run==true)
                     {
                      EPD.EPD_init_Full();
                      EPD.clearbuffer();
                      EPD.fontscale=2;
                      EPD.SetFont(FONT12); 
                      EPD.EPD_Set_Contrast(contrast);
                      EPD.DrawUTF(16,0,"和风天气请求超时");
                      EPD.DrawUTF(50,0,"请检查网络连接或按左侧EN键重试");
                      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
                     }
              }
     }
      }
   
   
   if(heweather.city!=""&&heweather.timeout==false)
   {
    #ifdef debug
    Serial.printf("Weather frame render finish at %lums\n",millis());
    Serial.printf("EPD full init begin at %lums\n",millis());
    #endif
    EPD.EPD_init_Full();
    #ifdef debug
    Serial.printf("EPD full init finish at %lums\n",millis());
    #endif
    EPD.EPD_Set_Contrast(contrast);
    #ifdef debug
    Serial.printf("EPD full refresh begin at %lums\n",millis());
    #endif
    EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);     
    #ifdef debug
    Serial.printf("EPD full refresh finish at %lums\n",millis());
    #endif
   }
   
 }
/**
 * @brief 显示电池电量
 * @param x,y 显示位置
 */
 
 void dis_batt(int16_t x, int16_t y)
{
  /*attention! calibrate it yourself */
   float batt_voltage=readBatteryVoltage(32);
   #ifdef debug
   Serial.println(String(batt_voltage)+"V"); 
   #endif

  if (batt_voltage<=3.6)  {
    EPD.clearbuffer();EPD.DrawXbm_P(39,98,100,50,(unsigned char *)needcharge);
    always_sleep();
    permanentSleepAfterDisplay=true;
    #ifdef debug
   Serial.println("baterry low");  
    #endif  
    }
  if (batt_voltage>3.6&&batt_voltage<=3.7)  EPD.DrawXbm_P(x,y,20,10,(unsigned char *)batt_1);
  if (batt_voltage>3.7&&batt_voltage<=3.8)  EPD.DrawXbm_P(x,y,20,10,(unsigned char *)batt_2);
  if (batt_voltage>3.8&&batt_voltage<=3.9)  EPD.DrawXbm_P(x,y,20,10,(unsigned char *)batt_3);
  if (batt_voltage>3.9&&batt_voltage<=4.0)  EPD.DrawXbm_P(x,y,20,10,(unsigned char *)batt_4);
  if (batt_voltage>4.0)  EPD.DrawXbm_P(x,y,20,10,(unsigned char *)batt_5);  
  }
/*
 * @brief 显示时间
 */
 void dis_time(int16_t x, int16_t y)
 {
  (void)x;
  (void)y;
  if(timeClient.localEpoc==0) return;
  const unsigned long displayMillis=
    (unsigned long)timeClient.getCurrentEpochWithUtcOffset();
  const byte displayHour=(byte)(displayMillis/3600000UL);
  const byte displayMinute=(byte)((displayMillis%3600000UL)/60000UL);
  const byte displayTime[4]={
    (byte)('0'+displayHour/10),(byte)('0'+displayHour%10),
    (byte)('0'+displayMinute/10),(byte)('0'+displayMinute%10)
  };
  
  EPD.fontscale=1;EPD.clearbuffer();  
  int b=0;
   if(EPD.EPD_Type==OPM42||EPD.EPD_Type==DKE42_3COLOR)
  { 
    b=105;
    EPD.DrawXbm_P(24,132+b,35,72,digi_num[displayTime[0]-0x30]);
    EPD.DrawXbm_P(24,169+b,35,72,digi_num[displayTime[1]-0x30]);
    EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
    EPD.DrawXbm_P(24,220+b,35,72,digi_num[displayTime[2]-0x30]);
    EPD.DrawXbm_P(24,255+b,35,72,digi_num[displayTime[3]-0x30]);
    EPD.EPD_Dis_Part(16,87,237,399,(unsigned char *)EPD.EPDbuffer,1);
  }    
    
  else if(EPD.EPD_Type==WF32)
  {
     b=5;
     //EPD.EPD_init_Part();
     char hour[2];//当前时间
     hour[0]=displayTime[0];
     hour[1]=displayTime[1];
     char minute[2];
     minute[0]=displayTime[2];
     minute[1]=displayTime[3];
     byte rtc_mem[4];//上一帧的时间
     ESP.rtcUserMemoryRead(12, (uint32_t*)&rtc_mem, sizeof(rtc_mem));
     sanitizeStoredTime(rtc_mem,displayTime);
     
     if(hour[0]!=rtc_mem[0])
     {
      EPD.frame=0;EPD.DrawXbm_P(15,132+b,35,72,digi_num[hour[0]-0x30]);
      EPD.frame=1;EPD.DrawXbm_P(15,132+b,35,72,digi_num[rtc_mem[0]-0x30]);
      }
     if(hour[1]!=rtc_mem[1])
     {
      EPD.frame=0;EPD.DrawXbm_P(15,169+b,35,72,digi_num[hour[1]-0x30]);
      EPD.frame=1;EPD.DrawXbm_P(15,169+b,35,72,digi_num[rtc_mem[1]-0x30]);
      }
     if(minute[0]!=rtc_mem[2])
     {
      EPD.frame=0;EPD.DrawXbm_P(15,220+b,35,72,digi_num[minute[0]-0x30]);
      EPD.frame=1;EPD.DrawXbm_P(15,220+b,35,72,digi_num[rtc_mem[2]-0x30]);
      }
     if(minute[1]!=rtc_mem[3])
     {
      EPD.frame=0; EPD.DrawXbm_P(15,255+b,35,72,digi_num[minute[1]-0x30]);
      EPD.frame=1; EPD.DrawXbm_P(15,255+b,35,72,digi_num[rtc_mem[3]-0x30]);
      }
     #ifdef debug
     Serial.printf("Epd transfer begin at %dms\n",millis());
     #endif
     EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer,1);
     EPD.EPD_Update();
     
     write_last_time(hour[0],hour[1],minute[0],minute[1]);

  /*int temp1=16,temp2=95;
  xStart=237;xEnd=399;
  yEnd=300-16-1;yStart=3-temp1-1;*/
  }
 else if(EPD.EPD_Type==WF29BZ03)
    {
    b=0;
    EPD.frame=0;
    EPD.DrawXbm_P(24,132+b,35,72,digi_num[displayTime[0]-0x30]);
    EPD.DrawXbm_P(24,169+b,35,72,digi_num[displayTime[1]-0x30]);
    EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
    EPD.DrawXbm_P(24,220+b,35,72,digi_num[displayTime[2]-0x30]);
    EPD.DrawXbm_P(24,255+b,35,72,digi_num[displayTime[3]-0x30]);
    #ifdef debug
   Serial.println("new frame:"+String(displayTime[0]-0x30)+String(displayTime[1]-0x30)+":"+String(displayTime[2]-0x30)+String(displayTime[3]-0x30));
   #endif 
    EPD.frame=1;
    byte rtc_mem[4];
    ESP.rtcUserMemoryRead(12, (uint32_t*)&rtc_mem, sizeof(rtc_mem));
    sanitizeStoredTime(rtc_mem,displayTime);
    EPD.DrawXbm_P(24,132+b,35,72,digi_num[rtc_mem[0]-0x30]);
    EPD.DrawXbm_P(24,169+b,35,72,digi_num[rtc_mem[1]-0x30]);
    EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
    EPD.DrawXbm_P(24,220+b,35,72,digi_num[rtc_mem[2]-0x30]);
    EPD.DrawXbm_P(24,255+b,35,72,digi_num[rtc_mem[3]-0x30]);
    EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer,1);
    EPD.EPD_Update();
    write_last_time(displayTime[0],displayTime[1],displayTime[2],displayTime[3]);
    
   #ifdef debug
   Serial.println("last frame:"+String(rtc_mem[0]-48)+String(rtc_mem[1]-48)+":"+String(rtc_mem[2]-48)+String(rtc_mem[3]-48));  
  #endif  
 
    }
  else if(EPD.EPD_Type==WF42)
  {
    b=105;
    char hour[2];//当前时间
    hour[0]=displayTime[0];
    hour[1]=displayTime[1];
    char minute[2];
    minute[0]=displayTime[2];
    minute[1]=displayTime[3];
    EPD.DrawXbm_P(24,132+b,35,72,digi_num[hour[0]-0x30]);
    EPD.DrawXbm_P(24,169+b,35,72,digi_num[hour[1]-0x30]);
    EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
    EPD.DrawXbm_P(24,220+b,35,72,digi_num[minute[0]-0x30]);
    EPD.DrawXbm_P(24,255+b,35,72,digi_num[minute[1]-0x30]);
    EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer,1);

    EPD.clearbuffer();  
    byte rtc_mem[4];
    ESP.rtcUserMemoryRead(12, (uint32_t*)&rtc_mem, sizeof(rtc_mem));
    sanitizeStoredTime(rtc_mem,displayTime);
    EPD.DrawXbm_P(24,132+b,35,72,digi_num[rtc_mem[0]-0x30]);
    EPD.DrawXbm_P(24,169+b,35,72,digi_num[rtc_mem[1]-0x30]);
    EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
    EPD.DrawXbm_P(24,220+b,35,72,digi_num[rtc_mem[2]-0x30]);
    EPD.DrawXbm_P(24,255+b,35,72,digi_num[rtc_mem[3]-0x30]);      
    EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer,4);
    EPD.EPD_Update();
    write_last_time(hour[0],hour[1],minute[0],minute[1]);
    }
  else
   {
    b=0;
    EPD.DrawXbm_P(24,132+b,35,72,digi_num[displayTime[0]-0x30]);
    EPD.DrawXbm_P(24,169+b,35,72,digi_num[displayTime[1]-0x30]);
    EPD.DrawXbm_P(24,206+b,35,72,digi_num[10]);
    EPD.DrawXbm_P(24,220+b,35,72,digi_num[displayTime[2]-0x30]);
    EPD.DrawXbm_P(24,255+b,35,72,digi_num[displayTime[3]-0x30]);
    EPD.EPD_Dis_Part(24,95,128,295,(unsigned char *)EPD.EPDbuffer,1);}
 }
/*
 * @brief 更新天气，校准时间
 */
void updateData() {  
  unsigned long updateStarted=millis();
  const bool hadTrustedTime=timeClient.hasValidUnixTime();
  // Deep sleep advances the cached clock by the requested sleep duration, but
  // the ESP8266 RTC oscillator drifts. Re-sync on every scheduled radio wake
  // instead of treating a restored RTC value as permanently accurate.
  const bool ntpSynchronized=timeClient.syncNtp(3000UL);
  const bool timeReady=ntpSynchronized||hadTrustedTime;
  #ifdef debug
  Serial.printf("Trusted time: %s at %lums\n",
                timeReady?"ready":"failed",millis()-updateStarted);
  #endif
  if(!timeReady)
  {
    heweather.timeout=true;
    lastUpdate="--:--";
    return;
  }

  bool requestAir=airBackoffAllowsRequest();
  heweather.update(timeClient.getUnixEpoch(),requestAir);
  // TLS is fully stopped or aborted when update() returns. Turn off RF before
  // parsing the HTTP date or writing the resolved location to flash.
  stopWiFiRadio();

  bool resolvedMetadataValid=
    heweather.resolvedLocationId.length()>0&&
    heweather.resolvedLocationId.length()<=32&&
    heweather.resolvedLocationName.length()>0&&
    heweather.resolvedLocationName.length()<=64&&
    heweatherclient::isValidAirCoordinates(
      heweather.resolvedAirCoordinates);
  if(resolvedMetadataValid&&
     (qweather_location_id!=heweather.resolvedLocationId||
      qweather_location_name!=heweather.resolvedLocationName||
      qweather_air_coordinates!=heweather.resolvedAirCoordinates))
  {
    // Geo is independently valid even if a later forecast request fails.
    // Persisting it once prevents every bounded retry from repeating Geo/TLS.
    qweather_location_id=heweather.resolvedLocationId;
    qweather_location_name=heweather.resolvedLocationName;
    qweather_air_coordinates=heweather.resolvedAirCoordinates;
    if(!persistRuntimeConfig())
    {
      #ifdef debug
      Serial.println("Resolved QWeather location could not be persisted");
      #endif
    }
  }

  heweatherclient::FailureClass weatherFailure=heweather.failureClass();
  if(heweather.timeout&&weatherFailure==heweatherclient::FailureClass::None)
    weatherFailure=heweatherclient::FailureClass::Transient;
  bool cachedLocationRejected=
    heweather.timeout&&!heweather.geoWasRequested()&&
    qweather_location_id.length()>0&&
    (heweather.httpStatus()==400||heweather.httpStatus()==404||
     heweather.qweatherCode()==400||heweather.qweatherCode()==404);
  if(cachedLocationRejected)
  {
    // Location IDs can be retired or reorganized by the provider. A cached ID
    // rejected by a core weather endpoint gets one bounded re-resolution on
    // the next allowed cycle instead of becoming a permanent offline state.
    qweather_location_id="";
    qweather_location_name="";
    qweather_air_coordinates="";
    if(!persistRuntimeConfig())
    {
      #ifdef debug
      Serial.println("Rejected QWeather location could not be cleared");
      #endif
    }
    weatherFailure=heweatherclient::FailureClass::Transient;
  }
  uint32_t retryAfter=heweather.hasRetryAfter()
    ?heweather.retryAfterSeconds()
    :0;
  recordQWeatherResult(weatherFailure,retryAfter);
  heweatherclient::FailureClass airFailure=
    heweather.airAvailable()
      ?heweatherclient::FailureClass::None
      :heweather.airFailureClass();
  // Air v1 also uses 400 for temporarily unavailable data. Treat that
  // endpoint-specific case as timed, while 401/403/404 remain configuration
  // failures which are retried after EN or a settings save.
  if(airFailure==heweatherclient::FailureClass::Configuration&&
     (heweather.airHttpStatus()==400||
      heweather.airQWeatherCode()==400))
    airFailure=heweatherclient::FailureClass::Transient;
  if(heweather.airWasRequested()&&!heweather.airAvailable()&&
     airFailure==heweatherclient::FailureClass::None)
    airFailure=heweatherclient::FailureClass::Transient;
  uint32_t airRetryAfter=heweather.airHasRetryAfter()
    ?heweather.airRetryAfterSeconds()
    :0;
  if(heweather.airWasRequested())
    recordAirResult(airFailure,airRetryAfter);
  #ifdef debug
  Serial.printf("QWeather update: %s at %lums, heap=%u, class=%u, "
                "http=%u, code=%u, retry=%lus\n",
                heweather.timeout?"failed":"ok",
                millis()-updateStarted,ESP.getFreeHeap(),
                (unsigned int)weatherFailure,
                (unsigned int)heweather.httpStatus(),
                 (unsigned int)heweather.qweatherCode(),
                 (unsigned long)retryAfter);
  Serial.printf("QWeather air: %s, requested=%u, class=%u, "
                "http=%u, code=%u, retry=%lus\n",
                heweather.airAvailable()?"ok":
                  (heweather.airWasRequested()?"failed":"deferred"),
                (unsigned int)heweather.airWasRequested(),
                (unsigned int)airFailure,
                (unsigned int)heweather.airHttpStatus(),
                (unsigned int)heweather.airQWeatherCode(),
                (unsigned long)airRetryAfter);
  #if HEWEATHER_ENABLE_DEBUG_STATUS
  Serial.printf("QWeather debug: endpoint=%u, stage=%u\n",
                (unsigned int)heweather.debugEndpoint(),
                (unsigned int)heweather.debugStage());
  #endif
  if(!heweather.timeout)
    Serial.printf("Localized display: city=%s, weather=%s, lunar=%s, air=%s(%s)\n",
                  heweather.citystr.c_str(),heweather.now_cond.c_str(),
                  heweather.nongli.c_str(),heweather.qlty.c_str(),
                  heweather.aqi.c_str());
  #endif
  // HTTP Date is a useful fallback when UDP/NTP is blocked, but it is captured
  // during the weather request and is less precise than a successful NTP sync.
  if(!ntpSynchronized&&!heweather.timeout&&heweather.responseDate.length())
    timeClient.updateTime(heweather.responseDate);

  String currentHours=timeClient.getHours();
  String currentMinutes=timeClient.getMinutes();
  bool validTime=isValidTimeText(currentHours,23)&&isValidTimeText(currentMinutes,59);
  if(validTime) lastUpdate=currentHours+":"+currentMinutes;
  else
  {
    lastUpdate="--:--";
    if(showtime==1) heweather.timeout=true;
  }

}



/*
 * @brief 读取rtc mem中存储的睡眠之前的时间
 */
void read_time_from_rtc_mem()
{ 
 #ifdef debug
    Serial.println("Reading time from rtc mem\n\n");
  #endif
  uint32_t legacyEpoch=0;
  if(!ESP.rtcUserMemoryRead(8,&legacyEpoch,sizeof(legacyEpoch))||
     legacyEpoch>86400000UL)
    legacyEpoch=0;
  timeClient.localEpoc=legacyEpoch;

  UnixTimeRtcCache unixCache;
  if(ESP.rtcUserMemoryRead(UNIX_TIME_RTC_OFFSET,
                           (uint32_t*)&unixCache,sizeof(unixCache))&&
     unixCache.magic==UNIX_TIME_RTC_MAGIC&&
     unixCache.epoch>=1577836800UL&&
     unixCache.milliseconds<1000UL&&
     unixCache.checksum==
       (unixCache.magic^unixCache.epoch^unixCache.milliseconds^
        0xa5c39e71UL))
  {
    timeClient.restoreUnixEpoch(unixCache.epoch,
                                uint16_t(unixCache.milliseconds));
  }
  else
  {
    // Upgrade the previous whole-second cache without invalidating time on the
    // first wake after installing this firmware.
    LegacyUnixTimeRtcCache legacyUnixCache;
    if(ESP.rtcUserMemoryRead(UNIX_TIME_RTC_OFFSET,
                             (uint32_t*)&legacyUnixCache,
                             sizeof(legacyUnixCache))&&
       legacyUnixCache.magic==LEGACY_UNIX_TIME_RTC_MAGIC&&
       legacyUnixCache.epoch>=1577836800UL&&
       legacyUnixCache.checksum==
         (legacyUnixCache.magic^legacyUnixCache.epoch^0xa5c39e71UL))
      timeClient.restoreUnixEpoch(legacyUnixCache.epoch);
  }
  #ifdef debug
  uint32_t restoredUnixEpoch=0;
  uint16_t restoredUnixMilliseconds=0;
  timeClient.getUnixTime(restoredUnixEpoch,restoredUnixMilliseconds);
  Serial.printf("RTC time restored: %lu.%03u at %lums\n",
                (unsigned long)restoredUnixEpoch,
                (unsigned int)restoredUnixMilliseconds,millis());
  #endif
  }
/*
 * @brief 将当前时间写入rtc mem
 */
void write_time_to_rtc_mem()
{
  //write time to rtc before sleep
  long time_before_sleep;
  time_before_sleep=timeClient.getCurrentEpoch();
  byte rtc_mem[4];
  rtc_mem[0] = byte(time_before_sleep);
  rtc_mem[1] = byte(time_before_sleep >> 8);
  rtc_mem[2] = byte(time_before_sleep >> 16);
  rtc_mem[3] = byte(time_before_sleep >> 24);
  ESP.rtcUserMemoryWrite(8, (uint32_t*)&rtc_mem, sizeof(rtc_mem));

  if(timeClient.hasValidUnixTime())
  {
    UnixTimeRtcCache unixCache;
    unixCache.magic=UNIX_TIME_RTC_MAGIC;
    uint16_t unixMilliseconds=0;
    timeClient.getUnixTime(unixCache.epoch,unixMilliseconds);
    unixCache.milliseconds=unixMilliseconds;
    unixCache.checksum=unixCache.magic^unixCache.epoch^
                       unixCache.milliseconds^0xa5c39e71UL;
    ESP.rtcUserMemoryWrite(UNIX_TIME_RTC_OFFSET,
                           (uint32_t*)&unixCache,sizeof(unixCache));
    #ifdef debug
    Serial.printf("RTC time stored: %lu.%03lu at %lums\n",
                  (unsigned long)unixCache.epoch,
                  (unsigned long)unixCache.milliseconds,millis());
    #endif
  }
}
void write_last_time(byte hour0,byte hour1,byte minute0,byte minute1)//上次更新时的时间是多少
{
  byte rtc_mem[4];
  rtc_mem[0] = byte(hour0);
  rtc_mem[1] = byte(hour1);
  rtc_mem[2] = minute0;
  rtc_mem[3] = minute1;
  ESP.rtcUserMemoryWrite(12, (uint32_t*)&rtc_mem, sizeof(rtc_mem));
}
/*
 * @brief 读取rtcmem中第六个byte
 * @detail rtcmem中第六个byte,若等于126，则代表连接WIFI超时，或者电池没电
 */
byte read_config()
{
  uint32_t marker=0;
  bool sleepingForever=
    ESP.rtcUserMemoryRead(PERMANENT_SLEEP_RTC_OFFSET,
                          &marker,sizeof(marker))&&
    marker==PERMANENT_SLEEP_MAGIC;
  return sleepingForever?126:0;
  
  }
/*
 * @brief 将标志位写入126,不再更新时间和天气，也不刷新屏幕
 */
void always_sleep()
{
  uint32_t marker=PERMANENT_SLEEP_MAGIC;
  ESP.rtcUserMemoryWrite(PERMANENT_SLEEP_RTC_OFFSET,
                         &marker,sizeof(marker));
 }
/*
 * @brief 检测是不是第一次上电运行此程序
 */
void check_rtc_mem(bool force_init)
{
  /*
  rtc_mem[0] sign for first run
  rtc_mem[1] how many hours left
  */
 
  uint32_t rtcWord=0;
  bool rtcValid=readRtcSchedule(rtcWord);
  byte* rtc_mem=(byte*)&rtcWord;
  if (!rtcValid||force_init==1)
  {
    #ifdef debug
    Serial.println("first time to run or en pressed");
    #endif
    first_run=true;
    byte times=(byte)weatherUpdateCountTarget(timeupdateinterval);
    //Serial.println("times");Serial.println(times);
    rtc_mem[2]=times;//time
    rtc_mem[3]=0;
    if(force_init==1) rtc_mem[2]=times=0;
    #ifdef debug
    Serial.println("rctmemblock0-2");Serial.println(rtc_mem[2]);
    #endif
    //Serial.println("rctmemblock0-2");Serial.println(rtc_mem[2]);
    writeRtcSchedule(rtcWord);
    }   
    else
    {
      first_run=false;
      }
    
  }
 /*
  * @brief 检测更新时间还是天气
  * 
  */
  void update_time()
  {
    
   uint32_t rtcWord=0;
   byte times=(byte)weatherUpdateCountTarget(timeupdateinterval);
   bool rtcValid=readRtcSchedule(rtcWord);
   byte* rtc_mem=(byte*)&rtcWord;
   if(!rtcValid)
   {
     // A corrupt schedule must never choose RF_DISABLED or a random partial
     // refresh. Rebuild it as weather-due and continue to the network path.
     rtcWord=0;
     rtc_mem=(byte*)&rtcWord;
     rtc_mem[2]=times;
     rtc_mem[3]=0;
     writeRtcSchedule(rtcWord);
     first_run=true;
     return;
   }
   #ifdef debug
   Serial.println(timeClient.getFormattedTime());
   Serial.printf("Update weather after %d times of time update\r\n",times);
   Serial.printf("Time updated %d times\r\n",rtc_mem[2]);  
   #endif 

  
  bool updateWindowOpen=weatherUpdateWindowOpen();
  if(rtc_mem[2]>=times-1&&updateWindowOpen)
  
  {
    rtc_mem[2]=0;
     #ifdef debug
     Serial.println("Need to update weather");
     #endif 
     writeRtcSchedule(rtcWord);
   }
  else
   {
      if(showtime==1)
       {        
         beginEpdSpi();
         EPD.EPD_Set_Model(epd_type_index);
         #ifdef debug
         Serial.printf("Epd init begin at %dms\n",millis());
         #endif
         EPD.EPD_init_Part();       
         #ifdef debug
         Serial.printf("Epd init finish at %dms\n",millis());
         #endif
         EPD.EPD_Set_Contrast(contrast);        
        }
     
      #ifdef debug
      Serial.println("Don't need to update weather, need time");
      #endif      
       if(updateWindowOpen)
        {
          #ifdef debug
          Serial.println("Update at daytime");
          #endif      
          if(rtc_mem[2]<times-1) rtc_mem[2]++;
          writeRtcSchedule(rtcWord);
        }
     else
        {
          #ifdef debug
          Serial.println("Update at night");
          #endif 
          rtc_mem[2]=times>1?times-2:0;
          writeRtcSchedule(rtcWord);
         }
       #ifdef debug
       Serial.printf("Epd begin at %dms\n",millis());
       #endif
      if(showtime==1)
       {
         dis_time(1, 240);
         if(EPD.hasBusyTimedOut())
         {
           #ifdef debug
           Serial.println("EPD initialization timed out; backing off");
           #endif
           // deepsleep() deliberately retries shutdown even after a prior BUSY
           // timeout, with a bounded wait, so do not leave the charge pump on.
           EPD.deepsleep();
           sleepAfterPowerFault();
           return;
         }
         #ifdef debug
         Serial.printf("Epd data transfer end at %dms\n",millis());
         Serial.printf("rtcmem[2]=%dtimes-1=%d\n",rtc_mem[2],times-1);
         #endif
                //判断下次是否要开启射频
               uint32_t secondsToNextMinute=
                 60U-timeClient.getSeconds_byte();
               if(rtc_mem[2]>=times-1&&
                  weatherUpdateWindowOpenAfterSeconds(secondsToNextMinute)&&
                  qweatherBackoffAllowsRadio())
             { 
               #ifdef debug     
              Serial.printf("RF ON\n",millis());
              #endif
              rtc_mem[3]=1;//要开启
              
             }
             else 
             {
              #ifdef debug
              Serial.printf("RFCLOSE\n",millis());
              #endif
               rtc_mem[3]=2;//不开启
             
             }
         writeRtcSchedule(rtcWord);
         
         timeClient.advanceMilliseconds((uint32_t)epd_time);
         write_time_to_rtc_mem();
         ESP.deepSleepInstant(epd_time * 1 * 1000,WAKE_RF_DISABLED);         
         
         #ifdef debug
         Serial.printf("Epd finish at %dms\n",millis());
         #endif       
         timeupdateinterval=60;        
        }
      
       timeClient.advanceMilliseconds((uint32_t)timeupdateinterval*1000UL);
       write_time_to_rtc_mem();
       
       #ifdef debug
       Serial.printf("Esp finish at %dms\n",millis());
       Serial.printf("rtcmem[2]=%dtimes-1=%d\n",rtc_mem[2],times-1);
       #endif
        bool nextWakeRadio=(rtc_mem[2]>=times-1)&&
                           weatherUpdateWindowOpen()&&
                           qweatherBackoffAllowsRadio();
        #ifdef debug
        Serial.println(nextWakeRadio?"RF ON":"RFCLOSE");
        #endif
        ESP.deepSleepInstant((uint64_t)timeupdateinterval*1000000ULL,
                             nextWakeRadio?WAKE_RF_DEFAULT:
                                           WAKE_RF_DISABLED);
    }
  
 }
void check_epd()
{
  #ifdef debug
    Serial.println("Checking if sleep command is needed\n");
  #endif
   uint32_t rtcWord=0;
   if(!readRtcSchedule(rtcWord)) return;
   byte* rtc_mem=(byte*)&rtcWord;
   if(rtc_mem[3]!=1&&rtc_mem[3]!=2) return;
  const byte pendingRadioMode=rtc_mem[3];
  beginEpdSpi();
  #ifdef debug
  Serial.printf("EPD shutdown phase begin at %lums\n",millis());
  #endif
  sleepEpdAfterTimedRefreshOrBackoff();
  #ifdef debug
  Serial.printf("EPD shutdown phase finish at %lums\n",millis());
  #endif

  timeupdateinterval=60-timeClient.getSeconds_byte();
  timeClient.advanceMilliseconds((uint32_t)timeupdateinterval*1000UL);
  write_time_to_rtc_mem();
  rtc_mem[3]=0;
  writeRtcSchedule(rtcWord);

  // A prior shutdown fault may have forced an immediate weather refresh.
  // Re-evaluate an RF-disabled phase instead of blindly carrying that mode
  // into the next wake. An RF-enabled phase preserves its earlier decision.
  RFMode nextWakeMode=pendingRadioMode==1
    ?WAKE_RF_DEFAULT
    :(nextWakeNeedsRadio()?WAKE_RF_DEFAULT:WAKE_RF_DISABLED);
  #ifdef debug
  Serial.println(nextWakeMode==WAKE_RF_DEFAULT
    ?"send sleep command to epd-WAKE_RF_DEFAULT\n"
    :"send sleep command to epd-WAKE_RF_DISABLED\n");
  #endif
  ESP.deepSleepInstant((uint64_t)timeupdateinterval*1000000ULL,nextWakeMode);
}
bool write_config_to_eeprom()
{
  /*eeprom map
  *[0]epd_type
  *[1]showtime
  *[2][3]sleeptime
  *[4][5]timeupdateinterval
  *[6]end_time
  *[7]start_time 
  *[8]contrast
  *[9]magic [10]version [11]crc
  *[12..15]QWeather config hash
  */
  
  #ifdef debug
  Serial.println("EEPROM write:");
  Serial.printf("epd_type_index=%d\n",epd_type_index);
  Serial.printf("showtime=%d\n",showtime);
  Serial.printf("sleeptime=%d\n",sleeptime);
  Serial.printf("timeupdateinterval=%d\n",timeupdateinterval);
  Serial.printf("end_time=%d\n",end_time);
  Serial.printf("start_time=%d\n",start_time);
  Serial.printf("contrast=%d\n",contrast);
  #endif
   EEPROM.write(0,epd_type_index); 
   EEPROM.write(1,showtime); 
   EEPROM.write(2,sleeptime>>8); 
   EEPROM.write(3,byte(sleeptime)); 
   EEPROM.write(4,timeupdateinterval>>8); 
   EEPROM.write(5,byte(timeupdateinterval));
   EEPROM.write(6,end_time); 
   EEPROM.write(7,start_time); 
   EEPROM.write(8,contrast); 
   EEPROM.write(9,EEPROM_CACHE_MAGIC);
   EEPROM.write(10,EEPROM_CACHE_VERSION);
   uint32_t qweatherConfigHash=currentQWeatherConfigHash();
   writeEepromQWeatherConfigHash(qweatherConfigHash);
   EEPROM.write(11,eepromConfigCrc());
   bool committed=EEPROM.commit();
   eepromQWeatherConfigHashValid=committed;
   if(committed) eepromQWeatherConfigHash=qweatherConfigHash;
   return committed;
   }

bool read_config_from_eeprom()
{
  /*eeprom map
  *[0]epd_type
  *[1]showtime
  *[2][3]sleeptime
  *[4][5]timeupdateinterval
  *[6]end_time
  *[7]start_time 
  *[8]contrast
  *[9]magic [10]version [11]crc
  *[12..15]QWeather config hash
  */
  
  byte rawEpdType=EEPROM.read(0);
  byte rawShowtime=EEPROM.read(1);
  int rawSleeptime=EEPROM.read(2)<<8|EEPROM.read(3);
  int rawTimeUpdateInterval=EEPROM.read(4)<<8|EEPROM.read(5);
  byte rawEndTime=EEPROM.read(6);
  byte rawStartTime=EEPROM.read(7);
  byte rawContrast=EEPROM.read(8);
  bool semanticValid=epdModelIsSupported(rawEpdType)&&
                     rawShowtime<=1&&
                     rawSleeptime>=30&&rawSleeptime<=240&&
                     rawSleeptime%30==0&&
                     rawTimeUpdateInterval==
                       (rawShowtime==1?60:1800)&&
                     rawStartTime<=23&&
                     rawEndTime>rawStartTime&&rawEndTime<=24;
  bool cacheValid=EEPROM.read(9)==EEPROM_CACHE_MAGIC&&
                  EEPROM.read(10)==EEPROM_CACHE_VERSION&&
                  EEPROM.read(11)==eepromConfigCrc()&&
                  semanticValid;
  eepromQWeatherConfigHashValid=cacheValid;
  if(cacheValid)
  {
    eepromQWeatherConfigHash=readEepromQWeatherConfigHash();
    epd_type_index=rawEpdType;
    showtime=rawShowtime;
    sleeptime=rawSleeptime;
    timeupdateinterval=rawTimeUpdateInterval;
    end_time=rawEndTime;
    start_time=rawStartTime;
    contrast=rawContrast;
  }
  else
  {
    eepromQWeatherConfigHash=0;
    // These values are only placeholders until LittleFS is validated. In
    // particular, the default model must never become trusted on this path.
    epd_type_index=WX29;
    showtime=0;
    sleeptime=60;
    timeupdateinterval=1800;
    end_time=24;
    start_time=0;
    contrast=0x15;
  }
  update_time_range=(start_time==0&&end_time==24)?"1":"2";
  /*
 #ifdef debug
  Serial.println("EEPROM read:");
  Serial.printf("epd_type_index=%d\n",epd_type_index);
  Serial.printf("showtime=%d\n",showtime);
  Serial.printf("sleeptime=%d\n",sleeptime);
  Serial.printf("timeupdateinterval=%d\n",timeupdateinterval);
  Serial.printf("end_time=%d\n",end_time);
  Serial.printf("start_time=%d\n",start_time);
  Serial.printf("contrast=%d\n\n",contrast);
 #endif*/
  return cacheValid;
  }
void clear_wifi()
{
  //Serial.println("clear wifi");
 
  }
void handleRoot() {
  #ifdef debug
    Serial.println("root page\n\n");
   #endif
  web.sendHeader("Cache-Control","no-store, no-cache, must-revalidate");
  web.send_P(200,"text/html",QWEATHER_INDEX);
}
void handlePhoto() {
  String page;  
  page+=FPSTR(PHOTO);
  page+="<script>";
  page+=FPSTR(PHOTO_UPLOAD_FEEDBACK_JS);
  page+="</script>";
  web.send(200, "text/html", page);
}

void handleSaveConfig(){
      String requestedCity=web.arg("city");
      String requestedWifiSsid=web.arg("ssid");
      String requestedWifiPassword=web.arg("password");
      String requestedQWeatherHost=web.arg("qweather_host");
      String requestedQWeatherApiKey=web.arg("qweather_api_key");
      String requestedEpdType=web.arg("screen");
      byte requestedEpdIndex=WX29;
      String requestedShowtime=web.arg("showtime");
      String requestedInterval=web.arg("interval");
      String requestedStart=web.arg("start");
      String requestedEnd=web.arg("end");
      String requestedContrast=web.arg("contrast");
      requestedCity.trim();
      requestedQWeatherHost=normalizeQWeatherHost(requestedQWeatherHost);
      requestedQWeatherApiKey.trim();
      int requestedContrastValue=requestedContrast.toInt();
      int requestedStartValue=requestedStart.toInt();
      int requestedEndValue=requestedEnd.toInt();
      if(!epdTypeToIndex(requestedEpdType,requestedEpdIndex))
      {
        web.send(400, "text/plain", "未知屏幕类型");
        return;
      }
      if(requestedEpdIndex==WF58)
      {
        web.send(400, "text/plain", "当前固件的单缓冲架构不支持 WF58，请选择其他屏型");
        return;
      }
      if((requestedShowtime!="0"&&requestedShowtime!="1")||
         (requestedInterval!="1"&&requestedInterval!="2"&&requestedInterval!="3"&&requestedInterval!="4")||
         requestedStart.length()==0||
         requestedStart!=String(requestedStartValue)||
         requestedStartValue<0||requestedStartValue>23||
         requestedEnd.length()==0||
         requestedEnd!=String(requestedEndValue)||
         requestedEndValue<=requestedStartValue||requestedEndValue>24||
         requestedContrast.length()==0||requestedContrast!=String(requestedContrastValue)||
         requestedContrastValue<0||requestedContrastValue>255)
      {
        web.send(400, "text/plain", "配置参数超出允许范围");
        return;
      }
      String effectiveWifiPassword=requestedWifiPassword;
      if(requestedWifiSsid==wifi_ssid&&requestedWifiPassword.length()==0)
        effectiveWifiPassword=wifi_password;
      bool passwordValid=effectiveWifiPassword.length()==0||
                         stringLengthInRange(effectiveWifiPassword,8,64);
      String effectiveApiKey=requestedQWeatherApiKey;
      if(effectiveApiKey.length()==0&&requestedQWeatherHost==qweather_host)
        effectiveApiKey=qweather_api_key;
      if(!stringLengthInRange(requestedWifiSsid,1,32))
      {
        web.send(400,"text/plain",F("Wi-Fi 名称不能为空，且最多为 32 字节"));
        return;
      }
      if(!passwordValid)
      {
        web.send(400,"text/plain",F("Wi-Fi 密码应留空，或为 8～64 字节"));
        return;
      }
      if(!heweatherclient::isValidLocation(requestedCity))
      {
        web.send(400,"text/plain",F("城市或 LocationID 不能为空，且最多为 64 字节"));
        return;
      }
      if(!heweatherclient::isValidApiHost(requestedQWeatherHost))
      {
        web.send(400,"text/plain",
                 F("API Host 无效：请填写控制台“设置”中的专属 .qweatherapi.com 主机名"));
        return;
      }
      if(requestedQWeatherApiKey.length()==0&&
         requestedQWeatherHost!=qweather_host)
      {
        web.send(400,"text/plain",F("API Host 已更改，请重新输入对应的 API Key"));
        return;
      }
      if(!heweatherclient::isValidApiKey(effectiveApiKey))
      {
        web.send(400,"text/plain",
                 F("API Key 无效：请输入 20～64 位字母、数字、下划线或连字符"));
        return;
      }
      byte requestedShowtimeValue=requestedShowtime.toInt();
      int requestedSleeptime=60*requestedInterval.toInt();
      int requestedTimeUpdateInterval=requestedShowtimeValue==1?60:1800;

      bool locationChanged=requestedCity!=city||
                           requestedQWeatherHost!=qweather_host;
      // From this point the globals no longer describe the previously
      // validated panel. Restore trust only after a verified file reload or a
      // successful file + EEPROM transaction.
      epdModelTrusted=false;
      city=requestedCity;
      wifi_ssid=requestedWifiSsid;
      wifi_password=effectiveWifiPassword;
      qweather_host=requestedQWeatherHost;
      qweather_api_key=effectiveApiKey;
      if(locationChanged)
      {
        qweather_location_id="";
        qweather_location_name="";
        qweather_air_coordinates="";
      }
      epd_type=requestedEpdType;
      epd_type_index=requestedEpdIndex;
      showtime=requestedShowtimeValue;
      sleeptime=requestedSleeptime;
      start_time=(byte)requestedStartValue;
      end_time=(byte)requestedEndValue;
      update_time_range=(start_time==0&&end_time==24)?"1":"2";
      contrast=requestedContrastValue;
      timeupdateinterval=requestedTimeUpdateInterval;
      if(!persistRuntimeConfig())
      {
        bool restored=loadConfigFromFilesystem();
        epdModelTrusted=restored&&epdModelIsSupported(epd_type_index);
        if(epdModelTrusted) EPD.EPD_Set_Model(epd_type_index);
        web.send(500, "text/plain", "配置文件写入、校验或替换失败");
        return;
      }
      // Do not let this wake use the newly selected panel until the early-boot
      // EEPROM cache is guaranteed to contain the same model. A failed commit
      // followed by a two-stage refresh could otherwise send the old model's
      // shutdown sequence on the next wake.
      config_ready=false;
      if(!write_config_to_eeprom())//往 EEPROM 中同步启动缓存
      {
        setAutomaticConfigPortalAttempted(true);
        web.send(500, "text/plain", "配置文件已保存，但 EEPROM 缓存提交失败");
        return;
      }
      EPD.EPD_Set_Model(epd_type_index);
      epdModelTrusted=true;
      config_ready=true;
      clearAutomaticConfigPortalAttempt();
      clearQWeatherBackoff();
      clearAirBackoff();
      // Do not start a background STA scan while the configuration AP is still
      // serving the browser. The normal bounded connector validates the saved
      // credentials immediately after the user closes the portal.
      web.sendHeader("Cache-Control","no-store");
      web.send(200, "text/plain", "设置已保存；可关闭热点开始运行");
    }  
void handleReadSetting(){
  String page;
  doc.clear();
  doc["ssid"]=wifi_ssid;
  doc["city"]=city;
  doc["qweather_host"]=qweather_host;
  doc["contrast"]=contrast;
  doc["screen"]=epd_type;
  doc["showtime"]=showtime;
  doc["interval"]=sleeptime/60;
  doc["start"]=start_time;
  doc["end"]=end_time;
  doc["has_wifi_password"]=wifi_password.length()>0;
  doc["has_qweather_api_key"]=qweather_api_key.length()>0;
  serializeJson(doc,page);
  web.sendHeader("Cache-Control","no-store, no-cache, must-revalidate");
  web.send(200, "application/json", page);
  }
 void handleReset()
 {
  web.send(200, "text/plain", "OK");
  exit_portal=1;
  }
void handleReadStatus()
{
  float voltage;  
  long sum=0; 
  ADC_ON;
  const int sampleCount=32;
  for(int i=0;i<sampleCount;i++) sum+=analogRead(A0);
  ADC_OFF;
  voltage=(float)sum*5.7/(1024.0f*sampleCount);
  String sta_ssid;
  if(WiFi.status()==WL_CONNECTED) sta_ssid=WiFi.SSID();
  else sta_ssid="未连接";
  #ifdef debug
  //Serial.println(sta_ssid);
  #endif
  String page;
  doc.clear();
  doc["sta_ssid"]=sta_ssid;
  doc["onenet_id"]=showtime;
  doc["batt_vol"]=String(voltage)+"V";
  serializeJson(doc,page);
  web.send(200,"application/json",page);
} 
void handleFile()
{
  String page;  
  page+=FPSTR(FILE_WEBPAGE);
  page+="<script>";
  page+=FPSTR(FILE_MANAGER_SAFE_JS);
  page+="</script>";
  web.send(200, "text/html", page);
  }

String escapeJsonText(const String& value)
{
  static const char hex[]="0123456789ABCDEF";
  String escaped;
  escaped.reserve(value.length()+8);
  for(size_t i=0;i<value.length();i++)
  {
    uint8_t c=(uint8_t)value[i];
    if(c=='"'||c=='\\')
    {
      escaped+='\\';
      escaped+=(char)c;
    }
    else if(c=='\b') escaped+="\\b";
    else if(c=='\f') escaped+="\\f";
    else if(c=='\n') escaped+="\\n";
    else if(c=='\r') escaped+="\\r";
    else if(c=='\t') escaped+="\\t";
    else if(c<0x20)
    {
      escaped+="\\u00";
      escaped+=hex[c>>4];
      escaped+=hex[c&0x0f];
    }
    else escaped+=(char)c;
  }
  return escaped;
}

void handleFileList()
{
  FSInfo fs_info={};
  if(!LittleFS.info(fs_info)||fs_info.totalBytes==0)
  {
    web.send(503,"application/json","{\"error\":\"filesystem unavailable\"}");
    return;
  }
  String page;
  page.reserve(256);
  uint32_t percentage=(uint32_t)(((uint64_t)fs_info.usedBytes*100ULL)/fs_info.totalBytes);
  page+="{\"used\":\""+String(fs_info.usedBytes)+"/"+String(fs_info.totalBytes)+"\",\"percentage\":\""+String(percentage)+"\",";
  page+="\"file_list\":[";
  Dir dir = LittleFS.openDir("/");
  bool first=true;
  while (dir.next()) {
    String fileName=dir.fileName();
    String normalizedFileName=fileName;
    if(!normalizedFileName.startsWith("/")) normalizedFileName="/"+normalizedFileName;
    if(normalizedFileName==PHOTO_UPLOAD_TEMP_PATH) continue;
    fileName=escapeJsonText(fileName);
    if(!first) page+=",";
    first=false;
    page+="{\"name\":\""+fileName+"\",\"size\":\""+dir.fileSize()+"\"}";
    }
  page+="]}";
  web.send(200, "application/json", page);
  #ifdef debug
  Serial.println(page);
  #endif
  }

void failFileUpload(int status,const char* message)
{
  uploadSucceeded=false;
  uploadResponseStatus=status;
  uploadResponseMessage=message;
  if(uploadFile) uploadFile.close();
  LittleFS.remove(PHOTO_UPLOAD_TEMP_PATH);
}

void handleFileUpload()
{
  HTTPUpload& upload=web.upload();
  if(upload.status==UPLOAD_FILE_START)
  {
    if(uploadFile) uploadFile.close();
    LittleFS.remove(PHOTO_UPLOAD_TEMP_PATH);
    uploadBytesReceived=0;
    uploadSucceeded=false;
    uploadResponseStatus=400;
    uploadResponseMessage="上传未完成";
    if(upload.contentLength<PHOTO_UPLOAD_BYTES||
       upload.contentLength>PHOTO_UPLOAD_REQUEST_MAX)
    {
      failFileUpload(413,"图片上传请求大小不合法");
      web.client().stop();
      return;
    }
    if(epd_type_index!=OPM42)
    {
      failFileUpload(409,"图片灰阶显示仅支持 4.2 寸 HINK/OPM 屏");
      return;
    }
    uploadFile=LittleFS.open(PHOTO_UPLOAD_TEMP_PATH,"w");
    if(!uploadFile)
    {
      failFileUpload(500,"无法创建图片临时文件");
      return;
    }
  }
  else if(upload.status==UPLOAD_FILE_WRITE)
  {
    if(uploadResponseStatus!=400||uploadSucceeded) return;
    if(!uploadFile)
    {
      failFileUpload(500,"图片临时文件未打开");
      return;
    }
    if(upload.currentSize>PHOTO_UPLOAD_BYTES-uploadBytesReceived)
    {
      failFileUpload(413,"图片数据必须恰好为 60000 字节");
      return;
    }
    size_t bytesWritten=uploadFile.write(upload.buf,upload.currentSize);
    if(bytesWritten!=upload.currentSize||uploadFile.getWriteError()!=0)
    {
      failFileUpload(500,"图片写入失败");
      return;
    }
    uploadBytesReceived+=bytesWritten;
  }
  else if(upload.status==UPLOAD_FILE_END)
  {
    if(uploadResponseStatus!=400||uploadSucceeded) return;
    if(!uploadFile)
    {
      failFileUpload(500,"图片临时文件未打开");
      return;
    }
    if(upload.totalSize!=PHOTO_UPLOAD_BYTES||uploadBytesReceived!=PHOTO_UPLOAD_BYTES)
    {
      failFileUpload(413,"图片数据必须恰好为 60000 字节");
      return;
    }
    uploadFile.flush();
    if(uploadFile.getWriteError()!=0)
    {
      failFileUpload(500,"图片写入失败");
      return;
    }
    uploadFile.close();
    if(!LittleFS.rename(PHOTO_UPLOAD_TEMP_PATH,PHOTO_UPLOAD_PATH))
    {
      failFileUpload(500,"无法替换图片文件");
      return;
    }
    uploadSucceeded=true;
    uploadResponseStatus=200;
    uploadResponseMessage="图片已上传，正在刷新屏幕";
    pictureDisplayPending=true;
  }
  else if(upload.status==UPLOAD_FILE_ABORTED)
  {
    failFileUpload(400,"图片上传已中止");
  }
}

void handleFileUploadComplete()
{
  web.send(uploadResponseStatus,"text/plain; charset=utf-8",uploadResponseMessage);
  uploadSucceeded=false;
  uploadResponseStatus=400;
  uploadResponseMessage="未收到图片";
  uploadBytesReceived=0;
}

void handleDeleteFile()
{
  if(!web.hasArg("filename"))
  {
    web.send(400,"application/json","{\"error\":\"missing filename\"}");
    return;
  }
  String filename=web.arg("filename");
  if(!filename.startsWith("/")) filename="/"+filename;
  if(filename!=PHOTO_UPLOAD_PATH)
  {
    web.send(403,"application/json","{\"error\":\"protected file\"}");
    return;
  }
  if(!LittleFS.exists(PHOTO_UPLOAD_PATH))
  {
    web.send(404,"application/json","{\"error\":\"file not found\"}");
    return;
  }
  if(!LittleFS.remove(PHOTO_UPLOAD_PATH))
  {
    web.send(500,"application/json","{\"error\":\"delete failed\"}");
    return;
  }
  web.send(200,"application/json","{\"deleted\":\"pic.xbm\"}");
}

void showpic()
{
      if(epd_type_index!=OPM42) return;
      EPD.EPD_Set_Model(epd_type_index);
      File picture=LittleFS.open(PHOTO_UPLOAD_PATH,"r");
      if(!picture||picture.size()!=PHOTO_UPLOAD_BYTES)
      {
        if(picture) picture.close();
        return;
      }
      picture.close();
      EPD.EPD_init_Full();
      EPD.ReadBusy();
      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,3);
    
      EPD.clearbuffer();
      EPD.EPD_Write((unsigned char *)LUT_gray_opm42,sizeof(LUT_gray_opm42));
      EPD.ReadBusy();
      EPD.EPD_WriteCMD(0x22);
      EPD.EPD_WriteData(0xc5);
      EPD.EPD_WriteCMD(0x04);
      EPD.EPD_WriteData(0x32);
      EPD.EPD_WriteData(0xa8);
      EPD.EPD_WriteData(0x32);
      const unsigned long pictureRefreshStarted=millis();
      for(int i=16;i>0&&millis()-pictureRefreshStarted<30000UL;i--)
      {
        EPD.DrawXbm_spiff_gray(0,0,400,300,i);
        EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
        
        }
      EPD.EPD_WriteCMD(0x04);
      EPD.EPD_WriteData(0x41);
      EPD.EPD_WriteData(0xa8);
      EPD.EPD_WriteData(0x32);
      sleepEpdOrBackoff();
  
  }
bool StartPortal()
{
        if(!epdModelTrusted||!epdModelIsSupported(epd_type_index))
        {
          // Park an unknown/unsupported controller before any AP work. If AP
          // startup stalls or fails, a stale panel phase cannot remain powered
          // for the portal's much longer deadline.
          clearEpdSleepPending();
          parkHighPowerHardware();
        }
        armPowerGuard(PORTAL_TIMEOUT_MS+currentWakeBudgetMs());
        exit_portal=0;
        #ifdef debug
        Serial.println("Changing to AP Mode");
        #endif
        WiFi.setOutputPower(20.5);
        bool apStarted=WiFi.mode(WIFI_AP_STA)&&
          WiFi.softAP("Epaper Weather Station","",11,0,4);
        if(!apStarted)
        {
          #ifdef debug
          Serial.println("AP start failed; keeping RF restart request");
          #endif
          stopWiFiRadio();
          return false;
        }
        setPortalRadioRestart(false);
        static bool routesRegistered=false;
        if(!routesRegistered)
        {
          web.on("/", handleRoot);
          web.on("/index", handleRoot);
          web.on("/saveconfig",HTTP_POST,handleSaveConfig);
          web.on("/readsettings",handleReadSetting);
          web.on("/reset",handleReset);
          web.on("/readstatus",handleReadStatus);
          web.on("/photo",handlePhoto);
          web.on("/file",handleFile);
          web.on("/get_file_list",handleFileList);
          web.on("/delete_file",handleDeleteFile);
          web.on("/upload",HTTP_POST,handleFileUploadComplete,handleFileUpload);
          routesRegistered=true;
        }
        web.begin();
        //web.setNoDelay(true);
        if(epdModelTrusted&&epdModelIsSupported(epd_type_index))
        {
          #ifdef debug
          //Serial.println("Init EPD\n");
          #endif
          beginEpdSpi();
          EPD.EPD_Set_Model(epd_type_index);
          EPD.EPD_init_Full();

          EPD.clearbuffer();
          EPD.fontscale=2;
          EPD.SetFont(FONT12);
          if(crc==false)
         {
           EPD.DrawUTF(0,0,"FLASH校验失败！！");
           EPD.DrawUTF(36,0,"程序不完整或已更改");
           EPD.DrawUTF(72,0,"无法正常运行");
           EPD.DrawUTF(104,0,"请重新烧录");
          }
          else{
          EPD.DrawUTF(0,0,"接入点已启动");
          EPD.DrawUTF(30,0,"请用手机连接 WIFI :");
          EPD.DrawUTF(60,0,"Epaper Weather Station");
          EPD.DrawUTF(90,0,"浏览器打开192.168.4.1");
          EPD.fontscale=1;
          long espid=ESP.getChipId();
          espid=(espid>>(32-6))|(espid<<6);
          espid^=114987395;
          EPD.DrawUTF(128-12,0,"设备ID"+String(espid));
         }
          EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);
          sleepEpdOrBackoff();
          clearEpdSleepPending();
        }
        else
        {
          // The AP and web UI remain available, but no controller-specific
          // command is safe until a validated config identifies the panel.
        }
        #ifdef debug
        //Serial.println("Web server running");
        #endif
        const unsigned long portalStarted=millis();
        while(millis()-portalStarted<PORTAL_TIMEOUT_MS)
         {
           web.handleClient();
           if(pictureDisplayPending)
           {
             pictureDisplayPending=false;
             delay(1);
             if(epdModelTrusted&&epdModelIsSupported(epd_type_index)) showpic();
            }
           if(exit_portal==1) {check_rtc_mem(1);break;}
           delay(2);
          }
        if(exit_portal!=1&&config_ready) check_rtc_mem(1);
         web.stop();
         WiFi.softAPdisconnect(true);
        stopWiFiRadio();
        return true;
   }
