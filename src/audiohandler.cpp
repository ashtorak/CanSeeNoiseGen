#include "audiohandler.h"
#include "serialhandler.h"

SdFat sd; // sd card reader
const int SDSpeedInMHz = 25; // depends on card reader and connection (empirical)
// too low speed might lead to issues when a lot of samples are played at once
const int samplerCount = 6; // 1 for idling, 4 for reving, 1 for a bang
Sampler sampler[samplerCount]; // interface for the samples

int numberOfSampleSets = 5; // includes empty set for silence (=0)
int sampleSet = 0; // here you can set inital sample set
int sampleSetOrder[5] = {0,2,1,3,4}; // for changing order in which to iterate through sample sets
int sampleSetIterator = 0; // iterates trough sampleSetOrder

boolean changeSample = false;  // for sample switching

int16_t audioSample[2]; // 16 bit left/right sample for I2S output
size_t m_bytesWritten;  // for I2S buffer, not really used
int32_t sampleMax = 0;

int16_t potiValue;  // to change volume manually

int volumeGlobal = 9; //1..16, int for ease of use with available Bluetooth integration

int elecRPM = 0; // range 0...12000
int pedal = 0; // range 0...250 corresponds to 0...100%

uint32_t serialMod = 16;
uint32_t serialMod2 = 3;
uint32_t serialMod3 = 5;

// these volume floats will be changed by the sound modulator in the main loop
float volumeIdling = 1.0;
float volumeRevingLow = 1.0;
float volumeRevingMid = 1.0;
float volumeRevingHigh = 1.0;
float volumeRevingMax = 1.0;
float volumeBang = 1.0;
float volumePedal = 1.0;
float fadingFactor = 0.04;

// additional stuff for filtering
float volumePedalTimeConstant = 500.0; // in ms

float revMod;
float revIdleRPM;
float revLowRPM;
float revMidRPM;
float revHighRPM;
float revMaxRPM;
float RPMCutOff;
float crossRPMIdleLow;
float crossRPMLowMid;
float crossRPMMidHigh;
float crossRPMHighMax;
float crossRPMRange=0; // +/-
int gear = 1;
int RPM = 0;
int pedalCounterBang = 0;
int pedalPosBang = 150;

// main sample volume is set when loading a car (the following are default values)
// it represents the max volume
float volumeIdlingMain = 1.0;
float volumeRevingMain = 1.0;
float volumeBangMain =   1.0;

// engine simulation
float pedalRPMx = 0.0;
float pedalRPMy = 0.0;
float T_T0 = 0.05;
float attenuation = 1.4;
float pedalRPMdx;
float pedalRPMdy;

void audio_init()
{
  // setup SD card and samples
  // max. speed depends on card reader and connection (empirical)
  sd.begin(chipSelectPin, SD_SCK_MHZ(SDSpeedInMHz));

  for(int i=0; i<samplerCount; i++)
  {
    sampler[i].init();
  }
 
  audio_selectSampleSet(sampleSet);

  // setup pins

  // setup audio

  i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 22050,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
        .dma_buf_count = 8,  // max buffers is 8
        .dma_buf_len = 64, // max value is 1024
        .use_apll=0,
        .tx_desc_auto_clear= true,  // new in V1.0.1
        .fixed_mclk=-1
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_pin_config_t pins = {
    .bck_io_num = 22,             // Bit Clock
    .ws_io_num =  14,             //  wclk, Left/Right Clock
    .data_out_num = 23,           // Data Out
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin(I2S_NUM_0, &pins);
}

void audio_setVolume(int in)
{
  volumeGlobal = in;
}

void audio_changeSample()
{
    // change sample set with button
      sampleSetIterator++;
      if(sampleSetIterator==numberOfSampleSets) sampleSetIterator = 0;
      audio_selectSampleSet(sampleSetOrder[sampleSetIterator]);
}

void audio_test(int i1, int i2, int i3)
{
  if(i1!=0) serialMod = i1;
  if(i2!=0) serialMod2 = i2;
  if(i3!=0) serialMod3 = i3;
}

bool playSample(int16_t sample[2]) {

    uint32_t s32;
    s32 = (sample[1]<<16) | (sample[0] & 0xffff); // combine left and right for one I2S write
    esp_err_t err=i2s_write(I2S_NUM_0, (const char*)&s32, sizeof(uint32_t), &m_bytesWritten, 100);
    if(err!=ESP_OK){
        return false;
    }
    if(m_bytesWritten<4){
        Serial.println("Can't stuff any more in I2S...");
        return false;
    }
    return true;
}

void audio_fast()
{
  // fill sampler buffers
  for(int i=0; i<samplerCount; i++)
  {
    sampler[i].buffill();
  }

  // synthesize sample

  if(sampleSet!=0)
  { 
    int32_t sampleCalc = 0;

    for(int i=0; i<samplerCount; i++)
    {
        sampler[i].next();
    
        // output() returns 0 if splay(true) has not been set
        sampleCalc += sampler[i].output();
    }
    sampleCalc = (volumeGlobal*sampleCalc)>>serialMod3;
    if(sampleCalc>sampleMax) sampleMax = sampleCalc;
    if(sampleCalc>0xffff) sampleCalc = 0xffff; // output is 16 bit max
    audioSample[0]=sampleCalc;
    audioSample[1]=audioSample[0]; // no stereo for the moment
    playSample(audioSample);
  }
}

// set a range of samples to play and loop
void playAndLoop(int i1, int i2)
{
  for(int i=i1; i<=i2; i++) 
  {
    sampler[i].splay(true);
    sampler[i].sloop(true);
    sampler[i].buffill(); // open file
    sampler[i].buffill(); // fill buffer
  }  
}

void setRPMCrossPoints()
{
  crossRPMLowMid = revLowRPM+(revMidRPM-revLowRPM)/2;
  crossRPMMidHigh = revMidRPM+(revHighRPM-revMidRPM)/2;
  crossRPMHighMax = revHighRPM+(revMaxRPM-revHighRPM)/2;
  crossRPMIdleLow = revIdleRPM*(1.3);
  RPMCutOff = 11000*revMod;   
}

void audio_selectSampleSet(int number)
{
  sampleSet = number;

  for(int i=0; i<samplerCount; i++)
  {
    sampler[i].sstop();
    sampler[i].sloop(false);
  }

  if(sampleSet==2)
  {
    sampler[0].load("porscheIdle.wav");
    sampler[1].load("porsche3000.wav");
    sampler[2].load("porsche3750.wav");
    sampler[3].load("porsche5500.wav");
    sampler[4].load("porsche7000.wav");
    revMod = 0.8;
    revIdleRPM = 1500*revMod;
    revLowRPM = 3000*revMod;
    revMidRPM = 3750*revMod;
    revHighRPM = 5500*revMod;
    revMaxRPM = 7000*revMod;
    volumeIdlingMain = 1.1;
    volumeRevingMain = 1.1;
    volumeBangMain =   1.0;
    
    setRPMCrossPoints();
    playAndLoop(0,4);
  }
  else if(sampleSet==3)
  {
    sampler[0].load("cobraIdle.wav");
    sampler[1].load("cobra2886.wav");
    sampler[2].load("cobra3209.wav");
    sampler[3].load("cobra4360.wav");
    sampler[4].load("cobra6823.wav");
    revMod = 0.8;
    revIdleRPM = 1500*revMod;
    revLowRPM = 2886*revMod;
    revMidRPM = 3209*revMod;
    revHighRPM = 4360*revMod;
    revMaxRPM = 6823*revMod;
    volumeIdlingMain = 0.9;
    volumeRevingMain = 0.85;
    volumeBangMain = 1.0;
    
    setRPMCrossPoints();
    playAndLoop(0,4);
  }
  else if(sampleSet==4)
  {
    sampler[0].load("slsidle.wav");
    sampler[1].load("sls3000.wav");
    sampler[2].load("sls4400.wav");
    sampler[3].load("sls6000.wav");
    sampler[4].load("sls7250.wav");
    revMod = 0.8;
    revIdleRPM =1500*revMod;
    revLowRPM = 3000*revMod;
    revMidRPM = 4400*revMod;
    revHighRPM = 6000*revMod;
    revMaxRPM = 7250*revMod;
    volumeIdlingMain = 0.67;
    volumeRevingMain = 0.82;
    volumeBangMain = 1.0;
    
    setRPMCrossPoints();
    playAndLoop(0,4);
  }
  else if(sampleSet==1)
  {
    sampler[3].load("horseGallop1.wav");
    sampler[4].load("horseGallop2.wav");
    revMod = 1.0;
    revIdleRPM = 0;
    revHighRPM = 800*revMod;
    revMaxRPM = 1800*revMod;
    volumeIdlingMain = 0.76;
    volumeRevingMain = 0.96;

    setRPMCrossPoints();
    
    playAndLoop(3,4);
  }

}

void audio_soundModulator(int pedal_IN, int elecRPM_IN)
{
  pedal = pedal_IN;
  elecRPM = elecRPM_IN;

  if(sampleSet!=0)
  { 
      // set volume with poti
      if(poti_ON)
      {
        potiValue = analogRead(potiPin);
        volumeGlobal = (potiValue>>8)+1; // 12 bit to 4 bit (4096 to 16)
        // pedal = potiValue/4095.0*250;
        elecRPM = analogRead(potiPin2)*12000/4095.0;
       
      }
     

      if((elecRPM<1 && sampleSet!=1)) // allow reving with pedal when car is not moving and not horse or on input simulation
      {
          // simulate engine with 2nd order transfer function
          if(pedalRPMx>0) pedalRPMdy = T_T0*pedalRPMx*serialMod/10.0; // for reving up
          else pedalRPMdy = T_T0*0.33*pedalRPMx; // for coasting down slower
          pedalRPMdx = T_T0*((pedal/250.0)-pedalRPMy-2*attenuation*pedalRPMx);
          pedalRPMy += pedalRPMdy;
          pedalRPMx += pedalRPMdx;
          // some limits, mostly for testing
          if(pedalRPMy>1.0) pedalRPMy = 1.0;
          if(pedalRPMy<0.0) pedalRPMy = 0.0;
          if(pedalRPMx>1.0) pedalRPMx = 1.0;
          if(pedalRPMx<-1.0) pedalRPMx = -1.0;
      }
      else // coast down simulated engine
      {
          pedalRPMdy = T_T0*0.33*pedalRPMx;
          pedalRPMdx = T_T0*(0-pedalRPMy-2*attenuation*pedalRPMx);
          pedalRPMy += pedalRPMdy;
          pedalRPMx += pedalRPMdx;
      }
      
      RPM = revIdleRPM + elecRPM + pedalRPMy*RPMCutOff*0.9;

      // adjust volumes and playback according to RPM

      if(sampleSet==1) // horse needs special treatment
      {
          volumeIdling = volumeIdlingMain;
          
          if(RPM<revMaxRPM)
          {    
          if(pedal>=200) 
          {
              if(!sampler[2].playSample)
              {
              sampler[2].load("horseAngry.wav");
              sampler[2].splay(true);
              sampler[2].setVolume(volumeIdling/1.0);
              sampler[2].setPhaseIncrement(1.0);
              sampler[1].sstop();
              sampler[0].sstop();
              }
          }
          else if(pedal>=100)
          { 
              if(!sampler[1].playSample && !sampler[2].playSample)
              {
              sampler[1].load("horseWhinny.wav");
              sampler[1].splay(true);
              sampler[1].setVolume(volumeIdling/1.0);
              sampler[1].setPhaseIncrement(1.0);
              sampler[2].sstop();
              sampler[0].sstop();
              }
          }
          else if(pedal>1)
          { 
              if(!sampler[0].playSample && !sampler[1].playSample && !sampler[2].playSample)
              {
              sampler[0].load("horseGrunts.wav");
              sampler[0].splay(true);
              sampler[0].setVolume(volumeIdling);
              //if(elecRPM<1) sampler[0].setVolume(volumeIdling/1.0);
              //else sampler[0].setVolume(volumeIdling/1.5);
              sampler[0].setPhaseIncrement(1.0);
              sampler[1].sstop();
              sampler[2].sstop();
              }
          } 
          }
          
          if(elecRPM<1)
          {
          volumeRevingHigh = 0;
          volumeRevingMax = 0;
          }
          else
          {
          volumeRevingHigh = volumeRevingMain/(1+pow(2.71828,(0.003*(RPM-(crossRPMHighMax+crossRPMRange)))));
          volumeRevingMax = volumeRevingMain/(1+pow(2.71828,(-0.003*(RPM-(crossRPMHighMax-crossRPMRange)))))/(1+pow(2.71828,(0.04*(RPM-revMaxRPM*1.5))));
          }
          
          sampler[3].setVolume(volumeRevingHigh*0.8); // gallop1
          sampler[4].setVolume(volumeRevingMax); // gallop2
          
          // adjust pitch with RPM
          sampler[3].setPhaseIncrement(0.8+RPM/revHighRPM/2.5);
          sampler[4].setPhaseIncrement(RPM/revMaxRPM);
      }
      else  // adjustments for normal reving
      {
          // make volume dependent on RPM for reving
          //volumePedal=serialMod*0.01+pedalRPMy*(1.0-serialMod*0.01);
          fadingFactor=0.001*serialMod2;
          // set volume according to RPM
          volumeIdling = volumePedal*volumeIdlingMain/(1+pow(2.71828,(fadingFactor*(RPM-(crossRPMIdleLow+crossRPMRange)))));
          volumeRevingLow = volumePedal*volumeRevingMain/(1+pow(2.71828,(-fadingFactor*(RPM-(crossRPMIdleLow-crossRPMRange)))))/(1+pow(2.71828,(fadingFactor*(RPM-(crossRPMLowMid+crossRPMRange)))));
          volumeRevingMid = volumePedal*volumeRevingMain/(1+pow(2.71828,(-fadingFactor*(RPM-(crossRPMLowMid-crossRPMRange)))))/(1+pow(2.71828,(fadingFactor*(RPM-(crossRPMMidHigh+crossRPMRange)))));
          volumeRevingHigh = volumePedal*volumeRevingMain/(1+pow(2.71828,(-fadingFactor*(RPM-(crossRPMMidHigh-crossRPMRange)))))/(1+pow(2.71828,(fadingFactor*(RPM-(crossRPMHighMax+crossRPMRange)))));
          volumeRevingMax = volumePedal*volumeRevingMain/(1+pow(2.71828,(-fadingFactor*(RPM-(crossRPMHighMax-crossRPMRange)))))/(1+pow(2.71828,(0.04*(RPM-RPMCutOff))));
          
          sampler[0].setVolume(volumeIdling);
          sampler[1].setVolume(volumeRevingLow);
          sampler[2].setVolume(volumeRevingMid);
          sampler[3].setVolume(volumeRevingHigh);
          sampler[4].setVolume(volumeRevingMax);

          // adjust pitch with RPM
          sampler[0].setPhaseIncrement(RPM/revIdleRPM);
          sampler[1].setPhaseIncrement(RPM/revLowRPM);
          sampler[2].setPhaseIncrement(RPM/revMidRPM);
          sampler[3].setPhaseIncrement(RPM/revHighRPM);
          sampler[4].setPhaseIncrement(RPM/revMaxRPM);
      }
      
      // add a bang
      
      if(pedalCounterBang>5000 && sampleSet!=1) // not for horse
      {
          if(RPM>(4000*revMod) && pedal<1) 
          {
          sampler[5].load("bang.wav");
          sampler[5].splay(true);
          volumeBang = volumeBangMain;
          sampler[5].setVolume(volumeBang);
          pedalCounterBang = 0;
          }
      }
      else if(pedalCounterBang>=0 && pedalCounterBang<5100) pedalCounterBang += pedal-pedalPosBang;
      else pedalCounterBang = 0;
      
      // play sample only if you can actually hear it
      for(int i=0; i<samplerCount; i++)
      {
          if(sampler[i].volume>0.02) sampler[i].splay(true);
          else sampler[i].splay(false);
      }
  }
}

String audio_debug1()
{
  return "p: " +String(pedal) + " elecRPM: " +String(elecRPM) + " vG: "+ String(volumeGlobal)+" sampleMax: "+String(sampleMax)+"\n";

   // writeOutgoingBluetooth("pedalrpmX: "+String(pedalRPMx,5)+"\n");
   // writeOutgoingBluetooth("pedalrpmY: "+String(pedalRPMy,5)+"\n");
}

String audio_debug2()
{
  sampleMax = 0;
  return "i: "+String(volumeIdling,2)+" l: "+String(volumeRevingLow,2)+" m: "+String(volumeRevingMid,2)+" h: "+String(volumeRevingHigh,2)+" m: "+String(volumeRevingMax,2)+"\n";
}