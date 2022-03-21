
#include <msp430.h> 
#include "ink.h"
/** Driver includes. */
#include <driverlib.h>
#include <profile.h>

/** Benchmark includes. */
#include <apps.h>

// indicates if this is the first boot.
__nv uint8_t __inited = 0;

//global time in ticks
extern uint32_t current_ticks;

static void prv_GPIOInit();
static void prv_CSInit();
static void prv_TimeraInit();
//__nv int src[3000];
//__nv int dst[3000];

int main(void)
{
    // always init microcontroller
    __mcu_init();

    /** Initialization */
    prv_GPIOInit();
    prv_CSInit();
    prv_TimeraInit();

    /** Initialize UART */
    UART_initGPIO();
    UART_init();
    /*
    DMA_initParam param = {0};
    param.channelSelect = DMA_CHANNEL_0;
    param.transferModeSelect = DMA_TRANSFER_BURSTBLOCK;
    param.triggerSourceSelect = DMA_TRIGGERSOURCE_0;
    param.transferUnitSelect = DMA_SIZE_SRCWORD_DSTWORD;
    param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&param);
    int i = 0;
    for (i = 0; i < 3000; i++)
    {
        src[i] = i;
        dst[i] = i-1;
    }
    int s = dst [0];
    int m = dst [1500];
    int e = dst [2999];
    int temp;
    temp = dst[20] *dst[100];
    printf("before DMA 0,1500,2999 value are %d,%d,%d\n",s,m,e);
    //DMA_disableTransferDuringReadModifyWrite();
    __dma_word_copy(src,dst,1500*sizeof(int));
    while(HWREG16(DMA_BASE + DMA_CHANNEL_0 + OFS_DMA1CTL)){
    }
    s = dst [0];
    m = dst [1500];
    e = dst [2999];
    printf("after DMA 0,1500,2999 value are %d,%d,%d\n",s,m,e);
    while(1);*/


	// if this is the first boot
	if(!__inited){
	    // init the scheduler state
        __scheduler_boot_init();
	    // init the event handler
        //__events_boot_init();

        /**
         * init the applications.
         */

	    _benchmark_sort_init();
        //_benchmark_ar_init();
	    //_benchmark_dijkstra_init();
	    //_benchmark_rsa_init();

	    //_benchmark_fft_init();
        //_benchmark_cem_init();
        //_benchmark_crc_init();
        //_benchmark_bc_init();
        //_benchmark_cuckoo_init();
	    // the first and initial boot is finished
        __inited = 1;
	}

	// will be called at each reboot of the application
	//__app_reboot();

	// activate the scheduler
	__scheduler_run();

	return 0;
}


void prv_GPIOInit()
{
    /* Terminate all GPIO pins to Output LOW to minimize power consumption */
    GPIO_setAsOutputPin(GPIO_PORT_PA, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PB, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PC, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PD, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PE, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PF, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PA, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PB, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PC, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PD, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PE, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PF, GPIO_PIN_ALL16);
}

/**
 * Clock System init.
 */
void prv_CSInit()
{
    /** Set DCO frequency to 16MHz */
    CS_setDCOFreq(CS_DCORSEL_1, CS_DCOFSEL_4);

    /**
     * Configure one FRAM waitstate as required by the device datasheet for MCLK
     * operation beyond 8MHz _before_ configuring the clock system.
     */
    FRCTL0 = FRCTLPW | NWAITS_1;

    CS_initClockSignal(CS_MCLK,CS_DCOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK,CS_DCOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK,CS_LFXTCLK_SELECT,CS_CLOCK_DIVIDER_1);
}

/**
 * Timer-A init.
 */
void prv_TimeraInit()
{
    Timer_A_initContinuousModeParam initContParam = {0};
    initContParam.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    initContParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initContParam.timerClear = TIMER_A_DO_CLEAR;
    initContParam.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam);
}
