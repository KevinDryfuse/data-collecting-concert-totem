# data_collecting_concert_totem

Because I can't just build a "normal" festival totem, I wanted to build something a little different for the summer of 2020.  I wanted the totem to accomplish a few things:

 1. Be fun.
 2. Be something with a bunch of LEDs that I could code some behavior around.
 3. Collect data about the festivals I attend.

# So how does that translate into reality?

The idea is to create a totem that meets festival height requirements, uses an addressable LED matrix to show ambient noise levels, allow the LED matrix to be controlled with a remote (change colors, display messages, re-calibrate, etc), capture data (GPS coordinates, date-time, decibel levels, remote key presses, etc) to an SD card.

Now I have no clue the actual look of the totem, but the running concept is to have a character that encourages people to generate enough noise to get the meter to light up red.

## Prototype Layout
![Image of prototype](/images/diagram.PNG)

## Parts List

 - [Arduino Mega](https://www.elegoo.com/product/elegoo-mega-2560-r3-board-blue-atmega2560-atmega16u2-usb-cable/)
 - Breadboard
 - [Gravity Analog Sound Sensor](https://wiki.dfrobot.com/Gravity__Analog_Sound_Level_Meter_SKU_SEN0232)
 - [GPS Module](https://www.dfrobot.com/product-1302.html)
 - [SD Card Logging Shield](https://www.velleman.eu/products/view/?id=435522)
 - 10kΩ Potentiometer
 - [RF Receiver](https://arduinomodules.info/ky-022-infrared-receiver-module/)
 - [Real Time Clock](https://create.arduino.cc/projecthub/MisterBotBreak/how-to-use-a-real-time-clock-module-ds3231-bc90fe)
 - [LCD Panel (16x2)](http://wiki.sunfounder.cc/index.php?title=LCD1602_Module)
 - [Addressable LED Matrix (32x8)](https://www.btf-lighting.com/products/ws2812b-panel-screen-8-8-16-16-8-32-pixel-256-pixels-digital-flexible-led-programmed-individually-addressable-full-color-dc5v?variant=20203594612836)

