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
   Schedule various tasks and wait for their completion.  
   Wait for task completion using polling wait.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtensa/idma.h>
#include "idma_tests.h"


#ifdef _FREERTOS_
#include "FreeRTOS.h"
#include "semphr.h"
extern  QueueHandle_t IDMA_SEMA;
#endif



#define IMAGE_CHUNK_SIZE	100
#define IDMA_XFER_SIZE		IDMA_SIZE(IMAGE_CHUNK_SIZE)
#define IDMA_IMAGE_SIZE		IMAGE_CHUNK_SIZE*200
#define IDMA_INTERRUPT_ENABLE  0x80000000
#define IDMA_INTERRUPT_DISABLE 0x00000000

IDMA_BUFFER_DEFINE(task_test1npu, 1, IDMA_1D_DESC);
IDMA_BUFFER_DEFINE(task, 10, IDMA_1D_DESC);
#define NPU_SIZE 0x100

ALIGNDCACHE char npu_base[NPU_SIZE] __attribute__ ((section(".sram.npu")));

ALIGNDCACHE char dst1[IDMA_XFER_SIZE] ; //__attribute__ ((section(".sram.npu")));
ALIGNDCACHE char src1[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst2[IDMA_XFER_SIZE];
ALIGNDCACHE char src2[IDMA_XFER_SIZE] IDMA_DRAM;
ALIGNDCACHE char dst3[IDMA_XFER_SIZE];
ALIGNDCACHE char src3[IDMA_XFER_SIZE] IDMA_DRAM;

void compareBuffers_task(idma_buffer_t* task, char* src, char* dst, int size)
{
  int status = idma_task_status(task);
  printf("Task %p status: %s\n", task,
                    (status == IDMA_TASK_DONE)    ? "DONE"       :
                    (status > IDMA_TASK_DONE)     ? "PENDING"    :
                    (status == IDMA_TASK_ABORTED) ? "ABORTED"    :
                    (status == IDMA_TASK_ERROR)   ? "ERROR"      :  "UNKNOWN"   );

  if(status == IDMA_TASK_ERROR) {
    idma_error_details_t* error = idma_error_details();
    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    return;
  }

  if(src && dst) {
    xthal_dcache_region_invalidate(dst, size);
    compareBuffers(src, dst, size);
  }
}


void seq_fill(unsigned char *dst, int size, unsigned char a)
{
   int i;

   for(i=0; i< size; i++){

	   *(dst+i) = a;
   }

}

#define NPU_TEST1_SYSRAM 0
#define NPU_TEST2_SYSRAM 0x4000000
#define NPU_TEST3_SYSRAM 0x8000008
#define NPU_TEST4_SYSRAM 0xC000000
#define XSIZE 512

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
//   bufRandomize(src1, IDMA_XFER_SIZE);
//   bufRandomize(src2, IDMA_XFER_SIZE);
//   bufRandomize(src3, IDMA_XFER_SIZE);
	int ret;

   unsigned char *npu_test1_base = npu_base + NPU_TEST1_SYSRAM;
   unsigned char *npu_test1_base_results = npu_test1_base + 0x1000000;
   unsigned char *npu_test2_base = npu_base + NPU_TEST2_SYSRAM;
   unsigned char *npu_test3_base = npu_base + NPU_TEST3_SYSRAM;
   unsigned char *npu_test4_base = npu_base + NPU_TEST4_SYSRAM;
   //uint32_t *pint = (unsigned int *)( npu_test1_base+1);

   idma_log_handler(idmaLogHander);
   //*pint = 0x5a;

   seq_fill(npu_test1_base, 128, 0xFF );
//   seq_fill(npu_test2_base, 128, 0x5A );
//   seq_fill(npu_test3_base, 128, 0xA5 );
//   seq_fill(npu_test4_base, 128, 0xAA );



   idma_init(0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, NULL);


   idma_init_task(task_test1npu, IDMA_1D_DESC, 1, NULL, NULL);

   idma_add_desc(task_test1npu, src1, npu_test1_base, XSIZE, IDMA_INTERRUPT_ENABLE );

   idma_schedule_task(task_test1npu);

#ifdef _FREERTOS_
   printf("xSemaphoreTake\n");
   ret = xSemaphoreTake(IDMA_SEMA, 0 );
   if(ret != pdPASS)
	   K_ASSERT(0, "xSemaphoreTake err =%x", ret);

   printf("xSemaphoreTake leaving ...\n");
#else
   while (idma_task_status(task_test1npu) > 0)
      idma_process_tasks();
#endif
   compareBuffers_task(task, src1, npu_test1_base, XSIZE );
   compareBuffers_task(task, npu_test1_base, npu_test1_base_results, XSIZE );


   //  move internal to 0x71000000


   idma_init_task(task_test1npu, IDMA_1D_DESC, 1, NULL, NULL);
   idma_add_desc(task_test1npu, npu_test1_base_results, src1, XSIZE, IDMA_INTERRUPT_ENABLE );
   idma_schedule_task(task_test1npu);

#ifdef _FREERTOS_
   printf("xSemaphoreTake\n");
   ret = xSemaphoreTake(IDMA_SEMA, portMAX_DELAY );
   if(ret != pdPASS)
	   K_ASSERT(0, "xSemaphoreTake err =%x", ret);

   printf("xSemaphoreTake leaving ...\n");
#else
   while (idma_task_status(task_test1npu) > 0)
      idma_process_tasks();
#endif

   compareBuffers_task(task, src1, npu_test1_base_results, XSIZE );
   compareBuffers_task(task, npu_test1_base, npu_test1_base_results, XSIZE );


   /*
   idma_init_task(task, IDMA_1D_DESC, 10, NULL, NULL);

   DPRINT("Setting IDMA request %p (4 descriptors)\n", task);
   idma_add_desc(task, dst1, src1, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst2, src2, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst3, src3, IDMA_XFER_SIZE, 0 );

   idma_add_desc(task, dst1, src1, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst2, src2, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst3, src3, IDMA_XFER_SIZE, 0 );

   idma_add_desc(task, dst1, src1, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst2, src2, IDMA_XFER_SIZE, 0 );
   idma_add_desc(task, dst3, src3, IDMA_XFER_SIZE, 0 );

   idma_add_desc(task, dst1, src1, IDMA_XFER_SIZE, 0 );

   DPRINT("Schedule IDMA request\n");
   idma_schedule_task(task);

   while (idma_task_status(task) > 0)
      idma_process_tasks();

   compareBuffers_task(task, src1, dst1, IDMA_XFER_SIZE );
   compareBuffers_task(task, src2, dst2, IDMA_XFER_SIZE );
   compareBuffers_task(task, src3, dst3, IDMA_XFER_SIZE );
*/
  // exit(0);

   // Set termination flag to alert Init_Task.
  // xEventGroupSetBits( TaskTermFlags, TASK_TERM_IDMA );

   // Terminate this task. RTOS will continue to run otfher tasks.
   //vTaskDelete( NULL );
   return -1;
}



//main (int argc, char**argv)
void idma_main( void * pdata )
{
   int ret = 0;
//  printf("\n\n\nTest '%s'\n\n\n", argv[0]);


 //  return ;
#if defined _XOS
   ret = test_xos();
#else
   ret = test(0, 0);
   ret = test(0, 0);
   void appframework( void * pdata );
   //appframework2( (void *)NULL );
   appframework( (void *)NULL );
#endif

   vTaskDelete( NULL );
   // test() exits so this is never reached
  //return ret;
}


/* move to k_debug.h
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
