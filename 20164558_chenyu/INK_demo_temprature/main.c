#include "ink.h"
#include "apps.h"
#include "profile.h"

void __mcu_init();
__nv volatile uint8_t nvInited = 0x0000;

/* 
 * --------------
 * Entry of Main.
 * --------------
 */
int main(void){
    __mcu_init();
    UART_initGPIO();
    UART_init();
    //printf("teststssssttt.\r\n");

	if(!nvInited){
		_demo_fire_warning();
		nvInited = 1;
	}
	// activate the scheduler
	__scheduler_run();

	// should never reach here
	printf("[ERROR] Should not reach here!\r\n");
	while(1);
}


static void __cs_init(){
    CS_setDCOFreq(CS_DCORSEL_1, CS_DCOFSEL_4);      //Set DCO frequency to 16MHz

    /**
     * Configure one FRAM waitstate as required by the device datasheet for MCLK
     * operation beyond 8MHz _before_ configuring the clock system.
     */
    FRCTL0 = FRCTLPW | NWAITS_1;

    CS_initClockSignal(CS_MCLK,CS_DCOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK,CS_DCOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK,CS_LFXTCLK_SELECT,CS_CLOCK_DIVIDER_1);
}

void __mcu_init(){
    WDTCTL = WDTPW | WDTHOLD;       //Stop watchdog.
    PM5CTL0 &= ~LOCKLPM5;           //Disable the GPIO power-on default high-impedance mode.

    P1DIR = 0x3F;                   //0b-0011 1111
    P1OUT = 0x00;
    __delay_cycles(10);
    P1OUT = 0b010011;               //Set P1.4, Turn both LEDs on
    __delay_cycles(100000);
    P1OUT ^= 0b000011;               //Set P1.4, Turn both LEDs on
    __cs_init();                    //Clock system
}
