/* 
CIVmasterlib CIVTest - W.Dilling/DK8RW

A very simple program to show the principles of using CIV ...

Entering "f" + "CR" on the keyboard starts a query for frequency 
of a IC7300 which is connected via CIV-bus and prints out some status info.

Tuning the frequency on the IC7300 gives a printout as well

Entering "l" + "CR" gives a readout of the CIV logbuffer

In order to get the best information from the test (CIV-logbuffer),
please activate(uncomment) "#define log_CIV" in file CIVmaster.h
in the library CIVbuslib first !
This is switched off by default due to saving data memory reasons

*/

/* includes -----------------------------------------------------------------*/

#include <CIVcmds.h>

#include <CIVmaster.h> // CIVcmds.h is automatically included in addition

// Global compile switches ===================================================================================

#define VERSION_STRING "CIVmasterLib CIV_IC7300_Test V0_7 22/05/04"

//-------------------------------------------------------------------------------
// create the civ object

  CIV     civ;  // create the CIV-Interface object

//-------------------------------------------------------------------------------

uint8_t lpCnt = 0;
CIVresult_t CIVresultL;

#define BASELOOP_TICK 10 

unsigned long time_current_baseloop;
unsigned long time_last_baseloop;

// some general defines ----------------------------------

enum onOff_t:uint8_t {
  OFF = 0,
  ON  = 1,
  UNDEF
};

enum keyPressed_t:uint8_t {
  NO_KEY_PRESSED    = 0,
  KEY_FREQ_PRESSED,
  KEY_LOG_PRESSED,
};

const String retValStr[7] = {
  "CIV_OK",
  "CIV_OK_DAV",

  "CIV_NOK",
  "CIV_HW_FAULT",
  "CIV_BUS_BUSY",
  "CIV_BUS_CONFLICT",
  "CIV_NO_MSG"
};

//---------------------------------------------------------------------------------------------
// check and get the status of the keys (+ simulated keys from the keyboard) 
// for different testcases

keyPressed_t get_key() {

  keyPressed_t ret_val = NO_KEY_PRESSED;
  uint8_t inByte = 0;

  if (Serial.available()>0)  inByte = Serial.read();

  if (inByte=='f') ret_val = KEY_FREQ_PRESSED;
  if (inByte=='l') ret_val = KEY_LOG_PRESSED;
    
  return ret_val;
}

//==========  General initialization  of  the device  =========================================
void setup() {

  civ.setupp();                       // initialize the civ object/module
                                      // and the ICradio objects

  civ.registerAddr(CIV_ADDR_7300);    // tell civ, that this is a valid address to be used

  Serial.begin(115200);
  delay(100);
  Serial.println("");
  Serial.println (VERSION_STRING);

  time_current_baseloop = millis();
  time_last_baseloop = time_current_baseloop;
  
}

//============================  main  procedure ===============================================
void loop() {

  keyPressed_t keyCmd;

  time_current_baseloop = millis();
  
  if ((time_current_baseloop - time_last_baseloop) > BASELOOP_TICK) {

//---------------------------------------------------------------------------------------------
// different test cases

    keyCmd = get_key();

    if (keyCmd==KEY_FREQ_PRESSED) {
      CIVresultL = civ.writeMsg (CIV_ADDR_7300, CIV_C_F_READ, CIV_D_NIX, CIV_wChk);
      Serial.print("retVal of writeMsg: "); Serial.println(retValStr[CIVresultL.retVal]);
/*
      //give the radio some time to answer version 1:
      delay(20);
      CIVresultL = civ.readMsg(CIV_ADDR_7300);
      Serial.print("retVal: ");      Serial.print(CIVresultL.retVal);
      Serial.print(" Frequency: "); Serial.println(CIVresultL.value);
*/
    }

    // give the radio some time to answer - version 2:  
    // do a cyclic polling until data is available
    CIVresultL = civ.readMsg(CIV_ADDR_7300);
    if (CIVresultL.retVal<=CIV_NOK) {  // valid answer received !
      Serial.print("retVal: ");      Serial.print(retValStr[CIVresultL.retVal]);
      if (CIVresultL.retVal==CIV_OK_DAV) // Data available
        Serial.print(" Frequency: "); Serial.println(CIVresultL.value);
    }

    // use "l",if "#define log_CIV" in file civ.h is active
    if (keyCmd==KEY_LOG_PRESSED) civ.logDisplay();

//---------------------------------------------------------------------------------------------
    lpCnt++;
    time_last_baseloop = time_current_baseloop;
	} // if BASELOOP_TICK
  
} // end loop
