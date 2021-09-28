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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};



/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y)((x)>(y)?(x):(y))

/*pack a size and allocated bit into a word*/
#define PACK(size, alloc)((size)|(alloc))

/*read and write a byte to address p*/
#define GET(p)          (*(unsigned int*)p)
#define PUT(p, val)     (*(unsigned int*)p=val)

/*read the size and allocated fields from address p*/
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

#define HDRP(bp)        ((char*)bp - WSIZE)
#define FTRP(bp)        ((char*)bp + GET_SIZE(HDRP(bp)) - DSIZE)

//#define NEXT_BLKP(bp)   ((char*)bp + GET_SIZE(((char*)bp - DSIZE)))
#define NEXT_BLKP(bp)   ((char*)bp + GET_SIZE(((char*)bp - WSIZE)))
#define PREV_BLKP(bp)   ((char*)bp - GET_SIZE(((char*)bp - DSIZE)))



static char *heap_listp;

static void *coalesce(void* bp){
   size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
   size_t size = GET_SIZE(HDRP(bp));
   
   if(prev_alloc && next_alloc){
       return bp;
   }else if(prev_alloc && !next_alloc){
       size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
       PUT(HDRP(bp), PACK(size, 0));
       PUT(FTRP(bp), PACK(size, 0));
   }else if(!prev_alloc && next_alloc){
       size += GET_SIZE(FTRP(PREV_BLKP(bp)));
       PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
       PUT(FTRP(bp), PACK(size, 0));
       bp = PREV_BLKP(bp);
   }else{
       size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
       PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
       PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
       bp = PREV_BLKP(bp);
   }
   return bp;
}

static void *extend_heap(size_t words){
    char* bp;
    size_t size;
    
    size = (words%2)? (words+1)*WSIZE:words*WSIZE;
    
    if((long)(bp = mem_sbrk(size)) == -1){
        return (void*)-1;
    }
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    return coalesce(bp);
}

int mm_init(void){
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1){
        return -1;
    }
    PUT(heap_listp, 0);
    PUT((unsigned int*)(heap_listp+WSIZE), PACK(DSIZE, 1));
    PUT((unsigned int*)(heap_listp+2*WSIZE), PACK(DSIZE, 1));
    PUT((unsigned int*)(heap_listp+3*WSIZE), PACK(0, 1));
    heap_listp += 2*WSIZE;
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
        return -1;
    }
    return 0;
}

void mm_free(void* bp){
    size_t size = GET_SIZE(HDRP(bp));
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    coalesce(bp);
}

static void *find_fit(size_t asize){
    	void *bp = heap_listp;
	size_t size;
	while((size = GET_SIZE(HDRP(bp))) != 0){	//遍历全部块
		if(size >= asize && !GET_ALLOC(HDRP(bp)))	//寻找大小大于asize的空闲块
			return bp;
		bp = NEXT_BLKP(bp);
	}
	return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    
    if((csize - asize) >= (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

void *mm_malloc(size_t size){
    size_t asize;
    size_t extendsize;
    char* bp;
    
    if(size==0){
         return NULL;
    }
    
    if(size<=DSIZE){
        asize = 2*DSIZE;
    }else{
        asize = DSIZE*((size + (DSIZE) + (DSIZE - 1))/DSIZE);
    }
    
    if((bp = find_fit(asize))!= NULL){
        place(bp, asize);
        return bp;
    }
    
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

void *mm_realloc(void *ptr, size_t size)
{
       size_t asize, ptr_size;
	void *new_bp;
	
	if(ptr == NULL)
		return mm_malloc(size);
	if(size == 0){
		mm_free(ptr);
		return NULL;
	}
	
	asize = size<=DSIZE ? 2*DSIZE : DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	new_bp = coalesce(ptr);	//尝试是否有空闲的
	ptr_size = GET_SIZE(HDRP(new_bp));
	PUT(HDRP(new_bp), PACK(ptr_size, 1));
	PUT(FTRP(new_bp), PACK(ptr_size, 1));
	if(new_bp != ptr)	//如果合并了前面的空闲块，就将原本的内容前移
		memcpy(new_bp, ptr, GET_SIZE(HDRP(ptr)) - DSIZE);
	if(ptr_size == asize)
		return new_bp;
	else if(ptr_size > asize){
		place(new_bp, asize);
		return new_bp;
	}else{
		ptr = mm_malloc(asize);
		if(ptr == NULL)
			return NULL;
		memcpy(ptr, new_bp, ptr_size - DSIZE);
		mm_free(new_bp);
		return ptr;
	}
}