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
 * FILE:  appFrameworkMain.c
 *
 * DESCRIPTION:
 *
 *    This file contains routines that illustrate one - how to use tileManager into their
 *    application
 *
 * ****************************************************************************/

#ifdef _FREERTOS_
  #include "FreeRTOS.h"
  #include "semphr.h"
  //#include "xtensa_rtos.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "tileManager.h"
#include "commonDef.h"
#include "defines.h"
#include "img_utils.h"

#if defined(__XTENSA__)
#include <sys/times.h>
#include <xtensa/sim.h>
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>
#include <xtensa/tie/xt_interrupt.h>
#endif

#include "k_debug.h"

//#define ERROR_CALLBACK_TEST
typedef struct intrCallbackDataStruct
{
  int32_t intrCount;
} intrCbDataStruct;

// Memory banks, used by tile manager's memory allocator

uint8_t ALIGN64 pBankBuffPool0[POOL_SIZE] _LOCAL_DRAM0_;
uint8_t ALIGN64 pBankBuffPool1[POOL_SIZE] _LOCAL_DRAM1_;
//uint8_t ALIGN64 memDstFrame[IMAGE_WIDTH * IMAGE_HEIGHT];
uint8_t ALIGN64 memDstFrame[1920*1080];

xvTileManager xvTMobj _LOCAL_DRAM1_;
IDMA_BUFFER_DEFINE(idmaObjBuff, DMA_DESCR_CNT, IDMA_2D_DESC);

intrCbDataStruct cbData _LOCAL_DRAM0_;

// IDMA error callback function
void errCallbackFunc(const idma_error_details_t* data)
{
  printf("ERROR CALLBACK: iDMA in Error\n");
  // To handle iDMA errors, XVTM_RAISE_EXCEPTION() must be called in error callback function.
  XVTM_RAISE_EXCEPTION();
#ifndef XV_EMULATE_DMA
  printf("COPY FAILED, Error %d at desc:%p, PIF src/dst=%x/%x\n", data->err_type, (void *) data->currDesc, data->srcAddr, data->dstAddr);
#endif
  return;
}

// IDMA Interrupt callback function
void intrCallbackFunc(void *pCallBackStr)
{
  //printf("INTERRUPT CALLBACK : processing iDMA interrupt\n");
  ((intrCbDataStruct *) pCallBackStr)->intrCount++;
  //printf("INTERRUPT CALLBACK : processing iDMA interrupt count=%d\n", ((intrCbDataStruct *) pCallBackStr)->intrCount);
  return;
}

/* ***********************************************************************
 * FUNCTION: processData()
 * DESCRIPTION: This is a place holder function for data processing.
 *				Currently data from input tile is copied into output tile
 * INPUTS:
 *          xvTile* pInTile
 * OUTPUTS:
 *          xvTile* pOutTile
 ************************************************************************/

void processData(xvTile* pInTile, xvTile* pOutTile)
{
  int32_t indx, indy, index, width, height, pitch;
  uint8_t *pSrc, *pDst;
  XV_TILE_SET_WIDTH(pOutTile, XV_TILE_GET_WIDTH(pInTile));
  XV_TILE_SET_PITCH(pOutTile, XV_TILE_GET_PITCH(pInTile));
  XV_TILE_SET_HEIGHT(pOutTile, XV_TILE_GET_HEIGHT(pInTile));
  XV_TILE_SET_X_COORD(pOutTile, XV_TILE_GET_X_COORD(pInTile));
  XV_TILE_SET_Y_COORD(pOutTile, XV_TILE_GET_Y_COORD(pInTile));

  width  = XV_TILE_GET_WIDTH(pInTile);
  height = XV_TILE_GET_HEIGHT(pInTile);
  pitch  = XV_TILE_GET_PITCH(pInTile);

  pSrc = (uint8_t *) XV_TILE_GET_DATA_PTR(pInTile);
  pDst = (uint8_t *) XV_TILE_GET_DATA_PTR(pOutTile);

  for (indy = 0; indy < height; indy++)
  {
    for (indx = 0; indx < width; indx++)
    {
      index       = indy * pitch + indx;
      pDst[index] = pSrc[index];
    }
  }
}


void process_eason(){

	  uint8_t *pSrc, *pDst;
	  int indy, indx, width, height, index, pitch,cycleStop, cycleStart ;
	  int loop = 20;

	  for(loop = 0; loop< 20; loop++){
#pragma no_reorder
      TIME_STAMP(cycleStart);

	  pSrc = pBankBuffPool0;
	  pDst = pBankBuffPool1;
	  width =64;
	  height = 64;
	  pitch = 64;


	  for (indy = 0; indy < height; indy++)
	  {
	    for (indx = 0; indx < width; indx++)
	    {
	      index       = indy * pitch + indx;
	      pDst[index] = pSrc[index];
	    }
	  }

#pragma no_reorder
      TIME_STAMP(cycleStop);
      printf("cycles = %d\n", cycleStop-cycleStart);
	  }
}

#include <idma.h>
static void idmaLogHander (const char* str)
{
  printf("**iDMAlib**: %s", str);
}

void appframework2( void * pdata )
{
	//USER_DEFINED_HOOKS_STOP();
printf("appframework2\n");
}
/*
 *  Ping-pong mechanism for is used data transfer.
 *  Two buffers/tiles are used for input and output
 *  After basic set up and initializations,
 *  data transfer is initiated for first two input tiles,
 *  one for ping buffer and other for pong buffer.
 *
 *  In the main for loop, once the input data transfer
 *  is completed for the earlier input buffer,
 *  tile is processed and result is written into output tile.
 *  Output tile data transfer is initiated
 *  Next tile from the input frame is transferred to current input tile
 *  We flip the ping pong flag and continue with the loop
 *
 */
//int main()
void appframework( void * pdata )
{
  //USER_DEFINED_HOOKS_STOP();
  // File & directory names
  char fname[256];           // input image name
  char oname[256];           // output image name
  int32_t str_len;

  // Pointer to PGM image structures
  PGMImage *image;
  PGMImage oimage;

  // Greyscale input frame
  uint8_t *gSrc;
  uint8_t *gOut;
  void *buffPool[2];
  int32_t buffSize[2];
  int32_t cycleStart, cycleStop, totalCycles, tileCount;
  int32_t cycleStart2, cycleStop2, totalCycles2; //, tileCount2;
  totalCycles = 0;
  totalCycles2 = 0;
  tileCount   = 0;

  // Source image dimensions and bit depth
  int32_t srcWidth, srcHeight, srcBytes;
  int32_t alignType = EDGE_ALIGNED_64;

  // Source and destination tiles. Will be working in ping pong mode.
  xvTile *pInTile[2], *pOutTile[2];
  // Data buffer pointers for source and destination tiles
  void *pinTileBuff[2], *poutTileBuff[2];
  // Source and destination frames
  xvFrame *pInFrame, *pOutFrame;
  // Indexes and flags
  int32_t dstx, dsty, pingPongFlag = 0;
  int32_t retVal, tileBuffSize, frameSize;

  xvTileManager *pxvTM = &xvTMobj;

  // Read input image
  sprintf(fname, "appFramework/data/Cars_1920x1080.pgm");
  //sprintf(fname, "data/Cars_320x180.pgm");
  //sprintf(fname, "appFramework/data/Cars_320x180.pgm");
  image = (PGMImage *) malloc(sizeof(PGMImage));
  image = readPGM(fname);

  IMAGE_WIDTH = image->x;
  IMAGE_HEIGHT = image->y;
  // Initialize DMA and tile manager objects
  idma_log_handler(idmaLogHander);  
  // Initialize the DMA
  retVal = xvInitIdma(pxvTM, (idma_buffer_t *) idmaObjBuff, DMA_DESCR_CNT, MAX_BLOCK_16, MAX_PIF, errCallbackFunc, intrCallbackFunc, (void *) &cbData);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
    //return(RET_ERROR);
  }

  // Initialize tile manager
  retVal = xvInitTileManager(pxvTM, (void *) idmaObjBuff);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  // Initializing Memory allocator
  buffPool[0] = pBankBuffPool0;
  buffPool[1] = pBankBuffPool1;
  buffSize[0] = POOL_SIZE;
  buffSize[1] = POOL_SIZE;
  retVal      = xvInitMemAllocator(pxvTM, 2, buffPool, buffSize);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }
  //taskDISABLE_INTERRUPTS();
  //taskENTER_CRITICAL();
  // Setup input and output frames.
  srcWidth  = image->x;
  srcHeight = image->y;
  srcBytes  = image->compWidth;
  gSrc      = (uint8_t *) image->data;
  gOut      = memDstFrame;
  frameSize = srcWidth * srcHeight;
  pInFrame  = xvAllocateFrame(pxvTM);
  if ((int32_t) pInFrame == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
   // return(RET_ERROR);
  }
  SETUP_FRAME(pInFrame, gSrc, frameSize, srcWidth, srcHeight, srcWidth, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);

  pOutFrame = xvAllocateFrame(pxvTM);
  if ((int32_t) pOutFrame == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }
  SETUP_FRAME(pOutFrame, gOut, frameSize, IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_WIDTH, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);

  memset(gOut, 0, IMAGE_WIDTH * IMAGE_HEIGHT);

  // Reset the interrupt count in the cbData structure
  cbData.intrCount = 0;


  K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  /////////////////////////// 
  //INNER pIN

  // Allocate buffers for source and destination tiles.
  // Memory is allocated from memory banks.
  // Allocate the tiles and initialize the elements
  tileBuffSize   = TILE_WIDTH * TILE_HEIGHT;
  pinTileBuff[0] = xvAllocateBuffer(pxvTM, tileBuffSize, XV_MEM_BANK_COLOR_0, 64);
  if ((int32_t) pinTileBuff[0] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  pInTile[0] = xvAllocateTile(pxvTM);
  if ((int32_t) pInTile[0] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
   // return(RET_ERROR);
  }
#ifndef ERROR_CALLBACK_TEST
  SETUP_TILE(pInTile[0], pinTileBuff[0], tileBuffSize, pInFrame, TILE_WIDTH, TILE_HEIGHT, TILE_WIDTH, XV_TILE_U8, 0, 0, 0, 0, alignType);
#else
  SETUP_TILE(pInTile[0], XV_FRAME_GET_BUFF_PTR(pInFrame), tileBuffSize, pInFrame, TILE_WIDTH, TILE_HEIGHT, TILE_WIDTH, XV_TILE_U8, 0, 0, 0, 0, alignType);
#endif

  pinTileBuff[1] = xvAllocateBuffer(pxvTM, tileBuffSize, XV_MEM_BANK_COLOR_0, 64);
  if ((int32_t) pinTileBuff[1] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  pInTile[1] = xvAllocateTile(pxvTM);
  if ((int32_t) pInTile[1] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  // return(RET_ERROR);
  }
  SETUP_TILE(pInTile[1], pinTileBuff[1], tileBuffSize, pInFrame, TILE_WIDTH, TILE_HEIGHT, TILE_WIDTH, XV_TILE_U8, 0, 0, 0, 0, alignType);

  /////////////////////////// 
  //inner pout
  poutTileBuff[0] = xvAllocateBuffer(pxvTM, tileBuffSize, XV_MEM_BANK_COLOR_1, 64);
  if ((int32_t) poutTileBuff[0] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  pOutTile[0] = xvAllocateTile(pxvTM);
  if ((int32_t) pOutTile[0] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
   // return(RET_ERROR);
  }
  SETUP_TILE(pOutTile[0], poutTileBuff[0], tileBuffSize, pOutFrame, TILE_WIDTH, TILE_HEIGHT, TILE_WIDTH, XV_TILE_U8, 0, 0, 0, 0, alignType);

  poutTileBuff[1] = xvAllocateBuffer(pxvTM, tileBuffSize, XV_MEM_BANK_COLOR_1, 64);
  if ((int32_t) poutTileBuff[1] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  pOutTile[1] = xvAllocateTile(pxvTM);
  if ((int32_t) pOutTile[1] == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }
  SETUP_TILE(pOutTile[1], poutTileBuff[1], tileBuffSize, pOutFrame, TILE_WIDTH, TILE_HEIGHT, TILE_WIDTH, XV_TILE_U8, 0, 0, 0, 0, alignType);


  /////////////////////////// 
  //END

  pingPongFlag = 0;
  int32_t tmIndX = 0;
  int32_t tmIndY = 0;

  XV_TILE_SET_X_COORD(pInTile[pingPongFlag], tmIndX);
  XV_TILE_SET_Y_COORD(pInTile[pingPongFlag], tmIndY);
  // Initiate data transfer of first tile into ping buffer
  retVal = xvReqTileTransferIn(pxvTM, pInTile[pingPongFlag], NULL, INTERRUPT_ON_COMPLETION);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
   // return(RET_ERROR);
  }



  pingPongFlag = pingPongFlag ^ 0x1;
  tmIndX      += TILE_WIDTH;
  if (tmIndX >= IMAGE_WIDTH || (tmIndX + TILE_WIDTH) > IMAGE_WIDTH)
  {
    tmIndY += TILE_HEIGHT;
    tmIndX  = 0;
    if (tmIndY >= IMAGE_HEIGHT || (tmIndY + TILE_HEIGHT) > IMAGE_HEIGHT)
    {
      tmIndY = 0;
    }
  }

  XV_TILE_SET_X_COORD(pInTile[pingPongFlag], tmIndX);
  XV_TILE_SET_Y_COORD(pInTile[pingPongFlag], tmIndY);
  // Initiate data transfer of second tile into pong buffer
  retVal = xvReqTileTransferIn(pxvTM, pInTile[pingPongFlag], NULL, INTERRUPT_ON_COMPLETION);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
   // return(RET_ERROR);
  }

  pingPongFlag = pingPongFlag ^ 0x1;
  tmIndX      += TILE_WIDTH;
  if (tmIndX >= IMAGE_WIDTH || (tmIndX + TILE_WIDTH) > IMAGE_WIDTH)
  {
    tmIndY += TILE_HEIGHT;
    tmIndX  = 0;
    if (tmIndY >= IMAGE_HEIGHT || (tmIndY + TILE_HEIGHT) > IMAGE_HEIGHT)
    {
      tmIndY = 0;
    }
  }

  for (dsty = 0; dsty < IMAGE_HEIGHT; dsty += TILE_HEIGHT)
  {
    for (dstx = 0; dstx < IMAGE_WIDTH; dstx += TILE_WIDTH)
    {
      WAIT_FOR_TILE(pxvTM, pInTile[pingPongFlag]);
      //vTaskDelay(100);
      if(XVTM_IS_TRANSFER_SUCCESS(pxvTM) == 0)
      {
	    // If iDMA error occurs, application can either reset the DMA, return from the current function or can exit.
		// In this example, iDMA is initialized again. 
		// Application needs to reset the exception and iDMA if it needs to use Tile Manager again.
  	    XVTM_RESET_EXCEPTION();
        retVal = xvInitIdma(pxvTM, (idma_buffer_t *) idmaObjBuff, DMA_DESCR_CNT, MAX_BLOCK_16, MAX_PIF, errCallbackFunc, intrCallbackFunc, (void *) &cbData);
        if (retVal == XVTM_ERROR)
        {
          xvGetErrorInfo(pxvTM);
     //     return(RET_ERROR);
        }
        K_PrintASSERT(0, " dma error\n");
      }
      // Process the input tile data and write results into output tile
      //USER_DEFINED_HOOKS_START();
#pragma no_reorder
      TIME_STAMP(cycleStart);
#pragma no_reorder
      processData(pInTile[pingPongFlag], pOutTile[pingPongFlag]);
#pragma no_reorder
      TIME_STAMP(cycleStop);
#pragma no_reorder
      //USER_DEFINED_HOOKS_STOP();
      tileCount++;
      printf("tileCount =%d  cycles=%d\n", tileCount, cycleStop-cycleStart);
      totalCycles += (cycleStop - cycleStart);

#pragma no_reorder
      TIME_STAMP(cycleStart2);
#pragma no_reorder
      // Initiate transfer from output tile data to output frame
      retVal = xvReqTileTransferOut(pxvTM, pOutTile[pingPongFlag], INTERRUPT_ON_COMPLETION);
#pragma no_reorder
      TIME_STAMP(cycleStop2);
      totalCycles2 += (cycleStop2 - cycleStart2);



      if (retVal == XVTM_ERROR)
      {
        xvGetErrorInfo(pxvTM);
        K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
    //    return(RET_ERROR);
      }

      //vTaskDelay(10);
      // Initiate transfer for next input tile
      XV_TILE_SET_X_COORD(pInTile[pingPongFlag], tmIndX);
      XV_TILE_SET_Y_COORD(pInTile[pingPongFlag], tmIndY);
      retVal = xvReqTileTransferIn(pxvTM, pInTile[pingPongFlag], NULL, INTERRUPT_ON_COMPLETION);
      if (retVal == XVTM_ERROR)
      {
        xvGetErrorInfo(pxvTM);
        K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
     //   return(RET_ERROR);
      }

      // flip the ping pong flag
      pingPongFlag = pingPongFlag ^ 0x1;
      tmIndX      += TILE_WIDTH;
      if (tmIndX >= IMAGE_WIDTH || (tmIndX + TILE_WIDTH) > IMAGE_WIDTH)
      {
        tmIndY += TILE_HEIGHT;
        tmIndX  = 0;
        if (tmIndY >= IMAGE_HEIGHT)
        {
          tmIndY = 0;
        }
      }
    }
  }
  //taskENABLE_INTERRUPTS();
  //taskEXIT_CRITICAL();
  // Wait for the last output tile transfer
  WAIT_FOR_TILE(pxvTM, pOutTile[pingPongFlag ^ 0x01]);
  oimage.compWidth = 8;
  //write output
  oimage.x    = IMAGE_WIDTH;
  oimage.y    = IMAGE_HEIGHT;
  oimage.data = gOut;
  str_len     = sprintf(oname, "data/Cars_%dx%d", oimage.x, oimage.y);
  str_len     = sprintf(oname + str_len, "_out.pgm");
  printf("Writing Output: %s\n", oname);
  //writePGM(oname, &oimage);

  printf("Total tiles: %d, Interrupt Count = %d\n", tileCount, cbData.intrCount);
  int32_t result = checkImage(gSrc, gOut, IMAGE_WIDTH / TILE_WIDTH * TILE_WIDTH, IMAGE_HEIGHT / TILE_HEIGHT * TILE_HEIGHT, IMAGE_WIDTH);
  if (result)
  {
    printf("\nappFramework\tprocessData\t%f\tCPP\tFAIL\n", (float) totalCycles / (float) (IMAGE_WIDTH * IMAGE_HEIGHT));
  }
  else
  {
    printf("\nappFramework\tprocessData\t%f\tCPP\tPASS\n", (float) totalCycles / (float) (IMAGE_WIDTH * IMAGE_HEIGHT));
  }

  printf("total IDMA config transfer time =%f\n", (float)totalCycles2/(float)tileCount);
  // Free input image
  free(gSrc);
  free(image);

  // Free frames
  retVal = xvFreeFrame(pxvTM, pInFrame);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  retVal = xvFreeFrame(pxvTM, pOutFrame);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  // Free tile data buffers
  retVal = xvFreeBuffer(pxvTM, pinTileBuff[0]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  retVal = xvFreeBuffer(pxvTM, pinTileBuff[1]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  retVal = xvFreeBuffer(pxvTM, poutTileBuff[0]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  retVal = xvFreeBuffer(pxvTM, poutTileBuff[1]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  // Free tiles
  retVal = xvFreeTile(pxvTM, pInTile[0]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  retVal = xvFreeTile(pxvTM, pInTile[1]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  retVal = xvFreeTile(pxvTM, pOutTile[0]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  retVal = xvFreeTile(pxvTM, pOutTile[1]);
  if (retVal == XVTM_ERROR)
  {
    xvGetErrorInfo(pxvTM);
    K_PrintASSERT(0, "framework_fail at line %d FILE=%s", __LINE__, __FILE__);
  //  return(RET_ERROR);
  }

  printf("\nDone\n");

  xvResetTileManager(pxvTM);
  vTaskDelete( NULL );
 // return(0);
}

