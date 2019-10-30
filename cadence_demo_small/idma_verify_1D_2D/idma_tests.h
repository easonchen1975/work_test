/*
 * Customer ID=14510; Build=0x82e2e; Copyright (c) 2015 by Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of S8ge, to any person obtaining a copy
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

#include <xtensa/config/core.h>
#include <xtensa/idma.h>

#ifdef _XOS
#  include <xtensa/xos.h>
#  include <xtensa/xtutil.h>
#  define printf      xt_printf
#endif

#ifdef IDMA_DEBUG
#define DPRINT(fmt...)	do { printf(fmt);} while (0);
#else
#define DPRINT(fmt...)	do {} while (0);
#endif

#ifndef U8
#define U8	unsigned char
#endif

#ifndef S8
#define S8	char
#endif

#ifndef U32
#define U32	unsigned int
#endif

int idma_status_error_code;
#define ALWAYS_DPRINT printf

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

void
bufRandomize(S8 *ptr, int size)
{
  int i;
  for(i = 0 ; i < size; i++)
    ptr[i] = rand();
#if 0
  DPRINT("Image %p :", ptr);
  for(i = 0; i < size; i++)
    DPRINT("%x-", ptr[i]);
  DPRINT("\n");
#endif
 // xthal_dcache_region_writeback_inv(ptr, size);
}


void idmaLogHander (const S8* str)
{
  DPRINT("**iDMAlib**: %s", str);
}

void printBuffer(S8* src, int size)
{
  int i;
  for(i = 0; i < size; i++)
    DPRINT("%x-", src[i]);
  DPRINT("\n");
}

void
compareBuffers(S8* src, S8* dst, int size)
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
idmaErrCB(const idma_error_details_t* data)
{
  DPRINT("ERROR CALLBACK: iDMA in Error\n");
  idma_error_details_t* error = idma_error_details();
  printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
  idma_status_error_code = error->err_type;
}

//************** XOS STUFF *****************//
#ifdef _XOS
#define NUM_THREADS 1

#define STACK_SIZE      (0x400 + XCHAL_TOTAL_SA_SIZE)
#define TICK_CYCLES     (xos_get_clock_freq()/100)
#define TIMER_INTERVAL   0x1423

XosThread test_tcb;
U8   test_stack[STACK_SIZE];

int test(void* arg, int unused);

XosThread test_threads[NUM_THREADS];
U8   test_stacks[NUM_THREADS][STACK_SIZE];

int
test_xos()
{
    int32_t ret;

    xos_set_clock_freq(XOS_CLOCK_FREQ);
    ret = xos_thread_create(&test_tcb,
                            0,
                            test,
                            0,
                            "test",
                            test_stack,
                            STACK_SIZE,
                            7,
                            0,
                            0);
    xos_start(0);
    return -1; // should never get here
}
#endif
