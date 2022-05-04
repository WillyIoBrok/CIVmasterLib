/* 
ICradio_IC705_SimpleBTtest -  W.Dilling/DK8RW

This device is based on a ESP32 D1 board and shall connect to a standalone IC705 via BT

Tasks: 
- get the current on/off state and the frequency of the radio

*/

/* includes -----------------------------------------------------------------*/

#include "a_defines.h"

#include <ICradio.h> // CIVcmds.h and CIVmaster.h are automatically included in addition

//-------------------------------------------------------------------------------
// create the civ and ICradio objects in use

CIV     civ;  // create the CIV-Interface object first (mandatory for the use of ICradio)

ICradio IC705(TypeIC705,CIV_ADDR_705);

//-------------------------------------------------------------------------------

uint16_t      lpCnt = 0;
CIVresult_t   CIVresultL;

unsigned long G_CAT_freq        = 0;
byte          G_curr_sel_band_O = NUM_BANDS; 
byte          G_IC705_state_O   = RADIO_NDEF;

const char    *DISPLAY_ON_OFF[6] = {
"OFF",
"ON ",
"OFF TR",     /* Transit up */
"ON TR",      /* Transit dn */
"ND",
"TG"
};

// timer  variables ...
unsigned long time_current_baseloop;       /* temporary time of the baseloop entry for calculations */
unsigned long time_last_baseloop;          /* wird geschrieben, wenn die Baseloop wieder arbeitet */

// Debugging ...
#ifdef debug
  unsigned long G_timemarker1;
  unsigned long G_timemarker1a;
#endif

//-----------------------------------------------------------------------------------------

constexpr unsigned long lowlimits[NUM_BANDS] = {
    30,1801,2001,3491,3811,5241,5461,6990, 7311,10091,10161,\
  13991,14361,18059,18179,20991,21461,24881,25001,27991,\
  30001,49991,54011,69901
};
constexpr unsigned long uplimits[NUM_BANDS] = {
  1800,2000,3490,3810,5240,5460,6990,7310,10090,10160,13990,\
  14360,18058,18178,20990,21460,24880,25000,27990,30000,\
  49990,54010,69900,74800
};

//-----------------------------------------------------------------------------------------

byte get_Band(unsigned long frq){
  byte i;
  for (i=0; i<NUM_BANDS; i++) {
    if ((frq >= lowlimits[i]) && (frq <= uplimits[i])){
      return i;
    }
  }
  return NUM_BANDS+1; /* no valid band found */
}

//------------------------------------------------------------
void set_KW_Bands() {

  unsigned long CAT_freq;
  
  // get frequency from radio (IC705)
  CAT_freq = IC705.getFrequency()/1000; // frequ in kHz

  if (G_CAT_freq!=CAT_freq) {
    G_CAT_freq = CAT_freq;
    Serial.print("f[kHz]: "); Serial.println(CAT_freq);

    if ((CAT_freq >= lowlimits[0]) && (CAT_freq <= uplimits[NUM_BANDS-1])) {  /* valid qrg available */
      G_curr_sel_band_O = get_Band(CAT_freq);       /* get band according the current frequency */
      Serial.print("Band: "); Serial.println(G_curr_sel_band_O);
    }
  }
}


//==========  General initialization  of  the device  =========================================
void setup() {

                        // initialize the serial interface (for debug messages)
  Serial.begin(115200);
  Serial.println("");
  delay(20);
  Serial.println (VERSION_STRING);

  civ.setupp(true);                   // initialize the civ object/module (true means "use BT")
                                      // and the ICradio objects

	IC705.setupp(millis());            // registering of the CIV-address in civ is done in ICradio

  pinMode(P_INT_LED,OUTPUT);
  digitalWrite(P_INT_LED,LOW);  

  pinMode(P_CONN_RADIO_LED,OUTPUT);
  digitalWrite(P_CONN_RADIO_LED,HIGH);  // the "radio connected" LED will be controlled by Radio on/off

  time_current_baseloop = millis();
  time_last_baseloop = time_current_baseloop;
  
}

//============================  main  procedure ===============================================
void loop() {

  byte L_IC705_state;

  time_current_baseloop = millis();
  
  if ((time_current_baseloop - time_last_baseloop) > BASELOOP_TICK) {

    // loop after t_RadioCheck ms (completion of the bootup-phase of the controller)
    if (time_current_baseloop>t_RadioCheck) IC705.loopp(time_current_baseloop);

    // is radio on or off? -> update database
    L_IC705_state = IC705.getAvailability();
    if (G_IC705_state_O!=L_IC705_state) {
      G_IC705_state_O=L_IC705_state;

      if (G_IC705_state_O==RADIO_ON) {
        digitalWrite(P_CONN_RADIO_LED,LOW);   // LED on
      }
      else {
        digitalWrite(P_CONN_RADIO_LED,HIGH);  // LED off
      }
      Serial.print("Radio is: "); Serial.println(DISPLAY_ON_OFF[G_IC705_state_O]);      
    }

    // set the chosen SW-band, if the radio is ON
    if (G_IC705_state_O==RADIO_ON) set_KW_Bands();   //---- RADIO_ON
                                                     // -> setze das KW-Band
    
    lpCnt++;
    time_last_baseloop = time_current_baseloop;
	} // if BASELOOP_TICK
  
} // end loop
