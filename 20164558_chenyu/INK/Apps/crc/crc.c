#include <apps.h>

#include <msp430.h>
#include "ink.h"
/** Driver includes. */
#include <driverlib.h>
#include "HAL_UART.h"
#include "scheduler/thread.h"
#include "ink.h"
//#include "may_war_profiles.h"
//#include "ar.h"
// define TASK priority here.
#define TASK_PRI    1


//extern const uint32_t may_war_set_crc[NUM_TEB_CRC][2];




const uint32_t may_war_set_crc[NUM_TEB_CRC][2]={
#if (OUR2) // optimized memory layout
    {0,0},{0,2},{0,2},{0,0}
#endif
#if (OUR1||INK1||INK1J) //actual memory layout
    {0,0},{0,2},{0,2},{0,0}
#endif
#if (INK_J||INK0) // page-wised memory layout copy on right
    {0,2},{0,2},{0,2},{0,2}
#endif
#if (DOUFIF) // optimized memory layout
    {0,0},{0,1},{0,2},{0,0}
#endif
};

const uint8_t teb_breaking_crc[NUM_TEB_CRC]={
#if (OUR1||OUR2)    //breaking
    0, 0, 0, 0
#endif
#if (INK_J||INK1J)           //no-breaking
    0, 0, 0, 0
#endif
#if (INK0||INK1||DOUFIF)          //all-breaking
    1, 1, 1, 1
#endif
};

#define RAISE_PIN

#define DATA_LEN 48
const unsigned int CRC_Init = 0xFFFF;
const unsigned int CRC_Input[] = {
                                  0x0fc0, 0x1096, 0x5042, 0x0010,           // 16 random 16-bit numbers
                                  0x7ff7, 0xf86a, 0xb58e, 0x7651,           // these numbers can be
                                  0x8b88, 0x0679, 0x0123, 0x9599,           // modified if desired
                                  0xc58c, 0xd1e2, 0xe144, 0xb691,
                                  0x0fc0, 0x1096, 0x5042, 0x0010,           // 16 random 16-bit numbers
                                  0x7ff7, 0xf86a, 0xb58e, 0x7651,           // these numbers can be
                                  0x8b88, 0x0679, 0x0123, 0x9599,           // modified if desired
                                  0xc58c, 0xd1e2, 0xe144, 0xb691,
                                  0x0fc0, 0x1096, 0x5042, 0x0010,           // 16 random 16-bit numbers
                                  0x7ff7, 0xf86a, 0xb58e, 0x7651,           // these numbers can be
                                  0x8b88, 0x0679, 0x0123, 0x9599,           // modified if desired
                                  0xc58c, 0xd1e2, 0xe144, 0xb691,
                                  0x0fc0, 0x1096, 0x5042, 0x0010,           // 16 random 16-bit numbers
                                  0x7ff7, 0xf86a, 0xb58e, 0x7651,           // these numbers can be
                                  0x8b88, 0x0679, 0x0123, 0x9599,           // modified if desired
                                  0xc58c, 0xd1e2, 0xe144, 0xb691
};

__shared(
 unsigned int SW_Results;                    // Holds results
 unsigned int cnt;
 )

// Debug defines and flags
#define DEBUG_PORT 3
#define DEBUG_PIN  5

__nv uint8_t full_run_started_crc = 0;


#define TASK_NUM 4

TEB(initTask);
TEB(firstByte);
TEB(secondByte);
TEB(task_finish);

unsigned int CCITT_Update(unsigned int init, unsigned int input);



TEB(initTask)
{
#ifdef TSK_SIZ
    cp_reset();
#endif

    full_run_started_crc = 1;
    __SET(cnt) = 0;
    __SET(SW_Results) = CRC_Init;

#ifdef TSK_SIZ
    cp_sendRes("initTask \0");
#endif

    NEXT(1);
}

TEB(firstByte)//1
{
#ifdef TSK_SIZ
       cp_reset();
#endif

    // First input lower byte
    __SET(SW_Results) = CCITT_Update(__GET(SW_Results), CRC_Input[__GET(cnt)] & 0xFF);

#ifdef TSK_SIZ
    cp_sendRes("firstByte \0");
#endif

    NEXT(2);
}

TEB(secondByte)//2
{

#ifdef TSK_SIZ
       cp_reset();
#endif

    // Then input upper byte
    __SET(SW_Results) = CCITT_Update(__GET(SW_Results), (CRC_Input[__GET(cnt)] >> 8) & 0xFF);
    __SET(cnt)++;

   /* msp_gpio_set(DEBUG_PORT, DEBUG_PIN);
    coala_partial_commit();
    msp_gpio_clear(DEBUG_PORT, DEBUG_PIN);
    msp_gpio_set(DEBUG_PORT, DEBUG_PIN);
    coala_partial_commit();
    msp_gpio_clear(DEBUG_PORT, DEBUG_PIN);*/

    if (__GET(cnt) < DATA_LEN) {
        NEXT(1);
        //return;
    }

#ifdef TSK_SIZ
    cp_sendRes("secondByte \0");
#endif

    NEXT(3);
}

TEB(task_finish)
{
#ifdef TSK_SIZ
    cp_reset();
#endif

    //coala_force_commit(); // force a commit after this task

    if (full_run_started_crc) {
        // msp_gpio_spike(DEBUG_PORT, DEBUG_PIN);
        full_run_started_crc = 0;
    }

#ifdef TSK_SIZ
    cp_sendRes("task_finish \0");
#endif

    NEXT(0);
}

// Software algorithm - CCITT CRC16 code
unsigned int CCITT_Update(unsigned int init, unsigned int input)
{
    unsigned int CCITT = (unsigned char) (init >> 8) | (init << 8);
    CCITT ^= input;
    CCITT ^= (unsigned char) (CCITT & 0xFF) >> 4;
    CCITT ^= (CCITT << 8) << 4;
    CCITT ^= ((CCITT & 0xFF) << 4) << 1;
    return CCITT;
}

void _benchmark_crc_init(void)
{
    TASK(TASK_PRI);
    unsigned int i;

    // our optimized memory layout
    //for(i = 0 ; i< TASK_NUM; i++ ){
        TEB_INIT(TASK_PRI,initTask,5,may_war_set_crc[0][0],may_war_set_crc[0][1],teb_breaking_crc[0]);
        TEB_INIT(TASK_PRI,firstByte,5,may_war_set_crc[1][0],may_war_set_crc[1][1],teb_breaking_crc[1]);
        TEB_INIT(TASK_PRI,secondByte,5,may_war_set_crc[2][0],may_war_set_crc[2][1],teb_breaking_crc[2]);
        TEB_INIT(TASK_PRI,task_finish,5,may_war_set_crc[3][0],may_war_set_crc[3][1],teb_breaking_crc[3]);
    //}
    //printf("Benchmark-crc\n");
    // our actual memorry layout
    /*
    for(i = 0 ; i< TASK_NUM; i++ ){
            TEB_INIT(TASK_PRI,initTask,1,may_war_set_cem_act[i][0],may_war_set_cem_act[i][1],teb_breaking_cem[i]);
    }*/


/*
    TEB_INIT(TASK_PRI,init, 3,0,4,0);
    TEB_INIT(TASK_PRI,selectMode, 9,0,4,0);
    TEB_INIT(TASK_PRI,resetStats, 3,0,4,0);
    TEB_INIT(TASK_PRI,sample, 26,0,4,0);
    TEB_INIT(TASK_PRI,transform, 8,0,4,0);
    TEB_INIT(TASK_PRI,featurize, 30,0,4,0);
    TEB_INIT(TASK_PRI,classify, 50,0,4,0);
    TEB_INIT(TASK_PRI,stats, 3,0,4,0);
    TEB_INIT(TASK_PRI,warmup, 13,0,4,0);
    TEB_INIT(TASK_PRI,train, 7,0,4,0);
    TEB_INIT(TASK_PRI,idle, 1,0,4,0);
    //TEB_INIT(TASK_PRI,,2,0,4,0);*/

    __SIGNAL(1);
}


