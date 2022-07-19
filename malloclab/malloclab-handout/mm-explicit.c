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

#define FIRST_FIT   1
#define NEXT_FIT    2
#define BEST_FIT    3

/* Programe control macro */
#define DEBUG           0
#define FIND_FIT        find_fit

/* Basic constants and macros */
#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE   chunksize

#define MAX(x, y)   ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)
#define ALLOC           1
#define FREE            0

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define BLK_SIZE(bp)    (GET_SIZE(HDRP(bp)))
#define FTRP(bp)        ((char *)(bp) + BLK_SIZE(bp) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char *)(bp) + BLK_SIZE(bp))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define SET_LIST_PREV(bp, p)   (((list_node_t *)(bp))->prev = p)
#define SET_LIST_NEXT(bp, n)   (((list_node_t *)(bp))->prev = n)

#if DEBUG
#define STATE_SLICE(msg)    {   \
    show_heap_status(msg);      \
    mm_check();                 \
}
#else
#define STATE_SLICE(msg)  
#endif

typedef struct list_node_t {
    struct list_node_t   *prev;
    struct list_node_t   *next;
} list_node_t;

static size_t       chunksize;
static char        *heap_listp;
static list_node_t *list_head = NULL;

void get_chunk_size (void)
{
    size_t chunk_size;

    chunk_size = mem_heapsize();
    if (chunk_size) {
        CHUNKSIZE = chunk_size;
    } else {
        CHUNKSIZE = (1 << 12);
    }
}

#if DEBUG
static void show_heap_status (const char *msg)
{
    printf("[%-16s]: heap from %p to %p, heap size %6u, page size %4u.\r\n", 
            msg, mem_heap_lo(), mem_heap_hi(), mem_heapsize(), mem_pagesize());
}

static int mm_check (void)
{
    char    *alloc_head;
    char    *bp;
    char     data[32];

    alloc_head = heap_listp - (2 * WSIZE);
    bp = heap_listp;

    printf("addr\t\toff\ttype\t  data\r\n");
    printf("%p\t%u\t%s\t%s\r\n", alloc_head, alloc_head - alloc_head, "pad", "     0");

    while (1) {
        snprintf(data, sizeof(data), "%4d/%d", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
        printf("%p\t%u\t%s\t%s\r\n", HDRP(bp), HDRP(bp) - alloc_head, "head", data);
        if (GET_SIZE(HDRP(bp)) != (2 * WSIZE)) {
            printf("%s\t\t%s\t%s\t   %s\r\n", "...", "...", "...", "...");
        }
        printf("%p\t%u\t%s\t%s\r\n", FTRP(bp), FTRP(bp) - alloc_head, "foot", data);

        bp = NEXT_BLKP(bp);
        if (GET_SIZE(HDRP(bp)) == 0) {
            break;
        }
    };
    printf("%p\t%u\t%s\t%s\r\n", HDRP(bp), HDRP(bp) - alloc_head, "end", "   0/1");

    return 0;
}
#endif

/*
 * 删除节点总共有四种情况：
 * 1. 删除的节点就是头部节点， 且链表中只有唯一节点
 * 2. 删除的节点就是头部节点， 链表中还有其他节点
 * 3. 删除的不是头部节点， 只有删除节点前一个有数据
 * 4. 删除的不是头部节点， 删除节点的前后都有数据
 */
static void list_remove_node (void *node)
{
    list_node_t *prev, *next;

    next = ((list_node_t *)node)->next;
    prev = ((list_node_t *)node)->prev;

    if (prev) {                 /* 移除非首个节点 */
        prev->next = next;
        if (next) {
            next->prev = prev;
        }
    } else {                    /* 移除首个节点 */
        if (next) {
            list_head = next;
            list_head->prev = NULL;
        } else {
            list_head = NULL;
        }
    }
}

static void list_insert_node (void *node)
{
    if (list_head) {
        list_head->prev = node;
    }
    ((list_node_t *)node)->prev = NULL;
    ((list_node_t *)node)->next = list_head;
    list_head = node;
}

static void *coalesce (void *bp)
{
    size_t  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t  size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        ;
    } else if (prev_alloc && !next_alloc) {
        list_remove_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));
    } else if (!prev_alloc && next_alloc) {
        list_remove_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, FREE));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, FREE));
        bp = PREV_BLKP(bp);
    } else {
        list_remove_node(PREV_BLKP(bp));
        list_remove_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)))
             +  GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, FREE));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, FREE));
        bp = PREV_BLKP(bp);
    }
    list_insert_node(bp);
    return bp;
}

static void *extend_heap (size_t words)
{
    char   *bp;
    size_t  size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (char *)-1) {
        return NULL;
    }

    /* Initialize free block header header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, FREE));
    PUT(FTRP(bp), PACK(size, FREE));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOC));

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void *find_fit (size_t size)
{
    /* First fit search */
    list_node_t *node;

    for (node = list_head; node != NULL; node = node->next) {
        if ((size <= GET_SIZE(HDRP(node)))) {
            return node;
        }
    }

    /* No fit */
    return NULL;
}

void place (char *bp, size_t size)
{
    size_t csize = GET_SIZE(HDRP(bp));

    list_remove_node(bp);

    if ((csize - size) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(size, ALLOC));
        PUT(FTRP(bp), PACK(size, ALLOC));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - size, FREE));
        PUT(FTRP(bp), PACK(csize - size, FREE));
        list_insert_node(bp);
    } else {
        PUT(HDRP(bp), PACK(csize, ALLOC));
        PUT(FTRP(bp), PACK(csize, ALLOC));
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    get_chunk_size();

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
        return -1;
    }

    PUT(heap_listp, 0);                                 /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, ALLOC));  /* Prologue header   */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, ALLOC));  /* Prologue footer   */
    PUT(heap_listp + (3 * WSIZE), PACK(0, ALLOC));      /* Epilogue header   */
    heap_listp += (2 * WSIZE);
    list_head = NULL;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }

    STATE_SLICE("mm_init");

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t      asize;
    size_t      extend_size;
    char       *bp;

    /* Ignore spurious requests */
    if (size == 0) {
        return NULL;
    }

    /* Adjust block size to include overload and alignment reqs */
    asize = ALIGN(size + DSIZE);

    /* Search the free list for a fit */
    if ((bp = FIND_FIT(asize)) != NULL) {
        place(bp, asize);
        STATE_SLICE("mm_malloc find_fit");
        return bp;
    }

    /* Not fit found, Get more memory and place the block */
    extend_size = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    STATE_SLICE("mm_malloc no find_fit");
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, FREE));
    PUT(FTRP(ptr), PACK(size, FREE));
    coalesce(ptr);

    STATE_SLICE("mm_free");
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL) {
        return NULL;
    }

    size     = GET_SIZE(HDRP(newptr)) - DSIZE;
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE;
    if (size < copySize) {
        copySize = size;
    }
      
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    STATE_SLICE("mm_realloc");
    return newptr;
}














