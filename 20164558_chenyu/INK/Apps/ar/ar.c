#include <apps.h>

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

//#include <may_war_profiles.h>
//#include <mspReseter.h>
//#include <mspProfiler.h>
//#include <mspDebugger.h>
#include <accel.h>
#include <Kernel/ink.h>
#include <Kernel/scheduler/thread.h>
#include <msp-math.h>

#include "./ar.h"

const uint32_t may_war_set_ar[NUM_TEB_AR][2];
const uint8_t teb_breaking_ar[NUM_TEB_AR];

// Only for profiling, removable otherwise
//#include <ctlflags.h>

// Profiling flags.
#if ENABLE_PRF
__nv uint8_t full_run_started = 0;
__nv uint8_t first_run = 1;
#endif

#ifndef RST_TIME
#define RST_TIME 25000
#endif

__nv unsigned _v_seed;

// Number of samples to discard before recording training set
#define NUM_WARMUP_SAMPLES 3

#define ACCEL_WINDOW_SIZE 3
#define MODEL_SIZE 16
#define SAMPLE_NOISE_FLOOR 10 // TODO: made up value

// Number of classifications to complete in one experiment
#define SAMPLES_TO_COLLECT 128

typedef threeAxis_t_8 accelReading;
typedef accelReading accelWindow[ACCEL_WINDOW_SIZE];

typedef struct {
    unsigned meanmag;
    unsigned stddevmag;
} features_t;

typedef enum {
    CLASS_STATIONARY,
    CLASS_MOVING,
} class_t;


typedef enum {
    MODE_IDLE = 3,
    MODE_TRAIN_STATIONARY = 2,
    MODE_TRAIN_MOVING = 1,
    MODE_RECOGNIZE = 0, // default
} run_mode_t;

/**
 * 1. TEB declaration here.
 */
TEB(task_init);
TEB(task_selectMode);
TEB(task_resetStats);
TEB(task_sample);
TEB(task_transform);
TEB(task_featurize);
TEB(task_classify);
TEB(task_stats);
TEB(task_warmup);
TEB(task_train);
TEB(task_idle);

/**
 * 2. Shared variable declaration here. (164 bytes)
 */
__shared(
#if (OUR2==0)   //1~15
    uint16_t _v_pinState;
    unsigned _v_discardedSamplesCount;
    class_t _v_class;
    unsigned _v_totalCount;
    unsigned _v_movingCount;
    unsigned _v_stationaryCount;
    accelReading _v_window[ACCEL_WINDOW_SIZE];
    features_t _v_features;
    unsigned _v_trainingSetSize;
    unsigned _v_samplesInWindow;
    run_mode_t _v_mode;
    unsigned _v_seed;
    unsigned _v_count;
    features_t _v_model_stationary[MODEL_SIZE];
    features_t _v_model_moving[MODEL_SIZE];
#else           //8-9-10-11-12-13-1-2-3-4-5-6-7-14-15
    features_t _v_features;
    unsigned _v_trainingSetSize;
    unsigned _v_samplesInWindow;
    run_mode_t _v_mode;
    unsigned _v_seed;
    unsigned _v_count;
    uint16_t _v_pinState;
    unsigned _v_discardedSamplesCount;
    class_t _v_class;
    unsigned _v_totalCount;
    unsigned _v_movingCount;
    unsigned _v_stationaryCount;
    accelReading _v_window[ACCEL_WINDOW_SIZE];
    features_t _v_model_stationary[MODEL_SIZE];
    features_t _v_model_moving[MODEL_SIZE];
#endif
)

void ACCEL_singleSample_(threeAxis_t_8* result){
    unsigned seed = _v_seed;

    result->x = (seed*17)%85;
    result->y = (seed*17*17)%85;
    result->z = (seed*17*17*17)%85;
    _v_seed = ++seed;
}

/**
 * 3. TEB definition here.
 */
TEB(task_init)//0
{
#if TSK_SIZ || EXECUTION_TIME
    cp_reset();
#endif

#if ENABLE_PRF
    full_run_started = 1;
#endif

    __SET(_v_pinState) = MODE_IDLE;
    __SET(_v_count) = 0;
    __SET(_v_seed) = 1;

    NEXT(1);

#if TSK_SIZ
    cp_sendRes("task_init \0");
#endif
}


TEB(task_selectMode)//1
{
    uint16_t pin_state = 1;
    ++__SET(_v_count);

    if(__GET(_v_count) >= 3) pin_state = 2;
    if(__GET(_v_count) >= 5) pin_state = 0;
    if (__GET(_v_count) >= 7) {
        
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

#ifdef EXECUTION_TIME
cp_sendRes("ar");
uart_sendHex16(fullpage_fault_counter);
uart_sendStr("\n\r\0");
uart_sendHex16(page_fault_counter);
uart_sendStr("\n\r\0");
#endif
        NEXT(0);
    }

    // Don't re-launch training after finishing training
    if ((pin_state == MODE_TRAIN_STATIONARY ||
                pin_state == MODE_TRAIN_MOVING) &&
            pin_state == __GET(_v_pinState)) {
        pin_state = MODE_IDLE;
    } else {
        __SET(_v_pinState) = pin_state;
    }

    switch(pin_state) {
        case MODE_TRAIN_STATIONARY:
            __SET(_v_discardedSamplesCount) = 0;
            __SET(_v_mode) = MODE_TRAIN_STATIONARY;
            __SET(_v_class) = CLASS_STATIONARY;
            __SET(_v_samplesInWindow) = 0;

            NEXT(8);

        case MODE_TRAIN_MOVING:
            __SET(_v_discardedSamplesCount) = 0;
            __SET(_v_mode) = MODE_TRAIN_MOVING;
            __SET(_v_class) = CLASS_MOVING;
            __SET(_v_samplesInWindow) = 0;

            NEXT(8);

        case MODE_RECOGNIZE:
            __SET(_v_mode) = MODE_RECOGNIZE;

            NEXT(2);

        default:
            NEXT(10);
    }

#if TSK_SIZ
    cp_sendRes("task_selectMode \0");
#endif
}


TEB(task_resetStats)//2
{
#if TSK_SIZ
    cp_reset();
#endif

    // NOTE: could roll this into selectMode task, but no compelling reason

    // NOTE: not combined into one struct because not all code paths use both
    __SET(_v_movingCount) = 0;
    __SET(_v_stationaryCount) = 0;
    __SET(_v_totalCount) = 0;

    __SET(_v_samplesInWindow) = 0;

    NEXT(3);

#if TSK_SIZ
    cp_sendRes("task_resetStats \0");
#endif
}


TEB(task_sample)//3
{
#if TSK_SIZ
    cp_reset();
#endif

    accelReading sample;
    ACCEL_singleSample_(&sample);
    __SET(_v_window[__GET(_v_samplesInWindow)].x) = sample.x;
    __SET(_v_window[__GET(_v_samplesInWindow)].y) = sample.y;
    __SET(_v_window[__GET(_v_samplesInWindow)].z) = sample.z;
    ++__SET(_v_samplesInWindow);

    if (__GET(_v_samplesInWindow) < ACCEL_WINDOW_SIZE) {
        NEXT(3);
    } else {
        __SET(_v_samplesInWindow) = 0;
        NEXT(4);
    }

#if TSK_SIZ
    cp_sendRes("task_sample \0");
#endif
}


TEB(task_transform)//4
{
#if TSK_SIZ
    cp_reset();
#endif

    unsigned i;

    for (i = 0; i < ACCEL_WINDOW_SIZE; i++) {
        if (__GET(_v_window[i].x) < SAMPLE_NOISE_FLOOR ||
                __GET(_v_window[i].y) < SAMPLE_NOISE_FLOOR ||
                __GET(_v_window[i].z) < SAMPLE_NOISE_FLOOR) {

            __SET(_v_window[i].x) = (__GET(_v_window[i].x) > SAMPLE_NOISE_FLOOR)
                ? __GET(_v_window[i].x) : 0;
            __SET(_v_window[i].y) = (__GET(_v_window[i].y) > SAMPLE_NOISE_FLOOR)
                ? __GET(_v_window[i].y) : 0;
            __SET(_v_window[i].z) = (__GET(_v_window[i].z) > SAMPLE_NOISE_FLOOR)
                ? __GET(_v_window[i].z) : 0;
        }
    }
    NEXT(5);

#if TSK_SIZ
    cp_sendRes("task_transform \0");
#endif
}


TEB(task_featurize)//5
{
#if TSK_SIZ
    cp_reset();
#endif

    accelReading mean, stddev;
    mean.x = mean.y = mean.z = 0;
    stddev.x = stddev.y = stddev.z = 0;
    features_t features;

    int i;
    for (i = 0; i < ACCEL_WINDOW_SIZE; i++) {
        mean.x += __GET(_v_window[i].x);
        mean.y += __GET(_v_window[i].y);
        mean.z += __GET(_v_window[i].z);
    }
    mean.x >>= 2;
    mean.y >>= 2;
    mean.z >>= 2;

    accelReading sample;

    for (i = 0; i < ACCEL_WINDOW_SIZE; i++) {
        sample.x = __GET(_v_window[i].x);
        sample.y = __GET(_v_window[i].y);
        sample.z = __GET(_v_window[i].z);

        stddev.x += (sample.x > mean.x) ? (sample.x - mean.x)
            : (mean.x - sample.x);
        stddev.y += (sample.y > mean.y) ? (sample.y - mean.y)
            : (mean.y - sample.y);
        stddev.z += (sample.z > mean.z) ? (sample.z - mean.z)
            : (mean.z - sample.z);
    }
    stddev.x >>= 2;
    stddev.y >>= 2;
    stddev.z >>= 2;

    unsigned meanmag = mean.x*mean.x + mean.y*mean.y + mean.z*mean.z;
    unsigned stddevmag = stddev.x*stddev.x + stddev.y*stddev.y + stddev.z*stddev.z;
    features.meanmag   = sqrt16(meanmag);
    features.stddevmag = sqrt16(stddevmag);

    switch (__GET(_v_mode)) {
        case MODE_TRAIN_STATIONARY:
        case MODE_TRAIN_MOVING:
            __SET(_v_features.meanmag) = features.meanmag;
            __SET(_v_features.stddevmag) = features.stddevmag;
            NEXT(9);
        case MODE_RECOGNIZE:
            __SET(_v_features.meanmag) = features.meanmag;
            __SET(_v_features.stddevmag) = features.stddevmag;
            NEXT(6);
        default:
            // TODO: abort
            break;
    }

#if TSK_SIZ
    cp_sendRes("task_featurize \0");
#endif
}


TEB(task_classify)//6
{
#if TSK_SIZ
    cp_reset();
#endif

    int move_less_error = 0;
    int stat_less_error = 0;
    int i;

    long int meanmag;
    long int stddevmag;
    meanmag = __GET(_v_features.meanmag);
    stddevmag = __GET(_v_features.stddevmag);

    features_t ms, mm;

    for (i = 0; i < MODEL_SIZE; ++i) {
        ms.meanmag = __GET(_v_model_stationary[i].meanmag);
        ms.stddevmag = __GET(_v_model_stationary[i].stddevmag);
        mm.meanmag = __GET(_v_model_moving[i].meanmag);
        mm.stddevmag = __GET(_v_model_moving[i].stddevmag);

        long int stat_mean_err = (ms.meanmag > meanmag)
            ? (ms.meanmag - meanmag)
            : (meanmag - ms.meanmag);

        long int stat_sd_err = (ms.stddevmag > stddevmag)
            ? (ms.stddevmag - stddevmag)
            : (stddevmag - ms.stddevmag);

        long int move_mean_err = (mm.meanmag > meanmag)
            ? (mm.meanmag - meanmag)
            : (meanmag - mm.meanmag);

        long int move_sd_err = (mm.stddevmag > stddevmag)
            ? (mm.stddevmag - stddevmag)
            : (stddevmag - mm.stddevmag);

        if (move_mean_err < stat_mean_err) {
            move_less_error++;
        } else {
            stat_less_error++;
        }

        if (move_sd_err < stat_sd_err) {
            move_less_error++;
        } else {
            stat_less_error++;
        }
    }

    __SET(_v_class) = (move_less_error > stat_less_error) ? CLASS_MOVING : CLASS_STATIONARY;

    NEXT(7);

#if TSK_SIZ
    cp_sendRes("task_classify \0");
#endif
}


unsigned resultStationaryPct;
unsigned resultMovingPct;
unsigned sum;

TEB(task_stats)//7
{
#if TSK_SIZ
    cp_reset();
#endif

    ++__SET(_v_totalCount);

    switch (__GET(_v_class)) {
        case CLASS_MOVING:
            ++__SET(_v_movingCount);
            break;
        case CLASS_STATIONARY:
            ++__SET(_v_stationaryCount);
            break;
    }

    if (__GET(_v_totalCount) == SAMPLES_TO_COLLECT) {
        resultStationaryPct = __GET(_v_stationaryCount) * 100 / __GET(_v_totalCount);
        resultMovingPct = __GET(_v_movingCount) * 100 / __GET(_v_totalCount);
        sum = __GET(_v_stationaryCount) + __GET(_v_movingCount);
        NEXT(7);
    } else {
        NEXT(3);
    }

#if TSK_SIZ
    cp_sendRes("task_stats \0");
#endif
}

TEB(task_warmup)//8
{
#if TSK_SIZ
    cp_reset();
#endif

    threeAxis_t_8 sample;

    if (__GET(_v_discardedSamplesCount) < NUM_WARMUP_SAMPLES) {

        ACCEL_singleSample_(&sample);
        ++__SET(_v_discardedSamplesCount);
        NEXT(8);
    } else {
        __SET(_v_trainingSetSize) = 0;
        NEXT(3);
    }

#if TSK_SIZ
    cp_sendRes("task_warmup \0");
#endif
}

TEB(task_train)//9
{
#if TSK_SIZ
    cp_reset();
#endif

    switch (__GET(_v_class)) {
        case CLASS_STATIONARY:
            __SET(_v_model_stationary[__GET(_v_trainingSetSize)].meanmag) = __GET(_v_features.meanmag);
            __SET(_v_model_stationary[__GET(_v_trainingSetSize)].stddevmag) = __GET(_v_features.stddevmag);
            break;
        case CLASS_MOVING:
            __SET(_v_model_moving[__GET(_v_trainingSetSize)].meanmag) = __GET(_v_features.meanmag);
            __SET(_v_model_moving[__GET(_v_trainingSetSize)].stddevmag) = __GET(_v_features.stddevmag);
            break;
    }

    ++__SET(_v_trainingSetSize);

    if (__GET(_v_trainingSetSize) < MODEL_SIZE) {
        NEXT(3);
    } else {
        NEXT(10);
    }

#if TSK_SIZ
    cp_sendRes("task_train \0");
#endif
}

TEB(task_idle)//10
{

#if TSK_SIZ
    cp_reset();
#endif

    NEXT(0);

#if TSK_SIZ
    cp_sendRes("task_idle \0");
#endif
}


/**
 * 0. Benchmark app Init here.
 */
void _benchmark_ar_init()
{
    TASK(TASK_PRI);

    TEB_INIT(TASK_PRI, task_init, 3, may_war_set_ar[0][0], may_war_set_ar[0][1], teb_breaking_ar[0]);       //0
    TEB_INIT(TASK_PRI, task_selectMode, 9, may_war_set_ar[1][0], may_war_set_ar[1][1], teb_breaking_ar[1]); //1
    TEB_INIT(TASK_PRI, task_resetStats, 3, may_war_set_ar[2][0], may_war_set_ar[2][1], teb_breaking_ar[2]); //2
    TEB_INIT(TASK_PRI, task_sample, 26, may_war_set_ar[3][0], may_war_set_ar[3][1], teb_breaking_ar[3]);    //3
    TEB_INIT(TASK_PRI, task_transform, 8, may_war_set_ar[4][0], may_war_set_ar[4][1], teb_breaking_ar[4]);  //4
    TEB_INIT(TASK_PRI, task_featurize, 30, may_war_set_ar[5][0], may_war_set_ar[5][1], teb_breaking_ar[5]); //5
    TEB_INIT(TASK_PRI, task_classify, 50, may_war_set_ar[6][0], may_war_set_ar[6][1], teb_breaking_ar[6]);  //6
    TEB_INIT(TASK_PRI, task_stats, 3, may_war_set_ar[7][0], may_war_set_ar[7][1], teb_breaking_ar[7]);      //7
    TEB_INIT(TASK_PRI, task_warmup, 13, may_war_set_ar[8][0], may_war_set_ar[8][1], teb_breaking_ar[8]);    //8
    TEB_INIT(TASK_PRI, task_train, 7, may_war_set_ar[9][0], may_war_set_ar[9][1], teb_breaking_ar[9]);      //9
    TEB_INIT(TASK_PRI, task_idle, 1, may_war_set_ar[10][0], may_war_set_ar[10][1], teb_breaking_ar[10]);     //10

    __SIGNAL(1);
}
