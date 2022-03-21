/*
 * File: demo.h
 * Project: LiteOS-intermitter
 * File Created: Tuesday, 24th September 2019 4:50:15 pm
 * Author: srLiu (liusongran@outlook.com)
 * -----
 * Last Modified: Tuesday, 24th September 2019 4:50:18 pm
 * Modified By: srLiu (liusongran@outlook.com>)
 * -----
 * Functionality:
 */
#ifndef SRC_APPS_INCLUDE_DEMO_CONFIG_H_
#define SRC_APPS_INCLUDE_DEMO_CONFIG_H_

#define configENABLE_SELFTEST           0
#define configENABLE_BLINKTEST          1

#define TSK0DL              1000        //delay time.
#define TSK1DL              900
#define TSK2DL              1800
#define TSK3DL              3700
#define TSK4DL              5600
#define TSK5DL              7500

#define TSK0WCET            500         //TODO: test!!!
#define TSK1WCET            100         //100ms
#define TSK2WCET            200
#define TSK3WCET            300
#define TSK4WCET            400
#define TSK5WCET            500

#if configENABLE_BLINKTEST
#include "blink.h"
#include <driverlib.h>
#include "HAL_UART.h"
#else
#include "image.h"
#endif
//UINT32 tsk3redstillinit();
//UINT32 tsk4greenstillinit();

//UINT32 tsk5bothstillinit();
//UINT32 tsk0Imageinit();

#endif /* SRC_APPS_INCLUDE_DEMO_H_ */
