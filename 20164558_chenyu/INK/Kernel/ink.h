// This file is part of InK.
// 
// author = "Kasım Sinan Yıldırım " 
// maintainer = "Kasım Sinan Yıldırım "
// email = "sinanyil81 [at] gmail.com" 
//  
// copyright = "Copyright 2018 Delft University of Technology" 
// license = "LGPL" 
// version = "3.0" 
// status = "Production"
//
// 
// InK is free software: you ca	n redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/*
 * ink.h
 *
 *  Created on: 14 Feb 2018
 *
 */

#ifndef INK_H_
#define INK_H_

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <driverlib.h>

#include "mcu/mcu.h"
#include "scheduler/scheduler.h"
#include "isr/isr.h"
#include "channel/channel.h"
#include "timer/timer.h"




typedef unsigned char                                       UINT8;
typedef unsigned short                                      UINT16;
typedef unsigned long                                       UINT32;
typedef float                                               FLOAT;
typedef double                                              DOUBLE;
typedef char                                                CHAR;
//#ifndef BOOL
//typedef unsigned int                                        BOOL;
//#endif
typedef unsigned long long                                  UINT64;
typedef signed long long                                    INT64;
typedef unsigned int                                        UINTPTR;
typedef signed int                                          INTPTR;
#ifndef NULL
#define NULL                                                ((VOID *)0)
#endif
#ifndef LOS_OK
#define LOS_OK                                              (0U)
#endif

#ifndef LOS_NOK
#define LOS_NOK                                             (1U)
#endif
/*
* Module settings
*/

//#define WKUP_TIMER
//#define XPR_TIMER
//#define TIMERS_ON

#endif /* INK_H_ */
