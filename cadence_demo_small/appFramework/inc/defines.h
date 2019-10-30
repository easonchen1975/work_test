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

#ifndef __DEFINES__
#define __DEFINES__

/*************************************************************
* Project definitions
*************************************************************/
// Tile and image dimensions
//#define TILE_WIDTH               (32)
//#define TILE_HEIGHT              (32)
//#define IMAGE_WIDTH              (1920)
//#define IMAGE_HEIGHT             (1080)
#define TILE_WIDTH               (64)
#define TILE_HEIGHT              (64)


int IMAGE_WIDTH;
int IMAGE_HEIGHT;


#define POOL_SIZE                (32 * 1024)
#define DMA_DESCR_CNT            (32) // number of DMA decsriptors
#define MAX_PIF                  (64)

#define INTERRUPT_ON_COMPLETION  (1)
#define RET_ERROR                (-1)

#endif //__DEFINES__
