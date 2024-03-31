/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "2021010521",
    /* First member's full name */
    "Liu Yadi",
    /* First member's email address */
    "liuyd21@mails.tsinghua.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};


#define VERBOSE 0
#ifdef DEBUG
#define VERBOSE 1
#endif

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Base constants and macros */
#define WSIZE		4		
#define DSIZE		8		
#define CHUNKSIZE	(1<<12)	

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)	((size) | (alloc))

#define GET(p)		(*(unsigned int *)(p))
#define PUT(p, val)	(*(unsigned int *)(p) = val)

#define GET_PTR_VAL(p)	(*(unsigned long *)(p))
#define SET_PTR(p, ptr)	(*(unsigned long *)(p) = (unsigned long)(ptr))

#define GET_PRED(p)	((char *)(*(unsigned long *)(p)))
#define GET_SUCC(p)	((char *)(*(unsigned long *)(p + DSIZE)))
#define SET_PRED(p, ptr)	(SET_PTR((char *)(p), ptr))
#define SET_SUCC(p, ptr)	(SET_PTR(((char *)(p)+(DSIZE)), ptr))

#define GET_SIZE(p)		(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

#define HDRP(bp)	((char* )(bp) - WSIZE)
#define FTRP(bp)	((char* )(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *free_list_headp;
static char *free_list_tailp;
static void *extend_heap(size_t size); //拓展堆块
static void *find_fit(size_t size); //寻找空闲块
static void place(void *bp, size_t size); //分割空闲块
static void *coalesce(void *bp); //合并空闲块

int mm_init(void)
{
	if ((free_list_headp = mem_sbrk(4*WSIZE+3*DSIZE)) == (void *)-1) {
		return -1;
	}
	PUT(free_list_headp, PACK(0, 0));					
	PUT(free_list_headp+WSIZE, PACK(24, 1));			
	free_list_headp += DSIZE;							
	free_list_tailp = NEXT_BLKP(free_list_headp);		
	SET_PRED(free_list_headp, NULL);					
	SET_SUCC(free_list_headp, free_list_tailp);			
	PUT(free_list_headp+(2*DSIZE), PACK(24, 1));		
	PUT(HDRP(free_list_tailp), PACK(0, 1));				
	SET_PRED(free_list_tailp, free_list_headp);			
	PUT(free_list_tailp+DSIZE, PACK(0, 0));				
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
		return -1;
	}
    
    return 0;
}

static void *extend_heap(size_t words)
{
	char *bp;
	char *ptr;
	size_t size;
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
	
	ptr = free_list_tailp;
	PUT(HDRP(ptr), PACK(size, 0));		
	PUT(FTRP(ptr), PACK(size, 0));		
	free_list_tailp = NEXT_BLKP(ptr);	
	SET_SUCC(ptr, free_list_tailp);		
	PUT(HDRP(free_list_tailp), PACK(0, 1));
	SET_PRED(free_list_tailp, ptr);
	PUT(free_list_tailp+DSIZE, PACK(0, 0));
	return coalesce(ptr);
	
}

void mm_free(void *ptr)
{
	char *p;
	size_t size = GET_SIZE(HDRP(ptr));
	for (p = GET_SUCC(free_list_headp); ; p = GET_SUCC(p)) {
		if (ptr < (void *)p) {		
			PUT(HDRP(ptr), PACK(size, 0));
			PUT(FTRP(ptr), PACK(size, 0));
			SET_SUCC(ptr, p);
			SET_PRED(ptr, GET_PRED(p));
			SET_SUCC(GET_PRED(p), ptr);
			SET_PRED(p, ptr);
			break;
		}
	}
	coalesce(ptr);
}

static void *coalesce(void *ptr)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
	size_t size = GET_SIZE(HDRP(ptr));
	
	void *prevptr = PREV_BLKP(ptr);
	void *nextptr = NEXT_BLKP(ptr);
	if (prev_alloc && next_alloc) {				
		return ptr;
	} else if (prev_alloc && !next_alloc) {		
		size += GET_SIZE(HDRP(nextptr));
		SET_SUCC(ptr, GET_SUCC(nextptr));
		SET_PRED(GET_SUCC(nextptr), ptr);
		PUT(HDRP(ptr), PACK(size, 0));
		PUT(FTRP(nextptr), PACK(size, 0));
	} else if (!prev_alloc && next_alloc) {		
		size += GET_SIZE(FTRP(prevptr));
		SET_SUCC(prevptr, GET_SUCC(ptr));
		SET_PRED(GET_SUCC(ptr), prevptr);
		PUT(HDRP(prevptr), PACK(size, 0));
		PUT(FTRP(ptr), PACK(size, 0));
		ptr = prevptr;
	} else if (!prev_alloc && !next_alloc) {	
		size += GET_SIZE(HDRP(prevptr)) 
			+ GET_SIZE(FTRP(nextptr));
		SET_SUCC(prevptr, GET_SUCC(nextptr));
		SET_PRED(GET_SUCC(nextptr), prevptr);
		PUT(HDRP(prevptr), PACK(size, 0));
		PUT(FTRP(nextptr), PACK(size, 0));
		ptr = prevptr;
	}
    
	return ptr;
}

void *mm_malloc(size_t size)
{
    size_t asize;		
    size_t extendsize;	
   	char *bp;

   	if (size == 0)
   		return NULL;
   	
	if (size <= 2*DSIZE) {
		asize = 3*DSIZE;
	} else {
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	}
	asize = ALIGN(asize);
   	if ((bp = find_fit(asize)) != NULL) {
   		place(bp, asize);
   		return bp;
   	}
   	
   	extendsize = MAX(asize, CHUNKSIZE);
   	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
   		return NULL;
   	place(bp, asize);
    
   	return bp; 	
}

static void *find_fit(size_t asize)
{
	void* p;
	for (p = GET_SUCC(free_list_headp); p != free_list_tailp; p = GET_SUCC(p)) {
		if (asize <= GET_SIZE(HDRP(p)))
			return p;
	}
	return NULL;
}

static void place(void *bp, size_t asize)
{
	size_t size = GET_SIZE(HDRP(bp));
	void *ptr;
	if (size - asize >= 3*DSIZE) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		ptr = NEXT_BLKP(bp);
		SET_SUCC(ptr, GET_SUCC(bp));
		SET_PRED(ptr, GET_PRED(bp));
		SET_SUCC(GET_PRED(ptr), ptr);
		SET_PRED(GET_SUCC(ptr), ptr);
		PUT(HDRP(ptr), PACK(size-asize, 0));
		PUT(FTRP(ptr), PACK(size-asize, 0));
		
	} else {
		PUT(HDRP(bp), PACK(size, 1));
		PUT(FTRP(bp), PACK(size, 1));
		SET_SUCC(GET_PRED(bp), GET_SUCC(bp));
		SET_PRED(GET_SUCC(bp), GET_PRED(bp));
	}

}

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
	void *nextptr;
	void *pred;
	void *succ;
	char *p;
	size_t blockSize;
    size_t extendsize;
	size_t asize;	
	size_t sizesum;

	if (ptr == NULL) { 			
		return mm_malloc(size);
	} else if (size == 0) { 	
		mm_free(ptr);
		return NULL;
	}
	if (size <= 2*DSIZE) {
		asize = 3*DSIZE;
	} else {
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	}
	asize = ALIGN(asize);
	blockSize = GET_SIZE(HDRP(ptr));

	if (asize == blockSize) {						
		return ptr;
	} else if (asize < blockSize) {					
		if (blockSize-asize >= 3*DSIZE) {	
			PUT(HDRP(ptr), PACK(asize, 1));
			PUT(FTRP(ptr), PACK(asize, 1));
			nextptr = NEXT_BLKP(ptr);
			PUT(HDRP(nextptr), PACK(blockSize-asize, 0));
			PUT(FTRP(nextptr), PACK(blockSize-asize, 0));
			for (p = GET_SUCC(free_list_headp); ; p = GET_SUCC(p)) {
				if (nextptr < (void *)p) {
					pred = GET_PRED(p);
					succ = p;
					SET_PRED(nextptr, pred);
					SET_SUCC(nextptr, succ);
					SET_SUCC(pred, nextptr);
					SET_PRED(p, nextptr);
					break;
				}
			}
		} 
		return ptr;
	} else {										
		nextptr = NEXT_BLKP(ptr);
		sizesum = GET_SIZE(HDRP(nextptr))+blockSize;	
		if (!GET_ALLOC(HDRP(nextptr)) && sizesum >= asize) {	
			pred = GET_PRED(nextptr);
			succ = GET_SUCC(nextptr);
			if (sizesum-asize >= 3*DSIZE) {
				PUT(HDRP(ptr), PACK(asize, 1));
				PUT(FTRP(ptr), PACK(asize, 1));
				nextptr = NEXT_BLKP(ptr);
				PUT(HDRP(nextptr), PACK(sizesum-asize, 0));
				PUT(FTRP(nextptr), PACK(sizesum-asize, 0));
				SET_PRED(nextptr, pred);
				SET_SUCC(nextptr, succ);
				SET_SUCC(pred, nextptr);
				SET_PRED(succ, nextptr);
			} else {
				PUT(HDRP(ptr), PACK(sizesum, 1));
				PUT(FTRP(ptr), PACK(sizesum, 1));
				SET_SUCC(pred, succ);
				SET_PRED(succ, pred);
			}
			return ptr;
		} else {		
			newptr = find_fit(asize);
			if (newptr == NULL) {
				extendsize = MAX(asize, CHUNKSIZE);
				if ((newptr = extend_heap(extendsize/WSIZE)) == NULL) {
					return NULL;
				}
			}
			place(newptr, asize);
			memcpy(newptr, oldptr, blockSize-2*WSIZE);
			mm_free(oldptr);
			return newptr;
		}
	}
}

