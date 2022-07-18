#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include "cachelab.h"

#define OK          0
#define ERR         (-1)

#define DEBUG(x)    if (v) {    \
    x;                          \
}

int v; 
/*  cache size = S * E * B */
int s, S;
int E;
int b, B;
/*  counter */
int hit_count, miss_count, eviction_count;

typedef unsigned long addr_t;
typedef struct cache_line{
    struct cache_line   *prior;
    struct cache_line   *next;
    u_int8_t        isValid;
    u_int32_t       tag;
    u_int8_t       *cache_buf;
} cache_line_t;

typedef struct {
    cache_line_t   *cache_line;
} cache_set_t;

typedef struct {
    cache_set_t    *cache_set;
}cache_t;

static void miss_state (void)
{
    miss_count++;
    DEBUG(printf("miss "));
}

static void hit_state (void)
{
    hit_count++;
    DEBUG(printf("hit "));
}

static void eviction_state (void)
{
    eviction_count++;
    DEBUG(printf("eviction "));
}

static cache_t *cache_init (void)
{
    cache_t    *cache;
    int         i, j;

    cache = malloc(sizeof(cache_t));
    memset(cache, 0, sizeof(cache_t));
    cache->cache_set = malloc(sizeof(cache_set_t) * S);
    memset(cache->cache_set, 0, sizeof(cache_set_t) * S);
    for (i = 0; i < S; i++) {
        cache->cache_set[i].cache_line = malloc(sizeof(cache_line_t) * E);
        memset(cache->cache_set[i].cache_line, 0, sizeof(cache_line_t) * E);
        for (j = 0; j < E; j++) {
            cache->cache_set[i].cache_line[j].cache_buf = malloc(sizeof(u_int8_t) * B);
            memset(cache->cache_set[i].cache_line[j].cache_buf, 0, sizeof(u_int8_t) * B);

            cache->cache_set[i].cache_line[j].prior = &cache->cache_set[i].cache_line[j - 1];
            cache->cache_set[i].cache_line[j].next  = &cache->cache_set[i].cache_line[j + 1];
            if (j == 0) {
                cache->cache_set[i].cache_line[j].prior = NULL;
            }
            if (j == E - 1) {
                cache->cache_set[i].cache_line[j].next  = NULL;
            }
        }
        printf("cache set[%d] = %p\r\n", i, &cache->cache_set[i]);
    }

    return cache;
}

static cache_line_t *find_head (cache_t *cache, u_int32_t index)
{
    cache_line_t *cache_line;

    cache_line = cache->cache_set[index].cache_line;

    while (cache_line->prior) {
        cache_line = cache_line->prior;
    }

    return cache_line;
}

static cache_line_t *find_tail (cache_t *cache, u_int32_t index)
{
    cache_line_t *cache_line;

    cache_line = cache->cache_set[index].cache_line;

    while (cache_line->next) {
        cache_line = cache_line->next;
    }

    return cache_line;
}

static void node_move_to_head (cache_t *cache, u_int32_t index, u_int32_t line)
{
    cache_line_t *moved_line;
    cache_line_t *head_line;

    moved_line = &cache->cache_set[index].cache_line[line];
    head_line  = find_head(cache, index);

    if (moved_line != head_line) {
        if (moved_line->prior) {
            moved_line->prior->next = moved_line->next;
        }
        if (moved_line->next) {
            moved_line->next->prior = moved_line->prior;
        }
        moved_line->prior = NULL;
        moved_line->next  = head_line;
        head_line->prior  = moved_line;
    }
}

static void node_replace_last (cache_t *cache, u_int32_t index, u_int32_t tag)
{
    cache_line_t *tail_line = find_tail(cache, index);
    cache_line_t *head_line = find_head(cache, index);

    if (tail_line->isValid) {
        /* eviction */
        eviction_state();
    }

    tail_line->tag     = tag;
    tail_line->isValid = 1;

    if (tail_line->prior) {
        tail_line->prior->next = tail_line->next;
        tail_line->prior = NULL;
        tail_line->next  = head_line;
        head_line->prior = tail_line;
    }
}

static void cache_deinit (cache_t *cache)
{
    int         i, j;

    for (i = 0; i < S; i++) {
        for (j = 0; j < E; j++) {
            free(cache->cache_set[i].cache_line[j].cache_buf);
        }
        free(cache->cache_set[i].cache_line);
    }
    free(cache->cache_set);
    free(cache);
}

static u_int32_t bit_mask (u_int32_t bit)
{
    u_int32_t   ret = 0;
    int         i;

    for (i = 0; i < bit; i++) {
        ret <<= 1;
        ret |= 0x1;
    }

    return ret;
}

static void access_cache (cache_t *cache, addr_t addr)
{
    u_int32_t index  = (addr >> b) & bit_mask(s);
    u_int32_t tag    = (addr >> b) >> s;
    int       i;

    for (i = 0; i < E; i++) {
        if (cache->cache_set[index].cache_line[i].tag == tag &&
            cache->cache_set[index].cache_line[i].isValid) {
            break;
        }
    }

    if (i == E) {
        /* miss */
        miss_state();
        node_replace_last(cache, index, tag);

    } else {
        /* hit */
        hit_state();
        node_move_to_head(cache, index, i);
    }
}

static int run_trace (FILE *fp, cache_t *cache)
{
    char    line_buf[512];
    char   *ptr;
    char    opt;
    addr_t  addr, addr_last;
    size_t  len;

    memset(line_buf, 0, sizeof(line_buf));
    while (fgets(line_buf, sizeof(line_buf), fp)) {
        ptr = line_buf;
        while(ptr[0] == ' ') {
            ptr++;
        }
        sscanf(ptr, "%c %lx,%ld", &opt, &addr, &len);

        DEBUG(printf("%c %lx,%ld ", opt, addr, len));

        addr_last   = addr + len - 1;
        addr        = addr & (~bit_mask(b));
        addr_last   = addr_last & (~bit_mask(b));

        switch (opt) {
            case 'L':
            case 'S':
                while (addr <= addr_last) {
                    access_cache(cache, addr);
                    addr += B;
                }
                break;

            case 'M':
                while (addr <= addr_last) {
                    access_cache(cache, addr);
                    access_cache(cache, addr);
                    addr += B;
                }
                break;

            case 'I':
                break;
        
            default:
                fprintf(stderr, "invalid operation symbol %c\r\n", opt);
                return ERR;
                break;
        }

        DEBUG(printf("\r\n"));
    }

    return OK;
}

static void printUsage ()
{
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
           "Options:\n"
           "  -h         Print this help message.\n"
           "  -v         Optional verbose flag.\n"
           "  -s <num>   Number of set index bits.\n"
           "  -E <num>   Number of lines per set.\n"
           "  -b <num>   Number of block offset bits.\n"
           "  -t <file>  Trace file.\n\n"
           "Examples:\n"
           "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi   trace\n"
           "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi   trace\n");
}

extern char *optarg;

int main (int  argc, char  *argv[])
{
    char      filePath[128];
    char     *ptr;
    cache_t  *cache;
    FILE     *fp;
    int       opt;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != ERR) {
        switch (opt) {
            case 'h':
                printUsage();
                break;

            case 'v':
                v = 1;
                break;

            case 's':
                s = atoi(optarg);
                S = 1 << s;
                break;

            case 'E':
                E = atoi(optarg);
                break;

            case 'b':
                b = atoi(optarg);
                B = 1 << b; 
                break;

            case 't':
                strcpy(filePath, optarg);
                break;

            default:
                printUsage();
                break;
        }
    }

    if (s <= 0 || E <= 0 || b <= 0 || filePath == NULL) {
        printf("Invalid paramater.\n");
        printUsage();
        return ERR;
    }

    ptr = filePath;
    while(ptr[0] == ' ') {
        ptr++;
    }
    fp = fopen(ptr, "r");
    if (!fp) {
        printf("Failed to open file %s\n", filePath);
        return ERR;
    }   

    cache = cache_init();

    run_trace(fp, cache);

    printSummary(hit_count, miss_count, eviction_count);

    cache_deinit(cache);

    return 0;
}
