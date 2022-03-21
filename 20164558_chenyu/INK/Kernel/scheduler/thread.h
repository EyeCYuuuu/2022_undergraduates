
/*
 * thread.h
 *
 *  Created on: 12 Feb 2018
 *
 */

#ifndef THREAD_H_
#define THREAD_H_

// by srliu for emsoft-paper
#define MAX_TEB_NUM     20


// the state of the threads
typedef enum {TASK_READY = 1, TASK_TEBFINISH =2,THREAD_STOPPED = 16} state_t;
    //TASK_FINISHED = 4, TASK_COMMIT = 8,THREAD_STOPPED = 16} state_t;

// each thread will hold the double buffer for the variables
// shared by the tasks it is encapsulating.
typedef struct {
    void *buf[3];                   // holds original and temporary stack pointers
    volatile uint8_t idx;           // index of the original buffer
    volatile uint8_t _idx;          // index of the new buffer
    uint16_t size;                  // sizes of the buffers
}buffer_t;

// by srliu for emsoft-paper
typedef struct {
    uint32_t range_offset_addr_begin;          // address of May(war) set beginning
    uint32_t range_size;           // address of May(war) set ending
}may_war_t;

// the task definition (single C function)
// the parameter param will be passed by the run-time
// and it holds the thread structure defined below.
typedef void* (*task_t) (buffer_t *);

//typedef void* (*task_t) (void);

// the entry task should take event data as an argument.
typedef void* (*entry_task_t) (buffer_t *,void *event);

//by srliu
typedef struct {
    void *fun_entry;        //function entry
    uint8_t teb_idx;
    may_war_t may_war;      //May(war) set
    uint8_t budget;         //energy budget
    uint8_t breaking;        //1:this AB is a breaking AB; 0: it is not a breaking AB.
}teb_t;

// the main thread structure that holds all necessary info
// to execute the computation represented by the wired
// tasks
typedef struct {
    buffer_t buffer;                // holds task shared persistent variables, must in the first.
    uint8_t priority;               // thread priority (unique)
    volatile state_t state;         // thread state
    //void *entry;                    // the first task to be executed
    //void *next;                     // the current task to be executed
    uint16_t sing_timer;            // holds the time when the thread will be executed
    uint16_t pdc_timer;             // holds the time for "periodic" execution of the thread
    uint16_t expr_timer;            // hold the expiration time of the thread from time of completion
    uint16_t pdc_period;            // holds the current period
    uint16_t CurTebId;

    //by srliu for emsoft-paper
    uint8_t idx_of_first_empty_teb; //index of the first empty location in teb array
    teb_t teb_array[MAX_TEB_NUM];
    //uint16_t breakingAB[MAX_TEB_NUM];
}thread_t;



// allocates a double buffer for the persistent variables in FRAM
#define __shared(...)   \
        typedef struct {    \
            __VA_ARGS__     \
        } FRAM_data_t  __attribute__ ((aligned (2)));    \
        static __nv FRAM_data_t __persistent_vars[3];

// runs one task inside the current thread.
void __tick(thread_t *thread);


#endif /* THREAD_H_ */
