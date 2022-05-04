/* 
	ICradio.h - Library for communication via ICOM's CI-V bus
	Created by Wilfried Dilling, DK8RW, May 04, 2022
	Released into the public domain

	Control layer for ICOM's radios
*/
#ifndef ICradio_h
#define ICradio_h

#ifndef CIVmaster_h

// CIVmaster.h must be inluded before ICradio.h!
// if this is NOT the case, then it will be done here !
#include <CIVmaster.h>

#endif

// some timing definitions (based on ms)
#define t_waitForAnswer 100
#define t_RadioCheck    1800

constexpr long unsigned t_radio_OFF_TR[4] = {
	5000, // boot time of the IC7100
	5000, // boot time of the IC7300
	6500, // boot time of the IC9700
	4000 	// boot time of the IC705
};


// Radio IDs used in radio module/class
enum radioType_t:uint8_t {
	TypeIC7100=0,
	TypeIC7300,
	TypeIC9700,
	TypeIC705,
  TypeICnone
};


// states of radio's DC-Power (on/Off State)
enum radioOnOff_t:uint8_t {
  RADIO_OFF = 0,
  RADIO_ON  = 1,
  RADIO_OFF_TR,     // transit from OFF to ON
  RADIO_ON_TR,      // transit from ON to OFF
  RADIO_NDEF,       // don't know
  RADIO_TOGGLE
};


// modulation mode according to ICOMs documentation
enum radioModMode_t:uint8_t {
  MOD_LSB 		= 0,
  MOD_USB,
  MOD_AM,
  MOD_CW,
  MOD_RTTY,
  MOD_FM,
  MOD_WFM,
  MOD_CW_R,
  MOD_RTTY_R,
	MOD_DV,
  MOD_NDEF
};


// RX filter chosen according to ICOMs documentation
enum radioFilter_t:uint8_t {
	FIL_NDEF		= 0,
  FIL1 				= 1,
  FIL2,
  FIL3
};


// modes of radio
enum radioMode_t:uint8_t {
	MODE_NDEF = 0,
	MODE_VOICE,
	MODE_DATA
};

// class definition
class ICradio {

public:

// ctor = constructor
  ICradio(radioType_t thisRadio, uint8_t myCIVaddr);
      
//------------------------------------------------------------------------
// public member functions

	//::::::::::::: initialisation of the class ICradio
  void    setupp(unsigned long currentTime);

  //::::::::::::: this method 
	CIVresult_t loopp(unsigned long currentTime);

  //::::::::::::: check, wether radio is switched on and connected
  radioOnOff_t getAvailability();
	
  //::::::::::::: switch radio ON/OFF
  radioOnOff_t setDCPower(radioOnOff_t onOff,unsigned long currentTime);

  //::::::::::::: get radio mode
	radioMode_t getMode();

  //::::::::::::: switch radio mode
	radioMode_t setMode(radioMode_t mode);

  //::::::::::::: set date_time (load time data of ICradio and send it ONCE to the radio if possible)
	void 				updateDateTime(uint8_t timeArr[3], uint8_t dateArr[5], uint8_t UTCdeltaArr[4]);
		// timeArr[3]       = {2, 0x20, 0x44};              // 20:44
		// dateArr[5]       = {4, 0x20, 0x22, 0x11, 0x06};  // 2022-11-06
		// UTCdeltaArr[4]   = {3, 0x01,0x00,0x00};          // 1h ahead

  //::::::::::::: set date_time (send time data to radio)
	void 				setDateTime();

  //::::::::::::: get operating frequency of the radio
	unsigned long getFrequency();

  //::::::::::::: get current Modulation Mode of the radio
	radioModMode_t getModMode();

  //::::::::::::: get current Modulation Filter of the radio
	radioFilter_t	getRxFilter();

  //::::::::::::: set/get the CI-V address
  void setCIVaddr(uint8_t myCIVaddr);
	
  uint8_t getCIVaddr();
	
	  //::::::::::::: get and process the CIV-answers from the radio
  CIVresult_t getNewMsg();



private:
//------------------------------------------------------------------------
// private methods


//------------------------------------------------------------------------
// private variables

	radioType_t     _radioType;
  uint8_t         _radioAddr;
  radioMode_t     _radioMode;
  radioOnOff_t    _radioOnOffState;
  bool            _waitForAnswer;
  bool            _waitForIDquery;
	bool						_DateTimeSent;
	uint8_t					_fModQuery;
  
  unsigned long   _frequency;
	radioModMode_t	_modMode;
	radioFilter_t		_modFilter;

	uint8_t _time[3]       = {0, 0x20, 0x44};              // 20:44				normally time[0]=2; 0 prevents uninitialized writing
	uint8_t _date[5]       = {0, 0x20, 0x22, 0x11, 0x06};  // 2022-11-06	normally time[0]=4; 0 prevents uninitialized writing
	uint8_t _UTCdelta[4]   = {0, 0x01,0x00,0x00};          // 1h ahead		normally time[0]=3; 0 prevents uninitialized writing

  radioMode_t     _sequMode;
  const uint8_t   *_sequPntr;
  uint8_t         _sequCmdIdx;
  uint8_t         _sequNoOfCmds;
  uint8_t         _sequNoOfNOKs;

  unsigned long   _ts_lastIDquery;
  unsigned long   _ts_waitForAnswer;
  unsigned long   _ts_lastOnCmd;

}; // end class ICradio


#endif
