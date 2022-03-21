
/*
 * task.h
 *
 *  Created on: 12 Feb 2018
 *
 */

#ifndef TASK_H_
#define TASK_H_

#define TEB(name)  static void * name(void *__buffer)
// TODO: refine. by srliu for emsoft-paper
#define TEB_INIT(priority, name, budget, addr_offset, addr_length, breaking) \
        __init_teb(priority, (void *)&name, (uint8_t)budget, (uint32_t)addr_offset, (uint32_t)addr_length, breaking)

#define ENTRY_TEB(name)  static void *name(void *__buffer)

// reads the value from the original stack
#define __GET(x) ( (FRAM_data_t *)__buffer)->x

// returns the address of the variable
#define __GET_ADDR(x) &( (FRAM_data_t *)__buffer)->x

// writes the value to the temporary stack
#define __SET(x) ( (FRAM_data_t *)__buffer)->x
//return next TEB zw 2019-11-16
#define NEXT(id)  return (uint16_t)id
// creates a thread -- by srliu
#define TASK(priority)  \
        __create_thread(priority,(void *)&__persistent_vars[0],(void *)&__persistent_vars[1],(void *)&__persistent_vars[2],sizeof(FRAM_data_t))

// puts the thread state into ACTIVE
#define __SIGNAL(priority) \
        __disable_interrupt();  \
        __start_thread(__get_thread(priority)); \
        __enable_interrupt()

// event related information
#define __EVENT __event
#define __EVENT_DATA __event->data
#define __EVENT_DATALEN __event->size
#define __EVENT_TIME __event->timestamp

#endif /* TASK_H_ */
