/*
 * livetemp.c
 *
 *  Created on: May 10, 2022
 *      Author: liusongran
 */

#include "driverlib.h"
#include "task.h"

__nv ADC12_B_initParam initParam = {0};
__nv ADC12_B_configureMemoryParam configureMemoryParam = {0};
uint16_t liveTemp(){
    uint16_t current_temp;
    /* Initialize the ADC12B Module
     * Base address of ADC12B Module Use internal ADC12B bit as sample/hold signal to start conversion USE MODOSC 5MHZ Digital Oscillator as clock source Use default clock divider/pre-divider of 1 Use Temperature Sensor and Battery Monitor internal channels
     */
    initParam.sampleHoldSignalSourceSelect = ADC12_B_SAMPLEHOLDSOURCE_SC;
    initParam.clockSourceSelect = ADC12_B_CLOCKSOURCE_ACLK;
    initParam.clockSourceDivider = ADC12_B_CLOCKDIVIDER_1;
    initParam.clockSourcePredivider = ADC12_B_CLOCKPREDIVIDER__1;
    initParam.internalChannelMap = ADC12_B_TEMPSENSEMAP;
    ADC12_B_init(ADC12_B_BASE, &initParam);

    // Enable the ADC12B module
    ADC12_B_enable(ADC12_B_BASE);

    // Sets up the sampling timer pulse mode
    ADC12_B_setupSamplingTimer(ADC12_B_BASE, ADC12_B_CYCLEHOLD_128_CYCLES, ADC12_B_CYCLEHOLD_128_CYCLES, ADC12_B_MULTIPLESAMPLESDISABLE);

    /*
     * Maps Temperature Sensor input channel to Memory 0 and select voltage references Base address of the ADC12B Module Configure memory buffer 0 Map input A1 to memory buffer 0 Vref+ = IntBuffer Vref- = AVss Memory buffer 0 is not the end of a sequence
     */
    configureMemoryParam.memoryBufferControlIndex = ADC12_B_MEMORY_0;
    configureMemoryParam.inputSourceSelect = ADC12_B_INPUT_TCMAP;
    configureMemoryParam.refVoltageSourceSelect = ADC12_B_VREFPOS_INTBUF_VREFNEG_VSS;
    configureMemoryParam.endOfSequence = ADC12_B_NOTENDOFSEQUENCE;
    configureMemoryParam.windowComparatorSelect = ADC12_B_WINDOW_COMPARATOR_DISABLE;
    configureMemoryParam.differentialModeSelect = ADC12_B_DIFFERENTIAL_MODE_DISABLE;
    ADC12_B_configureMemory(ADC12_B_BASE, &configureMemoryParam);

    /*
    // Clear memory buffer 0 interrupt
    ADC12_B_clearInterrupt(ADC12_B_BASE,
                           0,
                           ADC12_B_IFG0
                           );

    // Enable memory buffer 0 interrupt
    ADC12_B_enableInterrupt(ADC12_B_BASE,
                            ADC12_B_IE0,
                            0,
                            0);
     */

    // Configure internal reference
    while(Ref_A_isRefGenBusy(REF_A_BASE));              // If ref generator busy, WAIT
    Ref_A_enableTempSensor(REF_A_BASE);
    Ref_A_setReferenceVoltage(REF_A_BASE, REF_A_VREF2_5V);
    Ref_A_enableReferenceVoltage(REF_A_BASE);

    /*
     * Enable/Start sampling and conversion Base address of ADC12B Module Start the conversion into memory buffer 0 Use the single-channel, single-conversion mode
     */
    ADC12_B_startConversion(ADC12_B_BASE, ADC12_B_MEMORY_0, ADC12_B_SINGLECHANNEL);

    current_temp = *(uint16_t*)(0x860);
    // Disable ADC12 and Timer_A0
    ADC12_B_disable(ADC12_B_BASE);
    return current_temp;
}
