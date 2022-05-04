#ifndef defines_h
#define defines_h

/* 
ICradio_test - a_Defines.h
*/

// Global compile switches ===================================================================================

#define VERSION_STRING "CIVbusLib ICradio_selRadioTest V0_8 22/05/04"

// common switches -----------------------------------

// if defined, debug messages on the serial line will be generated
#define debug 

// some general defines ----------------------------------

enum onOff_t:uint8_t {
	OFF = 0,
	ON  = 1,
  UNDEF
};

enum keyPressed_t:uint8_t {
	NO_KEY_PRESSED 		= 0,
  KEY_EIN_PRESSED,
  KEY_AUS_PRESSED,
  KEY_VOICE_PRESSED,
  KEY_DATA_PRESSED,
  KEY_MODE_PRESSED,
  KEY_FREQ_PRESSED,
  KEY_MODMODE_PRESSED,
  KEY_HELP_PRESSED,
  KEY_LOG_PRESSED
};

// translation of the radio's general mode
const String ModeStr[3] = {
  "MODE_NDEF",
  "MODE_VOICE",
  "MODE_DATA"
};

// states of radio's DC-Power (on/Off State)
const String radioOnOffStr[6] = {
  "RADIO_OFF",
  "RADIO_ON",
  "RADIO_OFF_TR",     // transit from OFF to ON
  "RADIO_ON_TR",      // transit from ON to OFF
  "RADIO_NDEF",       // don't know
  "RADIO_TOGGLE"
};

// clear test translation of the modulation modes
const String modModeStr[11] = {
  "LSB   ", // 00 (00 .. 08 is according to ICOM's documentation) 
  "USB   ", // 01
  "AM    ", // 02
  "CW    ", // 03
  "RTTY  ", // 04
  "FM    ", // 05
  "WFM   ", // 06
  "CW-R  ", // 07
  "RTTY-R", // 08
  "DV    ", // 09 (Note: on the ICOM CIV bus, this is coded as 17 in BCD-code, i.e. 0x17)
  "NDEF  "  // 10
};

// clear text translation of the Filter setting
const String FilStr[4] = {
  "NDEF",
  "FIL1",   // 1 (1 .. 3 is according to ICOM's documentation)
  "FIL2",
  "FIL3"
};

// repeat time of the baseloop actions in ms
#define BASELOOP_TICK 10 


// Mapping of portpins to function ===========================================================================

#define P_STATUS_LED   4

#define P_INT_LED     13

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
