/*
 * Customer ID=14510; Build=0x82e2e; Copyright (c) 2014 by Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
/* EXAMPLE:
  Execute 2D transfer from main to the local memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>
#include "commonDef.h"

//#define IDMA_LIB_BUILD
#include "idma_tests.h"
#include "k_debug.h"

#define ROW_SIZE  32
#define NUM_ROWS  32
#define SRC_PITCH 128
#define DST_PITCH 128

#define UNALIGN_ROW_SIZE  65
#define UNALIGN_NUM_ROWS  13
#define SRC_PITCH 128
#define DST_PITCH 128


#define IDMA_IMAGE_SIZE		NUM_ROWS*ROW_SIZE

ALIGNDCACHE char _lbuf1[DST_PITCH*NUM_ROWS] IDMA_DRAM ;
ALIGNDCACHE char _lbuf2[DST_PITCH*NUM_ROWS] IDMA_DRAM ;
ALIGNDCACHE char _mem1[SRC_PITCH*NUM_ROWS] IDMA_DRAM;
ALIGNDCACHE char _mem2[SRC_PITCH*NUM_ROWS] IDMA_DRAM;

//ALIGNDCACHE char unaligned_mem1[SRC_PITCH*NUM_ROWS];
//ALIGNDCACHE char unaligned_mem2[SRC_PITCH*NUM_ROWS];

char dram_1[DST_PITCH*NUM_ROWS] __attribute__ ((section(".sram.data")));
char dram_2[DST_PITCH*NUM_ROWS] __attribute__ ((section(".sram.data")));
char golden_dram_1[DST_PITCH*NUM_ROWS] __attribute__ ((section(".sram.data")));
char golden_dram_2[DST_PITCH*NUM_ROWS] __attribute__ ((section(".sram.data")));

IDMA_BUFFER_DEFINE(task1, 1, IDMA_2D_DESC);
IDMA_BUFFER_DEFINE(task2, 1, IDMA_2D_DESC);
//IDMA_BUFFER_DEFINE(task3, 1, IDMA_2D_DESC);

//extern int idma_status_error_code;
int verify_item_num;

/*
#define K_ASSERT assert_msg2

#define assert_msg(val, _msg) \
{ \
	 if(val==0){ \
	    printf("%s \n",_msg); \
	    printf("ASSERT FAIL at"__FILE__" line:%d\n", __LINE__) ; \
	    while(0); \
     } \
}


#define assert_msg2(val, _msg... ) \
{ \
	 if(val==0){ \
	    printf("ASSERT FAIL at"__FILE__" line:%d\n", __LINE__) ; \
	    printf(_msg); \
	    while(1); \
     } \
}\
*/
//{
	
	/*if(!_cond){
		ALWAYS_DPRINT("Assert Fail at "__FILE__" line :%d\n", __LINE__);
		ALWAYS_DPRINT(fmt);
	}*/
//}//while(0)

#if 1
int32_t idma_read_status_reg()
{

 //error->err_type = (status >> IDMA_ERRCODES_SHIFT);
 //error->currDesc =
#define IDMA_ERRCODES_SHIFT 18
   return( READ_IDMA_REG(0, IDMA_REG_STATUS )>> IDMA_ERRCODES_SHIFT );

}

#endif

int32_t idma_read_status_runmode_reg()
{
/* Set error details for current task */
 //error->err_type = (status >> IDMA_ERRCODES_SHIFT);
 //error->currDesc =
   return( READ_IDMA_REG(0, IDMA_REG_STATUS )& 0x3 /*IDMA_ERRCODES_SHIFT*/);

}



void seq_fill(unsigned char *dst, int size)
{
   int i;

   for(i=0; i< size; i++){

	   *(dst+i) = i;
   }

}


void copy_2d_fill(unsigned char *dst,unsigned char *src, int row_size, int src_pitch, int dst_pitch, int size)
{
	int i;
	  /* Compare must skip the pitch for src and dst */
	  for(i = 0 ; i < size; i++) {
	    int src_i = (src_pitch) * (i / row_size)  + i % row_size;
	    int dst_i = (dst_pitch) * (i / row_size)  + i % row_size;
	    //DPRINT("byte src:%d dst:%d (dst:%x, src:%x)!!\n", src_i, dst_i, dst[dst_i], src[src_i]);
	    dst[dst_i] = src[src_i];
	  }

}

void
compareBuffers_All(S8* src, S8* dst, int size)
{
  int i;
  for(i = 0 ; i < size; i++)
   if (dst[i] != src[i]) {
     printf("COPY FAILED @ byte %d (dst:%x, src:%x)!!\n", i, dst[i], src[i]);
     return;
   }
   printf("COPY OK (src:%p, dst:%p)\n", src, dst);
}

void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for task @ %p is done\n", data);
}


void compareBuffers_task(idma_buffer_t* task, char* src, char* dst, int size)
{
  int i ;
  int status;

  status = idma_task_status(task);
  DPRINT("Task %p status: %s\n", task,
                    (status == IDMA_TASK_DONE)    ? "DONE"       :
                    (status > IDMA_TASK_DONE)     ? "PENDING"    :
                    (status == IDMA_TASK_ABORTED) ? "ABORTED"    :
                    (status == IDMA_TASK_ERROR)   ? "ERROR"      :  "UNKNOWN"   );

  if(status == IDMA_TASK_ERROR) {
    idma_error_details_t* error = idma_error_details();
    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    exit(0);
    return;
  }

  if(!src || !dst)
    return;

  /* Compare must skip the pitch for src and dst */
  for(i = 0 ; i < size; i++) {
    int src_i = (SRC_PITCH) * (i / ROW_SIZE)  + i % ROW_SIZE;
    int dst_i = (DST_PITCH) * (i / ROW_SIZE)  + i % ROW_SIZE;
    //DPRINT("byte src:%d dst:%d (dst:%x, src:%x)!!\n", src_i, dst_i, dst[dst_i], src[src_i]);
    if (dst[dst_i] != src[src_i]) {
     printf("COPY FAILED @ byte %d (dst:%x, src:%x, %x)!!\n", i, dst[dst_i], src[src_i], dst[dst_i+1]);
     return;
    }
  }
  printf("COPY OK (src:%p, dst:%p)\n", src, dst);
}

void error_handling_verify(){

#ifdef IDMA_PERF
	  uint32_t cyclesStart, cyclesStop,cyclesIVP, test[128];

	          cyclesStart=0;
	          cyclesStop=0;
	          cyclesIVP=0;
	          test[10] = 0x55;

	          test[0x20] = 0x88;

	  TIME_STAMP(cyclesStart);
#endif

	  idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 0, idmaErrCB);

#if 1
	  DPRINT("\n%d. Start to Verify TIMEOUT ...\n", verify_item_num)
      idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 20, idmaErrCB);
	  // 1.  32x32  dmem to dmem
	  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
	  DPRINT("  Verify STATUS¡@TIMEOUT \n");
	  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem1, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem2, SRC_PITCH*NUM_ROWS);

	  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
	  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);

	  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);

	  DPRINT("  Schedule DMA %p -> %p\n", _mem2, _lbuf2);
	  DPRINT("  Schedule DMA %p -> %p\n", _mem1, _lbuf1);

#ifdef IDMA_PERF
	  TIME_STAMP(cyclesStart);
#endif
	  idma_schedule_task(task1);
//	  idma_schedule_task(task2);

	  DPRINT("  Waiting for DMAs to finish...\n");
	  while (idma_task_status(task2) > 0){
		  idma_process_tasks();
	  	}

#ifdef IDMA_PERF	  	
  		  TIME_STAMP(cyclesStop);
#endif

	  idma_status_error_code = idma_read_status_reg();
	  if(idma_status_error_code != IDMA_ERR_REG_TIMEOUT){
	  	K_ASSERT(0 , "idma_status_error_code != IDMA_ERR_REG_TIMEOUT\n");
	  }
      DPRINT("idma_status_error_code =%x\n", idma_status_error_code);

#ifdef IDMA_PERF	  	
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif      

  	  DPRINT("===>[Item %d] :  Verify IDMA_TIMEOUT : PASS\n", verify_item_num);
  	  verify_item_num++;

    	//  idma_sleep();

#endif

#if 1
	  DPRINT("\n%d. Start to Verify IDMA_IDMA_ERR_FETCH_ADDR ...\n", verify_item_num)
	  idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 0, idmaErrCB);
	  // 1.  32x32  dmem to dmem
	  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
	  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem1, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem2, SRC_PITCH*NUM_ROWS);

	  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
	  memset(task2, 0, IDMA_BUFFER_SIZE(1, IDMA_2D_DESC)*sizeof(idma_buffer_t));
	  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
	  //idma_init_task(task3, IDMA_2D_DESC, 1, idma_cb_func, task3);
	  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);

	  DPRINT("  Schedule DMA %p -> %p\n", _mem2, _lbuf2);
	  DPRINT("  Schedule DMA %p -> %p\n", _mem1, _lbuf1);

	  idma_schedule_task(task1);
	  idma_schedule_task(task2);

	  DPRINT("  Waiting for DMAs to finish...\n");
	  while (idma_task_status(task2) > 0)
		  idma_process_tasks();

	  idma_status_error_code = idma_read_status_reg();
	  K_ASSERT(idma_status_error_code == IDMA_ERR_FETCH_ADDR, "idma_status_error_code != IDMA_ERR_FETCH_ADDR 0x%x\n", idma_status_error_code);

	  DPRINT("status code =0x%x\n", idma_status_error_code)
	  DPRINT("===>[Item %d] :  Verify IDMA_IDMA_ERR_FETCH_ADDR : PASS\n", verify_item_num);
	  verify_item_num++;


#endif
	  /*
	  if(idma_status_error_code != IDMA_ERR_FETCH_ADDR){
	  	K_ASSERT(0 , "idma_status_error_code != IDMA_ERR_FETCH_ADDR %d\n", idma_status_error_code);
	  }
	  else
	  	DPRINT("Verify IDMA_ERR_FETCH_ADDR : PASS");

	  	*/
	   //  idma_sleep();

/*
	  idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);
	  // 1.  32x32  dmem to dmem
	  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
	  DPRINT("  Verify STATUS¡@ Descriptor: unknown command + TIMEOUT \n");
	  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem1, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem2, SRC_PITCH*NUM_ROWS);

	  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
	  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
	  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);

	  DPRINT("  Schedule DMA %p -> %p\n", _mem2, _lbuf2);
	  DPRINT("  Schedule DMA %p -> %p\n", _mem1, _lbuf1);

	  idma_schedule_task(task1);
	  idma_schedule_task(task2);

	  DPRINT("  Waiting for DMAs to finish...\n");
	  while (idma_task_status(task2) > 0)
		  idma_process_tasks();

	  //idma_status_error_code =2;
	  K_ASSERT(0 , "idma_status_error_code != IDMA_ERR_FETCH_ADDR %d\n", idma_status_error_code);
*/
	  // 3. Verify IDMA reset,
	  DPRINT("\n%d. Start to Verify reset ...\n", verify_item_num)
	  idma_abort_tasks();
	  idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 0, idmaErrCB);
	  // 1.  32x32  dmem to dmem
	  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
	  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem1, SRC_PITCH*NUM_ROWS);
	  xthal_dcache_region_writeback_inv(_mem2, SRC_PITCH*NUM_ROWS);

	  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
	  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
	  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
	  idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);

	  DPRINT("  Schedule DMA %p -> %p\n", _mem2, _lbuf2);
	  DPRINT("  Schedule DMA %p -> %p\n", _mem1, _lbuf1);

	  idma_schedule_task(task1);
	  idma_schedule_task(task2);

	  DPRINT("  Waiting for DMAs to finish...\n");
	  while (idma_task_status(task2) > 0)
		  idma_process_tasks();

	  /*
	  if(idma_status_error_code != IDMA_ERR_REG_TIMEOUT){
	  	K_ASSERT(0 , "idma_status_error_code != IDMA_ERR_REG_TIMEOUT\n");
	  }
	  else
	  	DPRINT("Verify IDMA_TIMEOUT : PASS");
      */
	  idma_status_error_code = idma_read_status_reg();

	  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);
	  DPRINT("status code =0x%x\n", idma_status_error_code)
	  DPRINT("===>[Item %d] :  Verify IDMA_reset : PASS\n", verify_item_num);
	  verify_item_num++;



}

/* THIS IS THE EXAMPLE, main() CALLS IT */
//test(void* arg, int unused)
int idma_2d_verify()
{
//  int idma_status_error_code;

#ifdef IDMA_PERF
		uint32_t cyclesStart, cyclesStop,cyclesIVP, test[128];
  
				cyclesStart=0;
				cyclesStop=0;
				cyclesIVP=0;
				test[10] = 0x55;
  
				test[0x20] = 0x88;
  
		TIME_STAMP(cyclesStart);
#endif


  idma_log_handler(idmaLogHander);
  idma_init(IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);



  // 1.  32x32  dmem to dmem
  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  DPRINT("\n  2D DMEM to DMEM 32x32 \n");
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_writeback_inv(_mem1, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_writeback_inv(_mem2, SRC_PITCH*NUM_ROWS);

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
  
  DPRINT("  Schedule DMA %p -> %p\n", _mem2, _lbuf2);
  DPRINT("  Schedule DMA %p -> %p\n", _mem1, _lbuf1);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif
  idma_schedule_task(task1);    
  idma_schedule_task(task2); 



  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

#ifdef IDMA_PERF
	  TIME_STAMP(cyclesStop);
#endif  
   //  idma_sleep();
  
  xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_task(task1, _mem1, _lbuf1, IDMA_IMAGE_SIZE);
  compareBuffers_task(task2, _mem2, _lbuf2, IDMA_IMAGE_SIZE);

#ifdef IDMA_PERF	  	
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif      

  idma_status_error_code = idma_read_status_reg();  
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===> [Item %d]: Verify 2D DMEM to DMEM 32x32 : PASS]\n", verify_item_num);
  verify_item_num++;


#if 1
  // =======================================
  // 2.  32x32  dmem to dram
  DPRINT("\n%d.   2D DMEM to external dram 32x32 \n", verify_item_num);
  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, dram_1, _mem1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, dram_2, _mem2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", _mem2, dram_1);
  DPRINT("  Schedule DMA %p -> %p\n", _mem1, dram_2);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif
  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

#ifdef IDMA_PERF
	  TIME_STAMP(cyclesStop);
#endif


  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_task(task1, _mem1, dram_1, IDMA_IMAGE_SIZE);
  compareBuffers_task(task2, _mem2, dram_2, IDMA_IMAGE_SIZE);

#ifdef IDMA_PERF
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif

  idma_status_error_code = idma_read_status_reg();
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===>[Item %d]: Verify 2D DMEM to dram 32x32 : PASS\n", verify_item_num);
  verify_item_num++;


  // =======================================
  // 3.  32x32  dram to dmem
  DPRINT("\n%d.   2D external dram to DMEM 32x32 \n", verify_item_num);
  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, _mem1, dram_1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _mem2, dram_2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", dram_1, _mem2);
  DPRINT("  Schedule DMA %p -> %p\n", dram_2, _mem1);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif

  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStop);
#endif

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_task(task1, _mem1, dram_1, IDMA_IMAGE_SIZE);
  compareBuffers_task(task2, _mem2, dram_2, IDMA_IMAGE_SIZE);


#ifdef IDMA_PERF
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif

  idma_status_error_code = idma_read_status_reg();
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===>[Item %d]:Verify 2D dram to dmem 32x32 : PASS\n", verify_item_num);
  verify_item_num++;



  // =======================================
  // 4.  65x13  dmem to dmem
  DPRINT("\n%d.   2D UNALIGNED dmem to DMEM 65x13 \n", verify_item_num);
  //seq_fill(_mem1, SRC_PITCH*NUM_ROWS);
  //seq_fill(_mem2, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_2, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf1, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf2, SRC_PITCH*NUM_ROWS);

  //memset(_lbuf1, 0xAB, DST_PITCH*NUM_ROWS);
  //memset(_lbuf2, 0xAB, DST_PITCH*NUM_ROWS);

  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);
  //void copy_2d_fill(unsigned char *dst,unsigned char *src, int row_size, int src_pitch, int dst_pitch, int size)
  copy_2d_fill(dram_1+3, _mem1+1, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );
  copy_2d_fill(dram_2+5, _mem2+2, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, _lbuf1+3, _mem1+1, UNALIGN_ROW_SIZE, 0, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _lbuf2+5, _mem2+2, UNALIGN_ROW_SIZE, 0, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", dram_1, _mem2);
  DPRINT("  Schedule DMA %p -> %p\n", dram_2, _mem1);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif
  idma_schedule_task(task1);
  idma_schedule_task(task2);

  while(idma_read_status_runmode_reg() != IDMA_STATE_DONE);
#ifdef IDMA_PERF
		TIME_STAMP(cyclesStop);
#endif


  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_All( _lbuf1, dram_1, SRC_PITCH*NUM_ROWS);
  compareBuffers_All( _lbuf2, dram_2, SRC_PITCH*NUM_ROWS);

#ifdef IDMA_PERF
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif

  idma_status_error_code = idma_read_status_reg();
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===>[Item %d] : Verify 2D dmem to dmem 65*13 : PASS\n", verify_item_num);
  verify_item_num++;

  // =======================================
  // 5.  65x13  dmem to dram
  DPRINT("\n%d.   2D UNALIGNED DMEM to dram 65x13 \n", verify_item_num);
  seq_fill(golden_dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(golden_dram_2, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_2, SRC_PITCH*NUM_ROWS);
  //seq_fill(_lbuf1, SRC_PITCH*NUM_ROWS);
  //seq_fill(_lbuf2, SRC_PITCH*NUM_ROWS);

  //memset(_lbuf1, 0xAB, DST_PITCH*NUM_ROWS);
  //memset(_lbuf2, 0xAB, DST_PITCH*NUM_ROWS);

  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);
  //void copy_2d_fill(unsigned char *dst,unsigned char *src, int row_size, int src_pitch, int dst_pitch, int size)
  copy_2d_fill(golden_dram_1+3, _mem1+1, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );
  copy_2d_fill(golden_dram_2+5, _mem2+2, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, dram_1+3, _mem1+1, UNALIGN_ROW_SIZE, 0, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, dram_2+5, _mem2+2, UNALIGN_ROW_SIZE, 0, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", _mem1+1, dram_1+3);
  DPRINT("  Schedule DMA %p -> %p\n", _mem2+2, dram_2+5);

  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_All(golden_dram_1, dram_1, SRC_PITCH*NUM_ROWS);
  compareBuffers_All(golden_dram_2, dram_2, SRC_PITCH*NUM_ROWS);

  DPRINT("===>[Item %d]: Verify 2D UNALIGNED DMEM to dram 65x13  : PASS\n", verify_item_num);
  verify_item_num++;


  // =======================================
  // 6.  65x13  dram to dmem
  DPRINT("\n%d   2D UNALIGNED DRAM to dmem 65x13 \n", verify_item_num);
  seq_fill(golden_dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(golden_dram_2, SRC_PITCH*NUM_ROWS);
  //seq_fill(dram_1, SRC_PITCH*NUM_ROWS);
  //seq_fill(dram_2, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf1, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf2, SRC_PITCH*NUM_ROWS);

  //memset(_lbuf1, 0xAB, DST_PITCH*NUM_ROWS);
  //memset(_lbuf2, 0xAB, DST_PITCH*NUM_ROWS);

  //bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  //bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);
  //void copy_2d_fill(unsigned char *dst,unsigned char *src, int row_size, int src_pitch, int dst_pitch, int size)
  copy_2d_fill(golden_dram_1+3, dram_1+1, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );
  copy_2d_fill(golden_dram_2+5, dram_2+2, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, _lbuf1+3, dram_1+1, UNALIGN_ROW_SIZE, 0, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _lbuf2+5, dram_2+2, UNALIGN_ROW_SIZE, 0, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", dram_1+1, _lbuf1+3);
  DPRINT("  Schedule DMA %p -> %p\n", dram_2+2, _lbuf2+5);

  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);


  compareBuffers_All(golden_dram_1, _lbuf1, SRC_PITCH*NUM_ROWS);
  compareBuffers_All(golden_dram_2, _lbuf2, SRC_PITCH*NUM_ROWS);

  DPRINT("===>[Item %d] :  Verify 2D UNALIGNED DRAM to dmem 65x13  : PASS\n", verify_item_num);
  verify_item_num++;


#endif

  //exit(0);
  return -1;
}



int idma_2d_verify_interrupt()
{
//  int idma_status_error_code;

#ifdef IDMA_PERF
		uint32_t cyclesStart, cyclesStop,cyclesIVP, test[128];

				cyclesStart=0;
				cyclesStop=0;
				cyclesIVP=0;
				test[10] = 0x55;

				test[0x20] = 0x88;

		TIME_STAMP(cyclesStart);
#endif


  idma_log_handler(idmaLogHander);
  idma_init(IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);



  // 1.  32x32  dmem to dmem
  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  DPRINT("\n  2D DMEM to DMEM 32x32 \n");
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_writeback_inv(_mem1, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_writeback_inv(_mem2, SRC_PITCH*NUM_ROWS);

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, 0, NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", _mem2, _lbuf2);
  DPRINT("  Schedule DMA %p -> %p\n", _mem1, _lbuf1);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif
  idma_schedule_task(task1);
  idma_schedule_task(task2);



  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

#ifdef IDMA_PERF
	  TIME_STAMP(cyclesStop);
#endif
   //  idma_sleep();

  xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_task(task1, _mem1, _lbuf1, IDMA_IMAGE_SIZE);
  compareBuffers_task(task2, _mem2, _lbuf2, IDMA_IMAGE_SIZE);

#ifdef IDMA_PERF
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif

  idma_status_error_code = idma_read_status_reg();
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===> [Item %d]: Verify 2D DMEM to DMEM 32x32 : PASS]\n", verify_item_num);
  verify_item_num++;


#if 1
  // =======================================
  // 2.  32x32  dmem to dram
  DPRINT("\n%d.   2D DMEM to external dram 32x32 \n", verify_item_num);
  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, dram_1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, dram_2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", _mem2, dram_1);
  DPRINT("  Schedule DMA %p -> %p\n", _mem1, dram_2);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif
  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

#ifdef IDMA_PERF
	  TIME_STAMP(cyclesStop);
#endif


  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_task(task1, _mem1, dram_1, IDMA_IMAGE_SIZE);
  compareBuffers_task(task2, _mem2, dram_2, IDMA_IMAGE_SIZE);

#ifdef IDMA_PERF
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif

  idma_status_error_code = idma_read_status_reg();
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===>[Item %d]: Verify 2D DMEM to dram 32x32 : PASS\n", verify_item_num);
  verify_item_num++;


  // =======================================
  // 3.  32x32  dram to dmem
  DPRINT("\n%d.   2D external dram to DMEM 32x32 \n", verify_item_num);
  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, _mem1, dram_1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _mem2, dram_2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", dram_1, _mem2);
  DPRINT("  Schedule DMA %p -> %p\n", dram_2, _mem1);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif

  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStop);
#endif

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_task(task1, _mem1, dram_1, IDMA_IMAGE_SIZE);
  compareBuffers_task(task2, _mem2, dram_2, IDMA_IMAGE_SIZE);


#ifdef IDMA_PERF
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif

  idma_status_error_code = idma_read_status_reg();
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===>[Item %d]:Verify 2D dram to dmem 32x32 : PASS\n", verify_item_num);
  verify_item_num++;



  // =======================================
  // 4.  65x13  dmem to dmem
  DPRINT("\n%d.   2D UNALIGNED dmem to DMEM 65x13 \n", verify_item_num);
  //seq_fill(_mem1, SRC_PITCH*NUM_ROWS);
  //seq_fill(_mem2, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_2, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf1, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf2, SRC_PITCH*NUM_ROWS);

  //memset(_lbuf1, 0xAB, DST_PITCH*NUM_ROWS);
  //memset(_lbuf2, 0xAB, DST_PITCH*NUM_ROWS);

  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);
  //void copy_2d_fill(unsigned char *dst,unsigned char *src, int row_size, int src_pitch, int dst_pitch, int size)
  copy_2d_fill(dram_1+3, _mem1+1, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );
  copy_2d_fill(dram_2+5, _mem2+2, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, _lbuf1+3, _mem1+1, UNALIGN_ROW_SIZE, DESC_NOTIFY_W_INT, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _lbuf2+5, _mem2+2, UNALIGN_ROW_SIZE, DESC_NOTIFY_W_INT, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", dram_1, _mem2);
  DPRINT("  Schedule DMA %p -> %p\n", dram_2, _mem1);

#ifdef IDMA_PERF
		TIME_STAMP(cyclesStart);
#endif
  idma_schedule_task(task1);
  idma_schedule_task(task2);

  while(idma_read_status_runmode_reg() != IDMA_STATE_DONE);
#ifdef IDMA_PERF
		TIME_STAMP(cyclesStop);
#endif


  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_All( _lbuf1, dram_1, SRC_PITCH*NUM_ROWS);
  compareBuffers_All( _lbuf2, dram_2, SRC_PITCH*NUM_ROWS);

#ifdef IDMA_PERF
      DPRINT("IDMA cycles = %d\n", cyclesStop-cyclesStart);
#endif

  idma_status_error_code = idma_read_status_reg();
  K_ASSERT(idma_status_error_code==0 , "idma_status_error_code != 0 code=%d\n", idma_status_error_code);

  DPRINT("===>[Item %d] : Verify 2D dmem to dmem 65*13 : PASS\n", verify_item_num);
  verify_item_num++;

  // =======================================
  // 5.  65x13  dmem to dram
  DPRINT("\n%d.   2D UNALIGNED DMEM to dram 65x13 \n", verify_item_num);
  seq_fill(golden_dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(golden_dram_2, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(dram_2, SRC_PITCH*NUM_ROWS);
  //seq_fill(_lbuf1, SRC_PITCH*NUM_ROWS);
  //seq_fill(_lbuf2, SRC_PITCH*NUM_ROWS);

  //memset(_lbuf1, 0xAB, DST_PITCH*NUM_ROWS);
  //memset(_lbuf2, 0xAB, DST_PITCH*NUM_ROWS);

  bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  //bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);
  //void copy_2d_fill(unsigned char *dst,unsigned char *src, int row_size, int src_pitch, int dst_pitch, int size)
  copy_2d_fill(golden_dram_1+3, _mem1+1, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );
  copy_2d_fill(golden_dram_2+5, _mem2+2, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, dram_1+3, _mem1+1, UNALIGN_ROW_SIZE, DESC_NOTIFY_W_INT, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, dram_2+5, _mem2+2, UNALIGN_ROW_SIZE, DESC_NOTIFY_W_INT, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", _mem1+1, dram_1+3);
  DPRINT("  Schedule DMA %p -> %p\n", _mem2+2, dram_2+5);

  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);

  compareBuffers_All(golden_dram_1, dram_1, SRC_PITCH*NUM_ROWS);
  compareBuffers_All(golden_dram_2, dram_2, SRC_PITCH*NUM_ROWS);

  DPRINT("===>[Item %d]: Verify 2D UNALIGNED DMEM to dram 65x13  : PASS\n", verify_item_num);
  verify_item_num++;


  // =======================================
  // 6.  65x13  dram to dmem
  DPRINT("\n%d   2D UNALIGNED DRAM to dmem 65x13 \n", verify_item_num);
  seq_fill(golden_dram_1, SRC_PITCH*NUM_ROWS);
  seq_fill(golden_dram_2, SRC_PITCH*NUM_ROWS);
  //seq_fill(dram_1, SRC_PITCH*NUM_ROWS);
  //seq_fill(dram_2, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf1, SRC_PITCH*NUM_ROWS);
  seq_fill(_lbuf2, SRC_PITCH*NUM_ROWS);

  //memset(_lbuf1, 0xAB, DST_PITCH*NUM_ROWS);
  //memset(_lbuf2, 0xAB, DST_PITCH*NUM_ROWS);

  //bufRandomize(_mem1, SRC_PITCH*NUM_ROWS);
  //bufRandomize(_mem2, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_1, SRC_PITCH*NUM_ROWS);
  bufRandomize(dram_2, SRC_PITCH*NUM_ROWS);
  //void copy_2d_fill(unsigned char *dst,unsigned char *src, int row_size, int src_pitch, int dst_pitch, int size)
  copy_2d_fill(golden_dram_1+3, dram_1+1, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );
  copy_2d_fill(golden_dram_2+5, dram_2+2, UNALIGN_ROW_SIZE, SRC_PITCH, DST_PITCH, UNALIGN_ROW_SIZE*UNALIGN_NUM_ROWS );

  idma_init_task(task1, IDMA_2D_DESC, 1, idma_cb_func, task1);
  idma_init_task(task2, IDMA_2D_DESC, 1, idma_cb_func, task2);
  //idma_add_2d_desc(task1, _lbuf1, _mem1, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  //idma_add_2d_desc(task2, _lbuf2, _mem2, ROW_SIZE, DESC_NOTIFY_W_INT, NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task1, _lbuf1+3, dram_1+1, UNALIGN_ROW_SIZE, DESC_NOTIFY_W_INT, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);
  idma_add_2d_desc(task2, _lbuf2+5, dram_2+2, UNALIGN_ROW_SIZE, DESC_NOTIFY_W_INT, UNALIGN_NUM_ROWS, SRC_PITCH, DST_PITCH);

  DPRINT("  Schedule DMA %p -> %p\n", dram_1+1, _lbuf1+3);
  DPRINT("  Schedule DMA %p -> %p\n", dram_2+2, _lbuf2+5);

  idma_schedule_task(task1);
  idma_schedule_task(task2);

  DPRINT("  Waiting for DMAs to finish...\n");
  while (idma_task_status(task2) > 0)
	  idma_process_tasks();

  //xthal_dcache_region_invalidate(_mem1, SRC_PITCH*NUM_ROWS);
  //xthal_dcache_region_invalidate(_mem2, SRC_PITCH*NUM_ROWS);


  compareBuffers_All(golden_dram_1, _lbuf1, SRC_PITCH*NUM_ROWS);
  compareBuffers_All(golden_dram_2, _lbuf2, SRC_PITCH*NUM_ROWS);

  DPRINT("===>[Item %d] :  Verify 2D UNALIGNED DRAM to dmem 65x13  : PASS\n", verify_item_num);
  verify_item_num++;


#endif

  //exit(0);
  return -1;
}


//int
//main (int argc, char**argv)
void verify_1d_2d( void *pdata)
{
   int ret = 0;
   verify_item_num = 0;
   printf("\n\n\verify_1d_2d '%s'\n\n\n");

#if defined _XOS
   ret = test_xos();
#else
   //ret = test(0, 0);
#endif
  // test() exits so this is never reached

   idma_2d_verify();
   error_handling_verify();
   idma_2d_verify_interrupt();

   vTaskDelete( NULL );
  // exit(0);
 // return ret;
}

/*
#include "xtensa_ops.h"
void K_dump(){

    uint32_t exccause, epc1, epc2, epc3, excvaddr, depc, excsave1, excsave2, interrupt;
    uint32_t intenable, pc, ps, sp;
    uint32_t excinfo[8];

    SP(sp);
    //PC(pc);
    RSR(exccause, exccause);
    RSR(epc1, epc1);
    RSR(epc2, epc2);
    RSR(epc3, epc3);
    RSR(excvaddr, excvaddr);
    RSR(depc, depc);
    RSR(excsave1, excsave1);
    RSR(interrupt, interrupt);
    RSR(intenable, intenable);
    RSR(ps, ps);
    RSR(excsave2,excsave2);
    printf("Fatal interrupt (%d): \n", (int)exccause);
    printf("%s=0x%08x\n", "sp", sp);
    printf("%s=0x%08x\n", "epc1", epc1);
    printf("%s=0x%08x\n", "epc2", epc2);
    printf("%s=0x%08x\n", "epc3", epc3);
    printf("%s=0x%08x\n", "excvaddr", excvaddr);
    printf("%s=0x%08x\n", "depc", depc);
    printf("%s=0x%08x\n", "excsave1", excsave1);
    printf("%s=0x%08x\n", "excsave2", excsave2);
    printf("%s=0x%08x\n", "intenable", intenable);
    printf("%s=0x%08x\n", "interrupt", interrupt);
    printf("%s=0x%08x\n", "ps", ps);

}

void K_dump_exc(){

    uint32_t exccause, epc1, epc2, epc3, excvaddr, depc, excsave1, excsave2, interrupt;
    uint32_t intenable, pc, ps;
    uint32_t excinfo[8];

    WSR(intenable, 0);
    RSR(exccause, exccause);
    RSR(epc1, epc1);
    RSR(epc2, epc2);
    RSR(epc3, epc3);
    RSR(excvaddr, excvaddr);
    RSR(depc, depc);
    RSR(excsave1, excsave1);
    RSR(interrupt, interrupt);
    RSR(intenable, intenable);
    RSR(ps, ps);
    RSR(excsave2,excsave2);
    printf("Fatal exception (%d): \n", (int)exccause);
    printf("%s=0x%08x\n", "epc1", epc1);
    printf("%s=0x%08x\n", "epc2", epc2);
    printf("%s=0x%08x\n", "epc3", epc3);
    printf("%s=0x%08x\n", "excvaddr", excvaddr);
    printf("%s=0x%08x\n", "depc", depc);
    printf("%s=0x%08x\n", "excsave1", excsave1);
    printf("%s=0x%08x\n", "excsave2", excsave2);
    printf("%s=0x%08x\n", "intenable", intenable);
    printf("%s=0x%08x\n", "interrupt", interrupt);
    printf("%s=0x%08x\n", "ps", ps);

}
*/
