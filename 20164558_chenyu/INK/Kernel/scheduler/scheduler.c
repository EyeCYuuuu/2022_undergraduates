
/*
 * scheduler.c
 *
 *  Created on: 12 Feb 2018
 *
 */

#include "ink.h"
#include "scheduler.h"
#include "priority.h"
#include "profile.h"

// all threads in the system
static __nv thread_t _threads[MAX_THREADS];

#if PRF_TIME
extern float recover_time;
#endif

// variables for keeping track of the ready threads
static __nv priority_t _priorities;

enum { SCHED_SELECT, SCHED_BUSY };

// the id of the current thread being executed.
static __nv thread_t *_thread = NULL;

static volatile __nv uint8_t _sched_state = SCHED_SELECT;

extern void __dma_word_copy(unsigned int from, unsigned int to, unsigned short size);

void __scheduler_boot_init() {
    uint8_t i;

    // clear priority variables for the threads
    __priority_init(&_priorities);

    for (i = MAX_THREADS; i > 0; i--){
        // threads are not created yet
        _threads[i].state == THREAD_STOPPED;
    }
    _sched_state = SCHED_SELECT;
}

// Assigns a slot to a thread. Should be called ONLY at the first system boot
void __create_thread(uint8_t priority, void *data_org,
                     void *data_temp,void *data_ori, uint16_t size)
{
    // init properties
    _threads[priority].priority = priority;
    //_threads[priority].entry = entry;
    //_threads[priority].next = entry;
    _threads[priority].state = THREAD_STOPPED;
    _threads[priority].CurTebId = 0;

    // init shared buffer
    _threads[priority].buffer.buf[0] = data_org;
    _threads[priority].buffer.buf[1] = data_temp;
    _threads[priority].buffer.buf[2] = data_ori;
    _threads[priority].buffer.idx = 0;
    _threads[priority].buffer.size = size;

    // by srliu for emsoft-paper
    _threads[priority].idx_of_first_empty_teb = 0;
}

// Init. a new TEB
// @para. addr_length: in word(2bytes)
void __init_teb(uint8_t priority, void *teb_entry, uint8_t budget, uint32_t addr_offset, uint32_t addr_length,uint8_t breaking)
{
    uint16_t temp_index;

    temp_index = _threads[priority].idx_of_first_empty_teb;
    _threads[priority].teb_array[temp_index].breaking = breaking;
    _threads[priority].teb_array[temp_index].budget = budget;
    _threads[priority].teb_array[temp_index].fun_entry = teb_entry;
    _threads[priority].teb_array[temp_index].teb_idx = temp_index;
    _threads[priority].teb_array[temp_index].may_war.range_offset_addr_begin = addr_offset;
    _threads[priority].teb_array[temp_index].may_war.range_size = addr_length;

    _threads[priority].idx_of_first_empty_teb++;
}



// puts the thread in waiting state
inline void __stop_thread(thread_t *thread){
    __priority_remove(thread->priority, &_priorities);
    thread->state = THREAD_STOPPED;
}

// puts the thread in waiting state
void __evict_thread(thread_t *thread){
    __priority_remove(thread->priority, &_priorities);
    //thread->next = NULL;
    thread->state = THREAD_STOPPED;
}

void __set_sing_timer(thread_t *thread,uint16_t timing){
    thread->sing_timer = timing;
    return;
}

//TODO: update necessary
void __set_expr_timer(thread_t *thread,uint16_t timing){
    thread->expr_timer = timing;
    return;
}


void __set_pdc_timer(thread_t *thread,uint16_t timing){
    thread->pdc_timer = timing;
    return;
}

void __set_pdc_period(thread_t *thread,uint16_t period){
    thread->pdc_period = period;
    return;
}

uint16_t __get_pdc_timer(thread_t *thread){
    return thread->pdc_timer;
}

uint16_t __get_pdc_period(thread_t *thread){
    return thread->pdc_period;
}

// puts the thread in active state
inline void __start_thread(thread_t *thread) {
    thread->CurTebId = 0;
    __priority_insert(thread->priority, &_priorities);
    thread->state = TASK_READY;
}

// returns the highest-priority thread
static inline thread_t *__next_thread(){
    uint8_t idx = __priority_highest(&_priorities);
    if(idx)
        return &_threads[idx];

    return NULL;
}

inline thread_t *__get_thread(uint8_t priority){
    return &_threads[priority];
}

// finish the interrupted task before enabling interrupts
static inline void __task_commit(){
    if(_thread){
        __tick(_thread);
    }
}

static inline void __undo(thread_t *thread)
{
    may_war_t *may_war = &(thread->teb_array[thread->CurTebId].may_war);

    buffer_t *bufferTemp = (buffer_t *)thread;

    // copy bakup buffer to the working buffer
    __dma_word_copy((uint32_t)bufferTemp->buf[1]+(uint32_t)may_war->range_offset_addr_begin,\
                    (uint32_t)bufferTemp->buf[0]+(uint32_t)may_war->range_offset_addr_begin,\
                    may_war->range_size);

    while(HWREG16(DMA_BASE + DMA_CHANNEL_0 + OFS_DMA1CTL)){
    }
}

// at each step, the scheduler selects the highest priority thread and
// runs the next task within the thread

volatile __nv uint16_t recover_time_temp = 0;

void __scheduler_run()
{
#if PRF_TIME
    int recover_start,recover_end;
#endif
    uint8_t flagFirst = 0;
    while(1){
        switch (_sched_state){
        case SCHED_SELECT:
            // the scheduler selects the highest priority task right
            // after it has finished the execution of a single task
            _thread = __next_thread();
            _sched_state = SCHED_BUSY;
        case SCHED_BUSY:
            // always execute the selected task to completion
            // execute one task inside the highest priority thread
            while(1){
            if (_thread){
#if PRF_TIME
                //calculate recovery time
                PRB_START(recover_start);
#endif
                if(flagFirst){
                    __undo(_thread);
                }
                flagFirst = 1;
#if PRF_TIME
                PRB_END(recover_end);
                recover_time_temp = (recover_end - recover_start)/16.0;
                recover_time += recover_time_temp;
                //recovery time end
#endif
                __tick(_thread);
            }
            }
            _sched_state = SCHED_SELECT;
            __disable_interrupt();
            // check the ready queue for the last time
            if(!__next_thread()){
                __mcu_sleep();
                //__enable_interrupt();
            }
        }
    }
}
