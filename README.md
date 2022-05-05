### CIVmasterLib
Arduino library for accessing ICOM's Amateur Radios via ICOM CI-V Bus 
("onewire" bus or via Bluetooth for the IC-705).

CIVmasterLib is the successor of CIVbusLib, but doesn't contain the part of the CIVclientLib any more!

Version: 1.00

Date: 04-05-2022

IMPORTANT: 
I have kept the functions of the CIVmasterLib compatible to the CIVbusLib as much as possible!
So, in theory, it should be possible to delete CIVbusLib and install CIVmasterLIB without change in your user programs
They should simply compile and work as before!

Exceptions : When you are using the time set function and the CIV client SW (see below).


New Features/changes (for more info see also the comments in CIVmaster.h / ICradio.h):

class CIV:

- removal of CIV-client, since this is not used anymore. By this, the structure of the library is much clearer
  and easier to understand.

- Bluetooth-name of the ESP-device

    civ.setupp (true) ;    		  // take the pre defined BT name "CIV_BT_IFC"
    
    civ.setupp (true, "myBTdeviceName");  // new is the possibility of setting your individual BT-name at startup

- on processors with a lot of RAM (Atmega 2560 or ESP32) the size of various buffers has been increased


class ICradio:

- ICradio supports now also the readout of the Modulation Mode (USB, LSB, CW etc.) in addition.
   ( functions  .getModMode() and .getRxFilter() )

- setting time and date of the radio
	- ICradio doesn't need the global variables G_time, G_date and G_UTCdelta(to be defined in the main program) any more

	- There is a new function to provide the time/date info to the ICradio class and to the radio itself:
		( .updateDateTime(uint8_t timeArr[3], uint8_t dateArr[5], uint8_t UTCdeltaArr[4]) )

	- If you don't use ".updateDateTime" at all, the clock setting function to the radio is incative, so nothing bad happens.

	-	When using this function while the radio is connected, the clock in IC7300,IC9700 and IC705 is set ONCE at the
		first call of ".updateDateTime".

	- For IC7300 the time/date info will in addition be sent automatically to the radio every time the radio is switched on.
		So please be careful in case of the IC7300 - either you use the ".updateDateTime" permanently (e.g. every minute)
		or not at all !

- the ICradio.loopp() method returns now the result of the bus query! By this, you can process on your own those answers of the radio,
  which are not processed by ICradio!

		void loopp(unsigned long currentTime);
		-> 
		CIVresult_t loopp(unsigned long currentTime);

	This modification is backward compatible (since you can ignore this feature "return value" if you don't need it) !!
		

general:

- examples have been renamed and reworked in general. There are also new examples for the combination IC705 + ESP32 added.
- Most of the examples will NOT run on Arduino UNO, Nano etc. any more. The reason is lack of memory (2k is not very much ...).
The biggest reason for the memory consumption of the examples is the bigger number of "Serial.print(...)" in the examples in order to improve the usability. The library itself is not using much more memory! Another reason for not working on those small processors is - of course - the missing Bluetooth functionality of the Atmel chips.

- the Serial Monitor baudrate in the Examples has been changed from 19200Bd -> 115200Bd 
	(faster output of the messages and less interference with the BT channel)

- the order of including CIVcmds,CIVmaster and ICradio is (and has been from the beginning):
	
		#include CIVcmds.h
		#include CIVmaster.h
		#include ICradio.h
	
	Since this order is now essential to avoid compiler errors, I have taken this into the library.

	-> you need to include only the last header file needed (ICradio or CIVmaster), the others will be included automatically.
	However, you can also include all header files of the library as it has been necessary in the past, if you like -
	that doesn't do any harm.


Devices Supported:
* basically all Arduino devices with at least one (better two) serial HW ports
* tested on:
* ATMega328P  (Arduino UNO and Pro Mini)
* ATMega2560	(Arduino Mega 2560)
* ESP32				(with this processor, also Bluetooth is possible)

The SW library can be used in two different ways:

	1. use of the class CIV only (include CIVmaster.h) for direct access to the CIV-Bus via read and write methods
	   This is very universal and can virtually use all available commands of the different radios

	2. use of the class "ICradio" in addition to "CIV" (include CIVmaster.h and ICradio.h). In this case you have access 
		 to a limited set of higher level controls of the radios which I found to be useful.

	A SUBSET of commands as required for the master regarding ICOM's radios is stored in CIVcmds.h
	This can be extended by the user e.g. in own header files of the main program.

### Notes:
SW:
Getting started is easy! Download the .zip File from GitHub (Selection under green Button "Code").
Copy the complete directory "CIVmasterLibxxx" with all files included (pretty small size, though)
into the directory "...\...\Arduino\libraries\" on your PC.

Please delete CIVbusLib (the predecessor of this Library CIVmasterLib) if already installed, since otherwise you have 
old and new files of the same name in parallel in the Arduino library section which might create problems.

That's all, you are ready to go from the SW side. Please try out the examples in order to understand the usage of the library.

Best way to start is to read the documentation and the comments in the ...\CIVbusLib\examples\... and let those
testprograms run. They should be self explaining, hopefully ...

HW:
IF you have an Arduino with more than one serial Interfaces (ATMega2560), use the Pins TX1 and RX1
(or TX2 and RX2 on ESP32 ! ) as the interface to the CI-V bus
If you have an Arduino UNO, NANO, PRO or PRO MINI etc. there is only one serial interface available which
is used for the connection to the USB-bus. Therefore we have to use the Arduino pins 8 and 9 to connect
to the CI-V bus (see documentation of AltSoftSerial)
Switching between these three HW-possibilities will be done automatically in CIVmaster based on the processor type 
(ATMega2560 or ESP32) in use.
If you are using an ESP32, you have in addition the possibility to choose during setup between the Hardware Serial2 or 
the Bluethooth connection (including BT-device name). In the case of Bluetooth, no Hardware Serial ports are used.

*********************************************

LICENCE

The licence for HAMs is freeware !
Released into the public domain

Copyright(c) 2022 Wilfried Dilling - DK8RW      DK8RW@DARC.DE


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
