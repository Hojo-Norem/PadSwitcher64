# PadSwitcher64
![alt text](https://github.com/Hojo-Norem/PadSwitcher64/blob/master/dip_version.jpg "PadSwitcher64: DIP version")
Interface to allow Super Nintendo (SNES) control pads to be plugged into a C64's joystick ports.

The adapter connects to both C64 joystick ports and allows the active port to be changed on the fly with a push of a button.  The interface defaults to joystick port two on power on.

Also, the interface allows up to four custom mappings to be programmed using a C64-side application.  The custom mappings allow any combination of joystick port two and port one control lines to be mapped to any of the Super Nintendo controller action buttons (A,B,X,Y,L,R).  Any of the action buttons can also have an optional rapid-fire function applied, of which a normal and 'inverted' state is avaliable.  'Inverted' rapid fire is simply that when active the output state of the effected button will be opposite to of a normal rapid fire button when pushed.  For example, mapping a button to joystick left with rapid fire and another button to joystick right with inverted rapid fire will simulate a rapid left/right joystick waggle when both buttons are pushed.

This code was originally developed for the Atmel ATMega8 mcu and was then later ported over to the ATMega88.  This was mainly due to the availiablilty of the Mega8 and that the Mega88 has more power saving features.

This source code was written using AVR-GCC and AVR Libc.
	
# TODO :
  * Re-jig command mode for potential compatibility with non-C64 Atari joystick compatible machines.
  * Maybe figure out how to support two pads while still retaining some semblence of mapping support.
	
# Pad Mapping: (default)
|SNES|C64|
|---|---|
|DPAD UP|UP|
|DPAD DOWN|DOWN|
|DPAD LEFT|LEFT|
|DPAD RIGHT|RIGHT|
|Y|FIRE|
|X|TURBO FIRE|
|B|UP|
|A|'Special'(*1)|
|R|KEYBOARD MATRIX LINE(PORT 2 MODE ONLY)(*2)|

(*1) -  UP + FIRE or DOWN + FIRE, swapped with SELECT + L.

(*2) - Defaults to PB4 which most software will interpret as the SPACEBAR.

# Fixed Controls:
|Button/Combo|Function|
|---|---|
|SELECT + UP|TOGGLE DISABLE DPAD UP|
|START|SWAP CONTROL PORTS|
|SELECT + L|Swap 'Special' function|
|SELECT + R|Change keyboard matrix line (*3)|
|SELECT + (A,B,X,Y)|Select mapping| 
|SELECT + START|Default Mapping|	
|SELECT + START|Enter Command Mode (*4)| 
|SELECT + START + L + R| |
|(In normal or mapped mode)|Enter type 1 direct mode|
|(In any other mode)|Force normal mode|

(*3) - Joystick port 1, which corresponds to CIA 1 PB0 to PB4, PB4 which are normally used as the keyboard input lines during keyboard scanning.  By default at startup this is set to PB4, which most software will interpret as the SPACEBAR.
|CIA1 PortB|CIA1 PA7|PA6|PA5|PA4|PA3|PA2|PA1|PA0|
|---|---|---|---|---|---|---|---|---|
|**PB4**|SPACE|R-SHIFT|.|M|B|C|Z|F1/F2|
|**PB3**|2|CLR/HOME|-|0|8|6|4|F7/F8|
|**PB2**|CTRL|;|L|J|G|D|A|CRSR|->/<-|
|**PB1**|<-|*|P|I|Y|R|W|RETURN|
|**PB0**|1|£|+|9|7|5|3|INST/DEL|

(*4) - Only functional when prompted, only functional in default mapping mode.

# MAPPING
Four mappings, each with all 6 action buttons available.  Each button has a 2 byte pattern corresponding to the following:
|Byte/Bit|Function|
|---|---|
|Byte 1 - Bit 0 to 3|PB0 to PB3 (JOY 1 Directions & KB matrix)|
|Byte 1 - Bit 4 to 7|PA0 to PA3 (Joy 2 directions)|
|Byte 2 - Bit 0|PB4 (Joy 1 Fire & KB matrix)|
|Byte 2 - bit 1|PA4 (Joy 2 Fire)|
|Byte 2 - bit 2|Pad Button has turbofire action|
|Byte 2 - Bit 3|Pad Button has turbofire action (inverted)|
		
Bits 4 - 7 of byte 2 are not accessible through the command interface at this time.
			
# Direct mode - Type 1
Direct mode allows all the SNES control pad buttons to be read by the C64.  Type 1 uses both joystick port to report all buttons.  There is technically a direct mode type 2, and that is by using the command interface.  Type one is much faster as it just involves reading the joystick ports normally. 
|SNES Pad|C64 CIA|Bit pattern|C64 Joystick|
|---|---|---|---|
|Up|DC00| 11110|Port 2 up|
|Down|DC00|11101|Port 2 down|
|Left|DC00|11011|Port 2 left|
|Right|DC00|10111|Port 2 right|
|Y|DC00|01111|Port 2 fire|
|Start|DC00|00000|Port 2 all|
|B|DC01|11110|Port 1 up|
|X|DC01|11101|Port 1 down|
|L|DC01|11011|Port 1 left|
|R|DC01|10111|Port 1 right|
|A|DC01|01111|Port 1 fire|
|Select|DC01|00000|Port 1 all|
