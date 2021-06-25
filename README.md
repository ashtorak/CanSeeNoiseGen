# CanSeeNoiseGen
CanSee with Vehicle Noise Generator 3.0 add-on for Renault Zoe (pre 2019 models)

This is an ESP32 project using VSCode with PlatformIO (Arduino framework), CirculaBuffer library and SdFat library. With the latter the build gets too large to use the standard build partition size, so I had to use a custom.csv in the platformio.ini where I set the partition size to 0x200000.  
In my ini I also put some monitor flags and filters, which I tried out for debugging and testing. This is only for the serial interface via USB.

Everything is based on the wonderful work by Jeroen Meijer and co.: https://gitlab.com/jeroenmeijer/cansee

You can connect it to the "V-bus" at the diagnostic OBD port in the car or to the "E-bus" which goes directly to the Zoe VSP (https://canze.fisch.lu/the-pedestrian-horn/)
This NoiseGen software is set up for the E-bus. If you want to use the V-bus, you have to change the code in ticker10ms() in CanSee.ino.  
On E-bus not the same values are sent or different IDs are used. So CanZe won't display all the values.

This has been tested on a 2014 Q210. No guarantee that it works on other models.  
On 2019 or newer models a noise generator is mandatory and an off-switch is not allowed, afaik.

There is an integration with CanZe app in development to control the Noise Generator via bluetooth. But I haven't decided yet what to do with it. I might just release the .apk here.  
In any case, you can use a bluetooth terminal app and send text commands there. See CanSee.ino for reference.


More technical remarks:

I have to check how to properly connect the I2S board to the ESP. It seems very sensitive, producing crackling on the audio output easily. The best was to make the wires as short as possible. I also found that connecting the AGND directly to the GND of the ESP board seems to help. I am not sure, why.  
The manual for the ESP says to put pullup resistors at the I2S pins. But this didn't do anything for me.

Regarding SD card reading speed, I have tried different boards and combinations and found that the used components give the best speeds up to 25 MHz. If it is too slow (10 MHz), there will be errors when several samples are played in parallel.


Thread in German Zoe forum with more info in German: https://www.goingelectric.de/forum/viewtopic.php?f=63&t=52091&p=1571585#p1571585
