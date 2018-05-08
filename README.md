# Esp8266-weather-station-epaper
中文版说明在这里http://hlu.lu/2017/07/06/esp8266%E7%94%B5%E7%BA%B8%E5%B1%8F%E5%A4%A9%E6%B0%94%E9%A2%84%E6%8A%A5%E7%AB%99/
## Summary
Esp8266 is programed to display weather forecast on 2.9inch e-paper.
You can get a 2.9inch e-paper display in Waveshare's shop. Buy it on taobao.com if you are in China. https://detail.tmall.com/item.htm?id=550690109675&spm=a1z09.2.0.0.nyL5N4&_u=q2skmgl30cb

Esp8266 is in deep sleeping after update the weather forecast to save battery.

![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170715_113425.jpg)

![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170715_152231.jpg)

![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170715_152306.jpg)

![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170721_232400.jpg)

## BOM
1. 2.9inch e-paper from Waveshare
2. Nodemcu or Wemos or integrated PCB( gerber files in /PCB folder)
3. Li-Po battery
4. 3d printed case(STL files in /3d folder still in revising )

## Uploading code to esp8266
Add esp8266 to boards manager in arduino ide. Follow this guide https://github.com/esp8266/Arduino#installing-with-boards-manager.
Don't forget to upload font files. Follow this guide to upload font files to spiff meomory. https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system.

## Multi language support
modify the language in lang.h 
There is Strings for Chinese and English already.
You can add you own.
Weahter data supports zh,en,de,es,fr,it,jp,kr,ru,in,th

## Connect esp8266 to display module
BUSY---->gpio4
RST---->gpio2
DC---->gpio5
CS---->gpio15
CLK---->gpio14
DIN---->gpio13

Connect gpio16 to rst on esp8266 to wakeup esp8266 from deep sleeping with internal timer.

There will be two versions of my pcb. One without epaper driving components and One with driving components. I'm still testing them to make sure they are reliable. Also Mike is making that board, too.

## Send message to weather station
you can set a client name for your weather station in the program and send message to it (http://duckweather.tk/client.php). Useful to please girls.

## Font
I developped this tool to generate your own font (unicode ucs-2)
https://github.com/duck531a98/font-generator

Since arduino ide is compiling the code to utf-8 charset. There is a internal function to convert utf-8 to unicode ucs-2. Display strings typed in to arduino ide with function DrawUTF.

Font files should be uploaded into spiff memory of esp8266.

A full unicode ucs-s font with 16x16 size will be 2MB. So if you don't need to display multi language, just convert ascii characters.

## Low power issues
Since regulator AMS1117 and CP2102 have a quiescent current about several mA, so nodemcu is not suitable for battery powered.

A low quiescent current LDO is needed. I'm testing HT7333 now, see how long could my board works with a 400mAh battery. Esp8266 consumes about 20-25uA in deep sleeping mode.

## Weather Forecast Data
I'm using api from heweather.com. Because Esp8266 don't have enough RAM to handle HTTPS connection, so I build a website on 000webhost.com to transmit the data via HTTP. To reduce the consumption of the 3000 free requests per day, I build a cache, you will get the same returns in 20 mins.

Highly recommend you to build your own website. I can't guarantee mine will works forever.

You can find php files in /php folder.

## Thanks
Thanks to Mike Daniel Fred. They led me into the world of esp8266. Thanks to their work on esp8266 weahter stations, wifi manager, json parser and so on. Thanks to Mike, for now he's the only one which I could share and ask for help about electronic stuffs.