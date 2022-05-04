/* 
IC705toPA  - W.Dilling/DK8RW

This device is based on a ESP32 D1 board and shall connect to a standalone IC705 via BT in order 
to get the current frequency from the radio.
This frequency info can be used in multiple ways, e.g. control of an external PA (lowpass filter settings)

Tasks: 
- get the current frequency of the radio and send it via 4 HW-lines (binary coded) to the PA


Notes: 
- Please make sure, that in the menu of IC705 the "Transceive" function is set to ON ... otherwise it
  simply doesn't work.

- for a first check it is worth watching the output of the program in Serial Monitor.
  If everything is fine, you can suppress this output by setting the "debug" switch in
  comments (file a_defines.h). This switch is uncommented by default for obvious reasons ;-)

- if you are using external inverting drivers, pls. make sure that the switch "invDriver"
  is uncommented in a_defines.h

- pls check (and correct, if necessary) the HW Pin numbers in use (file a_defines.h)

- This example has been successfully tested with a PA HLVA 1k3Q from RFpower by Rainer, DK1RS

*/

/* includes -----------------------------------------------------------------*/

#include "a_defines.h"

#include <CIVmaster.h>  // CIVcmds.h is automatically included in addition

//-------------------------------------------------------------------------------
// create the civ and ICradio objects in use

CIV     civ;    // create the CIV-Interface object
                // ESP -> IC705 (via Bluetooth)

//-------------------------------------------------------------------------------

#define BASELOOP_TICK 10 

#define NUM_BANDS 11   /* Number of Bands (depending on the radio) */

//-------------------------------------------------------------------------------

uint16_t lpCnt = 0;
CIVresult_t CIVresultL;

uint8_t currentBand = NUM_BANDS;  // not defined
uint8_t currentBCDsetting = 0xff; // 0xff == undefined

bool    freqReceived = false; // initially, no frequency info has been received from the radio
uint8_t freqPoll     = 0;     // number of initial frequency querys in addtion to the broadcast info

/* module wide variables ----------------------------------------------------*/

/* timer  variables */
unsigned long time_current_baseloop;       // temporary time of the baseloop entry for calculations
unsigned long time_last_baseloop;          // will be updated at the end of every baseloop run

// for Debugging ...
#ifdef debug
  unsigned long G_timemarker1;
  unsigned long G_timemarker1a;

  unsigned long G_timemarker2;
  unsigned long G_timemarker2a;
#endif


//-----------------------------------------------------------------------------------------
// tables for band selection and bittpattern calculation

// lower limits[kHz] of the bands:
constexpr unsigned long lowlimits[NUM_BANDS] = {
  1791, 3491, 5291, 6991,  9991, 13991, 18051, 20991, 24881, 27991, 49991
};
// upper limits[kHz] of the bands:
constexpr unsigned long uplimits[NUM_BANDS] = {
  2100, 4000, 5400, 7500, 10200, 14500, 18200, 21600, 25000, 29800, 54100
};

constexpr uint8_t band2BCD [NUM_BANDS+1] = { 
// 160    80    60    40     30     20     17     15     12     10      6  NDEF
  0x01, 0x02, 0x03, 0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,  0x0A, 0x00
};

//------------------------------------------------------------
// set the bitpattern in the HW

void set_HW (uint8_t BCDsetting) {

  digitalWrite  (P_BCD0, ( BCDsetting     & 0b00000001));
  digitalWrite  (P_BCD1, ((BCDsetting>>1) & 0b00000001));
  digitalWrite  (P_BCD2, ((BCDsetting>>2) & 0b00000001));
  digitalWrite  (P_BCD3, ((BCDsetting>>3) & 0b00000001));

#ifdef debug
  // Test output to control the proper functioning:
  Serial.print ("Pins ");
  Serial.print (P_BCD3); Serial.print (' '); Serial.print (P_BCD2); Serial.print (' ');
  Serial.print (P_BCD1); Serial.print (' '); Serial.print (P_BCD0); Serial.print (" : "); 
  Serial.print (((BCDsetting>>3) & 0b00000001),BIN);
  Serial.print (' ');
  Serial.print (((BCDsetting>>2) & 0b00000001),BIN);
  Serial.print (' ');
  Serial.print (((BCDsetting>>1) & 0b00000001),BIN);
  Serial.print (' ');
  Serial.print (( BCDsetting     & 0b00000001),BIN);
  Serial.println (' ');
#endif

}

//-----------------------------------------------------------------------------------------
// get the bandnumber matching to the frequency (in kHz)

byte get_Band(unsigned long frq){
  byte i;
  for (i=0; i<NUM_BANDS; i++) {
    if ((frq >= lowlimits[i]) && (frq <= uplimits[i])){
      return i;
    }
  }
  return NUM_BANDS; // no valid band found -> return not defined
}

//------------------------------------------------------------
// process the frequency received from the radio

void set_PAbands(unsigned long frequency) {
  unsigned long freq_kHz;

  freq_kHz = frequency/1000;              // frequency is now in kHz
  currentBand = get_Band(freq_kHz);       // get band according the current frequency

#ifdef debug
  // Test-output to serial monitor:
  Serial.print("Frequency: ");  Serial.print(freq_kHz);
  Serial.print("  Band: ");     Serial.print(currentBand);  
  Serial.print("  BCD: ");      Serial.println(band2BCD[currentBand],BIN);  
#endif

  // load the bitpattern into the HW:
  // currentBand : 0 ... NUM_BANDS
  // BCD :         0 ... 0b00001010 ( == 0x0A )
  
  // "~" inverts the bitpattern!  (0 -> 1 ; 1 -> 0)
  // this can be used to compensate the effect of inverting HW buffers

#ifdef invDriver 
  set_HW ( ~ band2BCD[currentBand] );
#else
  set_HW (   band2BCD[currentBand] );
#endif

}

//==========  General initialization  of  the device  =========================================

void setup() {

  #ifdef debug                        // initialize the serial interface (for debug messages)
    Serial.begin(debugBdRate);
    Serial.println("");
    delay(20);
    Serial.println (VERSION_STRING);
  #endif

  civ.setupp(true);                   // initialize the civ object/module (true means "use BT")
  civ.registerAddr(CIV_ADDR_705);     // tell civ, that this is a valid address to be used

  // set the used HW pins (see defines.h!) as output and set it to 0V (at the Input of the PA!!) initially
  pinMode       (P_BCD0, OUTPUT);
  pinMode       (P_BCD1, OUTPUT);
  pinMode       (P_BCD2, OUTPUT);
  pinMode       (P_BCD3, OUTPUT);

#ifdef invDriver 
  digitalWrite  (P_BCD0, HIGH);
  digitalWrite  (P_BCD1, HIGH);
  digitalWrite  (P_BCD2, HIGH);
  digitalWrite  (P_BCD3, HIGH);
#else
  digitalWrite  (P_BCD0, LOW);
  digitalWrite  (P_BCD1, LOW);
  digitalWrite  (P_BCD2, LOW);
  digitalWrite  (P_BCD3, LOW);
#endif

  time_current_baseloop = millis();
  time_last_baseloop = time_current_baseloop;
  
}

//============================  main  procedure ===============================================

void loop() {

  time_current_baseloop = millis();
  
  if ((time_current_baseloop - time_last_baseloop) > BASELOOP_TICK) {


    // ----------------------------------  check, whether there is something new from the radio
    CIVresultL = civ.readMsg(CIV_ADDR_705);
    if (CIVresultL.retVal<=CIV_NOK) {               // valid answer received !
      #ifdef debug
        Serial.print('.');
      #endif
      if (CIVresultL.retVal==CIV_OK_DAV) {          // Data available

        if ((CIVresultL.cmd[1]==CIV_C_F_SEND[1]) ||  // command CIV_C_F_SEND received
            (CIVresultL.cmd[1]==CIV_C_F_READ[1])) {  // command CIV_C_F_READ received

          freqReceived = true;

          // send the band info to the PA:
          set_PAbands(CIVresultL.value);

        } // command CIV_C_F_SEND or CIV_C_F_READ received  

      } // Data available
    } // valid answer received


    // ----------------------------------  do a query for frequency, if necessary
    // poll every 500 * 10ms = 5sec until a valid frequency has been received

    if ( (freqReceived == false) && ((lpCnt%500)==0) ) { 
      civ.writeMsg (CIV_ADDR_705, CIV_C_F_READ, CIV_D_NIX, CIV_wChk);
      freqPoll++;
      Serial.print ("P ");
    }

    
    lpCnt++;
    time_last_baseloop = time_current_baseloop;
	} // if BASELOOP_TICK
  
} // end loop
