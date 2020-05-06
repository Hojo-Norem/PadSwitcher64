#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

/*
    This file is part of PadSwitcher64.

    PadSwitcher64 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PadSwitcher64 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PadSwitcher64.  If not, see <https://www.gnu.org/licenses/>.


	************************************

				PADSwitcher64
				
	************************************
	
	hardware and software design my Hojo Norem.
	
	*2018*

	Super Nintendo control pad interface for the Commodore 64 (and probably the C128).
	
	The adapter connects to both C64 joystick ports and allows the active port to be
	changed on the fly with a push of a button.
	
	Also, the interface allows up to four custom mappings to be programmed using a C64-
	side application.  The custom mappings allow any combination of joystick port two
	and port one control lines to be mapped to any of the Super Nintendo controller
	action buttons (A,B,X,Y,L,R).  Any of the action buttons can also have an optional
	rapid-fire function applied, of which a normal and 'inverted' state is avaliable.
	
	'Inverted' rapid fire is simply that when active the output state of the effected
	button will be opposite to of a normal rapid fire button when pushed.
	For example, mapping a button to joystick left with rapid fire and another button
	to joystick right with inverted rapid fire will simulate a rapid left/right
	joystick waggle when both buttons are pushed.
	
	This code was originally developed for the Atmel AVRMega8 mcu and was then later
	ported over to the AVRMega88.  This was mainly due to the availiablilty of the Mega8
	and that the Mega88 has more power saving features.
	
	This source code was written using AVR-GCC and AVR Libc.
	
	
	TODO :
		* Re-jig command mode for potential compatibility with non-C64 Atari joystick
		  compatible machines.
		* Maybe figure out how to support two pads while still retaining some semblence 
		  of mapping support.

	
	Pad Mappinng: (default)
	
	SNES				C64
	-----------------------
	DPAD UP				UP
	DPAD DOWN			DOWN
	DPAD LEFT			LEFT
	DPAD RIGHT			RIGHT
	Y					FIRE
	X					TURBO FIRE
	B					UP
	A					'Special' - UP + FIRE or DOWN + FIRE	
	R					KEYBOARD MATRIX LINE(PORT 2 MODE ONLY), defaults to PB4 which
						most software will interpret as the SPACEBAR.

	Fixed Controls:
	---------------
						
	SELECT + UP					TOGGLE DISABLE DPAD UP
	START 						SWAP CONTROL PORTS 
	SELECT + A					CHANGE SPECIAL (default mapping only)
	SELECT + X					Change TURBO FIRE speed between FAST and SLOW.  Power on default is FAST.
	SELECT + L					Swap 'Special' function between 'DOWN + FIRE' and 'UP + FIRE'.
	SELECT + R					SELECT KEYBOARD MATRIX LINE (CIA 1 PB0 to PB4, PB4 default at startup, PORT 2 MODE ONLY, default mapping only)
									PB4 = SPACE		R-SHIFT		.		M		B		C		Z		F1/F2
									PB3 = 2			CLR/HOME	-		0		8		6		4		F7/F8
									PB2	= CTRL		;			L		J		G		D		A		CRSR ->/<-
									PB1 = <-		*			P		I		Y		R		W		RETURN
									PB0 = 1			£			+		9		7		5		3		INST/DEL


	SELECT + (A,B,X,Y)			SELECT MAPPING 
	
	SELECT + START				DEFAULT MAPPING 
	
	SELECT + START				ENTER COMMAND MODE (only functional when prompted, only functional in default mapping mode.)
	
	SELECT + START + L + R		(In normal or mapped mode) 	Enter type 1 direct mode.
								(In any other mode)			Force normal mode.
								
	
	
	MAPPING
	-------
	
	Four mappings, each with all 6 action buttons availiable.  Each button has a 2 byte pattern corrisponding to the following:
	
		Byte 1 - Bit 0 to 3		PB0 to PB3 (JOY 1 Directions & KB matrix)
		Byte 1 - Bit 4 to 7		PA0 to PA3 (Joy 2 directions)
		Byte 2 - Bit 0			PB4 (Joy 1 Fire & KB matrix)
		Byte 2 - bit 1			PA4 (Joy 2 Fire)
		Byte 2 - bit 2			Pad Button has turbofire action
		Byte 2 - Bit 3			Pad Button has turbofire action (inverted)
		
		Bits 4 - 7 of byte 2 are not accessable through the command interface.
		
		
		
		
	DIRECT MODE: TYPE 1
	-------------------
	
	SNES    Port BitDown  C64 Joy Mapping
	Up      DC00 11110    up
	Down    DC00 11101    down
	Left    DC00 11011    left
	Right   DC00 10111    right
	Y       DC00 01111    fire
	Start   DC00 00000    all
	B       DC01 11110    up
	X       DC01 11101    down
	L       DC01 11011    left
	R       DC01 10111    right
	A       DC01 01111    fire
	Select  DC01 00000    all


*/


//Board selection - 

#define DIP_PRODUCTION 0	// ATMEGA88 DIP production pcb
//#define SMD_PRODUCTION 0	// ATMEGA88 SMD production pcb



//Constants
#define INVERT_MAIN 0

#if defined DIP_PRODUCTION
	#define SNES_DAT1_DDR DDRC
	#define SNES_DAT2_DDR DDRC
	#define SNES_CLK_DDR DDRC
	#define SNES_LCH_DDR DDRC
	#define SNES_DAT1_PORT PORTC
	#define SNES_DAT1_PINS PINC
	#define SNES_DAT2_PORT PORTC
	#define SNES_DAT2_PINS PINC
	#define SNES_CLK_PORT PORTC
	#define SNES_LCH_PORT PORTC

	#define SNES_CLOCK 1
	#define SNES_LATCH 2
	#define SNES_DATA_1 4
	#define SNES_DATA_2 8
	
	
#endif
#if defined SMD_PRODUCTION

	#define SNES_DAT1_DDR DDRD
	#define SNES_DAT2_DDR DDRD
	#define SNES_CLK_DDR DDRD
	#define SNES_LCH_DDR DDRD
	#define SNES_DAT1_PORT PORTD
	#define SNES_DAT1_PINS PIND
	#define SNES_DAT2_PORT PORTD
	#define SNES_DAT2_PINS PIND
	#define SNES_CLK_PORT PORTD
	#define SNES_LCH_PORT PORTD

	#define SNES_CLOCK 16
	#define SNES_LATCH 8
	#define SNES_DATA_1 4
	#define SNES_DATA_2 2
	
	
#endif



// SNES Pad buttons
#define PAD_B 0
#define PAD_Y 1
#define PAD_SELECT 2
#define PAD_START 3
#define PAD_UP 4
#define PAD_DOWN 5
#define PAD_LEFT 6
#define PAD_RIGHT 7
#define PAD_A 8
#define PAD_X 9
#define PAD_L 10
#define PAD_R 11
#define PAD_CONFIG1 12
#define PAD_CONFIG2 13
#define PAD_L2 14
#define PAD_R2 15

// C64 Control port lines
#define CP_UP 1
#define CP_DOWN 2
#define CP_LEFT 4
#define CP_RIGHT 8
#define CP_FIRE 16

#define CP_DIRS 15	//All directions pressed
#define CP_ALL 31	//with fire


#define SNES_LATCH_LOW()	do { SNES_LCH_PORT &= ~(SNES_LATCH); } while(0)
#define SNES_LATCH_HIGH()	do { SNES_LCH_PORT |= SNES_LATCH; } while(0)
#define SNES_CLOCK_LOW()	do { SNES_CLK_PORT &= ~(SNES_CLOCK); } while(0)
#define SNES_CLOCK_HIGH()	do { SNES_CLK_PORT |= SNES_CLOCK; } while(0)


//#define RAPIDMAX 10 // Riapid fire rate control

//Extended registers

#define EXREGCOUNT 2





volatile uint8_t PAD[17][2];
volatile uint8_t DISABLE_UP[2];
volatile uint8_t DISABLE_HELD[2];
volatile uint8_t C64_PORT[2];
//volatile uint8_t C64_FIRE[2];
volatile uint8_t TWO_PADS;

volatile uint8_t MAPPINGS[4][6][2];

volatile uint8_t EXREGS[EXREGCOUNT];

uint8_t ee_MAPPINGS[48] __attribute__((section(".eeprom")));

uint8_t ee_EXREGS[EXREGCOUNT] __attribute__((section(".eeprom")));;

uint8_t ee_EEPROM_INIT __attribute__((section(".eeprom")));

volatile uint8_t tick;

	

uint16_t RAPID;
uint8_t RAPIDSTATE;
uint8_t RAPIDMAX;
uint8_t NEWSCAN;
uint16_t COMTIMEOUT;

void ReadPads(void);

void SetupPorts(void);

uint8_t ReadCP2(void);

void SetCP2(uint8_t data);

void SetCP1(uint8_t data);

void SetLED(uint8_t data);

//void SetPOTs(Uint8_t data);

ISR(TIMER0_COMPA_vect)
{
	tick++;
}

void main(void)
{
uint8_t MATRIX_HELD;
uint8_t	SWAP_HELD;
uint8_t SPECIAL_HELD;
uint8_t RAPID_HELD;
uint8_t MATRIX;
uint8_t SWAP;
uint8_t SPECIAL;
uint8_t A;
uint8_t B;
uint8_t C;
uint8_t D;
uint8_t error;
uint8_t IMODE;
uint8_t COMMAND;
uint8_t OPERAND;
uint8_t ACKWAIT;
uint16_t NEGTIMEOUT;
uint8_t MAPPING;
uint8_t BUTTON;
uint8_t EEPROM_INIT;
uint8_t ISOPERAND;
uint8_t PADSEL;
uint8_t EXCOM;
uint8_t EXREG;
	cli();
	CLKPR=128;						// Set 4Mhz main clock
	CLKPR=1;						//
	
	//****************************
	//Setting up system tick clock
	//****************************
	
	TCCR0A = (WGM01<<1);			// Set timer0A CTC mode
	TCCR0B = (CS02<<1)|(CS00<<1);	// Set timer0A 1024 clock divider
	OCR0A = 19;						// set timer0A compare = 19 - should be roughly 205Hz
	
	TIMSK0 = OCIE0A<<1;				// Enable timer0A interrupt

	SetupPorts();
	
	COMMAND=0;
	
	SNES_LATCH_LOW();

	SWAP=0;

	MATRIX=16;
	
	TWO_PADS=0;
	
	RAPID=0;
	
	RAPIDSTATE=4;
	
	SPECIAL = 2;
	
	SPECIAL_HELD = 0;
	
	RAPID_HELD=0;
	
	NEGTIMEOUT =0;
	
	OPERAND = 0;
	
	ISOPERAND=0;
	
	PADSEL=0;
	
	NEWSCAN=0;
	
	COMTIMEOUT=0;
	
	RAPIDMAX=30;
	
	
	EXREG=0;
	
	DISABLE_UP[0]=0;
	DISABLE_UP[1]=0;
	DISABLE_HELD[0]=0;
	DISABLE_HELD[1]=0;
	
	MATRIX_HELD=0;
	SWAP_HELD=0;
	
	EEPROM_INIT=eeprom_read_byte(&ee_EEPROM_INIT);
	if(EEPROM_INIT==64){
		eeprom_read_block((void*)&MAPPINGS, (void*)&ee_MAPPINGS, sizeof(MAPPINGS));
		eeprom_read_block((void*)&EXREGS, (void*)&ee_EXREGS, sizeof(EXREGS));
	}else{
		for(MAPPING=0;MAPPING<=3;MAPPING++){
			for(BUTTON=0;BUTTON<=5;BUTTON++){
				MAPPINGS[MAPPING][BUTTON][0]=0;
				MAPPINGS[MAPPING][BUTTON][1]=0;
			}
		}
		//EXREGS[PADREADRATE]=13;
		//EXREGS[RAPIDMAX]=60;		//Default rapid-fire rate
	}
	
	
	MAPPING=4;
	
	BUTTON=0;
	
	ACKWAIT=0;
	


	power_adc_disable();
	power_usart0_disable();
	power_spi_disable();
	power_timer1_disable();
	power_timer2_disable(); 
	power_twi_disable();

	SetCP1(0);
	SetCP2(0);
	EXCOM=1;
	IMODE=0;
	error=10;
	tick=0;
	sei();

	for(;;){
		sleep_mode();
		if(error!=0){
			if(tick<100){
				SetLED(1);
			}else{
				SetLED(0);
			}
			if(tick==200){
				tick=0;
				error--;
			}
		}
		
		ReadPads();
		
		if(IMODE==0){
			for(A=0;A<=1;A++){
				if(PAD[PAD_SELECT][A]==1){
					if(PAD[PAD_L][A]==1){
						if((PAD[PAD_R][A]==1)&&(PAD[PAD_START][A]==1)){
							IMODE=3;
							PADSEL=A;
						}
					}
				}
			}
		}
		if((IMODE==1)||(IMODE==2)){
			if(PAD[PAD_SELECT][PADSEL]==1){
				if(PAD[PAD_L][PADSEL]==1){
					if((PAD[PAD_R][PADSEL]==1)&&(PAD[PAD_START][PADSEL]==1)){
						IMODE=4;
					}
				}
			}
		}
		if(PAD[16][PADSEL]==0){
			if(IMODE==3) IMODE=2;
			if(IMODE==4) IMODE=0;
		}
		
		//A = ReadCP2();
		if ((PAD[PAD_START][0]==1)&&(PAD[PAD_SELECT][0]==1)&&(IMODE==0)){
			if((ReadCP2()==CP_FIRE)&&(MAPPING==4)){
			/*
				Comamnd MODE
				-----------
				
				Sequence - 
				
					Interface will only enter mode if SELECT + START are pressed with bits 0-3 of port 2 from c64 low.
					Interface acks C64 by pulling all of port 1 low.
					C64 bit 4 of port 2 low and lets bit 0 go high.
					Interface acks this by inverting port 1.
					C64 acks this by pulling bit 0 of port 2 low and releasing bit 2.
					Interface aks this by inverting port 1 again.
					C64 aks this by pulling bit 2 down again.  Handshake is complete on C64 side.
					Interface waits for port 2 bit two to go low.  When it does it releases all of port 1 high.
					
					Handshake complete.

					Interface listens for commands on port 2.
					C64 issues commands by placing it on port 2.  pulls bit 4 HIGH to indicate a operand write.  Operands are written before commands.
					The operand register is 8 bits wide and is written and read in nybbles.  Before each nybble written the register is shifted left four
					bits.  After each nybble read the register is shifted right four bits.  The operand register does not wrap. 
					Interface acks commands by pulling port 1 fire (PB4) low and for read commands places register on PB3-PB0.  Reads from interface to 
					C64 are inverted.
					C64 acks reply by port 2 pulling port 2 low.
					
					
			*/
				if (IMODE==0){
					SetCP1(CP_ALL);
					SetLED(1);
					
					tick=0;
					NEGTIMEOUT=0;
					B=0;
					do{
						A=ReadCP2();
						if ((A==CP_UP)&&(B==0)){
							B=1;
							SetCP1(0);
						}
						//if ((B==1)&&((A!=CP_UP)&&(A!=CP_LEFT)))B=0;
						if ((B==1)&&(A==CP_LEFT)){
							B=2;
							SetCP1(CP_ALL);
						}
						if ((B==2)&&(A==0))B=3;
						if (tick==205){
							NEGTIMEOUT++;
							tick=0;
						}
					}while((B!=3)&&(NEGTIMEOUT<5));
					
					if(NEGTIMEOUT<5){
						IMODE=1;
						SetCP1(CP_ALL);
						COMTIMEOUT=0;
						EXCOM=1;
					}else{
						SetCP1(0);
						SetLED(0);
						IMODE=0;
						error=5;
					}
				}
			
			//DDRC = 0b11111100;
			//DDRD = 0;	
			}
		}
		
		/*
			COMMANDS/REGISTERS (C64 > interface)
			------------------------------------
			
			Command page 1
			
				00 - No command / ack read
				01 - Select pad 1
				02 - Select pad 2
				03 - READ NYB 1:  B, Y, SELECT, START
				04 - READ NYB 2:  UP, DOWN, LEFT, RIGHT
				05 - READ NYB 3:  A, X, L, R
				
				06 - Write EEPROM
				   (Operand = 14)
				
				07 - Select mapping
				   (Operand = 0 to 3)
				08 - Select button
				   (Operand = 0 to 5)
				09 - Read nybble 1
				10 - Read nybble 2
				11 - Read nybble 3
				12 - Write nybble 1
				   (Operand)
				13 - Write nybble 2
				   (Operand)
				14 - write nybble 3
				   (Operand)
				
				15 - Shift to command page 2
				
			Command page 2
			
				00 - No command / ack read
				01 - Shift to command page 1
				
				********** currently not implemented, if ever *************
					02 - Select extended register
					   (Operand = register:
											
											
											
					03 - Read Extended register
					04 - Write Extended Register
					   (Operand*2 writes - Register written on 2nd write)
				***********************************************************
				
				05 - Enter Direct Mode, type 1
						NOTE: Not ACK'd.  Exits command mode.  C64 software should act accordingly
						
				06 - Enter Direct Mode, type 2
						NOTE: Not currrently implemented, may never.
				
				15 - Exit command mode
						NOTE: Not ACK'd. 
		*/
		
		
		if(IMODE==1) {
			COMTIMEOUT++;
			if(COMTIMEOUT>1025){		// ~5 second comunications timeout
				SetCP1(0);
				SetCP2(0);
				EXCOM=1;
				IMODE=0;
				error=10;
			}
			COMMAND = ReadCP2()&CP_ALL;
			SetLED(COMMAND&CP_FIRE);
			if(COMMAND>=CP_FIRE) ISOPERAND=1; else ISOPERAND=0;
			COMMAND = COMMAND&CP_DIRS;
			
			
			if ((ISOPERAND==1)&&(ACKWAIT==0)){
				OPERAND = OPERAND<<4;
				OPERAND = OPERAND|COMMAND;
				SetCP1(CP_FIRE);
				SetLED(1);
				ACKWAIT=3;
			}
			
			if ((ISOPERAND==0)&&(ACKWAIT==0)){
				SetLED(0);
				if(EXCOM==1){
					if((COMMAND==7)&&((OPERAND&15)<=4)){
						MAPPING=OPERAND&15;
						SetCP1(CP_FIRE);
						ACKWAIT=1;
					}
					
					if((COMMAND==6)&&((OPERAND&CP_DIRS)==14)){
						SetCP1(CP_FIRE);
						SetLED(1);
						cli();
						do{asm volatile ("nop");}while (!eeprom_is_ready());
						eeprom_update_block((void*)&MAPPINGS, (void*)&ee_MAPPINGS, sizeof(MAPPINGS));
						eeprom_update_block((void*)&EXREGS, (void*)&ee_EXREGS, sizeof(EXREGS));
						eeprom_update_byte(&ee_EEPROM_INIT, 64);
						sei();
						ACKWAIT=1;
					}
					
					if(COMMAND==8){
						BUTTON=(OPERAND&15);
						SetCP1(CP_FIRE);
						ACKWAIT=1;
					}
					
					if(COMMAND==9){
						SetCP1((MAPPINGS[MAPPING][BUTTON][0]&15)|CP_FIRE);
						ACKWAIT=1;
					}
					if(COMMAND==10){
						SetCP1((MAPPINGS[MAPPING][BUTTON][0]>>4)|CP_FIRE);
						ACKWAIT=1;
					}
					if(COMMAND==11){
						SetCP1((MAPPINGS[MAPPING][BUTTON][1]&15)|CP_FIRE);
						ACKWAIT=1;
					}
					if(COMMAND==12){
						MAPPINGS[MAPPING][BUTTON][0]=(MAPPINGS[MAPPING][BUTTON][0]&240)|(OPERAND&15);
						SetCP1(CP_FIRE);
						ACKWAIT=1;
					}
					if(COMMAND==13){
						MAPPINGS[MAPPING][BUTTON][0]=(MAPPINGS[MAPPING][BUTTON][0]&15)|(OPERAND<<4);
						SetCP1(CP_FIRE);
						ACKWAIT=1;
					}
					if(COMMAND==14){
						MAPPINGS[MAPPING][BUTTON][1]=(MAPPINGS[MAPPING][BUTTON][1]&240)|(OPERAND&15);
						SetCP1(CP_FIRE);
						ACKWAIT=1;
					}

			
				
				
					if(COMMAND==15){
						SetCP1(CP_FIRE);
						EXCOM=4;
						ACKWAIT=1;
					}
					
					if((COMMAND==1)||(COMMAND==2)){
						PADSEL=COMMAND-1;
						SetCP1(CP_FIRE);
						//ReadPads();
						//AUTOREAD=0;
						ACKWAIT=1;
					}

						
					
					if((COMMAND>=3)&&(COMMAND<=5)){
						//if((AUTOREAD==1)&&(COMMAND==3)){
						//	ReadPads();
						//}
						B=0;
						C=1;
						D=(COMMAND-3)*4;
						for (A=0;A<=3;A++){
							B=B+(PAD[A+D][PADSEL]*C);
							C=C*2;
						}
						SetCP1(B|CP_FIRE);
						ACKWAIT=1;
						//AUTOREAD=1;
					}
				}
				if(EXCOM==2){
				
					if(COMMAND==1){
						SetCP1(CP_FIRE);
						EXCOM=3;
						ACKWAIT=1;
					}
				
					if(COMMAND==5){
						SetCP1(0);
						SetCP2(0);
						IMODE=2;
					}
						
				
					if(COMMAND==15){
						SetCP1(0);
						SetCP2(0);
						IMODE=0;
						EXCOM=1;
					}
				
				}
			}
			
			if((ACKWAIT==1)||(ACKWAIT==3)){	
				
				//OUT_PORT_2=OUT_PORT_2|2;
				ACKWAIT++;
				COMTIMEOUT=0;
			}
				
			if((ACKWAIT==2)&&(COMMAND==0)){
				//OUT_PORT_2=4;
				SetCP1(0);
				ACKWAIT=0;
				if(EXCOM>2) EXCOM=EXCOM-2;
			}

				
			if((ACKWAIT==4)&&(ISOPERAND==0)){
				//OUT_PORT_2=4;
				SetCP1(0);
				ACKWAIT=0;
			}
			
		}
		
		
		
		/*
			DIRECT MODE: TYPE 1
			-------------------
	
			SNES    Port BitDown  C64 Joy Mapping
			Up      DC00 11110    up
			Down    DC00 11101    down
			Left    DC00 11011    left
			Right   DC00 10111    right
			Y       DC00 01111    fire
			Start   DC00 00000    all
			B       DC01 11110    up
			X       DC01 11101    down
			L       DC01 11011    left
			R       DC01 10111    right
			A       DC01 01111    fire
			Select  DC01 00000    all
		*/
		
		if (IMODE==2){
			
			C64_PORT[0]=0;
			
			if(PAD[PAD_UP][PADSEL]==1) C64_PORT[0]|=CP_UP;
			if(PAD[PAD_DOWN][PADSEL]==1) C64_PORT[0]|=CP_DOWN;
			if(PAD[PAD_LEFT][PADSEL]==1) C64_PORT[0]|=CP_LEFT;
			if(PAD[PAD_RIGHT][PADSEL]==1) C64_PORT[0]|=CP_RIGHT;
			if(PAD[PAD_Y][PADSEL]==1) C64_PORT[0]|=CP_FIRE;
			if(PAD[PAD_START][PADSEL]==1) C64_PORT[0]|=CP_ALL;
			
			C64_PORT[1]=0;
			
			if(PAD[PAD_B][PADSEL]==1) C64_PORT[1]|=CP_UP;
			if(PAD[PAD_X][PADSEL]==1) C64_PORT[1]|=CP_DOWN;
			if(PAD[PAD_L][PADSEL]==1) C64_PORT[1]|=CP_LEFT;
			if(PAD[PAD_R][PADSEL]==1) C64_PORT[1]|=CP_RIGHT;
			if(PAD[PAD_A][PADSEL]==1) C64_PORT[1]|=CP_FIRE;
			if(PAD[PAD_SELECT][PADSEL]==1) C64_PORT[1]|=CP_ALL;
			
			SetCP2(C64_PORT[0]);
			SetCP1(C64_PORT[1]);
		
			NEWSCAN=0;
		}
		
		
		
		
		if (IMODE==0){
			if ((PAD[PAD_START][0]==1)&&(SWAP_HELD==0)&&(PAD[PAD_SELECT][0]==0)){
				SWAP=SWAP^1;
				SWAP_HELD=1;
			}
			if ((PAD[PAD_START][0]==0)&&(SWAP_HELD==1)) SWAP_HELD=0;
			B=1^SWAP;
			if(MAPPING==4){
				
				if ((PAD[PAD_L][0]==1)&&(SPECIAL_HELD==0)&&(PAD[PAD_SELECT][0]==1)){
					SPECIAL=SPECIAL<<1;
					if(SPECIAL>2) SPECIAL=1;
					SPECIAL_HELD=1;
				}
				if (((PAD[PAD_L][0]==0)||(PAD[PAD_SELECT][0]==0))&&(SPECIAL_HELD==1)) SPECIAL_HELD=0;
				
				if ((PAD[PAD_DOWN][0]==1)&&(RAPID_HELD==0)&&(PAD[PAD_SELECT][0]==1)){
					if(RAPIDMAX==30) RAPIDMAX=60; else RAPIDMAX=30;
					RAPID_HELD=1;
				}
				if (((PAD[PAD_DOWN][0]==0)||(PAD[PAD_SELECT][0]==0))&&(RAPID_HELD==1)) RAPID_HELD=0;
				C64_PORT[0]=0;
				C64_PORT[1]=0;
				A=0;
					
					if ((PAD[PAD_UP][A]==1)&&(DISABLE_HELD[A]==0)&&(PAD[PAD_SELECT][A]==1)){
						if (DISABLE_UP[A]==0){
							DISABLE_UP[A]=1;
						}else{
							DISABLE_UP[A]=0;
						}
						DISABLE_HELD[A]=1;
					}
					if (((PAD[PAD_UP][A]==0)||(PAD[PAD_SELECT][A]==0))&&(DISABLE_HELD[A]==1)) DISABLE_HELD[A]=0;
					
					if (PAD[PAD_SELECT][A]==1) SetLED(1);
					else{	
						if ((PAD[PAD_UP][A]==1)&&(DISABLE_UP[A]==0)&&(PAD[PAD_SELECT][A]==0)) C64_PORT[B]=C64_PORT[B]|CP_UP;
						if ((PAD[PAD_B][A]==1)&&(PAD[PAD_SELECT][A]==0)) C64_PORT[B]=C64_PORT[B]|CP_UP;
						if (PAD[PAD_DOWN][A]==1) C64_PORT[B]=C64_PORT[B]|CP_DOWN;
						if (PAD[PAD_LEFT][A]==1) C64_PORT[B]=C64_PORT[B]|CP_LEFT;
						if (PAD[PAD_RIGHT][A]==1) C64_PORT[B]=C64_PORT[B]|CP_RIGHT;					
						if ((PAD[PAD_Y][A]==1)&&(PAD[PAD_SELECT][A]==0)) C64_PORT[B]=C64_PORT[B]|CP_FIRE;
						if ((PAD[PAD_A][A]==1)&&(PAD[PAD_SELECT][A]==0)) C64_PORT[B]=C64_PORT[B]|SPECIAL|CP_FIRE;
						if ((PAD[PAD_X][A]==1)&&(PAD[PAD_SELECT][A]==0)) C64_PORT[B]=C64_PORT[B]|((RAPIDSTATE&4)<<2);
					}
					//if (SWAP!=A) C64_FIRE[A]=C64_FIRE[A]*2;
					
					
						//C64_PORT[B]=C64_PORT[B]*16;
						
						if ((PAD[PAD_A][A]==1)&&(PAD[PAD_SELECT][A]==1)) MAPPING=0;
						if ((PAD[PAD_B][A]==1)&&(PAD[PAD_SELECT][A]==1)) MAPPING=1;
						if ((PAD[PAD_X][A]==1)&&(PAD[PAD_SELECT][A]==1)) MAPPING=2;
						if ((PAD[PAD_Y][A]==1)&&(PAD[PAD_SELECT][A]==1)) MAPPING=3;
					if (SWAP==A){	
						if ((PAD[PAD_SELECT][A]==1)&&(MATRIX_HELD==0)&&(PAD[PAD_R][A]==1)){
							MATRIX=MATRIX<<1;
							if (MATRIX>16) MATRIX=1;
							MATRIX_HELD=1;
						}
						if (((PAD[PAD_SELECT][A]==0)||(PAD[PAD_R][A]==0))&&(MATRIX_HELD==1)) MATRIX_HELD=0;
						
						if ((PAD[PAD_R][A]==1)&&(TWO_PADS==0)&&(SWAP==0)&&(PAD[PAD_SELECT][A]==0)) C64_PORT[0]=C64_PORT[0]|MATRIX;
						
					}
				
			}else{
				if ((PAD[PAD_A][0]==1)&&(PAD[PAD_SELECT][0]==1)) MAPPING=0;
				if ((PAD[PAD_B][0]==1)&&(PAD[PAD_SELECT][0]==1)) MAPPING=1;
				if ((PAD[PAD_X][0]==1)&&(PAD[PAD_SELECT][0]==1)) MAPPING=2;
				if ((PAD[PAD_Y][0]==1)&&(PAD[PAD_SELECT][0]==1)) MAPPING=3;
				if((PAD[PAD_START][0]==1)&&(PAD[PAD_SELECT][0]==1)) MAPPING=4;
				
				if ((PAD[PAD_UP][0]==1)&&(DISABLE_HELD[0]==0)&&(PAD[PAD_SELECT][0]==1)){
					if (DISABLE_UP[0]==0){
						DISABLE_UP[0]=1;
					}else{
						DISABLE_UP[0]=0;
					}
					DISABLE_HELD[0]=1;
				}
				if (((PAD[PAD_UP][0]==0)||(PAD[PAD_SELECT][0]==0))&&(DISABLE_HELD[0]==1)) DISABLE_HELD[0]=0;
				
				C64_PORT[0]=0;
				C64_PORT[1]=0;
				if(PAD[PAD_SELECT][0]==0){
					if ((PAD[PAD_UP][0]==1)&&(DISABLE_UP[0]==0)) C64_PORT[B]=C64_PORT[B]|CP_UP;
					if (PAD[PAD_DOWN][0]==1) C64_PORT[B]=C64_PORT[B]|CP_DOWN;
					if (PAD[PAD_LEFT][0]==1) C64_PORT[B]=C64_PORT[B]|CP_LEFT;
					if (PAD[PAD_RIGHT][0]==1) C64_PORT[B]=C64_PORT[B]|CP_RIGHT;
						
					if (PAD[PAD_A][0]==1) {
						if((RAPIDSTATE&(MAPPINGS[MAPPING][0][1]&12))==0){
							C64_PORT[0]=C64_PORT[0]|(MAPPINGS[MAPPING][0][0]&15)|((MAPPINGS[MAPPING][0][1]&1)<<4);
							C64_PORT[1]=C64_PORT[1]|((MAPPINGS[MAPPING][0][0]&240)>>4)|((MAPPINGS[MAPPING][0][1]&2)<<3);
						}
					}
					if (PAD[PAD_B][0]==1) {
						if((RAPIDSTATE&(MAPPINGS[MAPPING][1][1]&12))==0){
							C64_PORT[0]=C64_PORT[0]|(MAPPINGS[MAPPING][1][0]&15)|((MAPPINGS[MAPPING][1][1]&1)<<4);
							C64_PORT[1]=C64_PORT[1]|((MAPPINGS[MAPPING][1][0]&240)>>4)|((MAPPINGS[MAPPING][1][1]&2)<<3);
						}
					}
					if (PAD[PAD_X][0]==1) {
						if((RAPIDSTATE&(MAPPINGS[MAPPING][2][1]&12))==0){
							C64_PORT[0]=C64_PORT[0]|(MAPPINGS[MAPPING][2][0]&15)|((MAPPINGS[MAPPING][2][1]&1)<<4);
							C64_PORT[1]=C64_PORT[1]|((MAPPINGS[MAPPING][2][0]&240)>>4)|((MAPPINGS[MAPPING][2][1]&2)<<3);
						}
					}
					if (PAD[PAD_Y][0]==1) {
						if((RAPIDSTATE&(MAPPINGS[MAPPING][3][1]&12))==0){
							C64_PORT[0]=C64_PORT[0]|(MAPPINGS[MAPPING][3][0]&15)|((MAPPINGS[MAPPING][3][1]&1)<<4);
							C64_PORT[1]=C64_PORT[1]|((MAPPINGS[MAPPING][3][0]&240)>>4)|((MAPPINGS[MAPPING][3][1]&2)<<3);
						}
					}
					if (PAD[PAD_L][0]==1) {
						if((RAPIDSTATE&(MAPPINGS[MAPPING][4][1]&12))==0){
							C64_PORT[0]=C64_PORT[0]|(MAPPINGS[MAPPING][4][0]&15)|((MAPPINGS[MAPPING][4][1]&1)<<4);
							C64_PORT[1]=C64_PORT[1]|((MAPPINGS[MAPPING][4][0]&240)>>4)|((MAPPINGS[MAPPING][4][1]&2)<<3);
						}
					}
					if (PAD[PAD_R][0]==1) {
						if((RAPIDSTATE&(MAPPINGS[MAPPING][5][1]&12))==0){
							C64_PORT[0]=C64_PORT[0]|(MAPPINGS[MAPPING][5][0]&15)|((MAPPINGS[MAPPING][5][1]&1)<<4);
							C64_PORT[1]=C64_PORT[1]|((MAPPINGS[MAPPING][5][0]&240)>>4)|((MAPPINGS[MAPPING][5][1]&2)<<3);
						}
					}
				}
				// use spiffy new putput subroutines here
			}
			
			SetCP2(C64_PORT[1]);
			SetCP1(C64_PORT[0]);
			
			SetLED(SWAP);
			
			
		}
		
	}
}

void ReadPads(void){
uint8_t i;	
	RAPID++;
	if (RAPID>RAPIDMAX){
		RAPID=0;
		RAPIDSTATE=RAPIDSTATE^12;
	}
	PAD[16][0]=0;
	PAD[16][1]=0;
	SNES_CLOCK_HIGH();
	_delay_us(4);
	SNES_LATCH_HIGH();
	_delay_us(11);
	SNES_LATCH_LOW();
	for (i=0; i<=15; i++)
	{
		_delay_us(7);
		SNES_CLOCK_LOW();
		if ((SNES_DAT1_PINS&SNES_DATA_1)!=0) PAD[i][0]=0;
		else{
			PAD[i][0]=1;
			PAD[16][0]=1;
		}
		if ((SNES_DAT2_PINS&SNES_DATA_2)!=0) PAD[i][1]=0;
		else {
			PAD[i][1]=1;
			PAD[16][1]=1;
			TWO_PADS=1;
		}
		_delay_us(7);
		
		SNES_CLOCK_HIGH();
	}
}

/*
	
	***** DIP and SMD prototype PCB PORT connections *****
	
	C64 control port 1 - PORTD bits 0 to 3 and PORTC bit 1
	C64 control port 2 - PORTD bits 4 to 7 and PORTC bit 0
	LED - PORTC bit 2
	
*/
#if (defined DIP_PROTO || defined SMD_PROTO)
void SetupPorts(void){

	SNES_DAT1_DDR = 0;
	SNES_DAT2_DDR = 0;
	SNES_CLK_DDR = 0;
	SNES_LCH_DDR = 0;
	
	SNES_DAT1_PORT = 255;
	SNES_DAT2_PORT = 255;
	
	DDRD = 0;
	DDRC = 4;
	
	PORTD = 0;
	PORTC = 0b11111000;
	
	//SNES_DAT1_DDR = SNES_DAT1_DDR | ~(SNES_DATA_1);
	//SNES_DAT2_DDR = SNES_DAT2_DDR | ~(SNES_DATA_2);
	SNES_CLK_DDR = SNES_CLK_DDR | SNES_CLOCK;
	SNES_LCH_DDR = SNES_LCH_DDR | SNES_LATCH;
	SetLED(0);
}

uint8_t ReadCP2(void){
	return ((PIND&240) >> 4)|((PINC&1) << 4);
}

void SetCP2(uint8_t data){
	DDRD = (DDRD & 15)|((data & 15) << 4);
	DDRC = (DDRC & 254)|((data & 16) >> 4);
}

void SetCP1(uint8_t data){
	DDRD = (DDRD & 240)|(data & 15);
	DDRC = (DDRC & 253)|((data & 16) >> 3);
}

void SetLED(uint8_t data){
	if (data!=0){
		PORTC = (PORTC & 251)|4;
	}else{
		PORTC = PORTC&251;
	}
}

	//#define PORT_1_FIRE 2
	//#define PORT_2_FIRE 1
	//#define LED 4


	//#define OUT_PORT PORTD
	//#define OUT_PORT_2 PORTC
	//#define LED_PORT PORTC
#endif


#if defined DIP_PRODUCTION
void SetupPorts(void){

	SNES_DAT1_DDR = 0;
	SNES_DAT2_DDR = 0;
	SNES_CLK_DDR = 0;
	SNES_LCH_DDR = 0;
	
	SNES_DAT1_PORT = 255;
	SNES_DAT2_PORT = 255;
	
	DDRD = 0;
	DDRC = 32;
	DDRB = 0;
	
	PORTD = 0;
	PORTC = 223;
	PORTB = 0b01111110;
	
	//SNES_DAT1_DDR = SNES_DAT1_DDR | ~(SNES_DATA_1);
	//SNES_DAT2_DDR = SNES_DAT2_DDR | ~(SNES_DATA_2);
	SNES_CLK_DDR = SNES_CLK_DDR | SNES_CLOCK;
	SNES_LCH_DDR = SNES_LCH_DDR | SNES_LATCH;
	SetLED(0);
}

uint8_t ReadCP2(void){
	return ((PIND&224) >> 5)|((PINB&1) << 3)|((PINB&128)>>3);
}

void SetCP2(uint8_t data){
	DDRD = (DDRD & 31)|((data & 7) << 5);
	DDRB = ((data & 8) >> 3)|((data & 16) << 3);
}

void SetCP1(uint8_t data){
	DDRD = (DDRD & 224)|(data & 31);
	//DDRB = (DDRB & 191)|((data & 16) << 2);
}
/*
void SetPOTs(uint8_t data){
	

}
*/
void SetLED(uint8_t data){
	if (data!=0){
		PORTC = (PORTC & 223)|32;
	}else{
		PORTC = PORTC&223;
	}
}
#endif
#if defined SMD_PRODUCTION
void SetupPorts(void){

	SNES_DAT1_DDR = 0;
	SNES_DAT2_DDR = 0;
	SNES_CLK_DDR = 0;
	SNES_LCH_DDR = 0;
	
	SNES_DAT1_PORT = 255;
	SNES_DAT2_PORT = 255;
	
	DDRC = 0;
	DDRD = 1;
	DDRB = 0;
	
	PORTB = 0;
	PORTC = 0;
	
	//SNES_DAT1_DDR = SNES_DAT1_DDR | ~(SNES_DATA_1);
	//SNES_DAT2_DDR = SNES_DAT2_DDR | ~(SNES_DATA_2);
	SNES_CLK_DDR = SNES_CLK_DDR | SNES_CLOCK;
	SNES_LCH_DDR = SNES_LCH_DDR | SNES_LATCH;
	SetLED(0);
}

uint8_t ReadCP2(void){
	return ((PINB>>1) & 31);
}

void SetCP2(uint8_t data){
	//DDRD = (DDRD & 31)|((data & 7) << 5);
	DDRB = (DDRB & 193)|((data & 31) << 1);
}

void SetCP1(uint8_t data){
	DDRC = (DDRC & 224)|((data & 15)<<1)|((data&16)>>4);
	//DDRB = (DDRB & 191)|((data & 16) << 2);
}

void SetLED(uint8_t data){
	if (data!=0){
		PORTD = (PORTD & 254)|1;
	}else{
		PORTD = PORTD&254;
	}
}
void SetPOTs(uint8_t data){
	

}

#endif