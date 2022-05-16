#include "task.h"
#include "ink.h"

uint16_t tempTaskID;
__nv thread_t _threads[MAX_THREAD_NUM];
// size should be in words
void __dma_word_copy(unsigned int from, unsigned int to, unsigned short size) {
    // Configure DMA channel 0
    __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) from);         // Source block address
    __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) to);           // Destination single address

    DMA0SZ = size;                                                              // Block size
    DMA0CTL = DMADT_5 | DMASRCINCR_3 | DMADSTINCR_3;                            // Rpt, inc
    DMA0CTL |= DMAEN;                                                           // Enable DMA0
    DMA0CTL |= DMAREQ;                                                          // Trigger block transfer
}


void __ink_backup(uint8_t taskID)
{
    //global data.  backup-->working
    buffer_t *buffer = &_threads[0].buffer;
    __dma_word_copy((unsigned int)buffer->buf[buffer->_idx],     \
                    (unsigned int)buffer->buf[buffer->idx],    \
                    (unsigned short)buffer->size>>1);
}

void __ink_commit(uint8_t taskID)
{
    buffer_t *buffer = &_threads[0].buffer;
    buffer->idx     = buffer->idx ^ 1;
    buffer->_idx    = buffer->_idx ^ 1;
    _threads[0].CurrTaskId = tempTaskID;
}

/* ------------------
 * [__scheduler_run]: done!!
 * LOG: scheduler of ELK.
 */
void __scheduler_run()
{
    uint16_t elkCurTaskID;
    while(1){
        elkCurTaskID = _threads[0].CurrTaskId;
        __ink_backup(elkCurTaskID);
        // working on working buffer (.idx)
        tempTaskID = (uint8_t)((taskfun_t)(_threads[0].task_array[elkCurTaskID].fun_entry))(_threads[0].buffer.buf[_threads[0].buffer.idx]);
        __ink_commit(elkCurTaskID);
    }
}
