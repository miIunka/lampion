#! /bin/sh

mkdir -p temp cache

/usr/share/arduino/arduino-builder -compile -hardware /usr/share/arduino/hardware -hardware $HOME/.arduino15/packages -tools /usr/share/arduino/tools-builder -tools $HOME/.arduino15/packages -libraries $HOME/Arduino/libraries -fqbn=arduino:avr:nano:cpu=atmega328 -vid-pid=0403_6001 -ide-version=10819 -build-path temp -warnings=all -build-cache cache -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.avrdude.path=$HOME/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17 -prefs=runtime.tools.avrdude-6.3.0-arduino17.path=$HOME/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17 -prefs=runtime.tools.arduinoOTA.path=$HOME/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.arduinoOTA-1.3.0.path=$HOME/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.avr-gcc.path=$HOME/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 -prefs=runtime.tools.avr-gcc-7.3.0-atmel3.6.1-arduino7.path=$HOME/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 lampion-neopixel.ino
