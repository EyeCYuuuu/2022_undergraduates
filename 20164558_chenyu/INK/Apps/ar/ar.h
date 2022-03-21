/*
 * ar.h
 *
 *  Created on: 22 Mar 2020
 *      Author: elk
 */

#ifndef APPS_AR_AR_H_
#define APPS_AR_AR_H_
#include <stdint.h>
#include <apps.h>
/**---------------------------------------------------------------------
 * ar here.
 */
extern const uint32_t may_war_set_ar[NUM_TEB_AR][2]={
#if (INK0||INK_J||INK1J)              //all-W. Base on A-mem.
    {0,82},{0,82},{0,82},{0,82},{0,82},
    {0,82},{0,82},{0,82},{0,82},{0,82},
    {0,82}
#endif
#if (OUR1||INK1)        //con-byte-wise-WAR. Base on A-mem.
    {1,17},{0,18},{0,0},{0,18},{0,18},
    {0,18},{0,18},{0,18},{16,1},{0,18},
    {0,18}
#endif
#if (OUR2)              //con-byte-wise-WAR. Base on O-mem.
    {6,3},{6,3},{0,10},{3,15},{4,14},
    {4,9},{5,8},{5,8},{4,1},{2,7},
    {6,3}
#endif
#if (DOUFIF)
    {0,0},{0,16},{0,0},{27,1},{12,5},
    {0,0},{0,0},{6,3},{0,0},{25,1},
    {0,0}
#endif
};
extern const uint8_t teb_breaking_ar[NUM_TEB_AR]={
#if (INK_J||INK1J)           //no-breaking
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0
#endif
#if (OUR1||OUR2)    //breaking
    0, 0, 0, 1, 0,
    0, 0, 0, 0, 1,
    0
#endif
#if (INK0||INK1||DOUFIF)    //all-breaking
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1
#endif

};



#endif /* APPS_AR_AR_H_ */
