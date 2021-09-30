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
    "jteam",
    /* First member's full name */
    "James",
    /* First member's email address */
    "huhuhujm@cs.cmu.edu",
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

#define NEXT_BLKP(bp)   ((char*)bp + GET_SIZE(((char*)bp - WSIZE)))
#define PREV_BLKP(bp)   ((char*)bp - GET_SIZE(((char*)bp - DSIZE)))


static char* p_array[25];
static char* heap;
static int max_length = 14;

static int get_index(unsigned int size){
    int i=0;
    while(size!=0){
        size = size>>1;
        i++;
    }
    if(i>max_length){
        max_length = i;
    }
    return i;
}

void trans(){
    for(int i=0; i<25; i++){
        printf("p[%d] is :%p\n", i, p_array[i]);
    }
}

static void add_node(char* p,int size){
    int index = get_index(size);
    //printf("add pinter %p, next is:%p\n", p, (char*)GET((p+WSIZE)));
    if(p_array[index]==p){
        return ;
    }
    if(p_array[index]!=NULL){
        PUT((p+WSIZE), (unsigned int)p_array[index]);
        PUT((p), 0);
        PUT(p_array[index], (unsigned int)p);
        p_array[index] = p;
        //printf("if succussfully add pointer%p, it's next is%p\n", p, GET((p+WSIZE)));
    }else{
        p_array[index] = p;
        //PUT(p, 0);
        //PUT((p+WSIZE), 0);
        //printf("else succussfully add pointer%p, it's next is%p\n", p, GET((p+WSIZE)));
    }
}

/*
** Need to configure node in the 
*/
static void cut_link(char* p){
    int size = GET_SIZE(HDRP(p));
    // printf("want to cut:%p index:%d theory p's next:%p\n",p, get_index(size), (char*)GET((p+WSIZE)));
    //trans();
    if(p_array[get_index(size)] == NULL){
        return ;
    }
    if(p_array[get_index(size)] == p){
        p_array[get_index(size)] = (char*)GET((p+WSIZE));
        PUT(p, 0);
        PUT((p+WSIZE), 0);
    }else{
        char* tmp = (char*)GET((p_array[get_index(size)]+WSIZE));
        while(tmp!=NULL){
             if(tmp==p){
                 //printf("there it is %p\n", tmp);
                 char* prev = (char*)GET(p);
                 char* next = (char*)GET((p+WSIZE));
                 //printf("prev_is:%p, next_is:%p\n",prev , next);
                 PUT((prev+WSIZE), (unsigned int)next);
                 if(next!=NULL){
                      PUT(next, (unsigned int)prev);
                 }
                 PUT(p, 0);
                 PUT((p+WSIZE), 0);
                 return ;             
            }else{
                //printf("%p\n", tmp);
            }
            tmp = (char*)GET((tmp+WSIZE));
        }        
    }
}

static char* coalesce(char* p){
    int p_alloc = GET_ALLOC(FTRP(PREV_BLKP(p)));
    int n_alloc = GET_ALLOC(HDRP(NEXT_BLKP(p)));
    int size = GET_SIZE(HDRP(p));
    if(p_alloc && n_alloc){
    }else if(!p_alloc && n_alloc){
        char *prev = (char*)PREV_BLKP(p);
        size += GET_SIZE(HDRP(prev));
        cut_link(prev);
        PUT(FTRP(prev), 0);
        PUT(HDRP(prev), PACK(size, 0));
        PUT((FTRP(p)), PACK(size, 0));
        PUT((HDRP(p)), 0);
        p = prev;
    }else if(p_alloc && !n_alloc){
        char* next = NEXT_BLKP(p);
        size += GET_SIZE(FTRP(next));
        cut_link(next);
        PUT(FTRP(next), PACK(size, 0));
        PUT(HDRP(next), 0);
        PUT(FTRP(p), 0);
        PUT(HDRP(p), PACK(size, 0));
    }else{
        char* pr = PREV_BLKP(p);
        char* ne = NEXT_BLKP(p);
        size += GET_SIZE(HDRP(pr)) + GET_SIZE(FTRP(ne));
        cut_link(pr);
        cut_link(ne);
        PUT(FTRP(pr), 0);
        PUT(HDRP(pr), PACK(size, 0));
        PUT(FTRP(ne), PACK(size, 0));
        PUT(HDRP(ne), 0);
        PUT(HDRP(p), 0);
        PUT(FTRP(p), 0);
        p = pr;
    }
    PUT(p, 0);
    PUT((p+WSIZE), 0);
    add_node(p, size);
    return p;
}

char* extend_heap(size_t size){
    size_t asize = (size%2)? (size+1)*DSIZE:size*DSIZE;
    char* p;
    if((long)(p=mem_sbrk(asize))==(void*)-1){
        return NULL;
    }
    PUT(p, 0);
    PUT((p+WSIZE), 0);
    PUT(HDRP(p), PACK(asize, 0));
    PUT(FTRP(p), PACK(asize, 0));
    PUT(HDRP(NEXT_BLKP(p)), PACK(0,1));
    return coalesce(p);
}

int mm_init(void){
    if((heap=(char*)mem_sbrk(6*WSIZE))==(void*)-1){
        return -1;
    }
    for(int i=0; i<25; i++){
        p_array[i] = NULL;
    }
    PUT((unsigned int*)heap, 0);
    PUT((unsigned int*)(heap+WSIZE), PACK(2*DSIZE, 1));
    PUT((unsigned int*)(heap+2*WSIZE), 0);
    PUT((unsigned int*)(heap+3*WSIZE), 0);
    PUT((unsigned int*)(heap+4*WSIZE), PACK(2*DSIZE, 1));
    PUT((unsigned int*)(heap+5*WSIZE), PACK(0, 1));
    char *cc;
    if((cc=extend_heap(CHUNKSIZE/WSIZE))==(void*)-1){
        return -1;
    }
    return 0;
}

char *find(int size){
    int end = get_index(size);
    int nw = max_length;
    while(nw>end){
        if(p_array[nw]==NULL){
            nw--;
            max_length--;
            continue;
        }
        char* res = p_array[nw];
        //printf("get pointer is%p ,suspect pointer is:%p\n",res, (char*)GET((res+WSIZE)));
        p_array[nw] = (char*)GET((res+WSIZE));
        //printf("p_array[%d] is :%p\n", nw, p_array[nw]);
        PUT((res+WSIZE), 0);
        return res;
    }
    return NULL;
}

void mm_free(void* bp){
    //printf("free mem %p\n", bp);
    int size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(bp, 0);
    PUT((bp+WSIZE), 0);
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void place(char* bp, int asize){
    int csize = GET_SIZE(HDRP(bp));
    if((csize - asize) > (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        PUT(bp, 0);
        PUT((bp+WSIZE), 0);
        add_node(bp, csize-asize);
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

void *mm_malloc(size_t size){
    //printf("get to alloc %d sizes\n", size);
    if(size == 0){
        return NULL;
    }
    size_t asize = size>=DSIZE? ((size+2*DSIZE-1)/DSIZE)*DSIZE:2*DSIZE;
    char* b = find(asize);
    if(b!=NULL){
        place(b, asize);
        //printf("finish normal alloc %d sizes,return pointer:%p\n", asize, b);
        //trans();
        return b;
    }
    int extendsize = MAX(asize, CHUNKSIZE);
    if((b = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    cut_link(b);
    place(b,asize);
    //printf("finish extend alloc %d sizes,return pointer:%p\n", asize, b);
    //trans();
    return b;
}

void *mm_realloc(void *ptr, size_t size){
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