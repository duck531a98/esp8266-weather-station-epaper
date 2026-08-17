/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

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

#include "TimeClient.h"

namespace {

constexpr uint32_t NTP_TO_UNIX_EPOCH = 2208988800UL;
constexpr uint32_t MIN_REASONABLE_NTP_EPOCH = 1577836800UL; // 2020-01-01
constexpr uint16_t NTP_PORT = 123;
constexpr uint32_t MAX_DNS_WAIT_MS = 1000UL;
const char* const NTP_SERVERS[] = {
  "ntp.aliyun.com",
  "cn.pool.ntp.org"
};

uint32_t readBigEndian32(const byte* data) {
  return (uint32_t(data[0]) << 24) |
         (uint32_t(data[1]) << 16) |
         (uint32_t(data[2]) << 8) |
         uint32_t(data[3]);
}

void writeBigEndian32(byte* data, uint32_t value) {
  data[0] = byte(value >> 24);
  data[1] = byte(value >> 16);
  data[2] = byte(value >> 8);
  data[3] = byte(value);
}

bool isDigitAt(const String& text, int position) {
  const char c = text[position];
  return c >= '0' && c <= '9';
}

int parseTwoDigits(const String& text, int position) {
  return (text[position] - '0') * 10 + (text[position + 1] - '0');
}

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

uint8_t daysInMonth(int year, uint8_t month) {
  static const uint8_t DAYS[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };
  return month == 2 && isLeapYear(year) ? 29 : DAYS[month - 1];
}

int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yearOfEra = unsigned(year - era * 400);
  const unsigned adjustedMonth =
      month > 2 ? month - 3 : month + 9;
  const unsigned dayOfYear =
      (153 * adjustedMonth + 2) / 5 + day - 1;
  const unsigned dayOfEra =
      yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 +
      dayOfYear;
  return int64_t(era) * 146097 + int64_t(dayOfEra) - 719468;
}

int parseMonth(const String& text, int position) {
  static const char MONTHS[] = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
  for (int month = 0; month < 12; ++month) {
    bool matches = true;
    for (int character = 0; character < 3; ++character) {
      char candidate = text[position + character];
      if (candidate >= 'a' && candidate <= 'z') {
        candidate -= 'a' - 'A';
      }
      if (candidate != MONTHS[month * 3 + character]) {
        matches = false;
        break;
      }
    }
    if (matches) {
      return month + 1;
    }
  }
  return 0;
}

int parseWeekday(const String& text) {
  static const char WEEKDAYS[] = "SUNMONTUEWEDTHUFRISAT";
  for (int weekday = 0; weekday < 7; ++weekday) {
    bool matches = true;
    for (int character = 0; character < 3; ++character) {
      char candidate = text[character];
      if (candidate >= 'a' && candidate <= 'z') {
        candidate -= 'a' - 'A';
      }
      if (candidate != WEEKDAYS[weekday * 3 + character]) {
        matches = false;
        break;
      }
    }
    if (matches) {
      return weekday;
    }
  }
  return -1;
}

bool expiredSince(uint32_t startedAt, uint32_t budgetMs) {
  return uint32_t(millis() - startedAt) >= budgetMs;
}

uint32_t remainingSince(uint32_t startedAt, uint32_t budgetMs) {
  const uint32_t elapsed = uint32_t(millis() - startedAt);
  return elapsed < budgetMs ? budgetMs - elapsed : 0;
}

} // namespace

TimeClient::TimeClient(float utcOffsetHours) : myUtcOffset(utcOffsetHours) {
}

void TimeClient::setUnixTime(uint32_t unixEpoch, uint16_t milliseconds) {
  unixEpochAtUpdate = unixEpoch;
  unixMillisAtUpdate = milliseconds % 1000;
  unixTimeValid = true;
  localMillisAtUpdate = millis();
  refreshLegacyEpochFromUnix();
}

void TimeClient::refreshLegacyEpochFromUnix() {
  if (!unixTimeValid) {
    return;
  }

  const uint32_t millisecondsOfDay =
      (unixEpochAtUpdate % 86400UL) * 1000UL + unixMillisAtUpdate;
  // localEpoc == 0 has historically meant "time unavailable".
  localEpoc = millisecondsOfDay == 0 ? 86400000L : long(millisecondsOfDay);
}

bool TimeClient::syncNtp(unsigned long budgetMs) {
  if (budgetMs == 0 || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  const uint32_t syncStartedAt = millis();
  WiFiUDP udp;
  if (!udp.begin(localPort)) {
    return false;
  }

  bool synchronized = false;
  constexpr size_t SERVER_COUNT =
      sizeof(NTP_SERVERS) / sizeof(NTP_SERVERS[0]);

  for (size_t serverIndex = 0;
       serverIndex < SERVER_COUNT && !synchronized &&
           !expiredSince(syncStartedAt, budgetMs);
       ++serverIndex) {
    const uint32_t globalRemaining =
        remainingSince(syncStartedAt, budgetMs);
    const uint32_t serversRemaining = SERVER_COUNT - serverIndex;
    const uint32_t attemptBudget =
        globalRemaining / serversRemaining;
    if (attemptBudget == 0) {
      break;
    }

    const uint32_t attemptStartedAt = millis();
    IPAddress serverAddress;
    const uint32_t dnsBudget =
        min(attemptBudget, min(globalRemaining, MAX_DNS_WAIT_MS));
    if (!WiFi.hostByName(
            NTP_SERVERS[serverIndex], serverAddress, dnsBudget)) {
      continue;
    }
    if (expiredSince(syncStartedAt, budgetMs) ||
        expiredSince(attemptStartedAt, attemptBudget)) {
      continue;
    }

    while (udp.parsePacket() > 0) {
      while (udp.available()) {
        udp.read();
      }
    }

    memset(packetBuffer, 0, sizeof(packetBuffer));
    packetBuffer[0] = 0x23; // LI=0, NTP v4, client mode
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;

    // A non-zero transmit timestamp lets us verify that the reply belongs to
    // this request. It need not be accurate when the local clock is unknown.
    const uint32_t nonceSeconds =
        hasValidUnixTime()
          ? getUnixEpoch() + NTP_TO_UNIX_EPOCH
          : NTP_TO_UNIX_EPOCH + MIN_REASONABLE_NTP_EPOCH +
                (ESP.getChipId() & 0xFFFFUL);
    const uint32_t nonceFraction = micros() ^ ESP.getChipId();
    writeBigEndian32(packetBuffer + 40, nonceSeconds);
    writeBigEndian32(packetBuffer + 44, nonceFraction);

    if (!udp.beginPacket(serverAddress, NTP_PORT) ||
        udp.write(packetBuffer, sizeof(packetBuffer)) !=
            sizeof(packetBuffer) ||
        !udp.endPacket()) {
      continue;
    }

    while (!expiredSince(syncStartedAt, budgetMs) &&
           !expiredSince(attemptStartedAt, attemptBudget)) {
      const int packetSize = udp.parsePacket();
      if (packetSize <= 0) {
        delay(1);
        continue;
      }

      if (udp.remoteIP() != serverAddress ||
          udp.remotePort() != NTP_PORT ||
          packetSize < NTP_PACKET_SIZE) {
        while (udp.available()) {
          udp.read();
        }
        continue;
      }

      const int bytesRead =
          udp.read(packetBuffer, sizeof(packetBuffer));
      while (udp.available()) {
        udp.read();
      }
      if (bytesRead != NTP_PACKET_SIZE) {
        continue;
      }

      const byte leapIndicator = packetBuffer[0] >> 6;
      const byte version = (packetBuffer[0] >> 3) & 0x07;
      const byte mode = packetBuffer[0] & 0x07;
      const byte stratum = packetBuffer[1];
      if ((version != 3 && version != 4) || mode != 4) {
        continue;
      }

      // The server's originate timestamp must echo our transmit timestamp.
      byte expectedOrigin[8];
      writeBigEndian32(expectedOrigin, nonceSeconds);
      writeBigEndian32(expectedOrigin + 4, nonceFraction);
      if (memcmp(packetBuffer + 24, expectedOrigin,
                 sizeof(expectedOrigin)) != 0) {
        continue;
      }

      // A valid stratum-zero response is a Kiss-o'-Death packet. Stop waiting
      // on this server immediately and let the outer loop try the next one.
      if (stratum == 0) {
        break;
      }
      if (leapIndicator == 3 || stratum > 15) {
        continue;
      }

      const uint32_t ntpSeconds =
          readBigEndian32(packetBuffer + 40);
      const uint32_t ntpFraction =
          readBigEndian32(packetBuffer + 44);
      uint64_t fullNtpSeconds = ntpSeconds;
      if (ntpSeconds < NTP_TO_UNIX_EPOCH) {
        fullNtpSeconds += (uint64_t(1) << 32);
      }
      const uint64_t candidateUnixEpoch =
          fullNtpSeconds - NTP_TO_UNIX_EPOCH;
      if (candidateUnixEpoch < MIN_REASONABLE_NTP_EPOCH ||
          candidateUnixEpoch > UINT32_MAX) {
        continue;
      }

      const uint16_t milliseconds =
          uint16_t((uint64_t(ntpFraction) * 1000ULL) >> 32);
      setUnixTime(uint32_t(candidateUnixEpoch), milliseconds);
      synchronized = true;
      break;
    }
  }

  udp.stop();
  return synchronized;
}

bool TimeClient::updateTime(const String& httpDate) {
  String dateValue = httpDate;
  dateValue.trim();

  const int colonPosition = dateValue.indexOf(':');
  const int commaPosition = dateValue.indexOf(',');
  if (colonPosition >= 0 &&
      (commaPosition < 0 || colonPosition < commaPosition)) {
    String headerName = dateValue.substring(0, colonPosition);
    headerName.trim();
    if (!headerName.equalsIgnoreCase("Date")) {
      return false;
    }
    dateValue = dateValue.substring(colonPosition + 1);
    dateValue.trim();
  }

  // IMF-fixdate: Sun, 06 Nov 1994 08:49:37 GMT
  if (dateValue.length() != 29 ||
      dateValue[3] != ',' || dateValue[4] != ' ' ||
      dateValue[7] != ' ' || dateValue[11] != ' ' ||
      dateValue[16] != ' ' || dateValue[19] != ':' ||
      dateValue[22] != ':' || dateValue[25] != ' ' ||
      !dateValue.substring(26).equalsIgnoreCase("GMT")) {
    return false;
  }

  static const byte DIGIT_POSITIONS[] = {
    5, 6, 12, 13, 14, 15, 17, 18, 20, 21, 23, 24
  };
  for (byte position : DIGIT_POSITIONS) {
    if (!isDigitAt(dateValue, position)) {
      return false;
    }
  }

  const int weekday = parseWeekday(dateValue);
  const int day = parseTwoDigits(dateValue, 5);
  const int month = parseMonth(dateValue, 8);
  const int year =
      (dateValue[12] - '0') * 1000 +
      (dateValue[13] - '0') * 100 +
      (dateValue[14] - '0') * 10 +
      (dateValue[15] - '0');
  const int hours = parseTwoDigits(dateValue, 17);
  const int minutes = parseTwoDigits(dateValue, 20);
  const int seconds = parseTwoDigits(dateValue, 23);

  if (weekday < 0 || month == 0 ||
      day < 1 || day > daysInMonth(year, month) ||
      hours > 23 || minutes > 59 || seconds > 59) {
    return false;
  }

  const int64_t days = daysFromCivil(year, month, day);
  const int actualWeekday = int((days + 4) % 7);
  if (days < 0 || actualWeekday != weekday) {
    return false;
  }

  const uint64_t unixEpoch =
      uint64_t(days) * 86400ULL +
      uint64_t(hours) * 3600ULL +
      uint64_t(minutes) * 60ULL +
      uint64_t(seconds);
  if (unixEpoch > UINT32_MAX) {
    return false;
  }

  setUnixTime(uint32_t(unixEpoch), 0);
  return true;
}

bool TimeClient::hasValidUnixTime() const {
  return unixTimeValid;
}

uint32_t TimeClient::getUnixEpoch() const {
  uint32_t unixEpoch = 0;
  uint16_t milliseconds = 0;
  getUnixTime(unixEpoch, milliseconds);
  return unixEpoch;
}

void TimeClient::getUnixTime(uint32_t& unixEpoch,
                             uint16_t& milliseconds) const {
  if (!unixTimeValid) {
    unixEpoch = 0;
    milliseconds = 0;
    return;
  }
  const uint32_t elapsed = uint32_t(millis() - localMillisAtUpdate);
  const uint64_t elapsedMilliseconds =
      uint64_t(unixMillisAtUpdate) + elapsed;
  unixEpoch = unixEpochAtUpdate +
              uint32_t(elapsedMilliseconds / 1000ULL);
  milliseconds = uint16_t(elapsedMilliseconds % 1000ULL);
}

void TimeClient::restoreUnixEpoch(uint32_t unixEpoch,
                                  uint16_t milliseconds) {
  // Zero is reserved as "not stored" in the existing RTC-memory layout.
  if (unixEpoch == 0) {
    unixTimeValid = false;
    unixEpochAtUpdate = 0;
    unixMillisAtUpdate = 0;
    localEpoc = 0;
    localMillisAtUpdate = millis();
    return;
  }
  setUnixTime(unixEpoch, milliseconds);
}

void TimeClient::advanceMilliseconds(uint32_t milliseconds) {
  const uint32_t now = millis();
  const uint32_t elapsed = uint32_t(now - localMillisAtUpdate);

  if (localEpoc != 0) {
    const uint64_t advancedLegacy =
        uint64_t(uint32_t(localEpoc)) + elapsed + milliseconds;
    const uint32_t millisecondsOfDay =
        uint32_t(advancedLegacy % 86400000ULL);
    localEpoc =
        millisecondsOfDay == 0 ? 86400000L : long(millisecondsOfDay);
  }

  if (unixTimeValid) {
    const uint64_t advancedUnixMilliseconds =
        uint64_t(unixMillisAtUpdate) + elapsed + milliseconds;
    unixEpochAtUpdate +=
        uint32_t(advancedUnixMilliseconds / 1000ULL);
    unixMillisAtUpdate =
        uint16_t(advancedUnixMilliseconds % 1000ULL);
  }

  localMillisAtUpdate = now;
}

String TimeClient::getHours() {
    if (localEpoc == 0) {
      return "--";
    }
    int hours = ((getCurrentEpochWithUtcOffset()  % 86400000L) / 3600000) % 24;
    if (hours < 10) {
      return "0" + String(hours);
    }
    return String(hours); // print the hour (86400 equals secs per day)

}
String TimeClient::getMinutes() {
    if (localEpoc == 0) {
      return "--";
    }
    int minutes = ((getCurrentEpochWithUtcOffset() % 3600000) / 60000);
    if (minutes < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      return "0" + String(minutes);
    }
    return String(minutes);
}
String TimeClient::getSeconds() {
    if (localEpoc == 0) {
      return "--";
    }
    int seconds = getCurrentEpochWithUtcOffset() % 60000/1000;
    if ( seconds < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      return "0" + String(seconds);
    }
    return String(seconds);
}
byte TimeClient::getHours_byte()
{
   if (localEpoc == 0) {
      return 0;
    }
    int hours = ((getCurrentEpochWithUtcOffset()  % 86400000L) / 3600000) % 24;
    return (byte) hours;
  }
byte TimeClient::getMinutes_byte()
{
   if (localEpoc == 0) {
      return 0;
    }
    int minutes = ((getCurrentEpochWithUtcOffset() % 3600000) / 60000);
    return (byte)minutes;
  
  }

byte TimeClient::getSeconds_byte() {
    if (localEpoc == 0) {
      return 0;
    }
    int seconds = getCurrentEpochWithUtcOffset() % 60000/1000;
    
    return byte(seconds);
}
String TimeClient::getFormattedTime() {
  return getHours() + ":" + getMinutes();
}

long TimeClient::getCurrentEpoch() {
  return localEpoc + ((millis() - localMillisAtUpdate) );
}

long TimeClient::getCurrentEpochWithUtcOffset() {
  const int64_t utcOffsetMillis =
      int64_t(round(3600000.0f * myUtcOffset));
  int64_t localTime =
      (int64_t(getCurrentEpoch()) + utcOffsetMillis) % 86400000LL;
  if (localTime < 0) {
    localTime += 86400000LL;
  }
  return long(localTime);
}
