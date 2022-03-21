#include <apps.h>

#include <msp430.h>
/** Driver includes. */
#include <driverlib.h>
#include "HAL_UART.h"
#include "scheduler/thread.h"
#include "ink.h"
#include <apps.h>
//#include "ar.h"
// define TASK priority here.
#define TASK_PRI    1

#if ENABLE_PRF
__nv uint8_t full_run_started = 0;
__nv uint8_t first_run = 1;
#endif

#ifndef RST_TIME
#define RST_TIME 25000
#endif

extern uint32_t lowestaddr;
extern uint32_t highestaddr;

//#define __SET
#define doufif 1
const uint32_t may_war_set_cem[NUM_TEB_CEM][2]={
#if (OUR2||OUR1_S) // optimized memory layout
    {384,7},{384,7},{384,6},{386,5},{388,4},
    {388,5},{394,5},{400,1},{404,5},{384,8},
    {384,7},{384,7}
#endif
#if (DOUFIF)
    {0,0},{384,1},{386,6},{388,1},{0,0},
    {396,1},{398,1},{398,1},{392,1},{390,1},
    {0,0},{0,0}
#endif
/*#if (OUR1||INK1) //actual memory layout
    {384,13},{384,13},{384,13},{386,13},{386,7},
    {384,13},{384,9},{398,1},{392,12},{384,13},
    {384,13},{384,13}
#endif*/
#if INK1J
    {384,18},{384,1554},{384,1554},{384,1554},{384,1554},
    {384,1554},{384,1554},{384,1554},{384,1554},{384,1554},
    {0,210},{384,18}
#endif
#if (TSN) // page-wised memory layout copy on right
    {384,16},{384,32},{384,32},{384,32},{384,32},
    {384,16},{384,16},{384,16},{384,32},{0,32},
    {384,16},{384,16}
#endif
#if (INK0||INK_J)
    {0,1746},{0,1746},{0,1746},{0,1746},{0,1746},
    {0,1746},{0,1746},{0,1746},{0,1746},{0,1746},
    {0,1746},{0,1746}
#endif
};


const uint8_t teb_breaking_cem[NUM_TEB_CEM]={
#if (alpha)
   0, 0, 0, 0, 0,
   0, 1, 0, 1, 0,
   0, 1
#endif
/*#if (OUR1||OUR2||OUR1_S)    //breaking
    0, 0, 0, 0, 0,
    0, 0, 0, 1, 0,
    0, 1
#endif*/
#if (INK_J||INK1J)           //no-breaking
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0
#endif
#if (INK0||INK1||DOUFIF)          //all-breaking
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1
#endif
};


#define NIL 0 // like NULL, but for indexes, not real pointers

#define DICT_SIZE         512
#define BLOCK_SIZE         64

#define NUM_LETTERS_IN_SAMPLE        2
#define LETTER_MASK             0x000F
#define LETTER_SIZE_BITS             8
#define NUM_LETTERS (LETTER_MASK + 1)


typedef uint16_t index_t;
typedef uint16_t letter_t;
typedef uint16_t sample_t;

// NOTE: can't use pointers, since need to ChSync, etc
typedef struct _node_t {
    letter_t letter; // 'letter' of the alphabet
    index_t sibling; // this node is a member of the parent's children list
    index_t child;   // link-list of children
} node_t;

// Tasks.
TEB(init);
TEB(init_dict);
TEB(sample);
TEB(measure_temp);
TEB(letterize);
TEB(compress);
TEB(find_sibling);
TEB(add_node);
TEB(add_insert);
TEB(append_compressed);
TEB(print);
TEB(done);


__shared(   //3520
#if (OUR2)
 node_t _v_compressed_data[BLOCK_SIZE];
unsigned _v_letter_idx;
sample_t _v_prev_sample;
index_t _v_parent_next;
index_t _v_sample_count;
 letter_t _v_letter;
 index_t _v_out_len;
 index_t _v_sibling;
 index_t _v_child;
 index_t _v_node_count;
 index_t _v_parent;
 node_t _v_parent_node;
 sample_t _v_sample;
 node_t _v_sibling_node;
 index_t _v_symbol;
 node_t _v_dict[DICT_SIZE];
#else //optimized memory layout
 node_t _v_compressed_data[BLOCK_SIZE];
 letter_t _v_letter;
 unsigned _v_letter_idx;
 sample_t _v_prev_sample;
 index_t _v_out_len;
 index_t _v_node_count;
 sample_t _v_sample;
 index_t _v_sample_count;
 index_t _v_sibling;
 index_t _v_child;
 index_t _v_parent;
 index_t _v_parent_next;
 node_t _v_parent_node;
 node_t _v_sibling_node;
 index_t _v_symbol;
 node_t _v_dict[DICT_SIZE];
#endif
 )


static sample_t acquire_sample(letter_t prev_sample)
{
    letter_t sample = (prev_sample + 1) & 0x03;
    return sample;
}


TEB(init)//0
{
#if TSK_SIZ
    cp_reset();
#endif

#if ENABLE_PRF
    full_run_started = 1;
#endif

    __SET(_v_parent_next) = 0;
    __SET(_v_out_len) = 0;
    __SET(_v_letter) = 0;
    __SET(_v_prev_sample) = 0;
    __SET(_v_letter_idx) = 0;
    __SET(_v_sample_count) = 1;

    NEXT(1);

#if TSK_SIZ
    cp_sendRes("task_init \0");
#endif
}


TEB(init_dict)
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t i = __GET(_v_letter);

    __SET(_v_dict[i].letter) = i ;
    __SET(_v_dict[i].sibling) =  NIL;
    __SET(_v_dict[i].child) = NIL;
    __SET(_v_letter)++;
    if (i < NUM_LETTERS) {
        NEXT(1);
    } else {
        __SET(_v_node_count) =  NUM_LETTERS;
        NEXT(2);
    }


#if TSK_SIZ
    cp_sendRes("task_init_dict \0");
#endif
}


TEB(sample)//2
{
#if TSK_SIZ
    cp_reset();
#endif

    unsigned next_letter_idx = __GET(_v_letter_idx) + 1;
    if (next_letter_idx == NUM_LETTERS_IN_SAMPLE)
        next_letter_idx = 0;

    if (__GET(_v_letter_idx) == 0) {
        __SET(_v_letter_idx) = next_letter_idx;
        NEXT(3);
    } else {
        __SET(_v_letter_idx) = next_letter_idx;
        NEXT(4);
    }

#if TSK_SIZ
    cp_sendRes("task_sample \0");
#endif
}


TEB(measure_temp)//3
{
#if TSK_SIZ
    cp_reset();
#endif

    sample_t prev_sample;
    prev_sample = __GET(_v_prev_sample);

    sample_t sample = acquire_sample(prev_sample);
    prev_sample = sample;
    __SET(_v_prev_sample) = prev_sample;
    __SET(_v_sample) = sample;
    NEXT(4);

#if TSK_SIZ
    cp_sendRes("task_measure_temp \0");
#endif
}


TEB(letterize)//4
{
#if TSK_SIZ
    cp_reset();
#endif

    unsigned letter_idx = __GET(_v_letter_idx);
    if (letter_idx == 0)
        letter_idx = NUM_LETTERS_IN_SAMPLE;
    else
        letter_idx--;

    unsigned letter_shift = LETTER_SIZE_BITS * letter_idx;
    letter_t letter = (__GET(_v_sample) & (LETTER_MASK << letter_shift)) >> letter_shift;

    __SET(_v_letter) = letter;
    NEXT(5);

#if TSK_SIZ
    cp_sendRes("task_letterize \0");
#endif
}


TEB(compress)//5
{
#if TSK_SIZ
    cp_reset();
#endif

    // pointer into the dictionary tree; starts at a root's child
    index_t parent = __GET(_v_parent_next);

    uint16_t __cry;

    __cry = __GET(_v_dict[parent].child);
    __SET(_v_sibling) = __cry ;
    __cry = __GET(_v_dict[parent].letter);
    __SET(_v_parent_node.letter) =  __cry;
    __cry = __GET(_v_dict[parent].sibling);
    __SET(_v_parent_node.sibling) = __cry;
    __cry = __GET(_v_dict[parent].child);
    __SET(_v_parent_node.child) = __cry;
    __SET(_v_parent) = parent;
    __cry = __GET(_v_dict[parent].child);
    __SET(_v_child) = __cry;
    __SET(_v_sample_count)++;

    NEXT(6);

#if TSK_SIZ
    cp_sendRes("task_compress \0");
#endif
}


TEB(find_sibling)//6
{
#if TSK_SIZ
    cp_reset();
#endif

    if (__GET(_v_sibling) != NIL) {
        int i = __GET(_v_sibling);

        uint16_t __cry = __GET(_v_letter);
        if (__GET(_v_dict[i].letter) == __cry ) { // found

            __cry = __GET(_v_sibling);
            __SET(_v_parent_next) = __cry;

            NEXT(4);
            return;
        } else { // continue traversing the siblings
            if(__GET(_v_dict[i].sibling) != 0){
                __cry = __GET(_v_dict[i].sibling);
                __SET(_v_sibling) = __cry;
                NEXT(6);
                return;
            }
        }

    }

    index_t starting_node_idx = (index_t)__GET(_v_letter);
    __SET(_v_parent_next) = starting_node_idx;

    if (__GET(_v_child) == NIL) {
        NEXT(8);
    }
    else {
        NEXT(7);
    }

#if TSK_SIZ
    cp_sendRes("task_find_sibling \0");
#endif
}


TEB(add_node)//7
{
#if TSK_SIZ
    cp_reset();
#endif

    int i = __GET(_v_sibling);


    if (__GET(_v_dict[i].sibling) != NIL) {
        index_t next_sibling = __GET(_v_dict[i].sibling);
        __SET(_v_sibling) = next_sibling;
        NEXT(7);

    } else { // found last sibling in the list

        uint16_t __cry;

        __cry = __GET(_v_dict[i].letter);
        __SET(_v_sibling_node.letter) = __cry;
        __cry = __GET(_v_dict[i].sibling);
        __SET(_v_sibling_node.sibling) = __cry;
        __cry = __GET(_v_dict[i].child);
        __SET(_v_sibling_node.child) = __cry;

        NEXT(8);
    }

#if TSK_SIZ
    cp_sendRes("task_add_node \0");
#endif
}


TEB(add_insert)//8
{
#if TSK_SIZ
    cp_reset();
#endif

    if (__GET(_v_node_count) == DICT_SIZE) { // wipe the table if full
        while (1);
    }

    index_t child = __GET(_v_node_count);
    uint16_t __cry;
    if (__GET(_v_parent_node.child) == NIL) { // the only child

        int i = __GET(_v_parent);

        __cry = __GET(_v_parent_node.letter);
        __SET(_v_dict[i].letter) = __cry;
        __cry  = __GET(_v_parent_node.sibling);
        __SET(_v_dict[i].sibling) = __cry;
        __cry = child;
        __SET(_v_dict[i].child) = __cry;

    } else { // a sibling

        index_t last_sibling = __GET(_v_sibling);

        __cry = __GET(_v_sibling_node.letter);
        __SET(_v_dict[last_sibling].letter) = __cry;
        __cry = child;
        __SET(_v_dict[last_sibling].sibling) = __cry;
        __cry  = __GET(_v_sibling_node.child);
        __SET(_v_dict[last_sibling].child) = __cry;
    }
    __cry = __GET(_v_letter);
    __SET(_v_dict[child].letter) = __cry;
    __SET(_v_dict[child].sibling) = NIL;
    __SET(_v_dict[child].child) = NIL;
    __cry = __GET(_v_parent);
    __SET(_v_symbol) = __cry;
    __SET(_v_node_count)++;

    NEXT(9);

#if TSK_SIZ
    cp_sendRes("task_add_insert \0");
#endif
}


TEB(append_compressed)//9
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t __cry;
    int i = __GET(_v_out_len);
    __cry = __GET(_v_symbol);
    __SET(_v_compressed_data[i].letter) = __cry;
    __SET(_v_out_len)++;

    if ( (__GET(_v_out_len)) == BLOCK_SIZE) {
        NEXT(10);
    } else {
        NEXT(2);
    }

#if TSK_SIZ
    cp_sendRes("task_append_compressed \0");
#endif
}


TEB(print)//10
{
#if TSK_SIZ
    cp_reset();
#endif

    unsigned i;

    //for (i = 0; i < BLOCK_SIZE; ++i) {
      //   index_t index = __GET(_v_compressed_data[i].letter);
        // printf("%u, ", index);
     //}
    //printf("\n");

    NEXT(11);

#if TSK_SIZ
    cp_sendRes("task_print \0");
#endif
}


TEB(done)
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

#if TSK_SIZ
    cp_sendRes("task_done \0");
#endif
}
//}




void _benchmark_cem_init(void)
{
    TASK(TASK_PRI);
    //TEB_INIT(TASK_PRI,initTask,5,may_war_set_crc[0][0],may_war_set_crc[0][1],teb_breaking_crc[0]);

    TEB_INIT(TASK_PRI,init, 5, may_war_set_cem[0][0], may_war_set_cem[0][1], teb_breaking_cem[0]);
    TEB_INIT(TASK_PRI,init_dict, 12,may_war_set_cem[1][0],may_war_set_cem[1][1],teb_breaking_cem[1]);
    TEB_INIT(TASK_PRI,sample, 4,may_war_set_cem[2][0],may_war_set_cem[2][1],teb_breaking_cem[2]);
    TEB_INIT(TASK_PRI,measure_temp, 4,may_war_set_cem[3][0],may_war_set_cem[3][1],teb_breaking_cem[3]);
    TEB_INIT(TASK_PRI,letterize, 6,may_war_set_cem[4][0],may_war_set_cem[4][1],teb_breaking_cem[4]);
    TEB_INIT(TASK_PRI,compress, 18,may_war_set_cem[5][0],may_war_set_cem[5][1],teb_breaking_cem[5]);
    TEB_INIT(TASK_PRI,find_sibling, 6,may_war_set_cem[6][0],may_war_set_cem[6][1],teb_breaking_cem[6]);
    TEB_INIT(TASK_PRI,add_node, 8,may_war_set_cem[7][0],may_war_set_cem[7][1],teb_breaking_cem[7]);
    TEB_INIT(TASK_PRI,add_insert, 22,may_war_set_cem[8][0],may_war_set_cem[8][1],teb_breaking_cem[8]);
    TEB_INIT(TASK_PRI,append_compressed, 6,may_war_set_cem[9][0],may_war_set_cem[9][1],teb_breaking_cem[9]);
    TEB_INIT(TASK_PRI,print, 1,may_war_set_cem[10][0],may_war_set_cem[10][1],teb_breaking_cem[10]);
    TEB_INIT(TASK_PRI,done, 1,may_war_set_cem[11][0],may_war_set_cem[11][1],teb_breaking_cem[11]);


    __SIGNAL(1);
}

