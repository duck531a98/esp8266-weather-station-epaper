#include "heweather.h"

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <uzlib.h>

#if HEWEATHER_ENABLE_DEBUG_STATUS
#define HEWEATHER_DEBUG_ENDPOINT(value) do { debugEndpoint_=(value); } while(0)
#define HEWEATHER_DEBUG_STAGE(value) do { debugStage_=(value); } while(0)
#else
#define HEWEATHER_DEBUG_ENDPOINT(value) do {} while(0)
#define HEWEATHER_DEBUG_STAGE(value) do {} while(0)
#endif

namespace {
const unsigned long DNS_TIMEOUT_MS=2500UL;
const unsigned long IO_TIMEOUT_MS=5500UL;
const unsigned long NETWORK_BUDGET_MS=22000UL;
const uint32_t MAX_RETRY_AFTER_SECONDS=86400UL;
const uint32_t MIN_TLS_FREE_HEAP=11500UL;
const uint32_t MIN_TLS_CONTIGUOUS_HEAP=6000UL;
const size_t MAX_COMPRESSED_BODY=4096;
const size_t MAX_JSON_BODY=8192;
const size_t MAX_HEADER_LINE=384;
// Geo, current conditions, and forecast parsing are strictly sequential. A
// shared fixed pool avoids heap fragmentation across repeated wake cycles.
StaticJsonDocument<1536> qweatherJsonData;

// Lunar years 2020-2099. Bits 0-12 store the chronological lunar-month
// lengths (1 means 30 days); bits 13-16 store the conventional leap month.
// The table is derived from the Chinese lunisolar calendar and stays in flash.
static const uint32_t CHINESE_LUNAR_YEARS[] PROGMEM={
  0x0952EUL, 0x00556UL, 0x00AB5UL, 0x055B2UL, 0x006D2UL, 0x0CEA5UL, 0x00725UL, 0x0064BUL,
  0x0AC97UL, 0x00CABUL, 0x0055AUL, 0x06AD6UL, 0x00B69UL, 0x17752UL, 0x00B52UL, 0x00B25UL,
  0x0DA4BUL, 0x00A4BUL, 0x004ABUL, 0x0A55BUL, 0x005ADUL, 0x00B6AUL, 0x05B52UL, 0x00D92UL,
  0x0FD25UL, 0x00D25UL, 0x00A55UL, 0x0B4ADUL, 0x004B6UL, 0x005B5UL, 0x06DAAUL, 0x00EC9UL,
  0x11E92UL, 0x00E92UL, 0x00D26UL, 0x0CA56UL, 0x00A57UL, 0x004D6UL, 0x086D5UL, 0x00755UL,
  0x00749UL, 0x06E93UL, 0x00693UL, 0x0F52BUL, 0x0052BUL, 0x00A5BUL, 0x0B55AUL, 0x0056AUL,
  0x00B65UL, 0x0974AUL, 0x00B4AUL, 0x11A95UL, 0x00A95UL, 0x0052DUL, 0x0CAADUL, 0x00AB5UL,
  0x005AAUL, 0x08BA5UL, 0x00DA5UL, 0x00D4AUL, 0x07C95UL, 0x00C96UL, 0x0F94EUL, 0x00556UL,
  0x00AB5UL, 0x0B5B2UL, 0x006D2UL, 0x00EA5UL, 0x08E4AUL, 0x0068BUL, 0x10C97UL, 0x004ABUL,
  0x0055BUL, 0x0CAD6UL, 0x00B6AUL, 0x00752UL, 0x09725UL, 0x00B45UL, 0x00A8BUL, 0x0549BUL
};
static_assert(sizeof(CHINESE_LUNAR_YEARS)/sizeof(CHINESE_LUNAR_YEARS[0])==80,
              "Lunar year table must cover 2020-2099");

// Public ISRG Root X1 trust anchor. No account-specific material belongs here.
static const char ISRG_ROOT_X1[] PROGMEM=R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

bool elapsed(unsigned long started, unsigned long duration)
{
  return millis()-started>=duration;
}

unsigned long remainingBudget(unsigned long started)
{
  unsigned long used=millis()-started;
  return used>=NETWORK_BUDGET_MS?0:NETWORK_BUDGET_MS-used;
}

String urlEncode(const String& value)
{
  static const char hex[]="0123456789ABCDEF";
  String encoded;
  encoded.reserve(value.length()*3);
  for(size_t i=0;i<value.length();i++)
  {
    uint8_t c=(uint8_t)value[i];
    if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
       (c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')
    {
      encoded+=(char)c;
    }
    else
    {
      encoded+='%';
      encoded+=hex[c>>4];
      encoded+=hex[c&0x0f];
    }
  }
  return encoded;
}

bool readByteWithDeadline(BearSSL::WiFiClientSecure& client,
                          unsigned long networkStarted,
                          unsigned long& lastProgress,
                          uint8_t& value)
{
  while(client.available()==0)
  {
    if(elapsed(networkStarted,NETWORK_BUDGET_MS)||
       elapsed(lastProgress,IO_TIMEOUT_MS)||
       (!client.connected()&&client.available()==0)) return false;
    delay(1);
  }
  int next=client.read();
  if(next<0) return false;
  value=(uint8_t)next;
  lastProgress=millis();
  return true;
}

bool readLineWithDeadline(BearSSL::WiFiClientSecure& client,
                          unsigned long networkStarted,
                          unsigned long& lastProgress,
                          String& line)
{
  line="";
  while(line.length()<=MAX_HEADER_LINE)
  {
    uint8_t c;
    if(!readByteWithDeadline(client,networkStarted,lastProgress,c)) return false;
    if(c=='\n')
    {
      if(line.endsWith("\r")) line.remove(line.length()-1);
      return true;
    }
    line+=(char)c;
  }
  return false;
}

bool parseDecimalSize(const String& text, size_t maximum, size_t& value)
{
  if(text.length()==0) return false;
  value=0;
  for(size_t i=0;i<text.length();i++)
  {
    char c=text[i];
    if(c<'0'||c>'9') return false;
    if(value>(maximum-(size_t)(c-'0'))/10U) return false;
    value=value*10U+(size_t)(c-'0');
  }
  return value<=maximum;
}

bool parseRetryAfterSeconds(const String& text, uint32_t& value)
{
  if(text.length()==0) return false;
  value=0;
  for(size_t i=0;i<text.length();i++)
  {
    char c=text[i];
    if(c<'0'||c>'9') return false;
    uint32_t digit=(uint32_t)(c-'0');
    if(value>(MAX_RETRY_AFTER_SECONDS-digit)/10U)
      value=MAX_RETRY_AFTER_SECONDS;
    else
      value=value*10U+digit;
  }
  return true;
}

bool parseHexSize(String text, size_t maximum, size_t& value)
{
  int extension=text.indexOf(';');
  if(extension>=0) text.remove(extension);
  text.trim();
  if(text.length()==0||text.length()>8) return false;
  value=0;
  for(size_t i=0;i<text.length();i++)
  {
    char c=text[i];
    byte digit;
    if(c>='0'&&c<='9') digit=c-'0';
    else if(c>='a'&&c<='f') digit=c-'a'+10;
    else if(c>='A'&&c<='F') digit=c-'A'+10;
    else return false;
    if(value>(maximum-digit)/16U) return false;
    value=value*16U+digit;
  }
  return value<=maximum;
}

bool headerHasToken(const String& value, const char* token)
{
  const size_t valueLength=value.length();
  const size_t tokenLength=strlen(token);
  size_t cursor=0;
  while(cursor<valueLength)
  {
    while(cursor<valueLength&&
          (value[cursor]==' '||value[cursor]=='\t'||value[cursor]==','))
      cursor++;
    size_t end=cursor;
    while(end<valueLength&&value[end]!=',') end++;
    size_t trimmedEnd=end;
    while(trimmedEnd>cursor&&
          (value[trimmedEnd-1]==' '||value[trimmedEnd-1]=='\t'))
      trimmedEnd--;
    if(trimmedEnd-cursor==tokenLength)
    {
      bool matches=true;
      for(size_t i=0;i<tokenLength;i++)
      {
        char c=value[cursor+i];
        if(c>='A'&&c<='Z') c=(char)(c+('a'-'A'));
        if(c!=token[i])
        {
          matches=false;
          break;
        }
      }
      if(matches) return true;
    }
    cursor=end+1;
  }
  return false;
}

bool parseHttpStatusLine(const String& line,
                         uint16_t& httpStatus,
                         bool& http10)
{
  httpStatus=0;
  http10=false;
  if(line.length()<12||
     !(line.startsWith("HTTP/1.0 ")||line.startsWith("HTTP/1.1 "))||
     (line.length()>12&&line[12]!=' '))
    return false;
  for(byte i=9;i<12;i++)
  {
    if(line[i]<'0'||line[i]>'9') return false;
    httpStatus=(uint16_t)(httpStatus*10U+(uint16_t)(line[i]-'0'));
  }
  if(httpStatus<100||httpStatus>599)
  {
    httpStatus=0;
    return false;
  }
  http10=line[7]=='0';
  return true;
}

bool readExact(BearSSL::WiFiClientSecure& client,
               unsigned long networkStarted,
               unsigned long& lastProgress,
               uint8_t* destination,
               size_t count)
{
  for(size_t i=0;i<count;i++)
  {
    if(!readByteWithDeadline(client,networkStarted,lastProgress,destination[i]))
      return false;
  }
  return true;
}

bool parseQWeatherIconCode(const char* value, int& code)
{
  if(value==nullptr) return false;
  code=0;
  for(byte i=0;i<3;i++)
  {
    char c=value[i];
    if(c<'0'||c>'9') return false;
    code=code*10+(c-'0');
  }
  return value[3]=='\0'&&code>=100&&code<=999;
}

bool parseQWeatherResponseCode(JsonVariantConst value, uint16_t& code)
{
  code=0;
  if(value.is<const char*>())
  {
    int parsed=0;
    if(!parseQWeatherIconCode(value.as<const char*>(),parsed)) return false;
    code=(uint16_t)parsed;
    return true;
  }
  if(value.is<unsigned int>())
  {
    unsigned int parsed=value.as<unsigned int>();
    if(parsed<100U||parsed>999U) return false;
    code=(uint16_t)parsed;
    return true;
  }
  return false;
}

heweatherclient::FailureClass classifyStatusCode(uint16_t code)
{
  if(code==0||code==200) return heweatherclient::FailureClass::None;
  if(code==402||code==429)
    return heweatherclient::FailureClass::RateLimited;
  if(code==204) return heweatherclient::FailureClass::Transient;
  if(code==400||code==401||code==403||code==404)
    return heweatherclient::FailureClass::Configuration;
  if(code==408||code==425||(code>=500&&code<=599))
    return heweatherclient::FailureClass::Transient;
  if(code>=300&&code<=499)
    return heweatherclient::FailureClass::Permanent;
  return heweatherclient::FailureClass::Transient;
}

heweatherclient::FailureClass classifyFailure(uint16_t httpStatus,
                                              uint16_t qweatherCode)
{
  heweatherclient::FailureClass result=classifyStatusCode(httpStatus);
  if(result!=heweatherclient::FailureClass::None) return result;
  result=classifyStatusCode(qweatherCode);
  if(result!=heweatherclient::FailureClass::None) return result;
  // A missing/invalid response code or structurally invalid success payload is
  // recoverable: the caller can retry it with a bounded backoff.
  return heweatherclient::FailureClass::Transient;
}

uint32_t readLittleEndian32(const uint8_t* value)
{
  return (uint32_t)value[0]|
         ((uint32_t)value[1]<<8)|
         ((uint32_t)value[2]<<16)|
         ((uint32_t)value[3]<<24);
}

bool decodeGzip(const uint8_t* compressed,
                size_t compressedLength,
                char*& decoded,
                size_t& decodedLength)
{
  decoded=nullptr;
  decodedLength=0;
  if(compressedLength<18) return false;
  uint32_t expectedLength=readLittleEndian32(compressed+compressedLength-4);
  if(expectedLength==0||expectedLength>MAX_JSON_BODY) return false;

  char* output=new(std::nothrow) char[expectedLength+1];
  if(output==nullptr) return false;

  uzlib_uncomp d;
  memset(&d,0,sizeof(d));
  uzlib_uncompress_init(&d,nullptr,0);
  d.source=compressed;
  d.source_limit=compressed+compressedLength;
  d.dest_start=(uint8_t*)output;
  d.dest=(uint8_t*)output;
  // Leave one guarded byte so uzlib can observe the end-of-stream marker
  // after producing exactly ISIZE bytes without writing out of bounds.
  d.dest_limit=(uint8_t*)output+expectedLength+1;
  int result=uzlib_gzip_parse_header(&d);
  if(result==TINF_OK) result=uzlib_uncompress_chksum(&d);
  size_t produced=(size_t)(d.dest-(uint8_t*)output);
  if(result!=TINF_DONE||produced!=expectedLength)
  {
    delete[] output;
    return false;
  }
  output[produced]='\0';
  decoded=output;
  decodedLength=produced;
  return true;
}

String csvAppend(const String& csv, const String& value)
{
  return csv.length()==0?value:csv+","+value;
}

bool validText(const char* value)
{
  return value!=nullptr&&value[0]!='\0';
}

bool validNumberText(const char* value, float minimum, float maximum)
{
  if(!validText(value)) return false;
  char* end=nullptr;
  float number=strtof(value,&end);
  return end!=value&&*end=='\0'&&number>=minimum&&number<=maximum;
}

int mapQWeatherIcon(int code)
{
  if(code==100||code==150) return 0;
  if(code>=101&&code<=103) return code-100;
  if(code>=151&&code<=153) return code-150;
  if(code==104||code==154) return 4;
  if(code>=200&&code<=213) return code-195;
  if(code>=300&&code<=313) return code-281;
  if(code>=314&&code<=399) return 32;
  if(code>=400&&code<=407) return code-367;
  if(code>=408&&code<=499) return 40;
  if(code>=500&&code<=504) return code-459;
  if(code==507) return 46;
  if(code==508) return 47;
  if(code>=509&&code<=515) return 41;
  if(code==900) return 48;
  if(code==901) return 49;
  return 50;
}

String weekdayForDate(const String& isoDate)
{
  if(isoDate.length()!=10||isoDate[4]!='-'||isoDate[7]!='-') return "";
  int y=isoDate.substring(0,4).toInt();
  int m=isoDate.substring(5,7).toInt();
  int d=isoDate.substring(8,10).toInt();
  if(y<2000||y>2099||m<1||m>12||d<1||d>31) return "";
  static const int monthTable[]={0,3,2,5,0,3,5,1,4,6,2,4};
  if(m<3) y--;
  int day=(y+y/4-y/100+y/400+monthTable[m-1]+d)%7;
  static const char* names[]={"周日","周一","周二","周三","周四","周五","周六"};
  return names[day];
}

bool parseIsoDate(const String& isoDate, int& year, byte& month, byte& day)
{
  if(isoDate.length()!=10||isoDate[4]!='-'||isoDate[7]!='-') return false;
  for(byte i=0;i<10;i++)
  {
    if(i==4||i==7) continue;
    if(isoDate[i]<'0'||isoDate[i]>'9') return false;
  }
  year=isoDate.substring(0,4).toInt();
  month=(byte)isoDate.substring(5,7).toInt();
  day=(byte)isoDate.substring(8,10).toInt();
  if(year<2020||year>2099||month<1||month>12||day<1) return false;
  static const byte DAYS_IN_MONTH[] PROGMEM={
    31,28,31,30,31,30,31,31,30,31,30,31
  };
  byte maximum=pgm_read_byte(&DAYS_IN_MONTH[month-1]);
  bool leapYear=(year%4==0&&year%100!=0)||year%400==0;
  if(month==2&&leapYear) maximum=29;
  return day<=maximum;
}

void appendLunarMonth(String& output, byte month)
{
  switch(month)
  {
    case 1: output+=F("正"); break;
    case 2: output+=F("二"); break;
    case 3: output+=F("三"); break;
    case 4: output+=F("四"); break;
    case 5: output+=F("五"); break;
    case 6: output+=F("六"); break;
    case 7: output+=F("七"); break;
    case 8: output+=F("八"); break;
    case 9: output+=F("九"); break;
    case 10: output+=F("十"); break;
    case 11: output+=F("冬"); break;
    default: output+=F("腊"); break;
  }
}

void appendLunarYearName(String& output, int year)
{
  switch((year-4)%10)
  {
    case 0: output+=F("甲"); break;
    case 1: output+=F("乙"); break;
    case 2: output+=F("丙"); break;
    case 3: output+=F("丁"); break;
    case 4: output+=F("戊"); break;
    case 5: output+=F("己"); break;
    case 6: output+=F("庚"); break;
    case 7: output+=F("辛"); break;
    case 8: output+=F("壬"); break;
    default: output+=F("癸"); break;
  }
  switch((year-4)%12)
  {
    case 0: output+=F("子鼠"); break;
    case 1: output+=F("丑牛"); break;
    case 2: output+=F("寅虎"); break;
    case 3: output+=F("卯兔"); break;
    case 4: output+=F("辰龙"); break;
    case 5: output+=F("巳蛇"); break;
    case 6: output+=F("午马"); break;
    case 7: output+=F("未羊"); break;
    case 8: output+=F("申猴"); break;
    case 9: output+=F("酉鸡"); break;
    case 10: output+=F("戌狗"); break;
    default: output+=F("亥猪"); break;
  }
  output+=F("年");
}

void appendLunarDigit(String& output, byte digit)
{
  switch(digit)
  {
    case 1: output+=F("一"); break;
    case 2: output+=F("二"); break;
    case 3: output+=F("三"); break;
    case 4: output+=F("四"); break;
    case 5: output+=F("五"); break;
    case 6: output+=F("六"); break;
    case 7: output+=F("七"); break;
    case 8: output+=F("八"); break;
    case 9: output+=F("九"); break;
    default: output+=F("十"); break;
  }
}

void appendLunarDay(String& output, byte day)
{
  if(day<=10)
  {
    output+=F("初");
    appendLunarDigit(output,day);
  }
  else if(day<20)
  {
    output+=F("十");
    appendLunarDigit(output,day-10);
  }
  else if(day==20)
  {
    output+=F("二十");
  }
  else if(day<30)
  {
    output+=F("廿");
    appendLunarDigit(output,day-20);
  }
  else
  {
    output+=F("三十");
  }
}

bool formatChineseLunarDate(const String& isoDate, String& output)
{
  int year=0;
  byte month=0;
  byte day=0;
  if(!parseIsoDate(isoDate,year,month,day)) return false;

  int32_t offset=-24; // 2020-01-25 is lunar 2020-01-01.
  for(int currentYear=2020;currentYear<year;currentYear++)
  {
    bool leapYear=(currentYear%4==0&&currentYear%100!=0)||
                  currentYear%400==0;
    offset+=leapYear?366:365;
  }
  static const byte DAYS_IN_MONTH[] PROGMEM={
    31,28,31,30,31,30,31,31,30,31,30,31
  };
  for(byte currentMonth=1;currentMonth<month;currentMonth++)
  {
    byte monthDays=pgm_read_byte(&DAYS_IN_MONTH[currentMonth-1]);
    bool leapYear=(year%4==0&&year%100!=0)||year%400==0;
    if(currentMonth==2&&leapYear) monthDays=29;
    offset+=monthDays;
  }
  offset+=(int32_t)day-1;
  if(offset<0) return false;

  int lunarYear=2020;
  uint32_t yearInfo=0;
  byte leapMonth=0;
  byte monthCount=0;
  while(lunarYear<=2099)
  {
    yearInfo=pgm_read_dword(&CHINESE_LUNAR_YEARS[lunarYear-2020]);
    leapMonth=(byte)((yearInfo>>13)&0x0f);
    monthCount=leapMonth?13:12;
    int yearDays=0;
    for(byte sequence=1;sequence<=monthCount;sequence++)
      yearDays+=(yearInfo&(1UL<<(sequence-1)))?30:29;
    if(offset<yearDays) break;
    offset-=yearDays;
    lunarYear++;
  }
  if(lunarYear>2099) return false;

  byte sequenceMonth=1;
  for(;sequenceMonth<=monthCount;sequenceMonth++)
  {
    byte monthDays=(yearInfo&(1UL<<(sequenceMonth-1)))?30:29;
    if(offset<monthDays) break;
    offset-=monthDays;
  }
  if(sequenceMonth>monthCount) return false;

  bool isLeapMonth=false;
  byte lunarMonth=sequenceMonth;
  if(leapMonth)
  {
    if(sequenceMonth==leapMonth+1)
    {
      lunarMonth=leapMonth;
      isLeapMonth=true;
    }
    else if(sequenceMonth>leapMonth+1)
    {
      lunarMonth=sequenceMonth-1;
    }
  }

  output="";
  output.reserve(40);
  appendLunarYearName(output,lunarYear);
  if(isLeapMonth) output+=F("闰");
  appendLunarMonth(output,lunarMonth);
  output+=F("月");
  appendLunarDay(output,(byte)(offset+1));
  return true;
}

bool validateForecastValue(const char* value)
{
  return validNumberText(value,-100.0f,100.0f);
}
}

heweatherclient::heweatherclient(const char* langstring):lang(langstring)
{
}

byte heweatherclient::getMeteoconIcon(int weathercodeindex)
{
  if(weathercodeindex==0) return 12;
  if(weathercodeindex>=1&&weathercodeindex<=3) return 58;
  if(weathercodeindex==4) return 54;
  if(weathercodeindex>=5&&weathercodeindex<=18) return 0;
  if(weathercodeindex>=19&&weathercodeindex<=32) return 19;
  if(weathercodeindex>=33&&weathercodeindex<=40) return 16;
  if(weathercodeindex>=41&&weathercodeindex<=43) return 37;
  return 17;
}

bool heweatherclient::isValidApiHost(const String& input)
{
  String host=input;
  host.trim();
  host.toLowerCase();
  if(host.length()<18||host.length()>96||
     !host.endsWith(".qweatherapi.com")||
     host.startsWith(".")||host.endsWith(".")||
     host.indexOf("://")>=0||host.indexOf('/')>=0||
     host.indexOf(':')>=0||host.indexOf('\r')>=0||
     host.indexOf('\n')>=0||host.indexOf(' ')>=0) return false;
  for(size_t i=0;i<host.length();i++)
  {
    char c=host[i];
    if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='-'))
      return false;
    if((c=='.'||c=='-')&&
       (i==0||i+1==host.length()||host[i-1]=='.'||host[i+1]=='.'))
      return false;
  }
  return true;
}

bool heweatherclient::isValidApiKey(const String& key)
{
  if(key.length()<20||key.length()>64) return false;
  for(size_t i=0;i<key.length();i++)
  {
    char c=key[i];
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
         (c>='0'&&c<='9')||c=='_'||c=='-')) return false;
  }
  return true;
}

bool heweatherclient::isValidLocation(const String& input)
{
  String value=input;
  value.trim();
  if(value.length()==0||value.length()>64) return false;
  for(size_t i=0;i<value.length();i++)
  {
    uint8_t c=(uint8_t)value[i];
    if(c<0x20||c==0x7f) return false;
  }
  return true;
}

bool heweatherclient::isValidAirCoordinates(const String& input)
{
  String value=input;
  value.trim();
  if(value.length()<3||value.length()>20) return false;
  int separator=value.indexOf('/');
  if(separator<=0||separator+1>=(int)value.length()||
     value.indexOf('/',separator+1)>=0) return false;

  String latitude=value.substring(0,separator);
  String longitude=value.substring(separator+1);
  auto validComponent=[](const String& component,
                         float minimum,float maximum)->bool
  {
    if(component.length()==0) return false;
    byte decimals=0;
    bool dotSeen=false;
    size_t offset=component[0]=='-'?1:0;
    if(offset==component.length()) return false;
    for(size_t i=offset;i<component.length();i++)
    {
      char c=component[i];
      if(c=='.'&&!dotSeen)
      {
        dotSeen=true;
        continue;
      }
      if(c<'0'||c>'9') return false;
      if(dotSeen&&++decimals>2) return false;
    }
    if(component.endsWith(".")) return false;
    return validNumberText(component.c_str(),minimum,maximum);
  };
  return validComponent(latitude,-90.0f,90.0f)&&
         validComponent(longitude,-180.0f,180.0f);
}

void heweatherclient::clearWeatherData()
{
  timeout=false;
  rain=false;
  failureClass_=FailureClass::None;
  httpStatus_=0;
  qweatherCode_=0;
  retryAfterSeconds_=0;
  retryAfterPresent_=false;
  airRequested_=false;
  airAvailable_=false;
  airFailureClass_=FailureClass::None;
  airHttpStatus_=0;
  airQweatherCode_=0;
  airRetryAfterSeconds_=0;
  airRetryAfterPresent_=false;
  geoRequested_=false;
  HEWEATHER_DEBUG_ENDPOINT(DebugEndpoint::None);
  HEWEATHER_DEBUG_STAGE(DebugStage::None);
  responseDate="";
  aqi=""; co=""; no2=""; o3=""; pm10=""; pm25=""; so2=""; aqitext="";
  airconditionbits_index=0;
  now_cond=""; now_hum=""; now_tmp=""; now_cond_index="";
  now_dir=""; now_sc=""; now_fl=""; now_pcpn=""; now_vis=""; now_pres="";
  today_cond_d=""; today_cond_d_index=""; today_cond_n="";
  today_cond_n_index=""; today_tmp_max=""; today_tmp_min="";
  today_txt_d=""; today_txt_n="";
  tomorrow_cond_d=""; tomorrow_cond_d_index=""; tomorrow_cond_n="";
  tomorrow_cond_n_index=""; tomorrow_tmp_max=""; tomorrow_tmp_min="";
  tomorrow_txt_d=""; tomorrow_txt_n="";
  thedayaftertomorrow_cond_d=""; thedayaftertomorrow_cond_d_index="";
  thedayaftertomorrow_cond_n=""; thedayaftertomorrow_cond_n_index="";
  thedayaftertomorrow_tmp_max=""; thedayaftertomorrow_tmp_min="";
  tmin_array=""; tmax_array=""; code_d_array=""; code_n_array="";
  text_d_array=""; text_n_array=""; date_array=""; week_array="";
  citystr=""; date=""; weekday=""; year=""; nongli=""; message="";
  qlty="--"; t=""; unknown="";
}

bool heweatherclient::fetchJson(BearSSL::WiFiClientSecure& client,
                                const String& path,
                                bool closeConnection,
                                unsigned long networkStarted,
                                char*& json,
                                size_t& jsonLength,
                                String& httpDate,
                                uint16_t& httpStatus,
                                uint32_t& retryAfterSeconds,
                                bool& retryAfterPresent,
                                bool allowReconnect)
{
  json=nullptr;
  jsonLength=0;
  httpDate="";
  httpStatus=0;
  retryAfterSeconds=0;
  retryAfterPresent=false;
  HEWEATHER_DEBUG_STAGE(DebugStage::Budget);
  if(remainingBudget(networkStarted)==0) return false;

  if(!client.connected())
  {
    if(!allowReconnect) return false;
    HEWEATHER_DEBUG_STAGE(DebugStage::Dns);
    unsigned long budgetLeft=remainingBudget(networkStarted);
    if(budgetLeft==0) return false;
    unsigned long dnsTimeout=budgetLeft<DNS_TIMEOUT_MS
                               ?budgetLeft
                               :DNS_TIMEOUT_MS;
    IPAddress address;
    if(WiFi.hostByName(apiHost.c_str(),address,dnsTimeout)!=1||
       remainingBudget(networkStarted)==0) return false;
    budgetLeft=remainingBudget(networkStarted);
    unsigned long ioTimeout=budgetLeft<IO_TIMEOUT_MS
                              ?budgetLeft
                              :IO_TIMEOUT_MS;
    if(ioTimeout==0) return false;
    client.setTimeout(ioTimeout);
    // The first lookup provides a bounded DNS deadline and warms lwIP's DNS
    // cache. Connecting by hostname is still required for SNI and certificate
    // hostname verification.
    HEWEATHER_DEBUG_STAGE(DebugStage::Tls);
    // BearSSL plus the SDK Wi-Fi callbacks need a real safety margin. On-device
    // fault injection showed that attempting a handshake below this floor can
    // crash inside the SDK rather than returning an allocation error.
    if(ESP.getFreeHeap()<MIN_TLS_FREE_HEAP||
       ESP.getMaxFreeBlockSize()<MIN_TLS_CONTIGUOUS_HEAP)
      return false;
    #if HEWEATHER_ENABLE_DEBUG_STATUS
    Serial.printf("QW TLS connect: path=%s, heap=%u, budget=%lums\n",
                  path.c_str(),ESP.getFreeHeap(),budgetLeft);
    #endif
    if(!client.connect(apiHost.c_str(),443)||
       remainingBudget(networkStarted)==0)
      return false;
    #if HEWEATHER_ENABLE_DEBUG_STATUS
    Serial.printf("QW TLS ready: heap=%u, elapsed=%lums\n",
                  ESP.getFreeHeap(),millis()-networkStarted);
    #endif
    budgetLeft=remainingBudget(networkStarted);
    ioTimeout=budgetLeft<IO_TIMEOUT_MS?budgetLeft:IO_TIMEOUT_MS;
    if(ioTimeout==0) return false;
    client.setTimeout(ioTimeout);
  }

  HEWEATHER_DEBUG_STAGE(DebugStage::Request);
  bool requestWritten=
      client.print(F("GET "))==sizeof("GET ")-1&&
      client.print(path)==path.length()&&
      client.print(F(" HTTP/1.1\r\nHost: "))==
          sizeof(" HTTP/1.1\r\nHost: ")-1&&
      client.print(apiHost)==apiHost.length()&&
      client.print(F("\r\nX-QW-Api-Key: "))==
          sizeof("\r\nX-QW-Api-Key: ")-1&&
      client.print(apiKey)==apiKey.length()&&
      client.print(F("\r\nAccept: application/json\r\nAccept-Encoding: gzip\r\n"
                     "User-Agent: EpaperWeather/2\r\nConnection: "))==
          sizeof("\r\nAccept: application/json\r\nAccept-Encoding: gzip\r\n"
                 "User-Agent: EpaperWeather/2\r\nConnection: ")-1;
  if(requestWritten)
  {
    requestWritten=closeConnection
        ?client.print(F("close\r\n\r\n"))==sizeof("close\r\n\r\n")-1
        :client.print(F("keep-alive\r\n\r\n"))==
             sizeof("keep-alive\r\n\r\n")-1;
  }
  if(!requestWritten)
  {
    client.stop();
    return false;
  }
  if(remainingBudget(networkStarted)==0) return false;

  HEWEATHER_DEBUG_STAGE(DebugStage::StatusLine);
  unsigned long lastProgress=millis();
  String line;
  bool statusRead=readLineWithDeadline(client,networkStarted,lastProgress,line);
  bool http10=false;
  if(!statusRead||!parseHttpStatusLine(line,httpStatus,http10)) return false;

  HEWEATHER_DEBUG_STAGE(DebugStage::Headers);
  bool contentLengthSeen=false;
  bool chunked=false;
  bool gzip=false;
  bool connectionClose=false;
  bool connectionKeepAlive=false;
  size_t contentLength=0;
  while(true)
  {
    if(!readLineWithDeadline(client,networkStarted,lastProgress,line)) return false;
    if(line.length()==0) break;
    int colon=line.indexOf(':');
    if(colon<=0)
    {
      if(httpStatus==200) return false;
      continue;
    }
    String name=line.substring(0,colon);
    String value=line.substring(colon+1);
    name.toLowerCase();
    value.trim();
    if(name=="content-length")
    {
      size_t parsedLength=0;
      bool validLength=!contentLengthSeen&&
        parseDecimalSize(value,MAX_COMPRESSED_BODY,parsedLength);
      if(!validLength)
      {
        if(httpStatus==200) return false;
      }
      else
      {
        contentLength=parsedLength;
        contentLengthSeen=true;
      }
    }
    else if(name=="transfer-encoding")
    {
      value.toLowerCase();
      if(value!="chunked")
      {
        if(httpStatus==200) return false;
      }
      else
      {
        chunked=true;
      }
    }
    else if(name=="content-encoding")
    {
      value.toLowerCase();
      if(value=="gzip") gzip=true;
      else if(value!="identity"&&httpStatus==200) return false;
    }
    else if(name=="date")
    {
      if(value.length()<=64) httpDate=value;
    }
    else if(name=="connection")
    {
      if(headerHasToken(value,"close")) connectionClose=true;
      if(headerHasToken(value,"keep-alive")) connectionKeepAlive=true;
    }
    else if(name=="retry-after"&&!retryAfterPresent)
    {
      uint32_t seconds=0;
      if(parseRetryAfterSeconds(value,seconds))
      {
        retryAfterSeconds=seconds;
        retryAfterPresent=true;
      }
    }
  }
  // QWeather v2 reports errors through HTTP status. Once the bounded headers
  // (including Retry-After) are available, aborting in update() is faster and
  // uses less radio time than downloading an error body. V1-style errors use
  // HTTP 200 and are still decoded so their top-level JSON code is classified.
  if(httpStatus!=200) return false;
  if(chunked==contentLengthSeen) return false;
  bool responseClose=connectionClose||(http10&&!connectionKeepAlive);
  #if HEWEATHER_ENABLE_DEBUG_STATUS
  Serial.printf("QW headers: status=%u, gzip=%u, chunked=%u, length=%u, "
                "close=%u\n",
                (unsigned int)httpStatus,(unsigned int)gzip,
                (unsigned int)chunked,(unsigned int)contentLength,
                (unsigned int)responseClose);
  #endif

  HEWEATHER_DEBUG_STAGE(DebugStage::Body);
  size_t bodyCapacity=contentLengthSeen?contentLength:MAX_COMPRESSED_BODY;
  if(bodyCapacity==0) return false;
  uint8_t* body=new(std::nothrow) uint8_t[bodyCapacity];
  if(body==nullptr) return false;
  size_t bodyLength=0;
  bool bodyOk=true;
  if(contentLengthSeen)
  {
    bodyOk=contentLength>0&&
           readExact(client,networkStarted,lastProgress,body,contentLength);
    bodyLength=bodyOk?contentLength:0;
  }
  else
  {
    while(bodyOk)
    {
      if(!readLineWithDeadline(client,networkStarted,lastProgress,line))
      {
        bodyOk=false;
        break;
      }
      size_t chunkLength=0;
      if(!parseHexSize(line,MAX_COMPRESSED_BODY-bodyLength,chunkLength))
      {
        bodyOk=false;
        break;
      }
      if(chunkLength==0)
      {
        // Consume bounded trailer fields through the final empty line.
        do
        {
          if(!readLineWithDeadline(client,networkStarted,lastProgress,line))
          {
            bodyOk=false;
            break;
          }
        } while(line.length()!=0);
        break;
      }
      if(!readExact(client,networkStarted,lastProgress,
                    body+bodyLength,chunkLength))
      {
        bodyOk=false;
        break;
      }
      bodyLength+=chunkLength;
      uint8_t cr;
      uint8_t lf;
      if(!readByteWithDeadline(client,networkStarted,lastProgress,cr)||
         !readByteWithDeadline(client,networkStarted,lastProgress,lf)||
         cr!='\r'||lf!='\n') bodyOk=false;
    }
  }

  if(!bodyOk||bodyLength==0||remainingBudget(networkStarted)==0)
  {
    delete[] body;
    return false;
  }

  if(responseClose) client.stop();

  if(gzip)
  {
    HEWEATHER_DEBUG_STAGE(DebugStage::Decode);
    bool decoded=decodeGzip(body,bodyLength,json,jsonLength);
    #if HEWEATHER_ENABLE_DEBUG_STATUS
    Serial.printf("QW gzip: compressed=%u, decoded=%u, ok=%u\n",
                  (unsigned int)bodyLength,(unsigned int)jsonLength,
                  (unsigned int)decoded);
    #endif
    delete[] body;
    return decoded;
  }

  if(bodyLength>MAX_JSON_BODY)
  {
    delete[] body;
    return false;
  }
  char* plain=new(std::nothrow) char[bodyLength+1];
  if(plain==nullptr)
  {
    delete[] body;
    return false;
  }
  memcpy(plain,body,bodyLength);
  plain[bodyLength]='\0';
  delete[] body;
  json=plain;
  jsonLength=bodyLength;
  return true;
}

bool heweatherclient::parseGeo(char* json,
                               size_t jsonLength,
                               uint16_t& qweatherCode)
{
  qweatherCode=0;
  HEWEATHER_DEBUG_STAGE(DebugStage::Json);
  StaticJsonDocument<192> filter;
  filter["code"]=true;
  filter["location"][0]["id"]=true;
  filter["location"][0]["name"]=true;
  filter["location"][0]["lat"]=true;
  filter["location"][0]["lon"]=true;
  if(filter.overflowed()) return false;
  qweatherJsonData.clear();
  StaticJsonDocument<1536>& data=qweatherJsonData;
  DeserializationError error=deserializeJson(
      data,json,jsonLength,DeserializationOption::Filter(filter));
  if(error||
     !parseQWeatherResponseCode(data["code"],qweatherCode)||
     qweatherCode!=200)
    return false;
  const char* id=data["location"][0]["id"];
  const char* name=data["location"][0]["name"];
  const char* latitude=data["location"][0]["lat"];
  const char* longitude=data["location"][0]["lon"];
  if(!validText(id)||!validText(name)||
     strlen(id)>32||strlen(name)>64||
     !validText(latitude)||strlen(latitude)>16||
     !validText(longitude)||strlen(longitude)>16||
     !validNumberText(latitude,-90.0f,90.0f)||
     !validNumberText(longitude,-180.0f,180.0f))
    return false;
  String coordinates=String(strtof(latitude,nullptr),2)+"/"+
                     String(strtof(longitude,nullptr),2);
  if(!isValidAirCoordinates(coordinates)) return false;
  resolvedLocationId=id;
  resolvedLocationName=name;
  resolvedAirCoordinates=coordinates;
  return true;
}

bool heweatherclient::parseNow(char* json,
                               size_t jsonLength,
                               uint16_t& qweatherCode)
{
  qweatherCode=0;
  HEWEATHER_DEBUG_STAGE(DebugStage::Json);
  StaticJsonDocument<384> filter;
  filter["code"]=true;
  JsonObject now=filter.createNestedObject("now");
  now["temp"]=true;
  now["feelsLike"]=true;
  now["icon"]=true;
  now["text"]=true;
  now["windDir"]=true;
  now["windScale"]=true;
  now["humidity"]=true;
  now["precip"]=true;
  now["pressure"]=true;
  now["vis"]=true;
  qweatherJsonData.clear();
  StaticJsonDocument<1536>& data=qweatherJsonData;
  DeserializationError error=deserializeJson(
      data,json,jsonLength,DeserializationOption::Filter(filter));
  if(error||
     !parseQWeatherResponseCode(data["code"],qweatherCode)||
     qweatherCode!=200)
    return false;
  JsonObjectConst current=data["now"];
  const char* temp=current["temp"];
  const char* feelsLike=current["feelsLike"];
  const char* icon=current["icon"];
  const char* text=current["text"];
  const char* windDir=current["windDir"];
  const char* windScale=current["windScale"];
  const char* humidity=current["humidity"];
  const char* precip=current["precip"];
  const char* pressure=current["pressure"];
  const char* visibility=current["vis"];
  int rawIcon=0;
  if(!validNumberText(temp,-100,100)||
     !validNumberText(feelsLike,-100,100)||
     !validNumberText(humidity,0,100)||
     !parseQWeatherIconCode(icon,rawIcon)||!validText(text)||
     !validText(windDir)||!validText(windScale)||
     !validText(precip)||!validText(pressure)||!validText(visibility))
    return false;
  now_tmp=temp;
  now_fl=feelsLike;
  now_cond=text;
  now_cond_index=String(mapQWeatherIcon(rawIcon));
  now_dir=windDir;
  now_sc=windScale;
  now_hum=humidity;
  now_pcpn=precip;
  now_pres=pressure;
  now_vis=visibility;
  rain=rawIcon>=300&&rawIcon<=399;
  return true;
}

bool heweatherclient::parseDaily(char* json,
                                 size_t jsonLength,
                                 uint16_t& qweatherCode)
{
  qweatherCode=0;
  HEWEATHER_DEBUG_STAGE(DebugStage::Json);
  StaticJsonDocument<512> filter;
  filter["code"]=true;
  JsonObject daily=filter["daily"][0].to<JsonObject>();
  daily["fxDate"]=true;
  daily["tempMax"]=true;
  daily["tempMin"]=true;
  daily["iconDay"]=true;
  daily["iconNight"]=true;
  daily["textDay"]=true;
  daily["textNight"]=true;
  // `json` is mutable, so ArduinoJson can keep string views in-place. The
  // filtered tree only needs variant slots in the shared fixed pool.
  qweatherJsonData.clear();
  StaticJsonDocument<1536>& data=qweatherJsonData;
  DeserializationError error=deserializeJson(
      data,json,jsonLength,DeserializationOption::Filter(filter));
  if(error||
     !parseQWeatherResponseCode(data["code"],qweatherCode)||
     qweatherCode!=200||
     data["daily"].size()<6)
    return false;

  for(byte i=0;i<6;i++)
  {
    JsonObjectConst item=data["daily"][i];
    const char* fxDate=item["fxDate"];
    const char* tempMax=item["tempMax"];
    const char* tempMin=item["tempMin"];
    const char* iconDay=item["iconDay"];
    const char* iconNight=item["iconNight"];
    const char* textDay=item["textDay"];
    const char* textNight=item["textNight"];
    int rawDayIcon=0;
    int rawNightIcon=0;
    if(!validText(fxDate)||!validateForecastValue(tempMax)||
       !validateForecastValue(tempMin)||!validText(iconDay)||
       !parseQWeatherIconCode(iconDay,rawDayIcon)||
       !parseQWeatherIconCode(iconNight,rawNightIcon)||
       !validText(textDay)||!validText(textNight))
      return false;
    String itemDate=fxDate;
    String itemWeek=weekdayForDate(itemDate);
    if(itemWeek.length()==0) return false;
    int dayIndex=mapQWeatherIcon(rawDayIcon);
    int nightIndex=mapQWeatherIcon(rawNightIcon);
    tmax_array=csvAppend(tmax_array,tempMax);
    tmin_array=csvAppend(tmin_array,tempMin);
    code_d_array=csvAppend(code_d_array,String(dayIndex));
    code_n_array=csvAppend(code_n_array,String(nightIndex));
    text_d_array=csvAppend(text_d_array,textDay);
    text_n_array=csvAppend(text_n_array,textNight);
    date_array=csvAppend(date_array,itemDate.substring(5));
    week_array=csvAppend(week_array,itemWeek);

    if(i==0)
    {
      date=itemDate;
      weekday=itemWeek;
      today_cond_d=iconDay;
      today_cond_n=iconNight;
      today_cond_d_index=String(dayIndex);
      today_cond_n_index=String(nightIndex);
      today_tmp_max=tempMax;
      today_tmp_min=tempMin;
      today_txt_d=textDay;
      today_txt_n=textNight;
    }
    else if(i==1)
    {
      tomorrow_cond_d=iconDay;
      tomorrow_cond_n=iconNight;
      tomorrow_cond_d_index=String(dayIndex);
      tomorrow_cond_n_index=String(nightIndex);
      tomorrow_tmp_max=tempMax;
      tomorrow_tmp_min=tempMin;
      tomorrow_txt_d=textDay;
      tomorrow_txt_n=textNight;
    }
    else if(i==2)
    {
      thedayaftertomorrow_cond_d=iconDay;
      thedayaftertomorrow_cond_n=iconNight;
      thedayaftertomorrow_cond_d_index=String(dayIndex);
      thedayaftertomorrow_cond_n_index=String(nightIndex);
      thedayaftertomorrow_tmp_max=tempMax;
      thedayaftertomorrow_tmp_min=tempMin;
    }
  }
  year=date+" "+weekday;
  return true;
}

bool heweatherclient::parseAir(char* json,
                               size_t jsonLength,
                               uint16_t& qweatherCode)
{
  qweatherCode=0;
  HEWEATHER_DEBUG_STAGE(DebugStage::Json);
  StaticJsonDocument<320> filter;
  filter["code"]=true;
  filter["indexes"][0]["code"]=true;
  filter["indexes"][0]["aqi"]=true;
  filter["indexes"][0]["aqiDisplay"]=true;
  filter["indexes"][0]["level"]=true;
  filter["indexes"][0]["category"]=true;
  if(filter.overflowed()) return false;
  qweatherJsonData.clear();
  StaticJsonDocument<1536>& data=qweatherJsonData;
  DeserializationError error=deserializeJson(
      data,json,jsonLength,DeserializationOption::Filter(filter));
  if(error) return false;
  if(!data["code"].isNull())
  {
    if(!parseQWeatherResponseCode(data["code"],qweatherCode)||
       qweatherCode!=200)
      return false;
  }
  if(data["indexes"].size()==0) return false;

  String selectedAqi;
  String selectedCategory;
  byte selectedLevel=0;
  byte selectedPriority=0xff;
  for(JsonObjectConst index:data["indexes"].as<JsonArrayConst>())
  {
    const char* code=index["code"];
    const char* display=index["aqiDisplay"];
    const char* category=index["category"];
    const char* level=index["level"];
    float numericAqi=index["aqi"]|-1.0f;
    if(!validText(code)||!validText(display)||!validText(category)||
       strlen(code)>24||strlen(display)>12||strlen(category)>48||
       numericAqi<0.0f||numericAqi>1000.0f)
      continue;
    byte priority=strcmp(code,"cn-mee")==0
                    ?0
                    :(strcmp(code,"qaqi")!=0?1:2);
    if(priority>=selectedPriority) continue;
    selectedAqi=display;
    selectedCategory=category;
    selectedLevel=validText(level)&&strlen(level)==1&&
                  level[0]>='1'&&level[0]<='6'
                    ?(byte)(level[0]-'0')
                    :0;
    selectedPriority=priority;
  }
  if(selectedPriority==0xff) return false;
  aqi=selectedAqi;
  qlty=selectedCategory;
  aqitext=selectedCategory;
  if(selectedLevel) airconditionbits_index=selectedLevel-1;
  qweatherCode=200;
  return true;
}

void heweatherclient::update(uint32_t unixTime, bool requestAir)
{
  clearWeatherData();
  HEWEATHER_DEBUG_STAGE(DebugStage::Validation);
  apiHost.trim();
  apiHost.toLowerCase();
  apiKey.trim();
  location.trim();
  locationId.trim();
  airCoordinates.trim();
  const char* apiLanguage=
    lang!=nullptr&&strcmp(lang,"ch")==0?"zh":(lang!=nullptr?lang:"zh");
  if(!isValidApiHost(apiHost)||!isValidApiKey(apiKey)||
     !isValidLocation(location))
  {
    failureClass_=FailureClass::Configuration;
    timeout=true;
    return;
  }
  if(unixTime<1609459200UL)
  {
    failureClass_=FailureClass::Transient;
    timeout=true;
    return;
  }

  BearSSL::WiFiClientSecure client;
  BearSSL::X509List trustAnchor(ISRG_ROOT_X1);
  client.setTrustAnchors(&trustAnchor);
  client.setX509Time((time_t)unixTime);
  client.setTimeout(IO_TIMEOUT_MS);
  // This QWeather API host was verified on-device to negotiate MFLN 512.
  // Avoiding a probe TLS connection saves both heap and radio-on time.
  client.setBufferSizes(512,512);

  unsigned long networkStarted=millis();
  bool ok=true;
  char* json=nullptr;
  size_t jsonLength=0;
  String httpDate;
  uint16_t responseStatus=0;
  uint16_t responseCode=0;
  uint32_t retryAfter=0;
  bool retryAfterPresent=false;

  resolvedLocationId=locationId;
  resolvedAirCoordinates=airCoordinates;
  if(!isValidAirCoordinates(resolvedAirCoordinates))
    resolvedAirCoordinates="";
  if(resolvedLocationId.length()==0||resolvedLocationName.length()==0||
     resolvedAirCoordinates.length()==0)
  {
    geoRequested_=true;
    HEWEATHER_DEBUG_ENDPOINT(DebugEndpoint::Geo);
    bool geoByCachedId=resolvedLocationId.length()>0;
    String geoQuery=resolvedLocationId.length()?resolvedLocationId:location;
    byte geoAttempts=geoByCachedId?2:1;
    for(byte geoAttempt=0;geoAttempt<geoAttempts;geoAttempt++)
    {
      String path="/geo/v2/city/lookup?location="+urlEncode(geoQuery)+
                  "&number=1&lang="+urlEncode(String(apiLanguage));
      responseCode=0;
      qweatherCode_=0;
      ok=fetchJson(client,path,false,networkStarted,json,jsonLength,httpDate,
                   responseStatus,retryAfter,retryAfterPresent);
      httpStatus_=responseStatus;
      retryAfterSeconds_=retryAfter;
      retryAfterPresent_=retryAfterPresent;
      if(ok)
      {
        bool parsed=parseGeo(json,jsonLength,responseCode);
        qweatherCode_=responseCode;
        ok=parsed&&responseStatus==200;
      }
      delete[] json;
      json=nullptr;
      if(httpDate.length()) responseDate=httpDate;
      if(ok) break;

      bool cachedIdNotFound=geoAttempt==0&&geoByCachedId&&
        (responseStatus==400||responseStatus==404||
         responseCode==400||responseCode==404);
      if(!cachedIdNotFound) break;
      // A stale direct-QWeather ID should not permanently strand an otherwise
      // valid configured city. Retry that migration once by city, within the
      // same 22-second budget; all later wakes use the newly persisted ID.
      client.abort();
      resolvedLocationId="";
      resolvedLocationName="";
      resolvedAirCoordinates="";
      geoQuery=location;
    }
    if(!ok) failureClass_=classifyFailure(responseStatus,responseCode);
  }
  if(ok&&resolvedLocationName.length()==0) resolvedLocationName=location;
  if(ok)
  {
    HEWEATHER_DEBUG_ENDPOINT(DebugEndpoint::Daily);
    // Decode the larger forecast while the heap is least fragmented. The
    // much smaller current-conditions payload is safe to process afterwards.
    String path="/v7/weather/7d?location="+urlEncode(resolvedLocationId)+
                "&lang="+urlEncode(String(apiLanguage))+"&unit=m";
    responseCode=0;
    ok=fetchJson(client,path,false,networkStarted,json,jsonLength,httpDate,
                 responseStatus,retryAfter,retryAfterPresent);
    httpStatus_=responseStatus;
    qweatherCode_=0;
    retryAfterSeconds_=retryAfter;
    retryAfterPresent_=retryAfterPresent;
    if(ok)
    {
      bool parsed=parseDaily(json,jsonLength,responseCode);
      qweatherCode_=responseCode;
      ok=parsed&&responseStatus==200;
    }
    delete[] json;
    json=nullptr;
    if(httpDate.length()) responseDate=httpDate;
    if(!ok) failureClass_=classifyFailure(responseStatus,responseCode);
  }
  if(ok)
  {
    HEWEATHER_DEBUG_ENDPOINT(DebugEndpoint::Now);
    String path="/v7/weather/now?location="+urlEncode(resolvedLocationId)+
                "&lang="+urlEncode(String(apiLanguage))+"&unit=m";
    responseCode=0;
    ok=fetchJson(client,path,!requestAir,networkStarted,json,jsonLength,httpDate,
                 responseStatus,retryAfter,retryAfterPresent);
    httpStatus_=responseStatus;
    qweatherCode_=0;
    retryAfterSeconds_=retryAfter;
    retryAfterPresent_=retryAfterPresent;
    if(ok)
    {
      bool parsed=parseNow(json,jsonLength,responseCode);
      qweatherCode_=responseCode;
      ok=parsed&&responseStatus==200;
    }
    delete[] json;
    json=nullptr;
    if(httpDate.length()) responseDate=httpDate;
    if(!ok) failureClass_=classifyFailure(responseStatus,responseCode);
  }
  if(ok&&requestAir)
  {
    airRequested_=true;
    HEWEATHER_DEBUG_ENDPOINT(DebugEndpoint::Air);
    String path="/airquality/v1/current/"+resolvedAirCoordinates+
                "?lang="+urlEncode(String(apiLanguage));
    responseCode=0;
    bool airOk=fetchJson(client,path,true,networkStarted,json,jsonLength,
                         httpDate,responseStatus,retryAfter,
                         retryAfterPresent,false);
    airHttpStatus_=responseStatus;
    airQweatherCode_=0;
    airRetryAfterSeconds_=retryAfter;
    airRetryAfterPresent_=retryAfterPresent;
    if(airOk)
    {
      bool parsed=parseAir(json,jsonLength,responseCode);
      airQweatherCode_=responseCode;
      airOk=parsed&&responseStatus==200;
    }
    delete[] json;
    json=nullptr;
    if(httpDate.length()) responseDate=httpDate;
    if(airOk)
    {
      airAvailable_=true;
      airFailureClass_=FailureClass::None;
    }
    else
    {
      airFailureClass_=classifyFailure(responseStatus,responseCode);
      client.abort();
    }
  }

  if(ok)
  {
    failureClass_=FailureClass::None;
    HEWEATHER_DEBUG_STAGE(DebugStage::Complete);
    client.stop();
    formatChineseLunarDate(date,nongli);
    citystr=resolvedLocationName.length()?resolvedLocationName:location;
    city=location;
    unknown="200";
  }
  else
  {
    client.abort();
  }
  timeout=!ok;
}
