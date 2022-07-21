// Copyright 2022 Quillian Rutherford
// A simplistic lockless circular queue.
// MIT license, see License.txt at https://github.com/cerrus/LLCirq

//Need memcpy
#include <string.h>
//And malloc
#include <stdlib.h>
//Long max 
#include <limits.h>
//Bool
#include <stdbool.h>
//printf
#include <stdio.h>

#define DEBUG_CRITICAL 1
#define DEBUG_INFO     2
#define DEBUG_VERBOSE  3

#define DEBUG_LEVEL DEBUG_CRITICAL

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _llcir_q {
	size_t element_size;
	unsigned long num_elements;
	size_t num_writers;
	unsigned long write_index;
	unsigned long read_index;
	void *datap;
} llcir_q, *llcir_qp;


//Elem size in bytes
llcir_qp init_llcirq(size_t element_size, size_t num_elements, size_t num_writers) {
	llcir_qp rtv = (llcir_qp)malloc(sizeof(llcir_q));
	rtv->element_size = element_size;
	rtv->num_writers = num_writers;
	rtv->num_elements = num_elements;
	rtv->write_index = 0;
	rtv->read_index = 0;
	rtv->datap = (void *)malloc(element_size * num_elements);
	return rtv;
}

bool enqueue (llcir_qp q, void *datap) {
	unsigned long index;
	unsigned long write_index;
	unsigned long read_index;
	
	write_index = __sync_and_and_fetch(&q->write_index, (unsigned long)ULONG_MAX);
	read_index  = __sync_and_and_fetch(&q->read_index, (unsigned long)ULONG_MAX);

	if(read_index == write_index + 1) {
#if DEBUG_LEVEL >= DEBUG_INFO
			printf("Q full\n");
#endif
         return false;
	}
	else if(q->read_index == 0 && write_index >= q->num_elements - 1) {
#if DEBUG_LEVEL >= DEBUG_INFO
			printf("Q full at wrap\n");
#endif
			return false;
	}

	unsigned long distance = 0;
	if(read_index  > write_index) {
		distance = read_index - write_index ;
	}
	else {
		distance = read_index;
      distance += q->num_elements - write_index ;
	}
#if DEBUG_LEVEL >= DEBUG_INFO 
		printf("Distance is %lu read %lu write %lu\n",distance, read_index, write_index);
#endif

	//If all threads are past this line at once, we could increment the index by num_threads
	//unexpectedly and overrun the write counter, so take that into account and we always need 
	//to wait for at least num_thread elements ready for data before continuing.  Threads: smh.
	if(distance <= q->num_writers){
		printf("Q dangerously full, forced to stall!\n");
		return false;
	}

	do {
		index = __sync_fetch_and_add(&q->write_index, (unsigned long)1);
		//If we are the bully, take charge.  This is a bit tricky, because we
		//don't have a flexible atomic swap function, so we reset the index to 0 and jump back
		//up to read it again, otherwise we would set it to 1 and write our data
		//out to index 0.  
	   if(index == q->num_elements) { 
			unsigned long tmp = __sync_and_and_fetch(&q->write_index, (unsigned long)0);
			if(tmp != 0) {
				printf("Unable to set q write index to 0, functionality impact!\n");
				return false;
			}
#if DEBUG_LEVEL >= DEBUG_INFO
			printf("Bully set write index to 0\n");
#endif
			continue; //Must fetch again, worst-case scenario we are starved and
			          //end up only being able to constantly reset the index to
			          //0 and never get a time slot, but this is unlikely unless
			          //we have a minimally-sized queue, and not sure we would 
			          //end up starved, as we are only doing a tight loop with
			          //this atomic.  caveat emptor - USE A SIZEABLE QUEUE
		}
      else if(index > q->num_elements){
			//Theory: What if we actually are able to hit the max long limit?  I think we'd
			//overflow and very bad things happen.  So, if we are beyond the wrap and not the
			//bully, we loop without incrementing.   
			while(__sync_and_and_fetch(&q->write_index, (unsigned long)ULONG_MAX) > q->num_elements);
		}
	}while(index >= q->num_elements);

   //Assuming our datap is valid, might have to add checks if safety is a problem.
#if DEBUG_LEVEL >= DEBUG_VERBOSE 
   printf("Enqueue something to <%lu>\n",index);
#endif
	memcpy(q->datap + q->element_size * index,datap,q->element_size);
	return true;
}

bool dequeue(llcir_qp q, void *datap){
	//Should never have these equal except at empty.
	if(q->write_index == q->read_index){ 
#if DEBUG_LEVEL >= DEBUG_INFO
   printf("Read/Write looks same %lu %lu\n",q->write_index, q->read_index);
#endif
		return false;
	}
#if DEBUG_LEVEL >= DEBUG_VERBOSE
   printf("Dequeue element %lu\n",q->read_index);
#endif
	memcpy(datap,q->datap+q->read_index * q->element_size, q->element_size);	
	if(q->read_index == q->num_elements -1)
		q->read_index = 0;
	else
		q->read_index++;
	return true;
}

#ifdef __cplusplus
}
#endif
