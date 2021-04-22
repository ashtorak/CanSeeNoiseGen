#ifndef AUDIOHANDLERH
#define AUDIOHANDLERH

#include "SdFat.h"
#include "serialhandler.h"
#include "sampler.h"
#include "driver/i2s.h"

static const int sampleSetSelectorPin = 15;
static const int chipSelectPin = 4;
static const int potiPin = 33;

void audio_init();
void audio_fast();
void audio_selectSampleSet(int number);
void audio_soundModulator(int pedal, int elecRPM);
String audio_debug1();
String audio_debug2();
void audio_setVolume(int volumeGlobal);
void audio_test(int serialMod1, int serialMod2, int serialMod3);

#endif
