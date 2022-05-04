/* 
CIVmasterLib ICradio_selRadioTest - W.Dilling/DK8RW

Following radios have been tested: IC705, IC7300 and IC9700!

!!! Please select one (and only ONE !) of the radios in the list              !!!
!!!                                                                           !!!
!!! It is best to start the Serial monitor, which is essential for this test, !!!
!!! before compiling and downloading the SW                                   !!!
!!!                                                                           !!!
!!! Set the baudrate of the serial monitor to 115200Bd                        !!!

Pressing "e" + "return" switches on an ICxxxx connected via CIV-bus 
!!! This does NOT work via Bluetooth !!!

Pressing "a" + "return" switches off an ICxxxx connected via CIV-bus 

Pressing "v" + "return" sets a ICxxxx connected via CIV-bus to Voice mode

Pressing "d" + "return" sets a ICxxxx connected via CIV-bus to data mode

Pressing "g" + "return" prints the general mode ("Voice" or "Data")

Pressing "f" + "return" gets the frequency of a ICxxxx connected via CIV-bus
(Note: if this command returns "0", the internal frequency of instance ICxxxx
 hasn't been updated. This can be changed by turning the main tuning knob 
 on the radio or by changing bands)

Pressing "m" + "return" gets the Modulation Mode and the RX filter setting of the radio.

Pressing "l" + "return" gives a readout of the CIV logbuffer
Note:
In order to get the best information from the test (CIV logbuffer),
please activate(uncomment) "#define log_CIV" in file civ.h
in the library CIVbuslib first !
This is deactivated by default due to saving data memory reasons.

*/

// select one of the radios in the list:
#define useIC705
//#define useIC7300
//#define useIC9700

/* includes -----------------------------------------------------------------*/

#include "a_defines.h"

#include <CIVcmds.h>
#include <CIVmaster.h>
#include <ICradio.h>    // this would include CIVcmds.h and CIVmaster.h, if not included before !

//-------------------------------------------------------------------------------
// create the civ and ICradio objects in use

CIV     civ;  // create the CIV-Interface object first (mandatory for the use of ICradio)

#ifdef useIC705
  ICradio ICxxxx(TypeIC705,CIV_ADDR_705);
#endif

#ifdef useIC7300
  ICradio ICxxxx(TypeIC7300,CIV_ADDR_7300);
#endif

#ifdef useIC9700
  ICradio ICxxxx(TypeIC9700,CIV_ADDR_9700);
#endif

//-------------------------------------------------------------------------------

uint8_t lpCnt = 0;
CIVresult_t CIVresultL;

radioOnOff_t  	radioS  = RADIO_NDEF;
radioOnOff_t  	localS  = RADIO_NDEF;
unsigned long 	radioF      = 0;
unsigned long 	localF      = 0;
uint8_t       	radioM      = MOD_NDEF;
uint8_t       	localM      = MOD_NDEF;
uint8_t       	radioFil    = FIL_NDEF;
uint8_t       	localFil    = FIL_NDEF;


unsigned long time_current_baseloop;
unsigned long time_last_baseloop;

#ifdef debug
  unsigned long G_timemarker1;
  unsigned long G_timemarker1a;
#endif

//---------------------------------------------------------------------------------------------
// check and get the status of the keys (+ simulated keys) for different testcases

keyPressed_t get_key() {

  keyPressed_t  ret_val = NO_KEY_PRESSED;
  uint8_t       inByte  = 0;
  
  if (Serial.available()>0)  inByte = Serial.read();

  if (inByte=='e') ret_val = KEY_EIN_PRESSED;
  if (inByte=='a') ret_val = KEY_AUS_PRESSED;
  if (inByte=='v') ret_val = KEY_VOICE_PRESSED;
  if (inByte=='d') ret_val = KEY_DATA_PRESSED;
  if (inByte=='g') ret_val = KEY_MODE_PRESSED;
  if (inByte=='f') ret_val = KEY_FREQ_PRESSED;
  if (inByte=='m') ret_val = KEY_MODMODE_PRESSED;
  if (inByte=='l') ret_val = KEY_LOG_PRESSED;
  if (inByte=='h') ret_val = KEY_HELP_PRESSED;

  return ret_val;
}

//---------------------------------------------------------------------------------------------

void  printHelp() {

  Serial.println ("");
  Serial.println ("ICradio_selRadioTest");
  Serial.println ("The following info is shown automatically if changed:");
  Serial.println ("On/Off State, Frequency, Modulation mode, RX Filter");
  Serial.println ("");
  Serial.println (" e -> switch the radio on (doesn't work via BT!!)");
  Serial.println (" a -> switch the radio off");
  Serial.println (" v -> set radio to voice mode");
  Serial.println (" d -> set radio to data mode");
  Serial.println (" g -> get the general radio mode");
  Serial.println (" f -> get the frequency of the radio");
  Serial.println (" m -> get the Modulation mode and RX filter setting of the radio");
  Serial.println (" l -> get the log (only if logging is switched on in file civmaster.h)");
  Serial.println (" h -> show this help screen");
  Serial.println ();

}

//---------------------------------------------------------------------------------------------
// test of the radio on/off state
void testRadioOnOff (uint8_t key) {

  if (key==KEY_EIN_PRESSED) { // ON
    key = NO_KEY_PRESSED;
    civ.logClear();
    Serial.println (" KEY_EIN*");
    SET_TIME_MARKER1;
    radioS = ICxxxx.setDCPower(RADIO_ON,time_current_baseloop);
    EVAL_TIME_MARKER1;
    radioS = ICxxxx.getAvailability(); // -> returns RADIO_ON (ON->ON) or RADIO_OFF_TR (OFF->ON)
    Serial.println (radioOnOffStr[radioS]);
  }

  if (key==KEY_AUS_PRESSED) { // OFF
    key = NO_KEY_PRESSED;
    civ.logClear();
    Serial.println (" KEY_AUS*");
    SET_TIME_MARKER1;
    radioS = ICxxxx.setDCPower(RADIO_OFF,time_current_baseloop);
    EVAL_TIME_MARKER1;
    radioS = ICxxxx.getAvailability(); // -> returns RADIO_OFF (OFF->OFF)or RADIO_ON_TR (ON->OFF)
    Serial.println (radioOnOffStr[radioS]);
  }    

}

//---------------------------------------------------------------------------------------------
// test of switching the "general" modes of the radio ("voice" or "data"
// use: press the "v" or "d" key on your computer keyboard
// if the logging is enabled, you can see the commands by pressing "l"


void mode_sequences(uint8_t key) {

  if (key==KEY_VOICE_PRESSED) {
    key = NO_KEY_PRESSED;
    Serial.println (" KEY_VOICE*");
    civ.logClear();
    ICxxxx.setMode(MODE_VOICE);
    Serial.println ("VOICE-MODE");
  }

  if (key==KEY_DATA_PRESSED) {
    key = NO_KEY_PRESSED;
    Serial.println (" KEY_DATA*");
    civ.logClear();
    ICxxxx.setMode(MODE_DATA);
    Serial.println ("DATA-MODE");
  }

}

//---------------------------------------------------------------------------------------------
// get the radio data and print them via the USB COM-port into the serial monitor

void	getradioInfo() {


    radioS = ICxxxx.getAvailability();
    if (radioS != localS) {               // if there is a change in ON/OFF state -> print it out!
      Serial.print ("radioState: "); 
      Serial.println (radioOnOffStr[radioS]);
      localS = radioS;
    }

    if (radioS==RADIO_ON) {               // only in case, the radio is switched on 
                                          // and the connection is up and running

      radioF = ICxxxx.getFrequency();
      if (localF != radioF) {             // if there is a frequency change -> print it out!
        Serial.print    ("Freq[Hz]: "); Serial.println  (radioF);
        localF = radioF;
      }

      radioM    = ICxxxx.getModMode();
      radioFil  = ICxxxx.getRxFilter();
      if ((localM != radioM) || (localFil != radioFil)) { // if there is a change -> print it out!
        Serial.print ("Mod:  "); Serial.print   (modModeStr[radioM]);
        Serial.print (" Fil: "); Serial.println (FilStr[radioFil]);
        localM = radioM; localFil = radioFil;
      }

		}

}


//=============================================================================================
//==========  General initialization  of  the device  =========================================
void setup() {

  // initialize the serial interface
  Serial.begin(115200);
  delay(100);
  Serial.println("");
  Serial.println (VERSION_STRING);
  Serial.println("");

  #ifdef useIC705
    Serial.println ("IC705(BT) selected");
  #endif

  #ifdef useIC7300
    Serial.println ("IC7300 selected");
  #endif

  #ifdef useIC9700
    Serial.println ("IC9700 selected");
  #endif

  #ifdef useIC705
    civ.setupp(true);                   // initialize the civ object/module with BT conn (possible only on ESP32)
  #else
    civ.setupp();                       // initialize the civ object/module
  #endif

  ICxxxx.setupp(millis());            // initialize the ICradio class

  printHelp();                        // show the commands
  
  time_current_baseloop = millis();
  time_last_baseloop = time_current_baseloop;
  
}

//============================  main  procedure ===============================================
void loop() {

  keyPressed_t keyCmd;

  time_current_baseloop = millis();
  
  if ((time_current_baseloop - time_last_baseloop) > BASELOOP_TICK) {

		//----------------------------------------------------------------------------------------
		// calling the loop function of each radio in use as often as possible (i.e. approx. 10ms)
		// after the end of the "radio bootup phase" :

    if (time_current_baseloop>t_RadioCheck) ICxxxx.loopp(time_current_baseloop); 

		// get the available info from the radio and print it out if necessary
		getradioInfo();

		// check, whether an additional action has been requested by the user
    keyCmd = get_key();  // get command input

		//----------------------------------------------------------------------------------------
		// different test cases started by key entry ...

    testRadioOnOff (keyCmd);  // use "e" or "a"

    mode_sequences (keyCmd);  // use "v" or "d"

    if (keyCmd==KEY_MODE_PRESSED) {      // use "g"
      Serial.print("Mode: "); Serial.println (ModeStr[ICxxxx.getMode()]);
    }

    if (keyCmd==KEY_FREQ_PRESSED) {			// use "f"
      Serial.print("Freq[Hz]: "); Serial.println (ICxxxx.getFrequency());
    }

    if (keyCmd==KEY_MODMODE_PRESSED) {	// use "m"
      Serial.print("Mod:  "); Serial.print (modModeStr[ICxxxx.getModMode()]);
      Serial.print(" Fil: "); Serial.println (FilStr[ICxxxx.getRxFilter()]);
    }

    if (keyCmd==KEY_HELP_PRESSED) {      // use "h"
      printHelp();
    }

    // use "l",if "#define log_CIV" in file civ.h is active
    if (keyCmd==KEY_LOG_PRESSED) civ.logDisplay();

		//----------------------------------------------------------------------------------------
    lpCnt++;
    time_last_baseloop = time_current_baseloop;
	} // if BASELOOP_TICK
  
} // end loop
