# Esp8266-weather-station-epaper
## Summary
Esp8266 is programed to display weather forecast on 2.9inch e-paper.
You can get a 2.9inch e-paper display in Waveshare's shop.

Esp8266 is in deep sleeping after update the weather forecast to save battery.
![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170623_232157.jpg)
![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170624_214454.jpg)
![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170624_214405.jpg)

## BOM
1. 2.9inch e-paper from Waveshare 
2. Nodemcu or Wemos or integrated PCB( gerber files in /PCB folder )
3. Li-Po battery
4. 3d printed case(STL files in /3d folder)

## Uploading code to esp8266
Add esp8266 to boards manager in arduino ide.
Use arduino ide to upload the code. Don't forget to upload font files.

## Font
I developped this tool to generate your own font (unicode ucs-2)
https://github.com/duck531a98/font-generator
Since arduino ide is compiling the code to utf-8 charset. There is a internal function to convert utf-8 to unicode ucs-2. Display strings typed in to arduino ide with function DrawUTF.
font should be uploaded into spiff memory of esp8266

## Low power issues
Since regulator AMS1117 have a quiescent current about several mA, so node mcu is not suitable for battery powered. A low quiescent current LDO is needed. Esp8266 consumes about 20-25uA in deep sleeping mode.