/*
 * sort.h
 *
 *  Created on: 22 Mar 2020
 *      Author: elk
 */

#ifndef APPS_SORT_SORT_H_
#define APPS_SORT_SORT_H_
#include <stdint.h>
#include <apps.h>

#define LENGTH 100

/**--------------
 * AB time cost:
 * TEB0: 135us
 * TEB1: 9us
 * TEB2: 6~7us
 * TEB3: 5us
 * --------------
 */

/**---------------------------------------------------------------------
 * sort may(war) profile here. In WORD.
 */
const uint32_t may_war_set_sort[NUM_TEB_SORT][2]={
#if (INK0||INK_J||INK1J)              //all-W. Base on A-mem.
    {0,103},{0,103},{0,103},{0,103},{0,0}
#endif
#if (INK1)              //con-byte-wise-WAR. Base on A-mem.
    {102,1},{0,103},{100,3},{102,1}
#endif
#if (OUR1)        //con-byte-wise-WAR. Base on A-mem.
    //{102,1},{0,103},{100,3},{102,1}
    {0,103},{0,103},{0,103},{0,103}
#endif
#if (OUR2)              //con-byte-wise-WAR. Base on O-mem.
    {102,1},{0,103},{101,2},{102,1}
#endif
#if (DOUFIF)              //con-byte-wise-WAR. Base on O-mem.
    {102,1},{0,102},{0,0},{0,0},{0,0}
#endif
};

const uint8_t teb_breaking_sort[NUM_TEB_SORT]={
#if (OUR1||OUR2)    //breaking
    //0, 1, 0, 0
    0, 0, 0, 0
#endif
#if (INK0||INK1||DOUFIF)    //all-breaking
    1, 1, 1, 1, 1
#endif
#if (INK_J||INK1J)    //breaking
    0, 0, 0, 0
#endif
};

const uint16_t a1[LENGTH] = {
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
    121, 177,  50,  55,  32, 173, 164, 132,  17, 174,
    61, 186,  96,  44, 120, 181,  24, 134,  68, 167,
    8,  26, 144, 138, 133,  48,  80,  60,  39,   6,
    86, 126, 154,  42, 150, 113, 105,  52, 139, 175,
    58,  98,  31, 182,  74, 169,   4,  23, 157, 189,
    72, 130,   9,  19,  12,  67,   2,  16,  49,  57,
    69,  94, 145, 136,  99, 152, 198,  59, 153, 127,
    92,  13,  14, 160,  35, 194, 107,  89, 199, 155,
    163, 156,  93, 140, 111,  63,  15,  79,  22, 159,
    65,  78,  81, 122, 135, 180,  76,   3,  38, 102,
    121, 177,  50,  55,  32, 173, 164, 132,  17, 174,
    61, 186,  96,  44, 120, 181,  24, 134,  68, 167,
    8,  26, 144, 138, 133,  48,  80,  60,  39,   6,
    86, 126, 154,  42, 150, 113, 105,  52, 139, 175,
    58,  98,  31, 182,  74, 169,   4,  23, 157, 189,
    72, 130,   9,  19,  12,  67,   2,  16,  49,  57,
    69,  94, 145, 136,  99, 152, 198,  59, 153, 127,
    92,  13,  14, 160,  35, 194, 107,  89, 199, 155,
    163, 156,  93, 140, 111,  63,  15,  79,  22, 159,
    65,  78,  81, 122, 135, 180,  76,   3,  38, 102,
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
    3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
};

const uint16_t a2[LENGTH] = {
    121, 177,  50,  55,  32, 173, 164, 132,  17, 174,
     61, 186,  96,  44, 120, 181,  24, 134,  68, 167,
      8,  26, 144, 138, 133,  48,  80,  60,  39,   6,
     86, 126, 154,  42, 150, 113, 105,  52, 139, 175,
     58,  98,  31, 182,  74, 169,   4,  23, 157, 189,
     72, 130,   9,  19,  12,  67,   2,  16,  49,  57,
     69,  94, 145, 136,  99, 152, 198,  59, 153, 127,
     92,  13,  14, 160,  35, 194, 107,  89, 199, 155,
    163, 156,  93, 140, 111,  63,  15,  79,  22, 159,
     65,  78,  81, 122, 135, 180,  76,   3,  38, 102,
     121, 177,  50,  55,  32, 173, 164, 132,  17, 174,
      61, 186,  96,  44, 120, 181,  24, 134,  68, 167,
       8,  26, 144, 138, 133,  48,  80,  60,  39,   6,
      86, 126, 154,  42, 150, 113, 105,  52, 139, 175,
      58,  98,  31, 182,  74, 169,   4,  23, 157, 189,
      72, 130,   9,  19,  12,  67,   2,  16,  49,  57,
      69,  94, 145, 136,  99, 152, 198,  59, 153, 127,
      92,  13,  14, 160,  35, 194, 107,  89, 199, 155,
     163, 156,  93, 140, 111,  63,  15,  79,  22, 159,
      65,  78,  81, 122, 135, 180,  76,   3,  38, 102,
      121, 177,  50,  55,  32, 173, 164, 132,  17, 174,
       61, 186,  96,  44, 120, 181,  24, 134,  68, 167,
        8,  26, 144, 138, 133,  48,  80,  60,  39,   6,
       86, 126, 154,  42, 150, 113, 105,  52, 139, 175,
       58,  98,  31, 182,  74, 169,   4,  23, 157, 189,
       72, 130,   9,  19,  12,  67,   2,  16,  49,  57,
       69,  94, 145, 136,  99, 152, 198,  59, 153, 127,
       92,  13,  14, 160,  35, 194, 107,  89, 199, 155,
      163, 156,  93, 140, 111,  63,  15,  79,  22, 159,
       65,  78,  81, 122, 135, 180,  76,   3,  38, 102,
       3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
      19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
      66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
      25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
      12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
       3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
      19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
      66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
      25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
      12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
      3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
     19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
     66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
     25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
     12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
      3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
     19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
     66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
     25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
     12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
     3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
     3,   1,   4,   6,   9,   5,  10,   8,  16,  20,
    19,  40,  16,  17,   2,  41,  80, 100,   5,  89,
    66,  77,   8,   3,  32,  55,   8,  11,  99,  65,
    25,  89,   3,  22,  25, 121,  11,  90,  74,   1,
    12,  39,  54,  20,  22,  43,  45,  90,  81,  40,
};


#endif /* APPS_SORT_SORT_H_ */
