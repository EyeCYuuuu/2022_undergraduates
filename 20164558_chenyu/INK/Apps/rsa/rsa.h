/*
 * rsa.h
 *
 *  Created on: 22 Mar 2020
 *      Author: elk
 */

#ifndef APPS_RSA_RSA_H_
#define APPS_RSA_RSA_H_
#include <stdint.h>
#include <apps.h>
/**---------------------------------------------------------------------
 * rsa here.
 */
const uint32_t may_war_set_rsa[NUM_TEB_RSA][2]={
#if (INK0||INK_J||INK1J)              //all-W. Base on A-mem.
    {0,140},{0,140},{0,140},{0,140},{0,140},
    {0,140},{0,140},{0,140},{0,140},{0,140},
    {0,140},{0,140},{0,140},{0,140},{0,140}
#endif
#if (OUR1||INK1)        //con-byte-wise-WAR. Base on A-mem.
    {12,106},{12,106},{12,106},{12,106},{6,112},
    {6,112},{6,112},{114,2},{120,20},{122,18},
    {132,8},{132,8},{132,8},{132,8},{0,0}
#endif
#if (OUR2)              //con-byte-wise-WAR. Base on O-mem.
    {100,24},{100,4},{100,4},{100,4},{94,10},
    {94,10},{96,8},{102,2},{124,16},{128,10},
    {132,8},{132,8},{132,8},{132,8},{0,0}
#endif
#if (DOUFIF)              //con-byte-wise-WAR. Base on O-mem.
    {0,0},{24,2},{0,0},{0,0},{0,0},
    {0,0},{16,2},{0,0},{208,6},{212,2},
    {0,0},{0,0},{232,6},{236,2},{0,0}
#endif
};
const uint8_t teb_breaking_rsa[NUM_TEB_RSA]={
#if (OUR1||OUR2)    //breaking
    1, 0, 0, 0, 1,
    0, 0, 0, 1, 0,
    0, 0, 0, 0, 0
#endif
#if (INK0||INK1||DOUFIF)    //all-breaking
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1
#endif
#if (INK_J||INK1J)    //breaking
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
#endif
};


#endif /* APPS_RSA_RSA_H_ */
