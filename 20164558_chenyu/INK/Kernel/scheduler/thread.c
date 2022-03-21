
#include "ink.h"
#include "math.h"
#include <profile.h>
extern void __dma_word_copy(unsigned int from, unsigned int to, unsigned short size);

//by srliu. 1: AB merging now. 0: at a breaking-point
volatile __nv uint8_t breaking_flag = 0;

//by srliu. For TSN energy tracking method.
volatile __nv uint8_t tsn_commit_flag = 0;   //
volatile __nv uint16_t count_ab_history = 0;
volatile __nv uint16_t count_ab_total_num = 0;
__nv float re_exe_tsk = 0;
volatile __nv uint64_t total_mem_copy = 0;
//volatile __nv uint8_t count_ab_cur = 0;

// get current capacity budget
#define Maximum_Budget 10

volatile __nv uint8_t Budgetidx = 0;
volatile __nv uint16_t ChargingTimes = 0;
float MaxAB = 0;
float MinAB = 1000;
uint8_t MaxID;
uint8_t MinID;
#define POWONPER 700
/*
#define MaximumTrace 7
const uint16_t EnergyTrace[MaximumTrace] = {
1000, 1000, 1000, 1000, 2000, 2000, 4000
};*/ //trace 1
#define MaximumTrace 10
const uint16_t EnergyTrace[MaximumTrace] = {
200, 500, 1000, 1000, 2000, 2000, 2000, 2000, 4000, 8000
}; // trace 2

static uint16_t __get_real_budget()
{
    //uint16_t usMeanBudget = POWER_DOWN_PERIOD;

    //add noise
    //int16_t usNoise;
    ChargingTimes++;
#if unstableenergy
    return EnergyTrace[rand()%MaximumTrace];

#else
    return POWER_DOWN_PERIOD;
#endif
    //return (usMeanBudget+usNoise);
}
#if (TSN2||TSN3)
static uint16_t __cal_tsn_count(uint16_t history_ab_count, uint16_t iteration){
/** DO use float.
    float rou = ROU;
    float gamma = GAMMA;

    float temp=1.0;
    uint8_t i;
    for(i=0;i<cur_cmt_iteration;i++){
        temp = temp*gamma;
    }
    temp = history_ab_count*rou*temp;

    if(temp>1){
        return (ceil(temp));
    }else{
        return 1;
    }
*/
    //float rou = ROU;
    //float gamma = GAMMA;
    if(iteration>=16){
        return (uint16_t)1;
    }
    history_ab_count = history_ab_count >> (iteration+1);
    if(history_ab_count>1){
        return (history_ab_count);
    }else{
        return (uint16_t)1;
    }
}
#endif

extern uint16_t recover_time_temp;
volatile __nv uint16_t backup_time_temp = 0;

// prepares the stack of the thread for the task execution
static inline void __prologue(thread_t *thread)
{
    may_war_t *may_war = &thread->teb_array[thread->CurTebId].may_war;
    buffer_t *buffer = &(thread->buffer);

#if OUR1_S
    int isize;
    int temp_counter;
    temp_counter = 0;
    isize = may_war->range_size;

    while(isize>2 && temp_counter<3){
        isize = isize-2;
        __dma_word_copy((uint32_t)buffer->buf[0]+2*temp_counter+(uint32_t)may_war->range_offset_addr_begin,\
                                           (uint32_t)buffer->buf[1]+2*temp_counter+(uint32_t)may_war->range_offset_addr_begin,\
                                           2);
        while(HWREG16(DMA_BASE + DMA_CHANNEL_0 + OFS_DMA1CTL)){
            }
        temp_counter++;
    }
        __dma_word_copy((uint32_t)buffer->buf[0]+2*temp_counter+(uint32_t)may_war->range_offset_addr_begin,\
                                        (uint32_t)buffer->buf[1]+2*temp_counter+(uint32_t)may_war->range_offset_addr_begin,\
                                    isize);
        while(HWREG16(DMA_BASE + DMA_CHANNEL_0 + OFS_DMA1CTL)){
                }

#else
    // copy original stack to the temporary stack
     total_mem_copy += (may_war->range_size);
    __dma_word_copy((uint32_t)buffer->buf[0]+(uint32_t)may_war->range_offset_addr_begin,\
                            (uint32_t)buffer->buf[1]+(uint32_t)may_war->range_offset_addr_begin,\
                            may_war->range_size);
    while(HWREG16(DMA_BASE + DMA_CHANNEL_0 + OFS_DMA1CTL)){
    }

#endif
}

volatile __nv uint8_t test_count = 0;
//#if TSN1
//by zw.
#define tsnstep 1
__nv uint8_t tsninit = 2;
//#endif

// by srliu. For TSN
volatile __nv uint16_t count_ab_tracking = 0;
volatile __nv uint16_t count_ab_temp = 0;
volatile __nv uint8_t force_cmt_iter = 0;
volatile __nv uint8_t energy_deplet;
#if PRF_TIME
// runs one task inside the current thread
__nv float backup_time=0;
__nv float recover_time=0;
#endif
__nv float task_time=0;
__nv float re_exe=0;
__nv uint16_t target=0;
static inline void __reset_state(){
    count_ab_history = count_ab_tracking;   //update
#if PRF_TRACKING
    printf("%d-th power-on period executed %d AB.\r\n",(ChargingTimes-1),(tsninit));
#endif
    count_ab_tracking = 0;
    force_cmt_iter = 0;
    tsn_commit_flag = 0;
    energy_deplet = 0;
    count_ab_temp = 0;
}

static inline float __cap_charging(){
    Budgetidx = (Budgetidx+1) % Maximum_Budget;
//#if (DOUFIF)
    return POWONPER;//return the power on time; test double 50
//#endif
    //return (__get_real_budget());
}
uint8_t CountTeb = 0;
uint16_t NoBackup = 0;
void __tick(thread_t *thread)
{
    //uint8_t cur_budget;
    float real_budget;

    energy_deplet = 0;
    tsn_commit_flag = 0;
    uint8_t ABperTEB;

#if PRF_TIME
    unsigned int backup_end, backup_start;
#endif
    unsigned int task_end, task_start;

    switch (thread->state)
    {
    case TASK_READY:
#if TSN1
        if(tsninit > tsnstep)
        {
            tsninit -= tsnstep;
        }
#endif
        // charge battery
        real_budget = __cap_charging();
#if (0)
        if(real_budget<=recover_time_temp){
            recover_time = recover_time - recover_time_temp +real_budget;
            energy_deplet = 1;
        }
        real_budget -= recover_time_temp;
#endif

#if PRF_ENERGY_EST
        printf("Current(%d-th) power period budget: %d.\r\n",Budgetidx-1,real_budget);
        printf("|+|History(%d-th power period) energy/AB count estimation:%d\r\n",(uint16_t)Budgetidx-2, count_ab_history);
#endif
        __reset_state();
        //printf("||||||||||--------past history is %d. No of backup times is %d \r\n",count_ab_history,NoBackup);
        //re_exe_tsk ++;
        while(!energy_deplet){//energy enough after commit
            uint8_t tempAB = 0;
            uint8_t firstFlag = 0;
            CountTeb ++;
#if PRF_TIME
            //count the backup time.
            PRB_START(backup_start);
#endif
            __prologue(thread); // by srliu: backup May(war) set

            NoBackup++;


#if PRF_TIME
            PRB_END(backup_end);
            backup_time_temp = (backup_end - backup_start)/16.0;
            if(real_budget<=backup_time_temp){

#if !NEVEROFF
                //backup_time += real_budget;
#endif
                backup_time += real_budget;
                re_exe += real_budget;
                energy_deplet = 1;
                printf("backup off, TASK is %d, backuptime is %d, timeline is %f\n",tempAB,backup_time_temp,task_time+backup_time);
                break;


            }
            backup_time += backup_time_temp;
            real_budget -= backup_time_temp;
            ABperTEB = 0;
            //backup time count end;
#endif

#if TSN1
            if(tsn_commit_flag)
                tsninit += tsnstep;
#endif
            breaking_flag = 0;  //clear the breaking_flag after each commit.
            tsn_commit_flag = 0; //clear the commit_flag after each commit.
            int curAB = thread->CurTebId;   //backup CurTebId
            //target = __cal_tsn_count(count_ab_history, force_cmt_iter);
           // printf("this period target is %d. curAB is %d\r\n",target,curAB);
            while(1) // each commit
            {
                float ABtime=0;

                //ABperTEB = 0;
#if PRF_AB_TIME
                printf("AB index: %d\r ,",curAB);
#endif
                //Executing AB first.
                tempAB = curAB;
                PRB_START(task_start);
                curAB = (void *)((task_t)(thread->teb_array[tempAB].fun_entry))(thread->buffer.buf[0]);

                PRB_END(task_end);
                ABtime = (task_end - task_start)/16.0;
                //printf("AB %d.  EXECUTION TIME IS %f\r\n",tempAB,ABtime);
                count_ab_total_num++;
                //count_ab_total_tracking += thread->teb_array[curAB].budget;
                //ABperTEB ++;

#if PRF_AB_TIME
                printf("Time cost: %d\r\n",(int)ABtime);
#endif
                /**
                 * Check budget.
                 */
#if AlwaysON    //No power outage.
                task_time += ABtime;
#else
                if(ABtime > POWONPER)
                {
                    printf("task %d execute time %f\n",tempAB,ABtime); // for debug
                }
                if(ABtime > MaxAB && tempAB !=0)
                {
                    MaxAB = ABtime;
                    MaxID = tempAB;
                }
                if(ABtime < MinAB)
                {
                     MinAB = ABtime;
                     MinID = tempAB;
                }
                if(real_budget <= ABtime){
                    //energy_deplet =1;           //energy is depleted.

                    //re_exe_tsk += real_budget;
#if !NEVEROFF
                    //task_time += real_budget;
#endif
                    task_time += real_budget;
                    re_exe +=real_budget;
                    re_exe += backup_time_temp;
                energy_deplet = 1;
                printf("TASK off, task %d, abtime is %d, timeline is %f\n",tempAB,ABtime,task_time+backup_time);
                break;
                }
                else{
                    task_time += ABtime;        //count the task execution time
                    real_budget = real_budget - ABtime; // task execution completed
                }
#endif

                if(curAB == 0)
                {
                    test_count ++;
                    printf("TASK:%d, AB:%d, memcopy:%d, No backup: %d ", test_count, count_ab_total_num, total_mem_copy, NoBackup);
                    printf("MaxAB : %f task %d, MinAB: %f task %d, re-exe: %f\r\n",MaxAB ,MaxID,MinAB,MinID,re_exe);
                    //printf("TASK:%d, AB:%d, NumB:%d, Chg:%d, No backup: %d\r\n", test_count, count_ab_total_num,NoBackup,ChargingTimes,NoBackup);
                                        //printf("Finalized tsninit is %d AB.\r\n", tsninit);
                                        //printf("number of TEB is%d, abs per TEB is %f \n", CountTeb,(float)count_ab_total_num/CountTeb);
                                        //printf("%d\r\n", test_count);

                                        //printf("|=|Backup time = %f\r\n", backup_time);
                                        //printf(" %f ", backup_time-recover_time);
                                        //printf("|=|Recovery time = %f\r\n", recover_time);
                                        //printf(" %f ", recover_time);
                                        //printf("|=|TASK execution time = %f\r\n", task_time);
                                        //printf(" %f \n", task_time);
                                        printf(" wcet: %f  ", task_time+backup_time-re_exe);
                                        printf(" rate: %f \n", re_exe/(task_time+backup_time-re_exe));
                                        //printf(" %d \n", total_mem_copy);
                                        backup_time = 0.0;
                                        recover_time = 0.0;
                                        ChargingTimes = 0;
                                        thread->CurTebId = 0;
                                        re_exe_tsk = 0;
                                        total_mem_copy = 0;

#if 0
                    test_count ++;
                    thread->CurTebId = 0;
                    //energy_deplet =1; // the task is finished; re-charge the battery.

                    printf("Iteration of this TASK:%d, with %d AB, total backup times is %d.\r\n", test_count, count_ab_total_num,NoBackup);
#if OUR1
                    printf("|||||||----------- our1   %d---------||||||||||\r\n",POWER_DOWN_PERIOD);
#endif
#if INK1J
                    printf("|||||||----------- no breaking   %d---------||||||||||\r\n",POWER_DOWN_PERIOD);
#endif
#if INK_J
                    printf("|||||||----------- INK_J   %d---------||||||||||\r\n",POWER_DOWN_PERIOD);
#endif
#if  INK1
                    printf("|||||||-----------  INK1  %d ---------||||||||||\r\n",POWER_DOWN_PERIOD);
#endif
#if INK0
                    printf("|||||||----------- INK0  %d ---------||||||||||\r\n",POWER_DOWN_PERIOD);
#endif

                    //printf("Finalized tsninit is %d AB.\r\n", tsninit);
                    //printf("number of TEB is%d, abs per TEB is %f \n", CountTeb,(float)count_ab_total_num/CountTeb);
                    //printf("%d\r\n", test_count);
#if PRF_TIME
                    //printf("|=|Backup time = %f\r\n", backup_time);
                    printf("backup time %f  ", backup_time-recover_time);
                    //printf("|=|Recovery time = %f\r\n", recover_time);
                    printf("recover time %f  ", recover_time);
                    //printf("|=|TASK execution time = %f\r\n", task_time);
                    printf("task time %f  \n", task_time);
                    backup_time = 0.0;
                    recover_time = 0.0;
                    //force_cmt_iter = 0;
#endif
#endif
                    task_time = 0.0;
                    count_ab_total_num = 0;
                    CountTeb = 0;
                    //tsninit = 2;
                    NoBackup = 0;
                    //if(count_ab_history) // if all ABs can finish in one energy period do not re-init the tracking.
                      //  count_ab_tracking = count_ab_history;


                    //recover original data;
                    __dma_word_copy(thread->buffer.buf[2], thread->buffer.buf[0],thread->buffer.size/2);
                    __dma_word_copy(thread->buffer.buf[2], thread->buffer.buf[1],thread->buffer.size/2);
                            //(uint32_t)buffer->buf[0]+(uint32_t)may_war->range_offset_addr_begin,\
                              //                  (uint32_t)buffer->buf[1]+(uint32_t)may_war->range_offset_addr_begin,\
                                //                may_war->range_size);
                     while(HWREG16(DMA_BASE + DMA_CHANNEL_0 + OFS_DMA1CTL)){
                     }
                    //if(test_count>=PRF_ITR){
                    if(test_count>=10){
                        //Experiments DONE!
                        while(1);
                    }
                    break;
                }
                /**
                 * Check TSN commit point.
                 */
#if TSN1
                //Temporary AB number update for current iteration.
                count_ab_temp++;
                //TSN strategy 1 estimation. force commit?
                if(count_ab_temp >= tsninit){
                    tsn_commit_flag = 1;
                    break;
                }
#endif
#if TSN2
                //Global AB number update for this power-on period.
                count_ab_tracking++;
                count_ab_temp++;
                //TSN strategy 2 estimation. force commit?
                if(count_ab_temp >= __cal_tsn_count(count_ab_history,force_cmt_iter)){
                    tsn_commit_flag = 1;
                    break;
                }
#endif



#if TSN3
#if !(INK0||INK1)
                if(!firstFlag)
                {
                    count_ab_tracking+=backup_time_temp;
                    count_ab_temp+=backup_time_temp;
                    firstFlag = 1;
                }
                    count_ab_tracking = count_ab_tracking + thread->teb_array[tempAB].budget;    //accumulate budget
                    count_ab_temp = count_ab_temp + thread->teb_array[tempAB].budget;
                    //TSN strategy 3 estimation. force commit?
                    if(count_ab_temp  > target){
                        tsn_commit_flag = 1;
                        break;
                    }
#endif
#endif

                /**
                 * Check Whisper.
                 */


                /**
                 * Check breaking point.
                 */
                if(thread->teb_array[curAB].breaking)
                {
                    breaking_flag = 1;
                    break;
                }

            }


            //printf("commit period tracking is %d.\r\n",count_ab_temp);
            if(!energy_deplet||tsn_commit_flag||breaking_flag){
                thread->CurTebId = curAB; // budget depleted, but the real energy is not, breaking
            }
            if(!energy_deplet && tsn_commit_flag){
#if PRF_TRACKING
                printf("Executed AB number/budget of %d-th TSN commit iteration: %d.\r\n", force_cmt_iter,count_ab_temp);
#endif
                count_ab_temp = 0;
                force_cmt_iter++;
            }

        }
    case TASK_TEBFINISH:
        thread->state = TASK_READY;
    }
}
