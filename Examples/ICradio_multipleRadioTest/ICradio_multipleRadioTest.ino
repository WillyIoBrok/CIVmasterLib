/* 
CIVmasterLib ICradio_multipleRadioTest - W.Dilling/DK8RW

Task: Show the use of two radios on the HW (onewire) bus!
(IC7300 and IC9700 have been tested)

Pressing "r" + "return" switches between Radio1 (IC7300) and Radio2 (IC9700)

Pressing "e" + "return" switches on an IC7xxx connected via CIV-bus 

Pressing "a" + "return" switches off an IC7xxx connected via CIV-bus 

Pressing "v" + "return" sets a IC7xxx connected via CIV-bus to "Voice" mode

Pressing "d" + "return" sets a IC7xxx connected via CIV-bus to "Data" mode

Pressing "f" + "return" gets the frequency of an IC7xxx connected via CIV-bus
(Note: if this command returns "0", the internal frequency of instance IC7xxx
 hasn't been updated. This can be changed by turning the main tuning knob 
 on the radio or by changing bands)

Pressing "l" + "return" gives a readout of the CIV logbuffer

In order to get the best information from the test (CIV logbuffer),
please activate(uncomment) "#define log_CIV" in file civ.h
in the library CIVbuslib first !
This is deactivated by default due to saving data memory reasons.

*/

/* includes -----------------------------------------------------------------*/

#include "a_defines.h"

#include <ICradio.h> // CIVcmds.h and CIVmaster.h are automatically included in addition

//-------------------------------------------------------------------------------
// create the civ and ICradio objects in use

CIV     civ;  // create the CIV-Interface object first (mandatory for the use of ICradio)

// create two ICradio objects for the two radios connected
ICradio IC7300(TypeIC7300,CIV_ADDR_7300);
ICradio IC9700(TypeIC9700,CIV_ADDR_9700);

//-------------------------------------------------------------------------------

uint8_t lpCnt = 0;
CIVresult_t CIVresultL;

radioOnOff_t radioState = RADIO_OFF;

// select the radio, which you want to control
bool  IC7300sel = true;

radioOnOff_t    radioS7     = RADIO_NDEF;
radioOnOff_t    localS7     = RADIO_NDEF;
unsigned long   radioF7     = 0;
unsigned long   localF7     = 0;
uint8_t         radioM7     = MOD_NDEF;
uint8_t         localM7     = MOD_NDEF;
uint8_t         radioFil7   = FIL_NDEF;
uint8_t         localFil7   = FIL_NDEF;

radioOnOff_t    radioS9     = RADIO_NDEF;
radioOnOff_t    localS9     = RADIO_NDEF;
unsigned long   radioF9     = 0;
unsigned long   localF9     = 0;
uint8_t         radioM9     = MOD_NDEF;
uint8_t         localM9     = MOD_NDEF;
uint8_t         radioFil9   = FIL_NDEF;
uint8_t         localFil9   = FIL_NDEF;

unsigned long time_current_baseloop;
unsigned long time_last_baseloop;

#ifdef debug
  unsigned long G_timemarker1;
  unsigned long G_timemarker1a;
#endif

//---------------------------------------------------------------------------------------------

void  printHelp() {

  Serial.println ();
  Serial.println ("The following info is shown automatically if changed:");
  Serial.println ("On/Off State, Frequency, Modulation mode, RX Filter");
  Serial.println ("");
  Serial.println (" r -> switch between radio1(IC7300) and radio2(IC9700)");
  Serial.println ();
  Serial.println (" e -> switch the radio on");
  Serial.println (" a -> switch the radio off");
  Serial.println (" v -> set radio to voice mode");
  Serial.println (" d -> set radio to data mode");
  Serial.println (" g -> get the general radio mode");
  Serial.println (" f -> get the frequency of the radio");
  Serial.println (" m -> get the Modulation mode and RX filter setting of the radio");
  Serial.println (" l -> get the log (only if logging is switched on in file civmaster.h)");
  Serial.println (" h -> show this help screen");
  Serial.println ();
  if (IC7300sel)  Serial.println ("Radio1 (IC7300) selected!");
  else            Serial.println ("Radio2 (IC9700) selected!");
  Serial.println ();

}

//---------------------------------------------------------------------------------------------
// check and get the status of the keys (+ simulated keys) for different testcases

keyPressed_t get_key() {

  keyPressed_t  ret_val = NO_KEY_PRESSED;
  uint8_t       inByte  = 0;
  
  if (Serial.available()>0)  inByte = Serial.read();

  if (inByte=='r') ret_val = KEY_SWRADIO_PRESSED;

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
// different test cases

// test of radio on/off and radio DC power state
void testRadioOnOff (uint8_t key) {

  if (key==KEY_EIN_PRESSED) { // ON
    key = NO_KEY_PRESSED;
    civ.logClear();
    Serial.println (" KEY_EIN*");
    SET_TIME_MARKER1;   // just for test: measure the time in [us], the setDCPower command takes ...
    if (IC7300sel)
      radioState = IC7300.setDCPower(RADIO_ON,time_current_baseloop);
    else
      radioState = IC9700.setDCPower(RADIO_ON,time_current_baseloop);
    EVAL_TIME_MARKER1;
    if (IC7300sel)
      radioState = IC7300.getAvailability(); // -> returns RADIO_OFF (OFF->OFF)or RADIO_ON_TR (ON->OFF)
    else
      radioState = IC9700.getAvailability(); // -> returns RADIO_OFF (OFF->OFF)or RADIO_ON_TR (ON->OFF)
    Serial.println (radioOnOffStr[radioState]);
  }

  if (key==KEY_AUS_PRESSED) { // OFF
    key = NO_KEY_PRESSED;
    civ.logClear();
    Serial.println (" KEY_AUS*");
    SET_TIME_MARKER1;
    if (IC7300sel)
      radioState = IC7300.setDCPower(RADIO_OFF,time_current_baseloop);
    else
      radioState = IC9700.setDCPower(RADIO_OFF,time_current_baseloop);
    EVAL_TIME_MARKER1;
    if (IC7300sel)
      radioState = IC7300.getAvailability(); // -> returns RADIO_OFF (OFF->OFF)or RADIO_ON_TR (ON->OFF)
    else
      radioState = IC9700.getAvailability(); // -> returns RADIO_OFF (OFF->OFF)or RADIO_ON_TR (ON->OFF)
    Serial.println (radioOnOffStr[radioState]);
  }    

}

//-----------------------------------------------------------------
// test of switching the general modes ( "Voice" and "Data" )
// use: press the "v" or "d" key on your computer keyboard
// if the logging is enabled, you can see the commands by pressing "l"

// !!! Problems with IC705 / SW Version 1.26:
// !!! I have noticed, that after using "v" or "d", it takes a while (up to 1 1/2min(!), until  !!!
// !!! the command "m" is giving the correct result.                                            !!!
// !!! The basic function, however, is also OK with IC705 ...                                   !!!
// !!! Reason must be in the radio itself, since these commands work perfectly                  !!!
// !!! on IC7300 and IC9700.                                                                    !!!

void mode_sequences(uint8_t key) {

  if (key==KEY_VOICE_PRESSED) {
    key = NO_KEY_PRESSED;
    Serial.println (" KEY_VOICE*");
    civ.logClear();
    if (IC7300sel)
      IC7300.setMode(MODE_VOICE);
    else
      IC9700.setMode(MODE_VOICE);
    Serial.println ("VOICE-MODE");
  }

  if (key==KEY_DATA_PRESSED) {
    key = NO_KEY_PRESSED;
    Serial.println (" KEY_DATA*");
    civ.logClear();
    if (IC7300sel)
      IC7300.setMode(MODE_DATA);
    else
      IC9700.setMode(MODE_DATA);
    Serial.println ("DATA-MODE");
  }

}

//---------------------------------------------------------------------------------------------
// get the radio data and print them via the USB COM-port into the serial monitor

void  getradioInfo() {


    radioS7 = IC7300.getAvailability();
    if (radioS7 != localS7) {               // if there is a change in ON/OFF state -> print it out!
      Serial.print ("7300 State: "); 
      Serial.println (radioOnOffStr[radioS7]);
      localS7 = radioS7;
    }
    radioS9 = IC9700.getAvailability();
    if (radioS9 != localS9) {               // if there is a change in ON/OFF state -> print it out!
      Serial.print ("9700 State: "); 
      Serial.println (radioOnOffStr[radioS9]);
      localS9 = radioS9;
    }

    if (radioS7==RADIO_ON) {              // only in case, the radio is switched on 
                                          // and the connection is up and running
      radioF7 = IC7300.getFrequency();
      if (localF7 != radioF7) {             // if there is a frequency change -> print it out!
        Serial.print    ("7300 Freq[Hz]: "); Serial.println  (radioF7);
        localF7 = radioF7;
      }
      radioM7    = IC7300.getModMode();
      radioFil7  = IC7300.getRxFilter();
      if ((localM7 != radioM7) || (localFil7 != radioFil7)) { // if there is a change -> print it out!
        Serial.print ("7300 Mod:  "); Serial.print   (modModeStr[radioM7]);
        Serial.print (" Fil: "); Serial.println (FilStr[radioFil7]);
        localM7 = radioM7; localFil7 = radioFil7;
      }
    }

    if (radioS9==RADIO_ON) {              // only in case, the radio is switched on 
                                          // and the connection is up and running
      radioF9 = IC9700.getFrequency();
      if (localF9 != radioF9) {             // if there is a frequency change -> print it out!
        Serial.print    ("9700 Freq[Hz]: "); Serial.println  (radioF9);
        localF9 = radioF9;
      }
      radioM9    = IC9700.getModMode();
      radioFil9  = IC9700.getRxFilter();
      if ((localM9 != radioM9) || (localFil9 != radioFil9)) { // if there is a change -> print it out!
        Serial.print ("9700 Mod:  "); Serial.print   (modModeStr[radioM9]);
        Serial.print (" Fil: "); Serial.println (FilStr[radioFil9]);
        localM9 = radioM9; localFil9 = radioFil9;
      }
    }

}



//==========  General initialization  of  the device  =========================================
void setup() {

  // initialize the serial interface
  Serial.begin(115200);
  delay(100);
  Serial.println("");
  Serial.println (VERSION_STRING);

  printHelp();

  civ.setupp();                       // initialize the civ object/module (serves both radios connected)

  IC7300.setupp(millis());            // initialize the ICradio class of radio 1
  IC9700.setupp(millis());            // initialize the ICradio class of radio 2

  time_current_baseloop = millis();
  time_last_baseloop = time_current_baseloop;
  
}

//============================  main  procedure ===============================================
void loop() {

  keyPressed_t keyCmd;

  time_current_baseloop = millis();
  
  if ((time_current_baseloop - time_last_baseloop) > BASELOOP_TICK) {

//---------------------------------------------------------------------------------------------
// calling the loop function of each radio in use as often as possible (i.e. approx. 20ms in this case) !

    // every time after end of the "bootup phase" (==t_RadioCheck) :
    if (time_current_baseloop>t_RadioCheck) {
      if ((lpCnt%2) == 0) IC7300.loopp(time_current_baseloop);   // check IC7300
      // time difference:  2 * BASELOOP_TICK
      if ((lpCnt%2) == 1) IC9700.loopp(time_current_baseloop);   // check IC9700
    }

    keyCmd = get_key();  // get command input


//---------------------------------------------------------------------------------------------
// different test cases

    getradioInfo();

    if (keyCmd==KEY_SWRADIO_PRESSED) {  // use "r"
      keyCmd=NO_KEY_PRESSED;
      IC7300sel = !IC7300sel;
      if (IC7300sel)  Serial.println ("Radio1 (IC7300) selected!");
      else            Serial.println ("Radio2 (IC9700) selected!");
    }

    testRadioOnOff (keyCmd);  // use "e", "a" or "t"

    mode_sequences (keyCmd);  // use "v" or "d"

    if (keyCmd==KEY_MODE_PRESSED) {      // use "g"
      if (IC7300sel) { Serial.print("7300 Mode: "); Serial.println (ModeStr[IC7300.getMode()]);}
      else           { Serial.print("9700 Mode: "); Serial.println (ModeStr[IC9700.getMode()]);}
    }

    if (keyCmd==KEY_FREQ_PRESSED) {
      keyCmd=NO_KEY_PRESSED;
      if (IC7300sel) { Serial.print("7300 Freq: "); Serial.println (IC7300.getFrequency());}
      else           { Serial.print("9700 Freq: "); Serial.println (IC9700.getFrequency());}
    }

    if (keyCmd==KEY_MODMODE_PRESSED) {
      keyCmd=NO_KEY_PRESSED;
      if (IC7300sel)  Serial.print("7300 Mod: "); 
      else            Serial.print("9700 Mod: "); 
      if (IC7300sel)  Serial.print (modModeStr[IC7300.getModMode()]);
      else            Serial.print (modModeStr[IC9700.getModMode()]);
      Serial.print(" Filter: "); 
      if (IC7300sel)  Serial.println (FilStr[IC7300.getRxFilter()]);
      else            Serial.println (FilStr[IC9700.getRxFilter()]);
    }

    if (keyCmd==KEY_HELP_PRESSED) {      // use "h"
      printHelp();
    }

    // use "l",if "#define log_CIV" in file civ.h is active
    if (keyCmd==KEY_LOG_PRESSED) {
      keyCmd=NO_KEY_PRESSED;
      civ.logDisplay();
    }

//---------------------------------------------------------------------------------------------
    lpCnt++;
    time_last_baseloop = time_current_baseloop;
	} // if BASELOOP_TICK
  
} // end loop
