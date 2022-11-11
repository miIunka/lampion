#! /bin/sh

avrdude -c arduino -p m328p -P /dev/ttyUSB0 -V -U flash:w:temp/lampion-neopixel.ino.hex:i
