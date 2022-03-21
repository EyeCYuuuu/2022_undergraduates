

#include <msp430.h>
#include "ink.h"
/** Driver includes. */
#include <driverlib.h>
#include "HAL_UART.h"
//#include "ar.h"
#include "scheduler/thread.h"
#include "ink.h"
#include "apps.h"
// define TASK priority here.
#define TASK_PRI    1

#if ENABLE_PRF
__nv uint8_t full_run_started = 0;
__nv uint8_t first_run = 1;
#endif

#ifndef RST_TIME
#define RST_TIME 25000
#endif

extern const uint32_t may_war_set_cuckoo[NUM_TEB_CUCKOO][2]={
#if (OUR2) // optimized memory layout
    {0,0},{0,0},{3,6},{3,6},{5,5},
    {3,6},{5,5},{0,139},{7,136},{7,5},
    {5,5},{0,3},{2,6},{0,0},{0,0},{7,136}
#endif
#if (DOUFIF) // optimized memory layout
    {0,0},{256,1},{258,1},{0,0},{0,0},
    {0,0},{0,0},{0,128},{0,136},{272,2},
    {0,0},{276,3},{0,0},{0,0},{0,0},{0,0}
#endif
#if (OUR1||INK1) //actual memory layout
    {0,0},{0,0},{258,12},{258,12},{258,12},
    {258,12},{258,12},{0,141},{0,141},{270,6},
    {258,12},{258,13},{258,13},{0,0},{0,0},{0,141}
#endif
#if (INK1J) //actual memory layout
    {256,5},{256,13},{0,141},{0,141},{0,141},
    {0,141},{0,141},{0,141},{0,141},{0,141},
    {0,141},{0,141},{0,142},{0,142},{256,14},{0,141}
#endif
#if (INK_J||INK0) // page-wised memory layout copy on right
    {0,142},{0,142},{0,142},{0,142},{0,142},
    {0,142},{0,142},{0,142},{0,142},{0,142},
    {0,142},{0,142},{0,142},{0,142},{0,142},{0,142}
#endif
};
extern const uint8_t teb_breaking_cuckoo[NUM_TEB_CUCKOO]={
#if (OUR1||OUR2)    //breaking
    1, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 0, 0,1
#endif
#if (INK_J||INK1J)           //no-breaking
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,0
#endif
#if (INK0||INK1||DOUFIF)          //all-breaking
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,1
#endif
};




#define NUM_BUCKETS 128 // must be a power of 2
#define NUM_INSERTS (NUM_BUCKETS / 4) // shoot for 25% occupancy
#define NUM_LOOKUPS NUM_INSERTS
#define MAX_RELOCATIONS 8
#define BUFFER_SIZE 32

typedef uint16_t value_t;
typedef uint16_t hash_t;
typedef uint16_t fingerprint_t;
typedef uint16_t index_t; // bucket index

typedef struct _insert_count {
    unsigned insert_count;
    unsigned inserted_count;
} insert_count_t;

typedef struct _lookup_count {
    unsigned lookup_count;
    unsigned member_count;
} lookup_count_t;

// Tasks.
TEB(init);
TEB(generate_key);
TEB(calc_indexes);
TEB(calc_indexes_index_1);
TEB(calc_indexes_index_2);
TEB(insert);
TEB(add);
TEB(relocate);
TEB(insert_done);
TEB(lookup);
TEB(lookup_search);
TEB(lookup_done);
TEB(print_stats);
TEB(done);
TEB(dummy);
// NOT USED.
//TEB(task_init_array);

// Task-shared protected variables.
__shared(
#if(OUR2)
    value_t _v_index2;
    bool _v_member;
    void* _v_next_task;
    value_t _v_key;
    value_t _v_member_count;
    value_t _v_lookup_count;
    value_t _v_insert_count;
    fingerprint_t _v_fingerprint;
    value_t _v_index1;
 fingerprint_t _v_filter[NUM_BUCKETS];
 value_t _v_relocation_count;
 value_t _v_inserted_count;
 index_t _v_index;
 bool _v_success;
#else
 fingerprint_t _v_filter[NUM_BUCKETS];
 index_t _v_index;
 value_t _v_key;
 void* _v_next_task;
 fingerprint_t _v_fingerprint;
 value_t _v_index1;
 value_t _v_index2;
 value_t _v_relocation_count;
 value_t _v_insert_count;
 value_t _v_inserted_count;
 value_t _v_lookup_count;
 value_t _v_member_count;
 bool _v_success;
 bool _v_member;
#endif
 )


static value_t init_key = 0x0001; // seeds the pseudo-random sequence of keys

static hash_t djb_hash(uint8_t* data, unsigned len)
{
    uint16_t hash = 5381;
    unsigned int i;

    for (i = 0; i < len; data++, i++)
        hash = ((hash << 5) + hash) + (*data);

    return hash & 0xFFFF;
}

static index_t hash_to_index(fingerprint_t fp)
{
    hash_t hash = djb_hash((uint8_t *) &fp, sizeof(fingerprint_t));
    return hash & (NUM_BUCKETS - 1); // NUM_BUCKETS must be power of 2
}

static fingerprint_t hash_to_fingerprint(value_t key)
{
    return djb_hash((uint8_t *) &key, sizeof(value_t));
}


TEB(init)//0
{
#if TSK_SIZ || EXECUTION_TIME
    cp_reset();
#endif

#if ENABLE_PRF
    full_run_started = 1;
#endif

    unsigned i;
    for (i = 0; i < NUM_BUCKETS ; ++i) {
        __SET(_v_filter[i]) = 0;
    }

    __SET(_v_insert_count) = 0;
    __SET(_v_lookup_count) = 0;
    __SET(_v_inserted_count) = 0;
    __SET(_v_member_count) = 0;
    __SET(_v_key) = init_key;
    __SET(_v_next_task) = insert;

    NEXT(2);

#if TSK_SIZ
    cp_sendRes("task_init \0");
#endif
}


TEB(init_array)//1
{
#if TSK_SIZ
    cp_reset();
#endif

    unsigned i;
    for (i = 0; i < BUFFER_SIZE - 1; ++i) {
        __SET(_v_filter[i + __GET(_v_index)*(BUFFER_SIZE-1)]) = 0;
    }

    __SET(_v_index)++;
    if (__GET(_v_index) == NUM_BUCKETS / (BUFFER_SIZE - 1)) {
        NEXT(2);
    }
    else {
        NEXT(1);
    }

#if TSK_SIZ
    cp_sendRes("task_init_array \0");
#endif
}


TEB(generate_key)//2
{
#if TSK_SIZ
    cp_reset();
#endif

    // Insert pseudo-random integers, for testing.
    // If we use consecutive ints, they hash to consecutive DJB hashes...
    // NOTE: we are not using rand(), to have the sequence available to verify
    // that there are no false negatives (and avoid having to save the values).

    uint16_t __cry;

    __cry = (__GET(_v_key) + 1) * 17;
    __SET(_v_key) = __cry;

    void* next_task = __GET(_v_next_task);

    if (next_task == insert) {
        NEXT(6);
    } else if (next_task == lookup) {
        NEXT(10);
    } else {
        while(1); // Debugging pu__GETose
    }

#if TSK_SIZ
   cp_sendRes("task_generate_key \0");
#endif
}


TEB(calc_indexes)//3
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t __cry;
    __cry = hash_to_fingerprint(__GET(_v_key));
    __SET(_v_fingerprint) = __cry;

    NEXT(4);

#if TSK_SIZ
    cp_sendRes("task_calc_indexes \0");
#endif
}


TEB(calc_indexes_index_1)//4
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t __cry;
    __cry = hash_to_index(__GET(_v_key));
    __SET(_v_index1) = __cry;

    NEXT(5);

#if TSK_SIZ
    cp_sendRes("task_calc_indexes_index_1 \0");
#endif
}


TEB(calc_indexes_index_2)//5
{
#if TSK_SIZ
    cp_reset();
#endif

    index_t fp_hash = hash_to_index(__GET(_v_fingerprint));
    uint16_t __cry;
    __cry = __GET(_v_index1) ^ fp_hash;
    __SET(_v_index2) = __cry;

    void* next_task = __GET(_v_next_task);

    if (next_task == add) {
        NEXT(7);//task_add
    } else if (next_task == lookup_search) {
        NEXT(11);//task_lookup_search
    } else {
        while(1); // Debugging pu__GETose
    }

#if TSK_SIZ
    cp_sendRes("task_calc_indexes_index_2 \0");
#endif
}


// This task is redundant.
// Alpaca never needs this but since Chain code had it, leaving it for fair comparison.
TEB(insert)//6
{
#if TSK_SIZ
    cp_reset();
#endif

    __SET(_v_next_task) = add;
    NEXT(3);//task_calc_indexes

#if TSK_SIZ
    cp_sendRes("task_insert \0");
#endif
}


TEB(add)//7
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t __cry;
    uint16_t __cry_idx = __GET(_v_index1);
    uint16_t __cry_idx2 = __GET(_v_index2);

    if (!__GET(_v_filter[__cry_idx])) {
        __SET(_v_success) = true;
        __cry = __GET(_v_fingerprint);
        __SET(_v_filter[__cry_idx]) = __cry;
        NEXT(9);//task_insert_done
    } else {
        if (!__GET(_v_filter[__cry_idx2])) {
            __SET(_v_success) = true;
            __cry = __GET(_v_fingerprint);
            __SET(_v_filter[__cry_idx2])  = __cry;
            NEXT(9);//task_insert_done
        } else { // evict one of the two entries
            fingerprint_t fp_victim;
            index_t index_victim;

            if (rand() % 2) {
                index_victim = __cry_idx;
                fp_victim = __GET(_v_filter[__cry_idx]);
            } else {
                index_victim = __cry_idx2;
                fp_victim = __GET(_v_filter[__cry_idx2]);
            }

            // Evict the victim
            __cry = __GET(_v_fingerprint);
            __SET(_v_filter[index_victim])  = __cry;
            __SET(_v_index1) = index_victim;
            __SET(_v_fingerprint) = fp_victim;
            __SET(_v_relocation_count) = 0;

            NEXT(8);//relocate dummy node
        }
    }

#if TSK_SIZ
    cp_sendRes("task_add \0");
#endif
}


TEB(relocate)//8
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t __cry;
    fingerprint_t fp_victim = __GET(_v_fingerprint);
    index_t fp_hash_victim = hash_to_index(fp_victim);
    index_t index2_victim = __GET(_v_index1) ^ fp_hash_victim;

    if (!__GET(_v_filter[index2_victim])) { // slot was free
        __SET(_v_success) = true;
        __SET(_v_filter[index2_victim]) = fp_victim;
        NEXT(9);//task_insert_done
    } else { // slot was occupied, rellocate the next victim
        if (__GET(_v_relocation_count) >= MAX_RELOCATIONS) { // insert failed
            __SET(_v_success) = false;
            NEXT(9);//task_insert_done
            //return;
        }

        __SET(_v_relocation_count)++;
        __SET(_v_index1) = index2_victim;
        __cry = __GET(_v_filter[index2_victim]);
        __SET(_v_fingerprint) = __cry;
        __SET(_v_filter[index2_victim]) = fp_victim;
        NEXT(8);//task_relocate
    }

#if TSK_SIZ
    cp_sendRes("task_relocate \0");
#endif
}


TEB(insert_done)//9
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t __cry;
    __SET(_v_insert_count)++;
    __cry = __GET(_v_inserted_count);
    __cry+= __GET(_v_success);
    __SET(_v_inserted_count) = __cry;

    if (__GET(_v_insert_count) < NUM_INSERTS) {
        __SET(_v_next_task) = insert;
        NEXT(2);//task_generate_key
    } else {
        __SET(_v_next_task) = lookup;
        __SET(_v_key) = init_key;
        NEXT(2);//task_generate_key
    }

#if TSK_SIZ
    cp_sendRes("task_insert_done \0");
#endif
}


TEB(lookup)//10
{
#if TSK_SIZ
    cp_reset();
#endif

    __SET(_v_next_task) = lookup_search;
    NEXT(3);//task_calc_indexes

#if TSK_SIZ
    cp_sendRes("task_lookup \0");
#endif
}


TEB(lookup_search)//11
{
#if TSK_SIZ
    cp_reset();
#endif

    if (__GET(_v_filter[__GET(_v_index1)]) == __GET(_v_fingerprint)) {
        __SET(_v_member) = true;
    } else {
        if (__GET(_v_filter[__GET(_v_index2)]) == __GET(_v_fingerprint)) {
            __SET(_v_member) = true;
        } else {
            __SET(_v_member) = false;
        }
    }

    NEXT(12);//lookup_done

#if TSK_SIZ
    cp_sendRes("task_lookup_search \0");
#endif
}


TEB(lookup_done)
{
#if TSK_SIZ
    cp_reset();
#endif

    uint16_t __cry;
    __SET(_v_lookup_count)++;
    __cry = __GET(_v_member_count) ;
    __cry+= __GET(_v_member);
    __SET(_v_member_count)  = __cry;

    if (__GET(_v_lookup_count) < NUM_LOOKUPS) {
        __SET(_v_next_task) = lookup;
        NEXT(2);
    } else {
        NEXT(13);
    }

#if TSK_SIZ
    cp_sendRes("task_lookup_done \0");
#endif
}


TEB(print_stats)//13
{
    // Print output here
/*
     unsigned i;
     for (i = 0; i < NUM_BUCKETS; ++i) {
         printf("%04x ", __GET(_v_filter[i]));
         if (i > 0 && (i + 1) % 8 == 0){
             printf("\n");
         }
     }
*/
    __no_operation();

    NEXT(14);
}


TEB(done)//14
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
#if AUTO_RST
        msp_reseter_resume();
#endif
    }
#endif

#if EXECUTION_TIME
cp_sendRes("cuckoo");
uart_sendHex16(fullpage_fault_counter);
uart_sendStr("\n\r\0");
uart_sendHex16(page_fault_counter);
uart_sendStr("\n\r\0");
#endif
    NEXT(0);

#if TSK_SIZ
    cp_sendRes("task_done \0");
#endif
}


TEB(dummy)//15
{
    NEXT(8);
}



void _benchmark_cuckoo_init(void)
{
    TASK(TASK_PRI);
    //may_war_set_cuckoo[0][0],may_war_set_cuckoo[0][1],teb_breaking_cuckoo[0]);

    TEB_INIT(TASK_PRI,init, 93,may_war_set_cuckoo[0][0],may_war_set_cuckoo[0][1],teb_breaking_cuckoo[0]);
    TEB_INIT(TASK_PRI,init_array, 106,may_war_set_cuckoo[1][0],may_war_set_cuckoo[1][1],teb_breaking_cuckoo[1]);//not used
    TEB_INIT(TASK_PRI,generate_key, 4,may_war_set_cuckoo[2][0],may_war_set_cuckoo[2][1],teb_breaking_cuckoo[2]);
    TEB_INIT(TASK_PRI,calc_indexes, 8,may_war_set_cuckoo[3][0],may_war_set_cuckoo[3][1],teb_breaking_cuckoo[3]);
    TEB_INIT(TASK_PRI,calc_indexes_index_1, 8,may_war_set_cuckoo[4][0],may_war_set_cuckoo[4][1],teb_breaking_cuckoo[4]);
    TEB_INIT(TASK_PRI,calc_indexes_index_2, 9,may_war_set_cuckoo[5][0],may_war_set_cuckoo[5][1],teb_breaking_cuckoo[5]);
    TEB_INIT(TASK_PRI,insert, 3,may_war_set_cuckoo[6][0],may_war_set_cuckoo[6][1],teb_breaking_cuckoo[6]);
    TEB_INIT(TASK_PRI,add, 15,may_war_set_cuckoo[7][0],may_war_set_cuckoo[7][1],teb_breaking_cuckoo[7]);
    TEB_INIT(TASK_PRI,relocate, 10,may_war_set_cuckoo[8][0],may_war_set_cuckoo[8][1],teb_breaking_cuckoo[8]);
    TEB_INIT(TASK_PRI,insert_done, 5,may_war_set_cuckoo[9][0],may_war_set_cuckoo[9][1],teb_breaking_cuckoo[9]);
    TEB_INIT(TASK_PRI,lookup, 3,may_war_set_cuckoo[10][0],may_war_set_cuckoo[10][1],teb_breaking_cuckoo[10]);
    TEB_INIT(TASK_PRI,lookup_search, 5,may_war_set_cuckoo[11][0],may_war_set_cuckoo[11][1],teb_breaking_cuckoo[11]);
    TEB_INIT(TASK_PRI,lookup_done, 5,may_war_set_cuckoo[12][0],may_war_set_cuckoo[12][1],teb_breaking_cuckoo[12]);
    TEB_INIT(TASK_PRI,print_stats, 1,may_war_set_cuckoo[13][0],may_war_set_cuckoo[13][1],teb_breaking_cuckoo[13]);
    TEB_INIT(TASK_PRI,done, 1,may_war_set_cuckoo[14][0],may_war_set_cuckoo[14][1],teb_breaking_cuckoo[14]);
    TEB_INIT(TASK_PRI,dummy, 1,may_war_set_cuckoo[15][0],may_war_set_cuckoo[15][1],teb_breaking_cuckoo[15]);
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
