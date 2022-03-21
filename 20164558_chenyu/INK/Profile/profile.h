/*
 * profile.h
 *
 *  Created on: 22 Mar 2020
 *      Author: elk
 */

#ifndef PROFILE_PROFILE_H_
#define PROFILE_PROFILE_H_

#include <msp430.h>
#include <profile.h>
#include <driverlib.h>
#include <stdio.h>
#include <HAL_UART.h>

#define uprint UART_transmitString

#define PRF_BUDGET      0
#define PRF_DEPLET      0
#define PRF_AB_TIME     0               //measure ab time
#define PRF_TIME        1               //!!!print evaluation time
#define PRF_ITR         10

/**--------------------------------------------
 * Energy trace setting.
 * */
#define ALWAYS_ON       0

#if (ALWAYS_ON)
#define POWER_DOWN_FRQ          0                   //DONOT change @this option.
#define JITTER_AMP              0
#define JITTER_PROB             0
#else
#define POWER_DOWN_PERIOD    32000                   //1s ~1000times: 1ms~1time
#define JITTER_AMP          (POWER_DOWN_PERIOD/10)     //jitter amplitude.
#define JITTER_PROB         0                       //0:Uniform distribution; 1:Normal distribution
#endif

#define unstableenergy  1
#define trace14  1
#define tracep28 0





/**--------------------------------------------
 * Evaluation case
 * */
//#define TSN     0   //A-mem+ no-breaking+ page-wise W(32 64 128 256)
#define OUR1    0//!!!A-mem+ breaking+ con-byte-wise-WAR
#define INK1J   0
#define INK_J   0
#define INK1    0//!!!A-mem+ all-breaking+ con-byte-wise-WAR
#define INK0   0//!!!A-mem+ all-breaking+ all-W
//#define NEVEROFF        1

#define DOUFIF  1

#define OUR1_S  0
#define OUR2    0   //O-mem+ breaking+ con-byte-wise-WAR
/**--------------------------------------------
 * For profiling TSN strategies.(TSN1,TSN2,TSN3)
 * */
#define PRF_ENERGY_EST  0
#define PRF_AB_CFG      0
#define PRF_TRACKING    0

#define PRB_START(start)    \
        Timer_A_startCounter( TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE );\
        start = Timer_A_getCounterValue(TIMER_A1_BASE)

#define PRB_END(end)    \
        end = Timer_A_getCounterValue(TIMER_A1_BASE);\
        Timer_A_stop(TIMER_A1_BASE);\
        Timer_A_clear(TIMER_A1_BASE)



/**------------------------------------------
 * TSN AB merging strategies.
 * !!! MUST choose one&only one !!!
 */
#define TSN1    0   //number-counting
#define TSN2    0   //
#define TSN3    0

#define ROU     (0.5)
#define GAMMA   (0.5)


int fputc(int _c, register FILE *_fp);
int fputs(const char *_ptr, register FILE *_fp);

#endif /* PROFILE_PROFILE_H_ */
