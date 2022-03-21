#include <apps.h>

#include <msp430.h>
/** Driver includes. */
#include <driverlib.h>
#include "HAL_UART.h"
#include "scheduler/thread.h"
#include "ink.h"
//#include "ar.h"
// define TASK priority here.
#define TASK_PRI    1

#if ENABLE_PRF
__nv uint8_t full_run_started = 0;
__nv uint8_t first_run = 1;
#endif



extern const uint32_t may_war_set_bc[NUM_TEB_BC][2]={
#if (OUR2) // optimized memory layout
    {0,0},{0,8},{0,11},{0,11},{0,11},
    {0,11},{0,11},{0,11},{0,11},{0,0}
#endif
#if (DOUFIF) // optimized memory layout
    {0,0},{14,1},{0,11},{2,10},{4,9},
    {6,8},{8,7},{10,6},{12,5},{0,0}
#endif
#if (OUR1||INK1) //actual memory layout
    {0,0},{0,8},{0,11},{0,11},{0,11},
    {0,11},{0,11},{0,11},{0,11},{0,0}
#endif
#if (INK_J||INK0||INK1J) // page-wised memory layout copy on right
    {0,11},{0,11},{0,11},{0,11},{0,11},
    {0,11},{0,11},{0,11},{0,11},{0,11}
#endif
};
extern const uint8_t teb_breaking_bc[NUM_TEB_BC]={
#if (OUR1||OUR2)    //breaking
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
#endif
#if (INK_J||INK1J)           //no-breaking
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
#endif
#if (INK0||INK1||DOUFIF)          //all-breaking
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1
#endif
};

#ifndef RST_TIME
#define RST_TIME 25000
#endif


#define SEED 4L
#define ITER 100
#define CHAR_BIT 8

__nv static char bits[256] =
{
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,  /* 0   - 15  */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,  /* 16  - 31  */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,  /* 32  - 47  */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,  /* 48  - 63  */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,  /* 64  - 79  */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,  /* 80  - 95  */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,  /* 96  - 111 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,  /* 112 - 127 */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,  /* 128 - 143 */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,  /* 144 - 159 */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,  /* 160 - 175 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,  /* 176 - 191 */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,  /* 192 - 207 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,  /* 208 - 223 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,  /* 224 - 239 */
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8   /* 240 - 255 */
};

// Tasks.
TEB(init);
TEB(select_func);
TEB(bit_count);
TEB(bitcount);
TEB(ntbl_bitcnt);
TEB(ntbl_bitcount);
TEB(BW_btbl_bitcount);
TEB(AR_btbl_bitcount);
TEB(bit_shifter);
TEB(end);

// Task-shared protected variables.
__shared(
 unsigned _v_n_0;
 unsigned _v_n_1;
 unsigned _v_n_2;
 unsigned _v_n_3;
 unsigned _v_n_4;
 unsigned _v_n_5;
 unsigned _v_n_6;
 unsigned _v_func; //16
 uint32_t _v_seed;
 unsigned _v_iter;
 )


TEB(init)//0
{
#if TSK_SIZ || EXECUTION_TIME
    cp_reset();
#endif

#if ENABLE_PRF
    full_run_started = 1;
#endif

    __SET(_v_func) = 0;
    __SET(_v_n_0) = 0;
    __SET(_v_n_1) = 0;
    __SET(_v_n_2) = 0;
    __SET(_v_n_3) = 0;
    __SET(_v_n_4) = 0;
    __SET(_v_n_5) = 0;
    __SET(_v_n_6) = 0;

    NEXT(1);

#if TSK_SIZ
    cp_sendRes("task_init \0");
#endif
}


TEB(select_func)//1
{
#if TSK_SIZ
    cp_reset();
#endif

    __SET(_v_seed) = (uint32_t) SEED; // for testing, seed is always the same
    __SET(_v_iter) = 0;
    if (__GET(_v_func) == 0) {
        __SET(_v_func)++;
        NEXT(2);
    }
    else if (__GET(_v_func) == 1) {
        __SET(_v_func)++;
        NEXT(3);
    }
    else if (__GET(_v_func) == 2) {
        __SET(_v_func)++;
        NEXT(4);
    }
    else if (__GET(_v_func) == 3) {
        __SET(_v_func)++;
        NEXT(5);
    }
    else if (__GET(_v_func) == 4) {
        __SET(_v_func)++;
        NEXT(6);
    }
    else if (__GET(_v_func) == 5) {
        __SET(_v_func)++;
        NEXT(7);
    }
    else if (__GET(_v_func) == 6) {
        __SET(_v_func)++;
        NEXT(8);
    }
    else {
        NEXT(9);
    }

#if TSK_SIZ
    cp_sendRes("task_select_func \0");
#endif
}


TEB(bit_count)//2
{
#if TSK_SIZ
    cp_reset();
#endif

    uint32_t tmp_seed = __GET(_v_seed);
    __SET(_v_seed) = tmp_seed + 13;
    unsigned temp = 0;

    if (tmp_seed) do
        temp++;
    while (0 != (tmp_seed = tmp_seed & (tmp_seed - 1)));

    __SET(_v_n_0) += temp;
    __SET(_v_iter)++;

    if (__GET(_v_iter) < ITER) {
        NEXT(2);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_bit_count \0");
#endif
}


TEB(bitcount)//3
{
#if TSK_SIZ
    cp_reset();
#endif

    uint32_t tmp_seed = __GET(_v_seed);
    __SET(_v_seed) = tmp_seed + 13;

    tmp_seed = ((tmp_seed & 0xAAAAAAAAL) >>  1) + (tmp_seed & 0x55555555L);
    tmp_seed = ((tmp_seed & 0xCCCCCCCCL) >>  2) + (tmp_seed & 0x33333333L);
    tmp_seed = ((tmp_seed & 0xF0F0F0F0L) >>  4) + (tmp_seed & 0x0F0F0F0FL);
    tmp_seed = ((tmp_seed & 0xFF00FF00L) >>  8) + (tmp_seed & 0x00FF00FFL);
    tmp_seed = ((tmp_seed & 0xFFFF0000L) >> 16) + (tmp_seed & 0x0000FFFFL);

    __SET(_v_n_1) += (int)tmp_seed;
    __SET(_v_iter)++;

    if (__GET(_v_iter) < ITER) {
        NEXT(3);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_bitcount \0");
#endif
}


int recursive_cnt(uint32_t x)
{
    int cnt = bits[(int)(x & 0x0000000FL)];

    if (0L != (x >>= 4))
        cnt += recursive_cnt(x);

    return cnt;
}

int non_recursive_cnt(uint32_t x)
{
    int cnt = bits[(int)(x & 0x0000000FL)];

    while (0L != (x >>= 4)) {
        cnt += bits[(int)(x & 0x0000000FL)];
    }

    return cnt;
}

TEB(ntbl_bitcnt)//4
{
#if TSK_SIZ
    cp_reset();
#endif

    // TRICK ALERT!
    uint32_t tmp_seed = __GET(_v_seed);
    __SET(_v_n_2) += non_recursive_cnt(tmp_seed);
    __SET(_v_seed) = tmp_seed + 13;
    __SET(_v_iter)++;

    if (__GET(_v_iter) < ITER) {
        NEXT(4);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_ntbl_bitcnt \0");
#endif
}


TEB(ntbl_bitcount)//5
{
#if TSK_SIZ
    cp_reset();
#endif
    // TRICK ALERT!
    uint16_t __cry = __GET(_v_seed);

    __SET(_v_n_3) +=
        bits[ (int) (__cry & 0x0000000FUL)] +
        bits[ (int)((__cry & 0x000000F0UL) >> 4) ] +
        bits[ (int)((__cry & 0x00000F00UL) >> 8) ] +
        bits[ (int)((__cry & 0x0000F000UL) >> 12)] +
        bits[ (int)((__cry & 0x000F0000UL) >> 16)] +
        bits[ (int)((__cry & 0x00F00000UL) >> 20)] +
        bits[ (int)((__cry & 0x0F000000UL) >> 24)] +
        bits[ (int)((__cry & 0xF0000000UL) >> 28)];

    // TRICK ALERT!
    uint32_t tmp_seed = __GET(_v_seed);
    __SET(_v_seed) = tmp_seed + 13;
    __SET(_v_iter)++;

    if (__GET(_v_iter) < ITER) {
        NEXT(5);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_ntbl_bitcount \0");
#endif
}


TEB(BW_btbl_bitcount)//6
{
#if TSK_SIZ
    cp_reset();
#endif

    union {
        unsigned char ch[4];
        long y;
    } U;

    U.y = __GET(_v_seed);

    __SET(_v_n_4) += bits[ U.ch[0] ] + bits[ U.ch[1] ] +
        bits[ U.ch[3] ] + bits[ U.ch[2] ];

    // TRICK ALERT!
    uint32_t tmp_seed = __GET(_v_seed);
    __SET(_v_seed) = tmp_seed + 13;
    __SET(_v_iter)++;

    if (__GET(_v_iter) < ITER) {
        NEXT(6);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_BW_btbl_bitcount \0");
#endif
}


TEB(AR_btbl_bitcount)//7
{
#if TSK_SIZ
    cp_reset();
#endif

    unsigned char * Ptr = (unsigned char *) &__GET(_v_seed);
    int Accu;

    Accu  = bits[ *Ptr++ ];
    Accu += bits[ *Ptr++ ];
    Accu += bits[ *Ptr++ ];
    Accu += bits[ *Ptr ];
    __SET(_v_n_5)+= Accu;

    // TRICK ALERT!
    uint32_t tmp_seed = __GET(_v_seed);
    __SET(_v_seed) = tmp_seed + 13;
    __SET(_v_iter)++;

    if (__GET(_v_iter) < ITER) {
        NEXT(7);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_AR_btbl_bitcount \0");
#endif
}

TEB(bit_shifter)//8
{
#if TSK_SIZ
    cp_reset();
#endif

    unsigned i, nn;
    uint32_t tmp_seed = __GET(_v_seed);

    for (i = nn = 0; tmp_seed && (i < (sizeof(long) * CHAR_BIT)); ++i, tmp_seed >>= 1)
        nn += (unsigned)(tmp_seed & 1L);

    __SET(_v_n_6) += nn;

    // TRICK ALERT!
    tmp_seed = __GET(_v_seed);
    tmp_seed += 13;
    __SET(_v_seed) = tmp_seed;

    __SET(_v_iter)++;

    if (__GET(_v_iter) < ITER) {
        NEXT(8);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_bit_shifter \0");
#endif
}

TEB(end)//9
{
#if TSK_SIZ
    cp_reset();
#endif

#if ENABLE_PRF
    if (full_run_started) {
#if AUTO_RST
        msp_reseter_halt();
#endif
        PRF_APP_END();
        full_run_started = 0;
        coala_force_commit();
#if AUTO_RST
        msp_reseter_resume();
#endif
    }
#endif

    NEXT(0);

#if EXECUTION_TIME
cp_sendRes("bc");
uart_sendHex16(fullpage_fault_counter);
uart_sendStr("\n\r\0");
uart_sendHex16(page_fault_counter);
uart_sendStr("\n\r\0");
#endif

#if TSK_SIZ
    cp_sendRes("task_end \0");
#endif
}

//void init()
//{
  //  msp_watchdog_disable();
   // msp_gpio_unlock();

#if ENABLE_PRF
    PRF_INIT();
    PRF_POWER();
#endif

    // msp_clock_set_mclk(CLK_8_MHZ);

#if TSK_SIZ || EXECUTION_TIME
    uart_init();
    cp_init();
#endif

#if LOG_INFO
    uart_init();
#endif

#if AUTO_RST
    msp_reseter_auto_rand(RST_TIME);
#endif

#if ENABLE_PRF
    if (first_run) {
        PRF_APP_BGN();
        first_run = 0;
    }
#endif
//}




void _benchmark_bc_init(void)
{
    TASK(TASK_PRI);

    //may_war_set_bc[0][1],teb_breaking_bc[0]);

    TEB_INIT(TASK_PRI,init, 7,may_war_set_bc[0][0],may_war_set_bc[0][1],teb_breaking_bc[0]);
    TEB_INIT(TASK_PRI,select_func, 7,may_war_set_bc[1][0],may_war_set_bc[1][1],teb_breaking_bc[1]);
    TEB_INIT(TASK_PRI,bit_count, 12,may_war_set_bc[2][0],may_war_set_bc[2][1],teb_breaking_bc[2]);
    TEB_INIT(TASK_PRI,bitcount, 16,may_war_set_bc[3][0],may_war_set_bc[3][1],teb_breaking_bc[3]);
    TEB_INIT(TASK_PRI,ntbl_bitcnt, 15,may_war_set_bc[4][0],may_war_set_bc[4][1],teb_breaking_bc[4]);
    TEB_INIT(TASK_PRI,ntbl_bitcount, 12,may_war_set_bc[5][0],may_war_set_bc[5][1],teb_breaking_bc[5]);
    TEB_INIT(TASK_PRI,BW_btbl_bitcount, 10,may_war_set_bc[6][0],may_war_set_bc[6][1],teb_breaking_bc[6]);
    TEB_INIT(TASK_PRI,AR_btbl_bitcount, 10,may_war_set_bc[7][0],may_war_set_bc[7][1],teb_breaking_bc[7]);
    TEB_INIT(TASK_PRI,bit_shifter, 15,may_war_set_bc[8][0],may_war_set_bc[8][1],teb_breaking_bc[8]);
    TEB_INIT(TASK_PRI,end, 5,may_war_set_bc[9][0],may_war_set_bc[9][1],teb_breaking_bc[9]);
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

