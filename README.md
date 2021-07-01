# CanSeeNoiseGen
CanSee with Vehicle Noise Generator 3.0 add-on for Renault Zoe (pre 2019 models)

This is an ESP32 project using VSCode with PlatformIO (Arduino framework), CirculaBuffer library and SdFat library. With the latter the build gets too large to use the standard build partition size, so I had to use a custom.csv in the platformio.ini where I set the partition size to 0x200000. See bottom for installation details.  
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


Installation instructions:

1. Install VSCode.
2. In VSCode install the PlatformIO IDE extension. 
3. Go to PlatformIO Home (there should appear a bar at the bottom with a house). Then in "Open Project" select this whole folder from Github. Then it should install also the libraries by itself from platform.ini. Else, you can install them in PIO Home. Also, sometimes it might be necessary to close and open VSCode again so that it finds all the dependencies. 
4. Copy custom.csv to \.platformio\packages\framework-arduinoespressif32\tools\partitions (the program is so large that it needs more memory than default)
5. Connect ESP32 via USB.
6. Build the project and upload it with the buttons on the PIO bar at the bottom.
7. For programming look into CanSee.ino and audiohandler.cpp in the src folder. The file names to load from the SD card are hard coded in the audiohandler. 22050 Hz @ 16 bit audio files are used. You can find the samples in the hardware folder. Just copy the folder content to the SD card.


With the PlatformIO: Serial Monitor or a bluetooth terminal, you can monitor some parameters and give commands that are defined in CanSee.ino.
Attention: The ESP32 and the programm store some configurations options on an EEPROM. For example, you can switch on and off bluetooth and it keeps the state also during power off. When you start the ESP and you don't have bluetooth or the serial output available, it might just be configured like this. See config.cpp for the initial configuration. If you change this configuration later on and upload it to the ESP, in order to actually load the new settings into the EEPROM you have to use the special function setConfigToEeprom(true) or 'z' via the terminal.
