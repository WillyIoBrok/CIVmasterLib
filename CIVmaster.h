/* 
	CIVmaster.h - Library for communication via ICOM's CI-V bus incl. BT
	Created by Wilfried Dilling, DK8RW, May 04, 2022
	Released into the public domain
*/
#ifndef CIVmaster_h
#define CIVmaster_h

#ifndef CIVcmds_h

// CIVcmds.h must be inluded before CIVmaster.h!
// if this is NOT the case, then it will be done here !
#include <CIVcmds.h>

#endif


// Debugging:

// switching the debugging features on or off; Pls be careful, in the case of
// "log_CIV is defined" a significant amount of RAM space will be
// used in addition (due to logging).
// Requires access to the serial line,i.e. Serial communication via USB
// has to be initialized in the main program

//  #define log_CIV           // switch on logging (command-structured, in and out)
															// ringbuffer
															
//  #define debugWithoutRadio // if defined, no reaction of a radio is expected
                              // dummy receive data are used instead 


// Selection of the serial port in use
// Note: there are certainly other arduino modules that have more than one serial
// HW port. If you own one, you can add it here in these statements.
// For me it's enough to differentiate between Uno, Pro, Pro mini, Nano versus Mega versus ESP32

// On processors with only one serial HW port, a software emulation is used
// see AltSoftSerial - documentation

#if defined(ARDUINO_AVR_UNO)||defined(ARDUINO_AVR_NANO)||defined(ARDUINO_AVR_MINI)||defined(ARDUINO_AVR_PRO)
  #define useAltSoftSerial
#elif defined(ARDUINO_AVR_MEGA2560)
  #define useSerial_1
	#define bigRamAv
#elif defined(ESP32)
  #define useSerial_2
	#define useBluetooth
	#define bigRamAv
#endif

// Note: If you are using an ESP32 and want to use the Bluetooth interface to control an IC-705, simple pass "true"
// as an parameter to the setupp method !

// It is highly probable, that you are running out of memory in the case you are using Bluetooth on ESP32
// there is a simple solution for that: go in the Arduino IDE to "tools" and change the "Partition Scheme" 
// to "Huge App" or any other partition scheme which gives you more than the 1,31MB Flash of the default.

#ifdef useAltSoftSerial
	#include <AltSoftSerial.h>
#endif

#ifdef useBluetooth
	#include "BluetoothSerial.h"
#endif

// general serial interface switches (do NOT apply for BT! )
#define CIV_BAUDRATE 19200

#define UART_TIMEOUT 1000

// Bluetooth device name (does apply for BT only! )
#define BT_NAME "CIV_BT_IFC"


// maximum length+1 of sent / received messages
#ifdef bigRamAv
	#define CIV_BUFFERSIZE 256
#else
	#define CIV_BUFFERSIZE 64
#endif	

#define CIV_TXBUFFERSIZE 64


// structure of return values of readMsg
typedef struct {
  uint8_t  retVal;
  uint8_t  address;
  uint8_t  cmd[5];
  uint8_t  datafield[10];
  unsigned long value;
} CIVresult_t;

#ifdef bigRamAv
	constexpr uint8_t  CIVresultBufSize = 6;
#else
	constexpr uint8_t  CIVresultBufSize = 3;
#endif


constexpr uint8_t  knownAddrListSize = 3;

// time definitions in multiple of 1ms
#define t_msDelay_5ms 5

// time definitions based on no of loops

#define t_sendCmd       t_usLoop_10ms
#define t_waitForRadio  t_usLoop_100ms
#define t_readMsg       t_usLoop_40ms

constexpr uint8_t  t_usLoop = 50;

constexpr uint16_t t_usLoop_5ms   =   5000/t_usLoop;
constexpr uint16_t t_usLoop_10ms  =  10000/t_usLoop;
constexpr uint16_t t_usLoop_40ms  =  40000/t_usLoop;
constexpr uint16_t t_usLoop_100ms = 100000/t_usLoop;
constexpr uint16_t t_usLoop_250ms = 250000/t_usLoop;
constexpr uint16_t t_usLoop_300ms = 300000/t_usLoop;

enum writeMode_t:uint8_t {
  CIV_wFast =0,
  CIV_wChk,
  CIV_wOn
};

//return codes used in CIV module/class
enum retVal_t :uint8_t {
	CIV_OK           =  0,
	CIV_OK_DAV       = 	1,

	CIV_NOK          =  2,	// this is the border between good and bad and will be used as such
	CIV_HW_FAULT     =  3,
	CIV_BUS_BUSY     =  4,
	CIV_BUS_CONFLICT =  5,
	CIV_NO_MSG    	 =  6
};

// state of the CIV-bus
enum CIV_State_t:uint8_t {
	CIV_idle 		= 0,
	CIV_sync 		= 1,
	CIV_collect	=	2,
	CIV_stop		= 3
};

// length of Cmd + Subcommands; 
// default: 1; if the command is in this list: 2
// 0x1A may have 4 bytes (if cmd is 0x1A+0x05). This has to be covered extra in source code, since this is
// currently (Jan 2021) the only 4 byte command ...

constexpr uint8_t CIV_C_LENGTH_2[]  {0x07,0x0E,0x13,0x14,0x15,0x16,0x19,0x1A,0x1B,0x1C,0x1E,0x21,0x27};


	
// CI-V (default-)addresses common to all ICOM radios on the bus:
constexpr uint8_t CIV_ADDR_ALL    	= 0x00; // address "to all"; can be set in the radio under "Transceive Address"
constexpr uint8_t CIV_ADDR_MASTER 	= 0xE0; // (Default-)address of the Master
constexpr uint8_t CIV_ADDR_NONE 		= 0xF9; // this is an invalid CIV address

// CI-V (default-)addresses of several ICOM radios:
constexpr uint8_t CIV_ADDR_7100 = 0x88; // (Default-)address of the IC7100
constexpr uint8_t CIV_ADDR_7300 = 0x94; // (Default-)address of the IC7300
constexpr uint8_t CIV_ADDR_9700 = 0xA2; // (Default-)address of the IC9700
constexpr uint8_t CIV_ADDR_705  = 0xA4; // (Default-)address of the IC705

constexpr uint8_t CIV_ADDR_ROTOR = 0x81; // homebrew Rotor device (DK8RW)

// fixed ICOM key bytes
constexpr uint8_t C_START  = 0xFE;
constexpr uint8_t C_STOP   = 0xFD;
constexpr uint8_t C_OK     = 0xFB;
constexpr uint8_t C_NOK    = 0xFA;


// class definition
class CIV {

public:

  // ctor
  CIV();
	
//------------------------------------------------------------------------
// public member functions

	//::::::::::::: initialisation of the class CIV
  void    setupp();	 													// only Serial1, Serial2 or AltSoftSerial supported, NO BT!
  void    setupp(bool ESP_BT);								// if a true is passed to CIV and it's running on an ESP -> BT will be used

  void    setupp(bool ESP_BT,String BTname);	// if a true is passed to CIV and it's running on an ESP -> BT with id BTname will be used
																							// Note: Pls delete the BT-object in the IC705 pairing menu and perform a new
																							// pairing every time you change this name - otherwise you won't see 
																							// this change in the IC705!

//::::::::::::: make the CIV address in use known to CIV
	void		registerAddr(const uint8_t deviceAddr);

//::::::::::::: remove a specific CIV address from the known address list
	void		unregisterAddr(const uint8_t deviceAddr);

//::::::::::::: is a specific address known to CIV ?
	bool 		isAddrKnown(const uint8_t deviceAddr);


	//::::::::::::: 
	CIVresult_t readMsg(const uint8_t deviceAddr);
  /*
	Main function to read incoming data in a system with more than one radio.
	Uses readMsgRaw and, in addition, a buffer for currently unwanted messages. 
	In case a message has been received which does not match to the address requested 
	but has been registered in CIV, it is stored into this buffer.

	If these messages are not fetched in time by their corresponding SW-instances, there is a certain
	risk (very small, though), that the buffer gets full.
		In this case no other message from the CI-V bus will be fetched until this buffer is cleared by calling 
	readMsg with the proper address.
	*/

	//::::::::::::: 
  CIVresult_t readMsgRaw();
  /*
	main function to read incoming data
	can be used independently from writeCmd for asynchronous receiving
  (takes approx. 4us without data received, 750us if e.g. frequency received)
	*/

	//::::::::::::: 
  CIVresult_t writeMsg (const uint8_t deviceAddr, const uint8_t cmd_body[], const uint8_t cmd_data[],writeMode_t mode);
	/*
  main function to write data to a specific radio
	CIV_wFast 		in this mode, the time used by this procedure is very short, i.e.
								significantly shorter than the time needed for sending the command via
	 							the bus -> veeeery dangerous !! -> Use this option only if you know what you are doing!

	CIV_wChk 			the time taken by writeCmd in this mode is almost exactly as long as
								the time needed to transfer the command to the radio via the bus.
								This is the preferred mode, if the answer from the radio is not
								interesting, since the correct transmission to the radio can be checked
								in this mode.

	CIV_wOn				only used when the radio shall be switched on. In this case, a number of 0xFE will be
								sent to the radio in order to wake it up. This is necessary according to ICOM's spec.
	*/

	//::::::::::::: logging

	#ifdef log_CIV
		#ifdef bigRamAv
			#define logMaxEntries 40
			#define logMsgLength  25
			#define logNameLength 5
		#else
			#define logMaxEntries 18
			#define logMsgLength  20
			#define logNameLength 5
		#endif

		static uint8_t   logEntry;
		static uint8_t   logBuffer	[logMaxEntries][logMsgLength];
    static uint8_t   logBufferState[logMaxEntries];
		static char      logBufferName	[logMaxEntries][logNameLength];
  #endif


  void logClear();
	void logNewEntry(uint8_t msg[],const char name[],const uint8_t state);
	void logNewInRing (uint8_t inByte);
	void logDisplayLine(uint8_t lineNo);
	void logDisplay();

private:
//------------------------------------------------------------------------
// private methods

	// low level serial access
	uint8_t 	serAvailable();
	uint8_t 	serRead();
	void 			serWrite(uint8_t ch);
  void 			serflushOutput();

//------------------------------------------------------------------------
// private variables

	uint8_t         rxBuffer[CIV_BUFFERSIZE];

	CIVresult_t 		CIVresultBuf[CIVresultBufSize];
	uint8_t 		 		CIVresultBufIdx; 

	uint8_t		 			knownAddress[knownAddrListSize];

	bool						_ESP_BT = false;

}; // end class CIV


#endif
