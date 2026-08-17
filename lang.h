#define LANG 1  //1 chinese 2 english
#if (LANG==1)
const char* todaystr="今天";
const char* tomorrowstr="明天";
const char* airstr="空气质量:";
const char* tonightstr="今天夜间:";
const char* lang="ch";
const char* config_page_line1="请用手机连接wifi";
const char* config_page_line2="weather widget";
const char* config_page_line3="浏览器打开任意网页";
const char* config_page_line4="进入配置页面";
const char* config_page_line5="输入用于天气更新的wifi";
const char* config_page_line6="名称，密码和城市(拼音)";
const char* config_timeout_line1="配置无效或连接超时";
const char* config_timeout_line2="① 关闭电源";
const char* config_timeout_line3="② 按住reset键数秒";
const char* config_timeout_line4="③ 开启电源重新进入配置模式";
#endif
#if (LANG==2)
const char* todaystr="Today";
const char* tomorrowstr="Tomorrow";
const char* airstr="Air Quality:";
const char* tonightstr="Tonight:";
const char* lang="en";
const char* config_page_line1="Connect WIFI ";
const char* config_page_line2="Weather widget";
const char* config_page_line3="Open any website";
const char* config_page_line4="to enter config page";
const char* config_page_line5="Input wifi name , password";
const char* config_page_line6="and city for weather";
const char* config_timeout_line1="Credentials not valid or config timeout";
const char* config_timeout_line2="① Turn off the power";
const char* config_timeout_line3="② Hold reset for seconds";
const char* config_timeout_line4="③ Turn on the power";
#endif
