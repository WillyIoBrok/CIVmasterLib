/* 
	ICradio.cpp - Library for communication via ICOM's CI-V bus
	Created by Wilfried Dilling, DK8RW, May 04, 2022
	Released into the public domain

	Control layer for ICOM's radios
*/


#include <Arduino.h>

#include "CIVcmds.h"
#include "CIVmaster.h"
#include "ICradio.h"


extern CIV civ;

// definition of cyclic query modes
#define noQuery				0
#define	query_mod 		1
#define	query_1_mod 	2
#define	query_f_mod 	3
#define	query_3_f_mod 6

//ctor = constructor
	ICradio::ICradio(radioType_t thisRadio, uint8_t myCIVaddr) :
	_radioType(thisRadio),_radioAddr(myCIVaddr),_radioMode(MODE_NDEF),_radioOnOffState(RADIO_NDEF),
	_waitForAnswer(false),_waitForIDquery(false),_DateTimeSent(false),_fModQuery(noQuery),_frequency(0),
	_modMode(MOD_NDEF),_modFilter(FIL_NDEF),_sequMode(MODE_NDEF)

	{
	}

//------------------------------------------------------------------------
// public member functions

	//::::::::::::: initialisation of the class ICradio
	void ICradio::setupp(unsigned long currentTime) {

    _ts_lastIDquery  = currentTime;
    _ts_waitForAnswer   = currentTime;
    _ts_lastOnCmd       = currentTime;

		civ.registerAddr(_radioAddr);

	}

  //::::::::::::: get the CIV-answers from the radio
  CIVresult_t ICradio::getNewMsg() {
		CIVresult_t radioMsg;
		
		// -----------------------------------------------------------------------------------
		// get an answer from the radio, do the low level processing and store it into radioMsg
		radioMsg = civ.readMsg(_radioAddr);
//		if (radioMsg.retVal <= CIV_NOK) {
//    	Serial.print (_radioType);Serial.print("  -  ");Serial.println (radioMsg.retVal);    
//		}


		// -----------------------------------------------------------------------------------
		// evaluate the received message from the radio
		if (radioMsg.retVal==CIV_OK)  {
			_waitForAnswer = false;
		}

		// ...................................................................................
    if (radioMsg.retVal==CIV_NOK) {
			_waitForAnswer = false;
      _sequNoOfNOKs++; 
      if (_waitForIDquery == true) { // if IC9700 is off, it answers with NOK to the query command
        _radioOnOffState = RADIO_OFF;
        _waitForIDquery = false;
      }
    }

    if (radioMsg.retVal==CIV_OK_DAV) {           // data for evaluation available
			_waitForAnswer = false;

			// ...................................................................................
      if ((radioMsg.cmd[1]==CIV_C_F_SEND[1]) || // frequency broadcast from radio received
          (radioMsg.cmd[1]==CIV_C_F_READ[1]))		// frequency query answered
        {
          _frequency = radioMsg.value;
					// Serial.println (_frequency);
        }
			// ...................................................................................
      if ((radioMsg.cmd[1]==CIV_C_MOD_SEND[1]) || // ModMode broadcast from radio received
          (radioMsg.cmd[1]==CIV_C_MOD_READ[1]))		// ModMode  query answered
        {
					if (radioMsg.datafield[1] == 0x17) 			// DV is coded in BCD according to ICOM
						_modMode = MOD_DV;
					else
						_modMode = radioModMode_t(radioMsg.datafield[1]);
					if (_modMode > MOD_NDEF) _modMode = MOD_NDEF;
					
					if (radioMsg.datafield[0]==2)						// Filter info has been sent as well
						_modFilter = radioFilter_t(radioMsg.datafield[2]);					
					if (_modFilter > FIL3) _modFilter = FIL_NDEF;
					
        }
			// ...................................................................................
      if ((radioMsg.cmd[1]==CIV_C_TRX_ID[1]) && 	// radio id received
          (radioMsg.cmd[2]==CIV_C_TRX_ID[2])) {
        _waitForIDquery = false;
        if (_radioOnOffState!=RADIO_ON) {					// change from "Radio OFF" to "Radio ON"
					_radioOnOffState = RADIO_ON;						// Radio is ON !

					_fModQuery = query_f_mod;								// initiate a query for frequency and modulation mode
																									// at power up of the radio
					if (_radioType==TypeIC7300)	 						// special treatment of IC7300
						setDateTime();												// set the clock after switching on
					
        }
      }
    }

		return radioMsg; 	// return the value just in case there is an answer, which is NOT handled by ICradio

  }

  //::::::::::::: this method will be called from the CIV loop
	CIVresult_t ICradio::loopp(unsigned long currentTime) {
		CIVresult_t radioMsg;

		// -----------------------------------------------------------------------------------
		// send a query for frequency and ModMode/RX-Filter to the Radio if requested
    if (_fModQuery>noQuery) {
			if (_fModQuery==query_f_mod) {
				civ.writeMsg(_radioAddr,CIV_C_F_READ,CIV_D_NIX,CIV_wChk);		// ask for the frequency
			}
			// delay of two loopticks (20ms) because of slow IC9700 reaction!

			if (_fModQuery==query_mod) {
				civ.writeMsg(_radioAddr,CIV_C_MOD_READ,CIV_D_NIX,CIV_wChk);	// ask for the ModMode and Filterinfo
			}
			_fModQuery--;
		}

		// -----------------------------------------------------------------------------------
		// get and process messages / answers from the readio
				radioMsg = getNewMsg();

		// -----------------------------------------------------------------------------------
		// check ON/OFF state timeout processing
    if (_waitForIDquery == true) {                               // still waiting for an ID query-answer 
      if ((currentTime - _ts_lastIDquery) > t_waitForAnswer) {   // no answer from radio !
        if ((currentTime - _ts_lastOnCmd) < t_radio_OFF_TR[_radioType])	_radioOnOffState = RADIO_OFF_TR;   // it's radio boot time
        else                                                						_radioOnOffState = RADIO_OFF;
				_waitForIDquery = false;
      }
			// Serial.print("waitForIDquery  "); Serial.println(_radioType);
    }

		// -----------------------------------------------------------------------------------
		// cyclic check for the availability of the radio
    if (((currentTime-_ts_lastIDquery)>t_RadioCheck) && (_sequMode==MODE_NDEF)) {     // it's time to send an ID query command to the radio
      civ.writeMsg(_radioAddr,CIV_C_TRX_ID,CIV_D_NIX,CIV_wFast);
      _waitForIDquery = true;
      _ts_lastIDquery = currentTime;
			// Serial.print("sendIDquery  "); Serial.print(_radioType); Serial.print(" * "); Serial.println(_radioOnOffState,HEX);

			_fModQuery = query_3_f_mod;	// in addition, trigger the cyclic query for frequency and modMode 2 loopticks later

    }

		// -----------------------------------------------------------------------------------
		// handle the sequences ...
    if (_sequMode!=MODE_NDEF) {                                      // set mode sequence requested

      if ((_sequCmdIdx<_sequNoOfCmds) && (_waitForAnswer==false)) {  // new command after ack to be sent
 				civ.writeMsg(_radioAddr,_sequPntr,CIV_D_NIX,CIV_wFast);
				_waitForAnswer = true;
				_sequPntr=_sequPntr + SEQU_MAX_CMD_LENGTH;		// next command in sequence
        _sequCmdIdx++;
      }

      if (_sequNoOfNOKs>0) { // in case of command not accepted: break immediately!
        _radioMode = MODE_NDEF;_sequMode=MODE_NDEF;
        #ifdef log_CIV
          Serial.print(_sequNoOfNOKs);Serial.print(" Idx: ");Serial.println(_sequCmdIdx);
        #endif
      };
      if (_sequCmdIdx>=_sequNoOfCmds) {               // sequence finished
        _radioMode = _sequMode;_sequMode=MODE_NDEF; 

			_fModQuery = query_1_mod;	// trigger the query for modMode after Mode change with delay of 1 looptick

			// civ.logDisplay();
      }

    }

		return radioMsg; 	// return the value just in case there is an answer, which is NOT handled by ICradio

	}

  //::::::::::::: check, wether radio is switched on and connected
  radioOnOff_t ICradio::getAvailability() {
    return _radioOnOffState;
	}
	
  //::::::::::::: switch radio ON/OFF
  radioOnOff_t ICradio::setDCPower(radioOnOff_t onOff, unsigned long currentTime) {
    uint8_t task;

    if (onOff==RADIO_TOGGLE) {  // calculate "On" or "Off" depending on state
      if ((_radioOnOffState==RADIO_ON)||(_radioOnOffState==RADIO_ON_TR)) 
        onOff=RADIO_OFF;
      else                      // States: RADIO_OFF, RADIO_OFF_TR, RADIO_NDEF
        onOff=RADIO_ON;
    }

    if (_radioOnOffState==RADIO_ON) task = 0;   // 0: ON  -> ON
    else                            task = 1;   // 1: OFF -> ON
    if (onOff==RADIO_OFF) task = task + 2;      // 2: ON  -> OFF
                                                // 3: OFF -> OFF

    if (task==0) { // ON -> ON
      _radioOnOffState         = RADIO_ON;
    }
    if (task==1) { // OFF -> ON
      // special write sequence according to ICOM manual because of 
      // possible standby of the radio

      civ.writeMsg(_radioAddr,CIV_C_TRX_ON_OFF,CIV_D_ON,CIV_wOn);
      _radioOnOffState         = RADIO_OFF_TR;
      _waitForAnswer      = true;
      _ts_waitForAnswer   = currentTime;
      _ts_lastOnCmd       = currentTime;
    }

    if (task==2) { // ON  -> OFF
      civ.writeMsg(_radioAddr,CIV_C_TRX_ON_OFF,CIV_D_OFF,CIV_wFast);
      _radioOnOffState         = RADIO_ON_TR;
      _waitForAnswer      = true;
      _ts_waitForAnswer   = currentTime;
    }
    if (task==3) { // OFF -> OFF
      _radioOnOffState         = RADIO_OFF;
    }

    return _radioOnOffState;
  }

  //::::::::::::: get radio mode
	radioMode_t ICradio::getMode() {
    return _radioMode;
	}

  //::::::::::::: switch radio mode
	radioMode_t ICradio::setMode(radioMode_t mode) {

    uint8_t task;

    task = _radioType;   // 0 ... 4
    if (mode == MODE_DATA) task = task + TypeICnone+1; // 5 .. 8

    switch (task) {
      case 0: _sequPntr         = &VOICE_MODE_7100[0][0];
              _sequNoOfCmds     = sizeof(VOICE_MODE_7100)/SEQU_MAX_CMD_LENGTH;
      break;
      case 1: _sequPntr         = &VOICE_MODE_7300[0][0];
              _sequNoOfCmds     = sizeof(VOICE_MODE_7300)/SEQU_MAX_CMD_LENGTH;
      break;
      case 2: _sequPntr         = &VOICE_MODE_9700[0][0];
              _sequNoOfCmds     = sizeof(VOICE_MODE_9700)/SEQU_MAX_CMD_LENGTH;
      break;
      case 3: _sequPntr         = &VOICE_MODE_7300[0][0];
              _sequNoOfCmds     = sizeof(VOICE_MODE_7300)/SEQU_MAX_CMD_LENGTH;
      break;
      case 4: // TypeICnone VOICE
      break;
      case 5: _sequPntr         = &DATA_MODE_7100[0][0];
              _sequNoOfCmds     = sizeof(DATA_MODE_7100)/SEQU_MAX_CMD_LENGTH;
      break;
      case 6: _sequPntr         = &DATA_MODE_7300[0][0];
              _sequNoOfCmds     = sizeof(DATA_MODE_7300)/SEQU_MAX_CMD_LENGTH;
      break;
      case 7: _sequPntr         = &DATA_MODE_9700[0][0];
              _sequNoOfCmds     = sizeof(DATA_MODE_9700)/SEQU_MAX_CMD_LENGTH;
      break;
      case 8: _sequPntr         = &DATA_MODE_7300[0][0];
              _sequNoOfCmds     = sizeof(DATA_MODE_7300)/SEQU_MAX_CMD_LENGTH;
      break;
      case 9: // TypeICnone DATA
      break;
    }
    _sequMode   = mode;
    _sequCmdIdx = 0;
    _sequNoOfNOKs = 0;

    
    return MODE_NDEF;
	}

  //::::::::::::: update date_time (load variables of ICradio and send it to the radio once, if possible)
	void ICradio::updateDateTime(uint8_t timeArr[3], uint8_t dateArr[5], uint8_t UTCdeltaArr[4]) {
		for (uint8_t idx=0;idx<3; idx++) {_time[idx]		 = timeArr[idx];}
		for (uint8_t idx=0;idx<5; idx++) {_date[idx]		 = dateArr[idx];}
		for (uint8_t idx=0;idx<4; idx++) {_UTCdelta[idx] = UTCdeltaArr[idx];}

		if ((_DateTimeSent==false) && (_radioOnOffState==RADIO_ON)) {
			setDateTime();
		}

	}

  //::::::::::::: set date_time (send data to radio)
	void ICradio::setDateTime() {
    civ.writeMsg(_radioAddr,CIV_C_UTC,  _UTCdelta,CIV_wChk);
    civ.writeMsg(_radioAddr,CIV_C_TIME, _time,		CIV_wChk);
    civ.writeMsg(_radioAddr,CIV_C_DATE, _date,		CIV_wChk);
		_DateTimeSent=true;
	}

  //::::::::::::: get operating frequency of the radio
	unsigned long ICradio::getFrequency() {
    return _frequency;
	}

  //::::::::::::: get current Modulation Mode of the radio
	radioModMode_t ICradio::getModMode() {
    return _modMode;
	}

  //::::::::::::: get current Modulation Filter of the radio
	radioFilter_t ICradio::getRxFilter() {
    return _modFilter;
	}

  //::::::::::::: set/get the CI-V address
  void ICradio::setCIVaddr(uint8_t myCIVaddr) {
		civ.unregisterAddr(_radioAddr);
		_radioAddr = myCIVaddr;
		civ.registerAddr(_radioAddr);
	}
	
  uint8_t ICradio::getCIVaddr() {
    return _radioAddr;
	}

//------------------------------------------------------------------------
// private methods

	
//------------------------------------------------------------------------
// private static variables

	