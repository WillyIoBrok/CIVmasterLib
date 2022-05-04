#ifndef defines_h
#define defines_h

/* 
AfuCtrl_test - a_Defines.h
*/

// Global compile switches ===================================================================================

#define VERSION_STRING "ICradio_IC705_SimpleBTtest V0_1 22/05/01"

// common switches -----------------------------------

// if defined, debug messages on the serial line will be generated
#define debug 

//-------------------------------------------------------------------------------

#define BASELOOP_TICK 10 

#define NUM_BANDS 24   /* Number of Bands (depending on the radio) */


// some general defines ----------------------------------

enum onOff_t:uint8_t {
	OFF = 0,
	ON  = 1,
  NDEF
};

// Mapping of portpins to function ===========================================================================

#define P_INT_LED          2
#define P_CONN_RADIO_LED   4

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
