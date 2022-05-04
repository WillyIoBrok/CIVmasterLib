#ifndef defines_h
#define defines_h

/* 
IC705toPA - a_Defines.h
*/

#define VERSION_STRING "IC705toPA V0_1 22/05/04"

// Global compile switches ===================================================================================

// if defined, the bit pattern of the output pins is inverted in order to compensate
// the effect of inverting HW drivers (active, i.e.uncommented by default)
#define invDriver 

// if defined debug messages on the serial line will be generated
// (active, i.e.uncommented by default)
#define debug 

// Speed of the Serial output messages
#define debugBdRate 115200

// some general defines ----------------------------------

enum onOff_t:uint8_t {
	OFF = 0,
	ON  = 1,
  NDEF
};

// Mapping of portpins to function ===========================================================================

#define P_BCD0            33
#define P_BCD1            25
#define P_BCD2            26
#define P_BCD3            27

// Debugging ...

#ifdef debug
  #define SET_TIME_MARKER1 G_timemarker1 = micros();
  #define EVAL_TIME_MARKER1 G_timemarker1a = micros();Serial.print("t1:  ");Serial.println(G_timemarker1a-G_timemarker1);
#endif

#ifndef debug
  #define SET_TIME_MARKER1
  #define EVAL_TIME_MARKER1
#endif

#endif // #ifndef defines_h
