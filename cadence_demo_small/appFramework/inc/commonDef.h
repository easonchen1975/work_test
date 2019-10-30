/*
 * Copyright (c) 2016 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */

/* *****************************************************************************
 * FILE:  commonDef.h
 *
 * DESCRIPTION:
 *
 *    This file contains prototype definitions of structures and Macros
 *    that are being used/accessed in other project files
 *
 * ****************************************************************************/

#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#define CREATOR  "TENSILICA"

#define IVP

#include <xtensa/tie/xt_ivpn.h>
#if defined(__XTENSA__)
#include <xtensa/tie/xt_timer.h>
#include <xtensa/xt_profiling.h>
#endif

#if defined(__XTENSA__) || defined(GCC)
#define ALIGN(x)  __attribute__((aligned(x)))
#ifndef ALIGN16
#define ALIGN16  __attribute__((aligned(16)))
#endif
#define ALIGN32  __attribute__((aligned(32)))
#define ALIGN64  __attribute__((aligned(64)))
#else
#define ALIGN(x)  _declspec(align(x))
#ifndef ALIGN16
#define ALIGN16  _declspec(align(16))
#endif
#define ALIGN32  _declspec(align(32))
#define ALIGN64  _declspec(align(64))
#define __restrict
#endif

#if defined(__XTENSA__) && defined(IVP)
#define _LOCAL_DRAM0_  __attribute__((section(".dram0.data")))
#define _LOCAL_DRAM1_  __attribute__((section(".dram1.data")))
#else
#define _LOCAL_DRAM0_
#define _LOCAL_DRAM1_

#endif

#ifdef IVP
#define IVP_SIMD_WIDTH  XCHAL_IVPN_SIMD_WIDTH
#else
#define IVP_SIMD_WIDTH  32
#endif

#define IVP_SIMD_WIDTH_LOG2  5

typedef struct
{
  unsigned char red, green, blue;
} PPMPixel8;

typedef struct
{
  unsigned short red, green, blue;
} PPMPixel16;

typedef struct
{
  unsigned char Y;
} PPMY8;

typedef struct
{
  unsigned short Y;
} PPMY16;

typedef struct
{
  int  x, y;
  int  compWidth;         // 8 or 16
  void *data;
} PPMImage, PGMImage;


// Enable PRINT_VEC_ENABLE to print the contents of a vector, mostly for debugging purposes
//#define PRINT_VEC_ENABLE

#ifdef PRINT_VEC_ENABLE
extern int16_t printBuff[];
#define printVEC(vec)                                         \
  {                                                           \
    int indx;                                                 \
    *((xb_vecNx16 *) printBuff) = vec;                        \
    for (indx = 0; indx < 32; indx++) {                       \
      printf("0X%04X    ", (unsigned short) printBuff[indx]); \
      if ((indx + 1) == (8 * ((indx + 1) / 8))) {             \
        printf("\n");                                         \
      }                                                       \
    }                                                         \
    printf("\n");                                             \
  }
#else
#define printVEC(vec)
#endif


#if defined(__XTENSA__)
#define TIME_STAMP(cyc_cnt)    \
  {                            \
    cyc_cnt = XT_RSR_CCOUNT(); \
  }

#define USER_DEFINED_HOOKS_START()             \
  {                                            \
    xt_iss_switch_mode(XT_ISS_CYCLE_ACCURATE); \
    xt_iss_trace_level(3);                     \
    xt_iss_client_command("all", "enable");    \
  }

#define USER_DEFINED_HOOKS_STOP()            \
  {                                          \
    xt_iss_switch_mode(XT_ISS_FUNCTIONAL);   \
    xt_iss_trace_level(0);                   \
    xt_iss_client_command("all", "disable"); \
  }

#else //__XTENSA__
#define TIME_STAMP(cyc_cnt) \
  {                         \
    cyc_cnt = 0;            \
  }

#define USER_DEFINED_HOOKS_START()

#define USER_DEFINED_HOOKS_STOP()

#endif //__XTENSA__

#endif  //__COMMON_DEF_H__
