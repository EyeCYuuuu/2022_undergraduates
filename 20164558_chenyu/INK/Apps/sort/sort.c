#include <driverlib.h>
#include <Kernel/ink.h>
#include <Kernel/scheduler/thread.h>
#include <profile.h>

#include <msp430.h>
#include <stdint.h>

#include "./sort.h"

#ifndef RST_TIME
#define RST_TIME 25000
#endif

/**
 * 1. TEB declaration here.
 */
TEB(task_init);
TEB(task_inner_loop);
TEB(task_outer_loop);
TEB(task_finish);
TEB(new_task);

/**
 * 2. Shared variable declaration here. (206 bytes)
 */
__shared(
//#if (OUR1||INK0||INK1||INK_J||INK1J)  //A-mem: 1234
uint16_t array[LENGTH];
uint16_t outer_idx;
uint16_t inner_idx;
uint8_t iteration;
//#endif
#if (OUR2)              //O-mem: 1324
uint16_t array[LENGTH];
uint16_t inner_idx;
uint16_t outer_idx;
uint8_t iteration;
#endif
)

uint16_t in_i, in_j, arr_i, arr_j;

/**
 * 3. TEB definition here.
 */
TEB(task_init)//0
{
    __SET(outer_idx) = 0;
    __SET(inner_idx) = 1;

    const uint16_t* array_pt;

    ++__SET(iteration);
    if (__GET(iteration) & 0x01) {
        array_pt = a2;      //original a1. by srliu
    } else {
        array_pt = a2;
    }

    uint16_t idx;
    for (idx = 0; idx < LENGTH; idx++) {
        __SET(array[idx]) = array_pt[idx];
    }

    NEXT(1);
}


TEB(task_inner_loop)//1
{
    uint16_t i, j, x_i, x_j, temp;

    i = __GET(outer_idx);
    j = __GET(inner_idx);

    x_i = __GET(array[i]);
    x_j = __GET(array[j]);

    if (x_i > x_j) {
        temp = x_j;
        x_j =  x_i;
        x_i =  temp;
    }

    __SET(array[i]) = x_i;
    __SET(array[j]) = x_j;
    NEXT(4);
    /*++__SET(inner_idx);

    if (__GET(inner_idx) < LENGTH) {
        NEXT(1);
    } else {
        NEXT(2);
    }*/
}




TEB(task_outer_loop)//2
{
    ++__SET(outer_idx);
    __SET(inner_idx) = __GET(outer_idx) + 1;

    if (__GET(outer_idx) < LENGTH - 1) {
        NEXT(1);
    } else {
        NEXT(3);
    }
}


TEB(task_finish)//3
{
    NEXT(0);
}
TEB(new_task)//4
{

    ++__SET(inner_idx);

    if (__GET(inner_idx) < LENGTH) {
        NEXT(1);
    } else {
        NEXT(2);
    }
}


void _benchmark_sort_init()
{
    TASK(TASK_PRI);

    TEB_INIT(TASK_PRI, task_init, 64, may_war_set_sort[0][0], may_war_set_sort[0][1], teb_breaking_sort[0]);       //0
    TEB_INIT(TASK_PRI, task_inner_loop, 5, may_war_set_sort[1][0], may_war_set_sort[1][1], teb_breaking_sort[1]); //1
    TEB_INIT(TASK_PRI, task_outer_loop, 3, may_war_set_sort[2][0], may_war_set_sort[2][1], teb_breaking_sort[2]); //2
    TEB_INIT(TASK_PRI, task_finish, 1, may_war_set_sort[3][0], may_war_set_sort[3][1], teb_breaking_sort[3]);    //3
    TEB_INIT(TASK_PRI, new_task, 1, may_war_set_sort[4][0], may_war_set_sort[4][1], teb_breaking_sort[4]);
    __SIGNAL(1);
}
