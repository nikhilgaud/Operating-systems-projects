/* 
 * mm-implicit.c -  Simple allocator based on implicit free lists, 
 *                  first fit placement, and boundary tag coalescing. 
 *
 * Each block has header and footer of the form:
 * 
 *      31                     3  2  1  0 
 *      -----------------------------------
 *     | s  s  s  s  ... s  s  s  0  0  a/f
 *      ----------------------------------- 
 * 
 * where s are the meaningful size bits and a/f is set 
 * iff the block is allocated. The list has the following form:
 *
 * begin                                                          end
 * heap                                                           heap  
 *  -----------------------------------------------------------------   
 * |  pad   | hdr(8:a) | ftr(8:a) | zero or more usr blks | hdr(8:a) |
 *  -----------------------------------------------------------------
 *          |       prologue      |                       | epilogue |
 *          |         block       |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
  /* Team name */
  "Nikhil Awadhoot Gaud",
  /* First member's full name */
  "Nikhil Awadhoot Gaud",
  /* First member's email address */
  "ngaud@mail.csuchico.edu",
  /* (leave blank) */
  "",
  /* (leave blank) */
  ""
};

/////////////////////////////////////////////////////////////////////////////
// Constants and macros
//
// These correspond to the material in Figure 9.43 of the text
// The macros have been turned into C++ inline functions to
// make debugging code easier.
//
/////////////////////////////////////////////////////////////////////////////
#define WSIZE       4       /* word size (bytes) */  
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    8       /* overhead of header and footer (bytes) */
#define MAXLIST     20      
#define REALLOC_BUFFER  (1<<7)

static inline int MAX(int x, int y) {
  return x > y ? x : y;
}

//
// Pack a size and allocated bit into a word
// We mask of the "alloc" field to insure only
// the lower bit is used
//
static inline size_t PACK(size_t size, int alloc) {
  return ((size) | (alloc & 0x1));
}

//
// Read and write a word at address p
//
static inline size_t GET(void *p) { return  *(size_t *)p; }
static inline void PUT( void *p, size_t val)
{
  *((size_t *)p) = val;
}

//
// Read the size and allocated fields from address p
//
static inline size_t GET_SIZE( void *p )  { 
  return GET(p)&~0x7;
}

static inline int GET_ALLOC( void *p  ) {
  return GET(p) & 0x1;
}

//
// Given block ptr bp, compute address of its header and footer
//
static inline void *HDRP(void *bp) {

  return ( (char *)bp) - WSIZE;
}
static inline void *FTRP(void *bp) {
  return ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
}

//
// Given block ptr bp, compute address of next and previous blocks
//
static inline void *NEXT_BLKP(void *bp) {
  return  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)));
}

static inline void* PREV_BLKP(void *bp){
  return  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)));
}

static void setPointer(void* blockPtr, void* assignPtr)
{
	*(unsigned int*)(blockPtr)=(unsigned int)(assignPtr);
}

static void* PreviousOf(void* blockPtr)
{
	return (char*)(blockPtr);
}

static void* NextOf(void* blockPtr)
{
	return (char*)(blockPtr)+WSIZE;
}

static void* AddressOfPreviousNode(void* blockPtr)
{
	return *(char**)(blockPtr);
}

static void* AddressOfNextNode(void* blockPtr)
{
	return *(char**)(NextOf(blockPtr));
}

static inline int SetAlignment(size_t size)
{
  return (((size)+7)&~0x7);
}

/////////////////////////////////////////////////////////////////////////////
//
// Global Variables
//

static char *heap_listp;
void* segListArray[MAXLIST];

//
// function prototypes for internal helper routines
//
static void *extend_heap(size_t words);
static void* place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkblock(void *bp);
static void create(size_t size, void *ptr);
static void removeNode(size_t size, void *ptr);

//
// mm_init - Initialize the memory manager 
//
int mm_init(void) 
{
  //
  // You need to provide this
  //
  static char* heap_origin;
    int index = 0;
       
    while(MAXLIST > index)
    {
		segListArray[index] = NULL;
		index++;
	}
    if((long)(heap_origin = mem_sbrk(WSIZE << 2))==-1)
    {
        return -1;
	}
    PUT(heap_origin, 0);
    PUT(heap_origin + WSIZE, PACK(DSIZE, 1));
    PUT(heap_origin + DSIZE, PACK(DSIZE, 1));
    PUT(heap_origin + DSIZE + WSIZE, PACK(0, 1));
    heap_origin = heap_origin + DSIZE;  
    if(extend_heap(CHUNKSIZE>>2)==NULL)
    {
        return -1;
	}    
    return 0;
}

static void create(size_t size, void *blockPtr)
{
    void* insertblockPtr = NULL;
    int index = 0;
    void* searchblockPtr = blockPtr;
    
    for(index = 0;index < (MAXLIST - 1) && (size > 1);)
    {
		size = size / 2;
		index = index + 1;
	}
    searchblockPtr = segListArray[index];    
    for(;(searchblockPtr != NULL) && (size > GET_SIZE(HDRP(searchblockPtr)));)
    {
        searchblockPtr = AddressOfPreviousNode(insertblockPtr = searchblockPtr);
	}
    void* tempPreviousPtr = PreviousOf(blockPtr);
    void* tempNextPtr = NextOf(blockPtr);
    if(searchblockPtr==NULL)
    {
		setPointer(tempPreviousPtr,NULL);
	}
    else
    {
		setPointer(tempPreviousPtr,searchblockPtr);
        setPointer(NextOf(searchblockPtr),blockPtr);
    }
    if (insertblockPtr==NULL)
	{
		setPointer(tempNextPtr,NULL);
		segListArray[index] = blockPtr;
	}
	else
	{
		setPointer(tempNextPtr,insertblockPtr);
		setPointer(PreviousOf(insertblockPtr),blockPtr);
	}
    
    return;
}


static void removeNode(size_t size, void *ptr)
{
    int index;
    
    void* tempPreviousPtr = AddressOfPreviousNode(ptr);
    void* tempNextPtr = AddressOfNextNode(ptr);
    
    for(index = 0;index < (MAXLIST - 1) && (size > 1);)
    {
		size = size / 2;
		index = index + 1;
	}    
    if(tempPreviousPtr != NULL)
    {
		setPointer(NextOf(tempPreviousPtr), tempNextPtr);
    }
    if(tempNextPtr != NULL)
	{
		setPointer(PreviousOf(tempNextPtr), tempPreviousPtr);
	}
	else
	{
		segListArray[index] = tempPreviousPtr;
	}
	return;
}

//
// extend_heap - Extend heap with free block and return its block pointer
//
static void *extend_heap(size_t words) 
{
  //
  // You need to provide this
  //
  void *blockPtr;
    size_t asize;    
    asize = SetAlignment(words);    
    if((blockPtr = mem_sbrk(asize)) == (void*)-1)
    {
        return NULL;
	}
    PUT(HDRP(blockPtr),PACK(asize,0));  
    PUT(FTRP(blockPtr),PACK(asize,0));   
    PUT(HDRP(NEXT_BLKP(blockPtr)),PACK(0,1)); 
    create(asize,blockPtr);
    return coalesce(blockPtr);
}


//
// Practice problem 9.8
//
// find_fit - Find a fit for a block with asize bytes 
//
static void *find_fit(size_t asize)
{
	int index = 0;
	void* blockPtr;
	for(;index < MAXLIST;index++)
	{
		for(blockPtr = segListArray[index];(blockPtr != NULL) && (asize > GET_SIZE(HDRP(blockPtr)));blockPtr = AddressOfPreviousNode(blockPtr))
		{
			
		}
		if (blockPtr != NULL)
		{
			return blockPtr;
		}
	}
	return NULL;
}

// 
// mm_free - Free a block 
//
void mm_free(void *bp)
{
  //
  // You need to provide this
  //
    size_t size = GET_SIZE(HDRP(bp)); 
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));    
    create(size,bp);
    coalesce(bp);    
    return;
}

//
// coalesce - boundary tag coalescing. Return ptr to coalesced block
//
static void *coalesce(void *bp) 
{
  size_t size = GET_SIZE(HDRP(bp));
    
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));

	size_t sizePreviousBlockPtrHeader = GET_SIZE(HDRP(PREV_BLKP(bp)));
	size_t sizeNextBlockPtrHeader = GET_SIZE(HDRP(NEXT_BLKP(bp)));    

    if(prev_alloc && next_alloc)
    {
		return bp;
    }
    removeNode(size,bp);
    if(prev_alloc && !next_alloc)
    {
		removeNode(sizeNextBlockPtrHeader,NEXT_BLKP(bp));		
		PUT(HDRP(bp),PACK(size += sizeNextBlockPtrHeader, 0));
		PUT(FTRP(bp),PACK(size, 0));
		create(size,bp);
		return bp;
    }
    else if(!prev_alloc && next_alloc)
    {
        removeNode(sizePreviousBlockPtrHeader,PREV_BLKP(bp));        
        PUT(FTRP(bp),PACK(size += sizePreviousBlockPtrHeader,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
        create(size,bp);
		return bp;
    }
    else
    {
        removeNode(sizePreviousBlockPtrHeader,PREV_BLKP(bp));
        removeNode(sizeNextBlockPtrHeader,NEXT_BLKP(bp));        
        PUT(HDRP(PREV_BLKP(bp)),PACK(size += sizePreviousBlockPtrHeader + sizeNextBlockPtrHeader,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
        create(size,bp);
		return bp;
    }
}

//
// mm_malloc - Allocate a block with at least size bytes of payload 
//
void *mm_malloc(size_t size) 
{
  //
  // You need to provide this
  //
  size_t asize;
    void* blockPtr = NULL;
    
    if(size == 0)
    {
		return NULL;
	}
	(size <= DSIZE)? (asize = DSIZE << 1):(asize = SetAlignment(size+DSIZE));
    if((blockPtr = find_fit(asize)) != NULL)
	{
		blockPtr = place(blockPtr, asize);
		return blockPtr;
	}
    else if((blockPtr=extend_heap(MAX(asize, CHUNKSIZE))) == NULL)
    {
		return NULL;
    }
    else
    {
		blockPtr = place(blockPtr, asize);    
	    return blockPtr;
	}
} 

//
//
// Practice problem 9.9
//
// place - Place block of asize bytes at start of free block bp 
//         and split if remainder would be at least minimum block size
//
static void* place(void *bp, size_t asize)
{
	size_t size = GET_SIZE(HDRP(bp));
    int flag1;
    int flag2;
    size_t adjustedSize1;
    size_t new_size;
        
    removeNode(size, bp);    
    if((DSIZE << 2)>=(size - asize))
    {
        PUT(HDRP(bp),PACK(size, 1)); 
        PUT(FTRP(bp),PACK(size, 1));
        return bp;
    }
    else if(asize >= 100)
    {	    
		new_size = size - asize;
		adjustedSize1 = asize;
		flag1 = 0;
		flag2 = 1;
	}
	else
	{
		new_size = asize;
		adjustedSize1 = size - asize;
		flag1 = 1;
		flag2 = 0;
	}
	PUT(HDRP(bp),PACK(new_size, flag1));
    PUT(FTRP(bp),PACK(new_size, flag1));
    PUT(HDRP(NEXT_BLKP(bp)),PACK(adjustedSize1, flag2));
    PUT(FTRP(NEXT_BLKP(bp)),PACK(adjustedSize1, flag2));
    create(size - asize,(asize >= 100) ? bp : NEXT_BLKP(bp));
    return (asize >= 100) ? NEXT_BLKP(bp) : bp;
}


//
// mm_realloc -- implemented for you
//
void *mm_realloc(void *ptr, size_t size)
{
	int tempBlock;
	size_t nextBlockPtrSize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
	void *blockPtr = ptr;
	int required;
    size_t asize = size;
    int capacityExpand;
    
    if(size == 0)
    {
        return NULL;
	}
    asize = (asize <= DSIZE)? (DSIZE << 1) + REALLOC_BUFFER : (SetAlignment(asize+DSIZE)) + REALLOC_BUFFER;
    tempBlock = GET_SIZE(HDRP(ptr)) - asize;
    if(tempBlock < 0)
    {
        if(nextBlockPtrSize && GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
        {
            blockPtr = mm_malloc(asize - DSIZE);
            memcpy(blockPtr, ptr, size < asize ? size : asize);
            mm_free(ptr);
        }
        else if((required = GET_SIZE(HDRP(ptr)) + nextBlockPtrSize - asize) < 0 && extend_heap(capacityExpand = MAX(-required, CHUNKSIZE)) != NULL)
        {
			removeNode(nextBlockPtrSize,NEXT_BLKP(ptr));
			PUT(HDRP(ptr),PACK(asize+(required = required + capacityExpand),1));
			PUT(FTRP(ptr),PACK(asize+required,1));
        }
        else
		{
			return NULL;
		}
    }
    return blockPtr;
}

//
// mm_checkheap - Check the heap for consistency 
//
void mm_checkheap(int verbose) 
{
  //
  // This provided implementation assumes you're using the structure
  // of the sample solution in the text. If not, omit this code
  // and provide your own mm_checkheap
  //
  void *bp = heap_listp;
  
  if (verbose) {
    printf("Heap (%p):\n", heap_listp);
  }

  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp))) {
	printf("Bad prologue header\n");
  }
  checkblock(heap_listp);

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (verbose)  {
      printblock(bp);
    }
    checkblock(bp);
  }
     
  if (verbose) {
    printblock(bp);
  }

  if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))) {
    printf("Bad epilogue header\n");
  }
}

static void printblock(void *bp) 
{
  size_t hsize, halloc, fsize, falloc;

  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));  
  fsize = GET_SIZE(FTRP(bp));
  falloc = GET_ALLOC(FTRP(bp));  
    
  if (hsize == 0) {
    printf("%p: EOL\n", bp);
    return;
  }

  printf("%p: header: [%d:%c] footer: [%d:%c]\n",
	 bp, 
	 (int) hsize, (halloc ? 'a' : 'f'), 
	 (int) fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
  if ((size_t)bp % 8) {
    printf("Error: %p is not doubleword aligned\n", bp);
  }
  if (GET(HDRP(bp)) != GET(FTRP(bp))) {
    printf("Error: header does not match footer\n");
  }
}

