# Esp8266-weather-station-epaper
## Summary
use esp8266 to show weather forecast on 2.9inch e-paper
you can get 2.9inch e-paper from Waveshare

Esp8266 is in deep sleeping after update the weahter forecast
![](https://github.com/duck531a98/esp8266-weather-station-epaper/raw/master/pics/20170623_232157.jpg)

## BOM
1. 2.9inch e-paper from Waveshare 
2. nodemcu or wemos or integrated PCB(/PCB folder)
3. Li-Po battery
4. 3d printed case(/3d folder)

## Uploading code to esp8266
Use arduino ide to upload the code. Don't forget to upload font files.

## Font
This tool could generate your own font 
https://github.com/duck531a98/font-generator
font should be uploaded into spiff memory of esp8266

## Low power
since AMS1117 have a quiescent current about several mA, so node mcu is not proper for battery powered.