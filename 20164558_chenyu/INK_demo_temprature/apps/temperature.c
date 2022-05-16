#include <driverlib.h>
#include <ink.h>
#include <msp430.h>
#include <stdint.h>
#include "apps.h"

#define TASK_PRI 0
extern uint16_t liveTemp();

void uart_warning(uint16_t uTempValue){
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, '$');
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, 'W');
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)(uTempValue));
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)(uTempValue>>8));
}

void uart_debug(uint16_t uTempValue){
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, '!');
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, 'D');
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)(uTempValue));
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)(uTempValue>>8));
}
/**
 * 1. TASK declaration here.
 */
TASK(task_init);
TASK(task_get_temp);//TODO: simulate or measure
TASK(task_knn);
TASK(task_warning);
TASK(task_debug);
TASK(task_finish);


/**
 * 2. Shared variable declaration here. (206 bytes)
 */
__shared(
uint16_t uCount;    // round number
uint16_t uCurrTemp;
uint16_t uTempData[10];
)


/**
 * 3. TASK definition here.
 */
//0
TASK(task_init){
    __SET(uCount) = 0;
    int i = 0;
    for(i=0;i<10;i++){
        __SET(uTempData[i]) = 0x0650;
    }
    NEXT(1);
}
//1
TASK(task_get_temp){
    int i = 0;
    uint16_t curr_temp;
    i = rand()%21;
    if(i<18){
        curr_temp = liveTemp()+(rand()%40-20);
    }else{
        curr_temp = 0x8650; //set a fake value to minmic warning.
    }
    __SET(uCurrTemp) = curr_temp;
    NEXT(2);
}
//2
TASK(task_knn){
    //res = knn();
    uint16_t uAve = 0;
    uint32_t ulSum = 0;
    int i;
    __delay_cycles(10000000);
    for(i=0;i<10;i++){
        ulSum += __GET(uTempData[i]);
    }
    uAve = ulSum/10;
    uAve = uAve + (uAve>>1);

    if(__GET(uCurrTemp)<uAve){
        __SET(uTempData[__GET(uCount)%10]) = __GET(uCurrTemp);
        NEXT(4);//normal
    }else{
        NEXT(3);//warning
    }
}
//3
TASK(task_warning){
    uart_warning(__GET(uCurrTemp));
    __delay_cycles(1000000);
    P1OUT ^= 0b000011;               //Set P1.4, Turn both LEDs on
    __delay_cycles(1000000);
    P1OUT ^= 0b000011;               //Set P1.4, Turn both LEDs on
    __delay_cycles(1000000);
    P1OUT ^= 0b000011;               //Set P1.4, Turn both LEDs on
    __delay_cycles(1000000);
    P1OUT ^= 0b000011;               //Set P1.4, Turn both LEDs on
    __delay_cycles(1000000);
    NEXT(4);
}
//4
TASK(task_debug){
    __delay_cycles(10000000);
    uart_debug(__GET(uCurrTemp));
    NEXT(5);
}
//5
TASK(task_finish){
    __delay_cycles(10000000);
    __SET(uCount)++;
    NEXT(1);
}


void _demo_fire_warning(){
    THREAD_INIT(TASK_PRI);
    TASK_INIT(TASK_PRI, task_init);      //0
    TASK_INIT(TASK_PRI, task_get_temp);  //1
    TASK_INIT(TASK_PRI, task_knn);       //2
    TASK_INIT(TASK_PRI, task_warning);   //3
    TASK_INIT(TASK_PRI, task_debug);     //4
    TASK_INIT(TASK_PRI, task_finish);    //5
}
