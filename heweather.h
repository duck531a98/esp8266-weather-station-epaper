/*
 * Direct QWeather API client.
 *
 * The public weather fields intentionally retain the names used by the
 * existing drawing code. QWeather credentials are supplied at runtime from
 * the device configuration and are never compiled into the firmware.
 */
#pragma once

#include <Arduino.h>
#include <WiFiClientSecureBearSSL.h>

// Enable only as a global compiler definition so every translation unit sees
// the same class layout. At the default value, debug state has no RAM/flash
// path in production builds.
#ifndef HEWEATHER_ENABLE_DEBUG_STATUS
#define HEWEATHER_ENABLE_DEBUG_STATUS 0
#endif

class heweatherclient
{
public:
  enum class FailureClass : uint8_t
  {
    None=0,
    Transient,
    RateLimited,
    Configuration,
    Permanent
  };

#if HEWEATHER_ENABLE_DEBUG_STATUS
  enum class DebugEndpoint : uint8_t
  {
    None=0,
    Geo,
    Daily,
    Now,
    Air
  };

  enum class DebugStage : uint8_t
  {
    None=0,
    Validation,
    Budget,
    Dns,
    Tls,
    Request,
    StatusLine,
    Headers,
    Body,
    Decode,
    Json,
    Complete
  };
#endif

  explicit heweatherclient(const char* langstring);

  bool timeout=false;
  bool rain=false;
  unsigned char* EPDbuffer=nullptr;

  String apiHost;
  String apiKey;
  String location;
  String locationId;
  String resolvedLocationId;
  String resolvedLocationName;
  // Slash-separated latitude/longitude rounded to the two decimals required
  // by QWeather Air Quality v1, for example "36.67/117.00".
  String airCoordinates;
  String resolvedAirCoordinates;
  String responseDate;

  String aqi;
  String co;
  String no2;
  String o3;
  String pm10;
  String pm25;
  String so2;
  String aqitext;
  byte airconditionbits_index=0;

  String now_cond;
  String now_hum;
  String now_tmp;
  String now_cond_index;
  String now_dir;
  String now_sc;
  String now_fl;
  String now_pcpn;
  String now_vis;
  String now_pres;

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

  String thedayaftertomorrow_cond_d;
  String thedayaftertomorrow_cond_d_index;
  String thedayaftertomorrow_cond_n;
  String thedayaftertomorrow_cond_n_index;
  String thedayaftertomorrow_tmp_max;
  String thedayaftertomorrow_tmp_min;

  String tmin_array;
  String tmax_array;
  String code_d_array;
  String code_n_array;
  String text_d_array;
  String text_n_array;
  String date_array;
  String week_array;

  String citystr;
  String date;
  String weekday;
  String year;
  String nongli;
  String message;
  String qlty;

  // Retained for source compatibility with older drawing paths.
  String t;
  String unknown;
  String city;

  byte getMeteoconIcon(int weathercodeindex);
  void update(uint32_t unixTime, bool requestAir=true);

  FailureClass failureClass() const { return failureClass_; }
  uint16_t httpStatus() const { return httpStatus_; }
  uint16_t qweatherCode() const { return qweatherCode_; }
  uint32_t retryAfterSeconds() const { return retryAfterSeconds_; }
  bool hasRetryAfter() const { return retryAfterPresent_; }
  bool airWasRequested() const { return airRequested_; }
  bool airAvailable() const { return airAvailable_; }
  FailureClass airFailureClass() const { return airFailureClass_; }
  uint16_t airHttpStatus() const { return airHttpStatus_; }
  uint16_t airQWeatherCode() const { return airQweatherCode_; }
  uint32_t airRetryAfterSeconds() const { return airRetryAfterSeconds_; }
  bool airHasRetryAfter() const { return airRetryAfterPresent_; }
  bool geoWasRequested() const { return geoRequested_; }

#if HEWEATHER_ENABLE_DEBUG_STATUS
  DebugEndpoint debugEndpoint() const { return debugEndpoint_; }
  DebugStage debugStage() const { return debugStage_; }
#endif

  static bool isValidApiHost(const String& host);
  static bool isValidApiKey(const String& key);
  static bool isValidLocation(const String& location);
  static bool isValidAirCoordinates(const String& coordinates);

private:
  const char* lang;

  void clearWeatherData();
  bool fetchJson(BearSSL::WiFiClientSecure& client,
                 const String& path,
                 bool closeConnection,
                 unsigned long networkStarted,
                 char*& json,
                 size_t& jsonLength,
                 String& httpDate,
                 uint16_t& httpStatus,
                 uint32_t& retryAfterSeconds,
                 bool& retryAfterPresent,
                 bool allowReconnect=true);
  bool parseGeo(char* json, size_t jsonLength, uint16_t& qweatherCode);
  bool parseNow(char* json, size_t jsonLength, uint16_t& qweatherCode);
  bool parseDaily(char* json, size_t jsonLength, uint16_t& qweatherCode);
  bool parseAir(char* json, size_t jsonLength, uint16_t& qweatherCode);

  FailureClass failureClass_=FailureClass::None;
  uint16_t httpStatus_=0;
  uint16_t qweatherCode_=0;
  uint32_t retryAfterSeconds_=0;
  bool retryAfterPresent_=false;
  bool airRequested_=false;
  bool airAvailable_=false;
  FailureClass airFailureClass_=FailureClass::None;
  uint16_t airHttpStatus_=0;
  uint16_t airQweatherCode_=0;
  uint32_t airRetryAfterSeconds_=0;
  bool airRetryAfterPresent_=false;
  bool geoRequested_=false;

#if HEWEATHER_ENABLE_DEBUG_STATUS
  DebugEndpoint debugEndpoint_=DebugEndpoint::None;
  DebugStage debugStage_=DebugStage::None;
#endif
};
