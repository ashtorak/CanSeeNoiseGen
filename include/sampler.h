#include <Arduino.h>

#include "SdFat.h"
#include "serialhandler.h"

const int bufSize = 2*512;          // buffer size in bytes. You can change this data.

// Header of a wave file.
// Num_channels will tell us if we are in stereo (2) or mono (1)
typedef struct {
  char RIFF[4]; 
  int32_t chunk_size;
  char WAVE[4]; 
  char fmt_[4]; 
  int32_t subchunk1_size;
  int16_t audio_format;
  int16_t num_channels;
  int32_t sample_rate;
  int32_t byte_rate;
  int16_t block_align;
  int16_t bits_per_sample;
  char DATA[4]; 
  int32_t subchunk2_size;
} wave_header;


// Here is our sampler class
class Sampler
{
	
public:
   boolean playSample;         // is the sample playing or not ?
   float volume;
   float phaseIncrement;       // speed at which is iterated through buffer
   float indBuf;               // buffer number
   uint16_t bufRead;           // buffer being read number
   uint16_t lastBuf;           // last buffer read number
   int16_t buf[2][bufSize];    // buffer
   uint32_t posSample;         // sample position
   uint32_t endoffile;         // end of file
   
   SdFile myFile;              // The wave file
   wave_header header;         // The wave header
   
   boolean openfile ;          // Are we opening file ?
   boolean closefile ;         // Are we closing file ?
   boolean loopFile;

   const char* samplen;        // Storing the sample name  

   int bufContent = bufSize;   // some additional stuff for looping
   int bufEnd;
   int endBuf;
   boolean weAreAtEnd;

// Init of the sampler object. Here you can put your default values.
   void init()
   {
     indBuf = 0.0;
     bufRead = 0;
     lastBuf = 0;
     volume = 0.0;
     phaseIncrement = 1.0;
     playSample = false;
     openfile = false;	
     closefile = false;
     loopFile = false;	
     bufContent = bufSize;
     bufEnd = 0;
     endBuf = 0;
     weAreAtEnd = false; 
   }

// Play the sample
   void splay(boolean in)
   {
     playSample=in;
   }

// loop it
   void sloop(boolean in)
   {
     loopFile=in;
   }

// load the wave header in the header data, and the beginning of the wave file in the buffer with openfile = true via buffill()
   void load(const char* samplename)
   {
     volume = 0.0;
     indBuf = 0.0;
     bufRead = 0;
     lastBuf = 0;
     posSample = 0;
     samplen = samplename;
     closefile = false;
     openfile = true;
     bufEnd = 0;
     endBuf = 0;
     weAreAtEnd = false; 
   }

//Stop playing the sample
   void sstop()
   {
       openfile = false;
       closefile = true;
       playSample = true; // to run buffill()
       buffill();
       volume = 0.0;
   }

//Set the volume
   void setVolume(float vol)
   {
     volume = vol;
   }

//Set the phase increment
   void setPhaseIncrement(float p)
   {
     phaseIncrement = p;
   }

// Fill the buffer if it has to be. 
// This method must not be called in the main program loop.
// The return of the method indicates that it is reading a sound file
   boolean buffill()
   {
     boolean ret = false;
     if(playSample)
     {
       if(openfile)
         {
           myFile.open(samplen, O_READ);
           myFile.read(&header, sizeof(header));
           endoffile = header.chunk_size;
           myFile.read(buf[0], sizeof(buf[0]));
           posSample = sizeof(buf[0]) + sizeof(header);
           openfile = false;
         }
       else if(closefile) 
         {
           myFile.close();
           playSample = false;
           loopFile = false;
         }
       // The buffer must be filled only if the previous buffer was finished to be read
       else if(bufRead==lastBuf)
       {
      	 ret = true;

  	     if(lastBuf==0) lastBuf = 1;
  	     else lastBuf = 0;

         int me = 0;
         if(posSample<(endoffile-sizeof(buf[lastBuf])))
         {
           if(bufRead!=endBuf) weAreAtEnd = false;
           bufContent = bufSize;
           
    	     me = myFile.read(buf[lastBuf], sizeof(buf[lastBuf]));
    	     posSample += sizeof(buf[lastBuf]);

           
         }
         else if(loopFile)
         {
           weAreAtEnd = true; // this is checked when iterating through buffer
           endBuf = lastBuf;
           // make sure that only actual data will be read from the buffer
           bufEnd = (endoffile-posSample)/sizeof(buf[lastBuf][0]);
           
           me = myFile.read(buf[lastBuf], (endoffile-posSample));
           
           myFile.seekSet(sizeof(header));
           posSample = sizeof(header);
         }
         else
         {
           weAreAtEnd = true;
           endBuf = lastBuf;
           bufEnd = (endoffile-posSample)/sizeof(buf[lastBuf][0]);
           
           me = myFile.read(buf[lastBuf], (endoffile-posSample));
           
           closefile=true;
         }
         
  	     // If there is an error while reading sample file, we stop the reading and close the file
         if(me<=0) 
  	     {
            writeOutgoingSerial("error reading file during buffer fill \r\n");
  	        myFile.close();
            openfile=true;
  	     }
      	}
      }
     return ret;
   }
 
   
// Compute the position of the next sample. Must be called in the sound generating loop, before the method "output()"   
   void next()
   {
     if(playSample)  
     {
      indBuf += phaseIncrement;
      
      if(weAreAtEnd && bufRead==endBuf) bufContent = bufEnd;
      
      // If a buffer is finished to be read, we read the other buffer.
      if(indBuf>=bufContent)
      {
        if(bufRead==0) bufRead = 1;
        else bufRead = 0;
	       
        indBuf=0;
      }
    } 
  }

// Read the wave file at a position multiplied by the samplers volume
  int16_t output()
  {
    int16_t ret = 0;
	   
    if(playSample)
    {
      ret = (buf[bufRead][int(indBuf)])*volume;
    }

    return ret;
  }
};
