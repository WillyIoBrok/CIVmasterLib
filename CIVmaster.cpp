/* 
	CIVmaster.cpp - Library for communication via ICOM's CI-V bus and/or Bluetooth
	Created by Wilfried Dilling, DK8RW, May 04, 2022
	Released into the public domain
*/


#include <Arduino.h>

#include "CIVmaster.h"
#include "CIVcmds.h"

#if defined(useAltSoftSerial)
	AltSoftSerial CIV_SERIAL;
#elif defined(useSerial_1)
	#define CIV_SERIAL Serial1
#elif defined (useSerial_2)
	#define CIV_SERIAL Serial2
	BluetoothSerial CIV_BTSER;
#endif



//ctor = constructor
CIV::CIV()
{ uint8_t idx;

	CIVresultBufIdx = 0; // CIVresultBuf is empty, "idx" points to first empty buffer element
	for (idx=0;idx<knownAddrListSize;idx++)	// no valid address registered yet
		knownAddress[idx] = CIV_ADDR_NONE;
	
}

//------------------------------------------------------------------------
// public methods


//::::::::: initialize the HW /Interfaces
void CIV::setupp(bool ESP_BT, String BTname) {

#if defined(ESP32)
	_ESP_BT = ESP_BT;
	if (_ESP_BT) {												// initialize BT if required (ESP only)
		CIV_BTSER.begin(BTname);
	}	
	else
#endif
	{
		_ESP_BT = false;										// initialize Altsoftserial, Serial1 or Serial2
		CIV_SERIAL.begin(CIV_BAUDRATE);
		CIV_SERIAL.setTimeout(UART_TIMEOUT);
	}

}

void CIV::setupp(bool ESP_BT) {

#if defined(ESP32)
	_ESP_BT = ESP_BT;
	if (_ESP_BT) {												// initialize BT if required (ESP only)
		CIV_BTSER.begin(BT_NAME);
	}	
	else
#endif
	{
		_ESP_BT = false;										// initialize Altsoftserial, Serial1 or Serial2
		CIV_SERIAL.begin(CIV_BAUDRATE);
		CIV_SERIAL.setTimeout(UART_TIMEOUT);
	}

}

void CIV::setupp() {

		_ESP_BT = false;										// initialize Altsoftserial, Serial1 or Serial2
		CIV_SERIAL.begin(CIV_BAUDRATE);
		CIV_SERIAL.setTimeout(UART_TIMEOUT);

}

//::::::::::::: is a specific address known to CIV ?
bool CIV::isAddrKnown(const uint8_t deviceAddr) {
	uint8_t idx;

//	Serial.print ("A:"); Serial.println (deviceAddr, HEX);


	for (idx=0;idx < knownAddrListSize;idx++)
		if (knownAddress[idx]==deviceAddr)	break;				// deviceAddr known ?

///	Serial.print ("idx:");Serial.println (idx, HEX);

	if (idx < knownAddrListSize)	return true;					// deviceAddr known !
	else													return false;

}


//::::::::::::: make a specific CIV address known to CIV
void	CIV::registerAddr(const uint8_t deviceAddr) {
	uint8_t idx;
	
	if (!isAddrKnown(deviceAddr)) {											// deviceAddr not known yet -> add
		for (idx=0;idx < knownAddrListSize;idx++) {			// search for empty space
			if (knownAddress[idx]==CIV_ADDR_NONE)	break;	// empty space available
		}
		if (idx<knownAddrListSize)	knownAddress[idx]=deviceAddr;	// -> store
	}

//	Serial.print(knownAddress[0],HEX);
//	Serial.print(knownAddress[1],HEX);
//	Serial.println(knownAddress[2],HEX);

}


//::::::::::::: remove a specific CIV address from the known address list
void	CIV::unregisterAddr(const uint8_t deviceAddr) {
	uint8_t idx;

	for (idx=0;idx < knownAddrListSize;idx++)
		if (knownAddress[idx]==deviceAddr)	break;				// deviceAddr known ?
	if (idx<knownAddrListSize) {												// deviceAddr known -> remove
 	 knownAddress[idx]=CIV_ADDR_NONE;
	}

}

//::::::::: read data in a Multi Radio System (two or three devices connected to CI-V-bus)
CIVresult_t CIV::readMsg(const uint8_t deviceAddr) {

  CIVresult_t CIVresultL;
	uint8_t idx;

	
	// check and read buffer
	for (idx=0;idx<CIVresultBufIdx;idx++) {						// first, check the buffer for a proper message
		if (CIVresultBuf[idx].address==deviceAddr) {		// message from requested device found

			CIVresultL = CIVresultBuf[idx];								// get message from buffer

			for ( ;(idx+1)<CIVresultBufIdx;idx++) {				// close the gap in the buffer list
				CIVresultBuf[idx]=CIVresultBuf[idx+1];
			}
			if (CIVresultBufIdx>0) CIVresultBufIdx--;
			
			return CIVresultL;														// return result -> done for now
		}
	}

	// check and read HW-CIVBus (if possible)
	if  (CIVresultBufIdx < CIVresultBufSize) {				// static buffer could store a new result, if required
																										// i.e. "buffer not full"
		CIVresultL = readMsgRaw();													// -> check the HW for a new message

		if (CIVresultL.retVal<=CIV_NOK) {								// valid message received, "CIV_NO_MSG" will be discarded
			if (CIVresultL.address==deviceAddr) {					// got it - the correct device has answered
				return CIVresultL;													// return result -> done for now
			}
			else {
				if (isAddrKnown(deviceAddr)) {							// deviceAddr known ?!
					CIVresultBuf[CIVresultBufIdx]=CIVresultL;	// store the new result into the buffer
					CIVresultBufIdx++;												// points to the next empty element
				}
			}
		}
	}

	CIVresultL.retVal 		= CIV_NO_MSG;									// nothing valid received from the correct device
	CIVresultL.address 	= CIV_ADDR_NONE;							// this may happen also in the case of "buffer full"
	CIVresultL.cmd[0]=0;
	CIVresultL.datafield[0]=0;
	CIVresultL.value=0;

	return CIVresultL;																// return result

}

  
//::::::::: 
CIVresult_t CIV::readMsgRaw() {

// read the data (complete commands)coming in from every radio connected to the CIV Bus
// return: info about success (CIV_OK, CIV_OK_DAV, CIV_NOK,CIV_NO_MSG)+Data eventually
// output: answer from the radio in rxBuffer

	uint8_t	inByte; 
	uint16_t lpCounter = 0; 
  CIV_State_t CIV_State;

  uint8_t idx;
  uint8_t DstartIdx;
  uint8_t DstopIdx;
  unsigned long mul = 1;

// length of Cmd + Subcommands; 
// default: 1; if the command is in this list: 2
// 0x1A may have 4 bytes (if cmd is 0x1A+0x05). This has to be covered extra in source code, since this is
// currently (Jan 2021) the only 4 byte command ...

//   constexpr uint8_t CIV_C_LENGTH_2[]  {0x07,0x0E,0x13,0x14,0x15,0x16,0x19,0x1A,0x1B,0x1C,0x1E,0x21,0x27};
// table moved to CIVcmds.h !!! 
	
  CIVresult_t CIVresultL;


  CIVresultL.retVal       = CIV_OK;
  CIVresultL.address      = CIV_ADDR_NONE;
  CIVresultL.cmd[0]       = 0;
  CIVresultL.datafield[0] = 0;
  CIVresultL.value        = 0;

  #ifdef debugWithoutRadio

    uint8_t idx;
    // various test patterns
    constexpr uint8_t rxBufDummy[] = { 6,C_START,C_START,0xE0,0x94, C_OK,C_STOP};                           // OK
//    constexpr uint8_t rxBufDummy[] = { 6,C_START,C_START,0xE0,0x94,C_NOK,C_STOP};                           // NOK
//		constexpr uint8_t rxBufDummy[] = {11,C_START,C_START,0xE0,0x94,0x00,0x89,0x67,0x45,0x23,0x01,C_STOP};   // OK_DAV

		for (idx=0; idx<=rxBufDummy[0];idx++) rxBuffer[idx]=rxBufDummy[idx];
  
  #else
		// quickcheck, whether something has been received
		if (serAvailable()==0) {CIVresultL.retVal = CIV_NO_MSG; return CIVresultL;}

		//.... receive the answer of the radio (exactly ONE message)

		lpCounter=0;CIV_State=CIV_idle; // we start from scratch ...

		while ((CIV_State!=CIV_stop) && (lpCounter<t_readMsg)) {

      lpCounter++;delayMicroseconds(t_usLoop);

			if (serAvailable()>0) {
				inByte = serRead();
				switch (CIV_State) {
			
					case CIV_idle:
						if (inByte==C_START)					    	// first Startbyte
							{rxBuffer[0]=1; rxBuffer[1]=inByte; CIV_State=CIV_sync;}
            else
							rxBuffer[0]=0;
					break;

					case CIV_sync:
						if (inByte==C_START)						    // second Startbyte
							{rxBuffer[0]=2; rxBuffer[2]=inByte; CIV_State=CIV_collect;}
            else 
							{rxBuffer[0]=0; CIV_State=CIV_idle;}
					break;

					case CIV_collect:										    // collect Data
            rxBuffer[0]++; rxBuffer[rxBuffer[0]]=inByte;
            if (                                  // some plausibility checks ...
                ((rxBuffer[0]==3) &&                      // Target-Address wrong
                  !((inByte==CIV_ADDR_ALL)||(inByte==CIV_ADDR_MASTER)))
								||
								(inByte==C_START)																			// erroneous Startbyte
               ) 
              CIV_State = CIV_idle;               // discard the received bytes, wait for Start byte              

            if (inByte == C_STOP)                 // Stop byte received -> end of message
              CIV_State = CIV_stop;               // leave the while loop ...
					break;
					case CIV_stop:
					break;
				} // case state
			}	// if serAvailable
		}	// while loop

		if (lpCounter==t_readMsg) // timeout -> error: no complete answer from the radio(unexpected break of transmission)
			{logNewEntry(rxBuffer,"RX", CIV_NO_MSG); CIVresultL.retVal = CIV_NO_MSG; return CIVresultL;}
    
  #endif

  CIVresultL.address      = rxBuffer[4];                  // Source address

  if (rxBuffer[5]==C_NOK) {      													// command not accepted by the radio
		CIVresultL.retVal=CIV_NOK;
		CIVresultL.cmd[0] = CIV_C_NOK[0];
		CIVresultL.cmd[1] = CIV_C_NOK[1];
	}
  else if (rxBuffer[5]==C_OK) {    												// command accepted by the radio / all ok
		CIVresultL.retVal=CIV_OK;
		CIVresultL.cmd[0] = CIV_C_OK[0];
		CIVresultL.cmd[1] = CIV_C_OK[1];
	}
	else {                                                // Data are available in the rxBuffer
    CIVresultL.retVal = CIV_OK_DAV;

    //............. process the radio's answer
    // it is assumed, that the rxBuffer has been correctly filled by the readMsg method before

    // Index              0       1     2     3     4     5     6     7     8     9     10     11   
		//Example "OK"								FE    FE    E0    94   C_OK   FD
    //                lengthbyte  FE    FE    E0    94   cmd    |     |           |      |     FD
    // 1 Byte cmd                                            D-Start1 |           |      |
    // 2 Byte cmd                                                  D-Start2       |      |
    // 4 Byte cmd                                                              D-Start4 D-Stop=rxBuffer[0]-1

    // calculate the start of the datafield depending on command / subcommnd(s)
    DstartIdx = 6;    // 1 byte command

    if (rxBuffer[0] < DstartIdx) {CIVresultL.retVal=CIV_NOK; return CIVresultL;}  // no valid content in Buffer

    // if cmd is found in list -> 2 Byte command
    for (idx=0; idx<sizeof(CIV_C_LENGTH_2); idx++) {if (rxBuffer[5]==CIV_C_LENGTH_2[idx]) DstartIdx = 7;}
    if ((rxBuffer[5]==0x1A) && (rxBuffer[6]==0x05)) DstartIdx = 9;  // 4-Byte Command+Subcommand

    if (rxBuffer[0] < DstartIdx) {CIVresultL.retVal=CIV_NOK; return CIVresultL;}  // no valid content in Buffer

    DstopIdx = rxBuffer[0]-1;

    for (idx=5; idx<DstartIdx; idx++)                       // load cmdfield of result
      CIVresultL.cmd[idx-4]=rxBuffer[idx];
    CIVresultL.cmd[0] = DstartIdx-5;

    for (idx = DstartIdx; idx <= DstopIdx; idx++)           // load datafield of result
      CIVresultL.datafield[idx-DstartIdx+1] = rxBuffer[idx];
    CIVresultL.datafield[0] = DstopIdx-DstartIdx+1;

    mul = 1; CIVresultL.value = 0;                          // load value of result

    if ((DstopIdx-DstartIdx)==0)  {                         // 1 byte data
      CIVresultL.value = (unsigned long)rxBuffer[DstartIdx];
    }
 
    if ((DstopIdx-DstartIdx)==1)  {                         // 2 byte data -> first byte is of highest order
      for (idx = DstopIdx; idx >= DstartIdx; idx--) {
        CIVresultL.value += (rxBuffer[idx] & 0x0f) * mul; mul *= 10;
        CIVresultL.value += (rxBuffer[idx] >> 4) * mul; mul *= 10;
      }
    }
  
    if ((DstopIdx-DstartIdx) == 4){                         // 5 byte data -> first byte is of lowest order
      for (idx = DstartIdx; idx <= DstopIdx; idx++) {
        CIVresultL.value += (rxBuffer[idx] & 0x0f) * mul; mul *= 10;
        CIVresultL.value += (rxBuffer[idx] >> 4) * mul; mul *= 10;
      }
    }
  
  } // data are available ...

  logNewEntry(rxBuffer,"RX", CIVresultL.retVal);
  
  return CIVresultL;

} // readMsgRaw



//::::::::: 
CIVresult_t CIV::writeMsg (const uint8_t deviceAddr, const uint8_t cmd_body[], const uint8_t cmd_data[], const writeMode_t mode) {

// this is the main function to write data to a specific radio
// write a command from the MASTER(this is me) to a radio
// return: info about success (CIV_OK, CIV_OK_DAV, CIV_NOK, or CIV_BUS_BUSY, CIV_HW_FAULT, CIV_BUS_CONFLICT,)
// output: answer from the radio in rxBuffer (if mode == CIV_wAck)
//
// processing time: approx.  18ms if answer from radio is read and processed (ack)
//                  approx. 105ms if no answer from radio
//
//                  approx.   5ms without ack from the radio
//                  approx.  10ms when bus is shortcut


	uint8_t idx; uint8_t waitCounter;

  uint8_t txBuffer[CIV_TXBUFFERSIZE];


  CIVresult_t CIVresultL;

  CIVresultL.retVal     	= CIV_OK;

	CIVresultL.address			= deviceAddr;
	CIVresultL.cmd[0] 			= 0;
  for (idx=1; idx<=cmd_body[0];idx++) {           // command body into CIVresultL
    CIVresultL.cmd[0]++; CIVresultL.cmd[CIVresultL.cmd[0]] = cmd_body[idx];}

	CIVresultL.datafield[0] = 0;
  for (idx=1; idx<=cmd_data[0];idx++) {           // data part into CIVresultL
    CIVresultL.datafield[0]++; CIVresultL.datafield[CIVresultL.datafield[0]] = cmd_data[idx];}

  CIVresultL.value        = 0;										// value will NOT! be set in writeMsg

  //............. build the complete command and write to txBuffer

  txBuffer[1] = C_START;txBuffer[2] = C_START;		// preamble into buffer
  txBuffer[3] = deviceAddr;
  txBuffer[4] = CIV_ADDR_MASTER;
  txBuffer[0] = 4;                      					// length of data in use

  for (idx=1; idx<=cmd_body[0];idx++) {       		// command body into buffer
    txBuffer[0]++; txBuffer[txBuffer[0]] = cmd_body[idx];}

  for (idx=1; idx<=cmd_data[0];idx++) {       		// data part into buffer
    txBuffer[0]++; txBuffer[txBuffer[0]] = cmd_data[idx];}

  txBuffer[0]++; txBuffer[txBuffer[0]] = C_STOP;  // postamble into buffer


	//.............	let's get access to the CIV bus and write the command

	if (CIVresultBufIdx < CIVresultBufSize)	{			// static buffer not full -> store a cmd, if available
		CIVresultBuf[CIVresultBufIdx]=readMsgRaw();	// store the new result into the buffer
		if ((isAddrKnown(CIVresultBuf[CIVresultBufIdx].address)) &&
		    (CIVresultBuf[CIVresultBufIdx].retVal<=CIV_NOK))
			CIVresultBufIdx++;												// points to the next empty element
	}
  if (serAvailable()>0) {                 	// still data to be read -> give up!
 	  CIVresultL.retVal=CIV_BUS_BUSY; 					// CIV bus is not available -> break
    logNewEntry(txBuffer,"CHK", CIVresultL.retVal); 
    return CIVresultL;
 	}

  if (mode==CIV_wOn) {														// wakeup radio requested !
    for (idx=0; idx<40;idx++) serWrite(C_START); 	// wakeup radio: (40)*C_START
  }

	for (idx=1; idx<=txBuffer[0];idx++) {serWrite(txBuffer[idx]);}

  if (mode==CIV_wChk) {

    serflushOutput(); // wait, until the message really has been sent - this takes approx 5ms

	  //............. read the own command back, and check, whether the bytes were sent correctly
	  // there must be the complete command exactly as sent available in the rxBuffer

    rxBuffer[0]=0; waitCounter = 0;
    while ((rxBuffer[0]< txBuffer[0]) && (waitCounter<t_sendCmd)) { 
      waitCounter++; delayMicroseconds (t_usLoop);
      if (serAvailable()>0) {
        rxBuffer[0]++; rxBuffer[rxBuffer[0]] = serRead();
  		  // even if only one byte hasn't been sent correctly, the whole command is corrupted
        if (rxBuffer[rxBuffer[0]]!=txBuffer[rxBuffer[0]]) CIVresultL.retVal = CIV_BUS_CONFLICT;
      }
    }

    if (waitCounter>=t_sendCmd)            // CIV bus is shortcut -> break
			{CIVresultL.retVal = CIV_HW_FAULT; logNewEntry(rxBuffer,"TX_S",CIVresultL.retVal); return CIVresultL;}
		else
			if (CIVresultL.retVal==CIV_BUS_CONFLICT)  			// CIV bus conflict -> break
				{logNewEntry(rxBuffer,"TX_C",CIVresultL.retVal); return CIVresultL;}

  } //(mode==CIV_wChk)
  
  logNewEntry(txBuffer,"TXok",CIVresultL.retVal);

	return CIVresultL;

} // writeCmd

//::::::::::::: logging

#ifdef log_CIV
	uint8_t CIV::logEntry=0;
	uint8_t CIV::logBuffer[logMaxEntries][logMsgLength];
  uint8_t CIV::logBufferState[logMaxEntries];
	char CIV::logBufferName[logMaxEntries][logNameLength];
#endif


//.............
void CIV::logClear() {
  #ifdef log_CIV
		uint8_t idx;
		for (idx=0; idx<logMaxEntries-1;idx++) { // clear all buffer entries
			logBuffer[idx][0]=0;
			logBufferName[idx][0]='\0';
		}
		logEntry = 0;
  #endif
}

//.............
void CIV::logNewEntry(uint8_t msg[],const char name[], const uint8_t state) {

#ifdef log_CIV
	
	uint8_t idx;

  // copy the message into the log
  if (msg[0]>(logMsgLength-1)) msg[0] = (logMsgLength-1); // limit the msg to the length of the 
                                                          // log buffer
  for (idx=0;idx<=(msg[0]);idx++) {logBuffer[logEntry][idx] = msg[idx];}

  // copy the name of the entry into the log
  for (idx=0;idx<logNameLength; idx++)
	  logBufferName[logEntry][idx] = name[idx];
  logBufferName[logEntry][logNameLength-1] = '\0'; // just to be on the safe side ....

  // copy the state into the log
  logBufferState[logEntry] = state;
		
  if (logEntry<(logMaxEntries-1)) logEntry++;
	else logEntry=0;

#endif

}

//.............
void CIV::logDisplayLine(uint8_t lineNo) {

#ifdef log_CIV

  uint8_t idx;
 
	for (idx=1;idx<=logBuffer[lineNo][0];idx++) {
		Serial.print(".");
		Serial.print(logBuffer[lineNo][idx],HEX);
	}
	Serial.print(" : ");
	Serial.print(logBufferName[lineNo]);
  Serial.print(" * ");
  Serial.print(logBufferState[lineNo]);
	Serial.println("");

#endif // log_CIV

}

//.............
void CIV::logDisplay() {

#ifdef log_CIV

	uint8_t entry_idx;
	uint8_t display_idx=0;

	// display oldest lines first
	for (entry_idx = logEntry; entry_idx < logMaxEntries; entry_idx++) {
		if (logBuffer[entry_idx][0]>0) {
			logDisplayLine(entry_idx);
			display_idx++;
		}
	}
	// display latest lines last
	for (entry_idx = 0; entry_idx < logEntry; entry_idx++) {
		logDisplayLine(entry_idx);
		display_idx++;
	}
  if (display_idx>0) Serial.println("**");

#endif // log_CIV

}


//------------------------------------------------------------------------
// private methods

// in order to make the support of other serial-interfaces easier,
// an extra set of internal CIV-bus access routines have been created

//::::::::: 
uint8_t CIV::serAvailable() {
#if defined(ESP32)
	if (_ESP_BT)
		return CIV_BTSER.available();
	else	
		return CIV_SERIAL.available();
#else
	return CIV_SERIAL.available();
#endif
}

//::::::::: 
void CIV::serWrite(uint8_t ch) {
#if defined(ESP32)
	if (_ESP_BT)
		CIV_BTSER.write(ch);
	else
		CIV_SERIAL.write(ch);
#else
	CIV_SERIAL.write(ch);
#endif
}

//::::::::: 
uint8_t CIV::serRead() {
#if defined(ESP32)
	if (_ESP_BT)
		return CIV_BTSER.read();
	else
		return CIV_SERIAL.read();
#else
	return CIV_SERIAL.read();	
#endif
}

//::::::::: 
void CIV::serflushOutput(){

#if defined(ESP32)
	if (_ESP_BT)
		CIV_BTSER.flush();
	else
		CIV_SERIAL.flush();
#elif defined(useAltSoftSerial)
	CIV_SERIAL.flushOutput();
#else
	CIV_SERIAL.flush();
#endif

}

//------------------------------------------------------------------------
// private static variables

//     - none -
