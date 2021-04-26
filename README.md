# CanSeeNoiseGen
CanSee with Vehicle Noise Generator add-on for Renault Zoe (pre 2019 models)

This is using VSCode with PlatformIO and SdFat library. With the latter the build gets too large to use the standard build partition size, so I had to use a custom.csv in the platformio.ini where I set the partition size to 0x200000.  
In my ini I also put some monitor flags and filters, which I tried out for debugging and testing. This is only for the serial interface via USB.

Everything is based on the wonderful work by Jeroen Meijer and co.: https://gitlab.com/jeroenmeijer/cansee

You can connect it to the "V-bus" at the diagnostic OBD port in the car or to the "E-bus" which goes directly to the Zoe VSP (https://canze.fisch.lu/the-pedestrian-horn/)
This NoiseGen software is set up for the E-bus. If you want to use the V-bus, you have to change the code in ticker10ms() in CanSee.ino.  
On E-bus not the same values are sent or different IDs are used. So CanZe won't display all the values.

This has been tested on a 2015 Q210. No guarantee that it works on other models.  
On 2019 or newer models a noise generator is mandatory and an off-switch is not allowed, afaik.

There is an integration with CanZe app in development to control the Noise Generator via bluetooth. But I haven't decided yet what to do with it. I might just release the .apk here.  
In any case, you can use a bluetooth terminal app and send text commands there. See CanSee.ino for reference.
