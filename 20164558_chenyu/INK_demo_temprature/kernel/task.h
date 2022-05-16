
#ifndef TASK_H_
#define TASK_H_
#include <stdint.h>
#include <stdlib.h>

#define MAX_TASK_NUM        20
#define MAX_THREAD_NUM      2

#define __nv            __attribute__((section(".nv_vars")))

typedef struct {
    volatile uint8_t idx;
    volatile uint8_t _idx;
}buffer_idx_t;

// each thread will hold the double buffer for the variables shared by the tasks it is encapsulating.
typedef struct {
    void *buf[2];                   // holds original and temporary stack pointers
    uint16_t size;                  // sizes of the buffers
    volatile uint8_t idx;
    volatile uint8_t _idx;
} buffer_t;

// TASK structure
typedef struct {
    void    *fun_entry;             //function entry
    uint8_t task_idx;
} task_t;

// the main thread structure that holds all necessary info to execute the computation represented by the wired tasks
typedef struct {
    uint8_t  priority;                  // thread priority (unique)
    buffer_t buffer;                    // holds task shared persistent variables, must in the first.
    uint16_t CurrTaskId;
    uint8_t  idx_of_first_empty_task;   //index of the first empty location in teb array
    task_t   task_array[MAX_TASK_NUM];
} thread_t;

// allocates a double buffer for the persistent variables in FRAM
#define __shared(...)                                   \
        typedef struct {                                \
            __VA_ARGS__                                 \
        } FRAM_data_t  __attribute__ ((aligned (2)));   \
        static __nv FRAM_data_t __persistent_vars[2];

// use to declare a TASK
#define TASK(name)  static void * name(void *__buffer)

// The task definition (single C function)
typedef uint8_t (*taskfun_t) (buffer_t *);

// use to init. a TASK
void __init_task(uint8_t priority, void *task_entry);
#define TASK_INIT(priority, name) \
        __init_task(priority, (void *)&name)

// use to init. a THREAD
void __create_thread(uint8_t priority, void *data_org, void *data_temp, uint16_t size);
#define THREAD_INIT(priority) \
        __create_thread(priority, (void *)&__persistent_vars[0], (void *)&__persistent_vars[1], sizeof(FRAM_data_t));  \
        memset((void *)&__persistent_vars[1], 0, sizeof(FRAM_data_t));\
        memset((void *)&__persistent_vars[0], 0, sizeof(FRAM_data_t))

// reads the value from the original stack
#define __GET(x) ((FRAM_data_t *)__buffer)->x

// writes the value to the temporary stack
#define __SET(x) ((FRAM_data_t *)__buffer)->x

//return next TASK Index
#define NEXT(id)  return (uint16_t)id

// runs one task inside the current thread.
void __tick(thread_t *thread);

extern thread_t _threads[MAX_THREAD_NUM];

#endif /* TASK_H_ */
