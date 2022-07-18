#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"

#define DIMENSION 3

pixel g_src[DIMENSION][DIMENSION];
pixel g_dst[DIMENSION][DIMENSION];
int   g_dim = DIMENSION;

typedef struct {
   unsigned int red;
   unsigned int green;
   unsigned int blue;
} pixel_sum_t;

#define SHOW(temp, msg) show_state(g_dim, (pixel *)g_src, (pixel *)g_dst, (pixel *)temp, __LINE__, msg)

static 
void show_state(int dim, pixel *src, pixel *dst, pixel *temp, int LINE, char *msg)
{
    int i, j;

    printf("show state at line %d %s: \r\n", LINE, msg);
    for (i = 0; i < dim; i++) {
        for (j = 0; j < dim; j++) {
            printf("%6d ", src[RIDX(i, j, dim)].red);
        }
        printf("      ");
        for (j = 0; j < dim; j++) {
            printf("%6d ", dst[RIDX(i, j, dim)].red);
        }
        printf("      ");

        if((i == 0) && temp) {
            printf("temp: %6d %6d %6d", temp[0].red, temp[1].red, temp[2].red);
        }

        printf("\r\n");
    }
}

static inline 
pixel_sum_t add_2_pixel(pixel *src1, pixel *src2)
{
    pixel_sum_t ret;

    ret.red   = (unsigned int)src1->red   + src2->red;
    ret.green = (unsigned int)src1->green + src2->green;
    ret.blue  = (unsigned int)src1->blue  + src2->blue;

    return ret;
}

static inline 
pixel_sum_t add_3_pixel(pixel *src1, pixel *src2, pixel *src3)
{
    pixel_sum_t ret;

    ret.red   = (unsigned int)src1->red   + src2->red   + src3->red;
    ret.green = (unsigned int)src1->green + src2->green + src3->green;
    ret.blue  = (unsigned int)src1->blue  + src2->blue  + src3->blue;

    return ret;
}

static inline 
pixel_sum_t add_2_sum(pixel_sum_t *src1, pixel_sum_t *src2)
{
    pixel_sum_t ret;

    ret.red   = src1->red   + src2->red;
    ret.green = src1->green + src2->green;
    ret.blue  = src1->blue  + src2->blue;

    return ret;
}

static inline 
pixel_sum_t add_3_sum(pixel_sum_t *src1, pixel_sum_t *src2, pixel_sum_t *src3)
{
    pixel_sum_t ret;

    ret.red   = src1->red   + src2->red   + src3->red;
    ret.green = src1->green + src2->green + src3->green;
    ret.blue  = src1->blue  + src2->blue  + src3->blue;

    return ret;
}

static inline
pixel pixel_div(pixel_sum_t *src, int div)
{
    pixel ret;

    ret.red   = src->red   / div;
    ret.green = src->green / div;
    ret.blue  = src->blue  / div;

    return ret;
}

static inline 
void process_first_line(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j;
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_2_pixel(src, src + dim);

    for (j = 0; j < dim; j++) {
        if (j == 0) {   // 左上角
            sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 + dim);
            temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
            *dst = pixel_div(&temp, 4);
            SHOW(sum_temp, "process_first_line left");
        } else if (j == dim - 1) {  // 右上角
            temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
            *dst = pixel_div(&temp, 4);
            SHOW(sum_temp, "process_first_line right");
        } else {    // 上边沿
            sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 + dim);
            temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
            *dst = pixel_div(&temp, 6);
            SHOW(sum_temp, "process_first_line mid");
        }

        dst++;
        src++;
    }

    *ppSrc = src;
    *ppDst = dst;
}

static inline 
void process_last_line(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j;
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_2_pixel(src, src - dim);

    for (j = 0; j < dim; j++) {
        if (j == 0) {   // 左下角
            sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 - dim);
            temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
            *dst = pixel_div(&temp, 4);
            SHOW(sum_temp, "process_last_line left");
        } else if (j == dim - 1) {  // 右下角
            temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
            *dst = pixel_div(&temp, 4);
            SHOW(sum_temp, "process_last_line right");
        } else {    // 下边沿
            sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 - dim);
            temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
            *dst = pixel_div(&temp, 6);
            SHOW(sum_temp, "process_last_line mid");
        }

        dst++;
        src++;
    }

    *ppSrc = src;
    *ppDst = dst;
}

static inline
void process_middle_line(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j;
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_3_pixel(src, src - dim, src + dim);

    for (j = 0; j < dim; j++) {
        if (j == 0) {   // 左侧第一个
            sum_temp[(j + 1) % 3] = add_3_pixel(src + 1, src + 1 - dim, src + 1 + dim);
            temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
            *dst = pixel_div(&temp, 6);
            SHOW(sum_temp, "process_middle_line left");
        } else if (j == dim - 1) {  // 右侧第一个
            temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
            *dst = pixel_div(&temp, 6);
            SHOW(sum_temp, "process_middle_line right");
        } else {    // 中间
            sum_temp[(j + 1) % 3] = add_3_pixel(src + 1, src + 1 - dim, src + 1 + dim);
            temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
            *dst = pixel_div(&temp, 9);
            SHOW(sum_temp, "process_middle_line mid");
        }

        dst++;
        src++;
    }
    
    *ppSrc = src;
    *ppDst = dst;
}

char v2_smooth_descr[] = "v2_smooth: Add aditional value to save temporary sum";
void v2_smooth(int dim, pixel *src, pixel *dst) 
{
    pixel **ppSrc, **ppDst;
    int i;

    ppSrc = &src;
    ppDst = &dst;

    for (i = 0; i < dim; i++) {
        if (i == 0) {   // 首行
            process_first_line(dim, ppSrc, ppDst);
        } else if (i == dim - 1) {  // 尾行
            process_last_line(dim, ppSrc, ppDst);
        } else {
            process_middle_line(dim, ppSrc, ppDst);
        }
    }
}

#define UNWIND_WAY      8
#define REPEAT(x)       REPEAT_8(x)

#define REPEAT_4(x)     {x; x; x; x;}
#define REPEAT_8(x)     {REPEAT_4(x); REPEAT_4(x);}
#define REPEAT_16(x)    {REPEAT_8(x); REPEAT_8(x);}
#define REPEAT_32(x)    {REPEAT_16(x); REPEAT_16(x);}

char v3_smooth_descr[] = "v3_smooth: Loop unwind middle traversal";
void v3_smooth(int dim, pixel *src, pixel *dst)
{
        pixel **ppSrc, **ppDst;
    int i;

    ppSrc = &src;
    ppDst = &dst;

    // 首行
    process_first_line(dim, ppSrc, ppDst);

    // 中间行
    for (i = 0; i < dim - 2; i += UNWIND_WAY) {
        REPEAT({
            process_middle_line(dim, ppSrc, ppDst);
        });
    }

    for (; i < dim - 2; i++) {
        process_middle_line(dim, ppSrc, ppDst);
    }

    // 尾行
    process_last_line(dim, ppSrc, ppDst);
}

static 
void pixel_init(int dim, pixel *src)
{
    int i, j;

    for (i = 0; i < dim; i++) {
        for (j = 0; j < dim; j++) {
            src[RIDX(i, j, dim)].red = RIDX(i, j, dim) * 100;
        }
    }
}

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
    int red;
    int green;
    int blue;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->num++;
    return;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned short) (sum.red/sum.num);
    current_pixel->green = (unsigned short) (sum.green/sum.num);
    current_pixel->blue = (unsigned short) (sum.blue/sum.num);
    return;
}

/* 
 * avg - Returns averaged pixel value at (i,j) 
 */
static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
	    accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth 
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

int main ()
{
    bzero(g_src, sizeof(g_src));
    bzero(g_dst, sizeof(g_dst));

    pixel_init(g_dim, (pixel *)g_src);
    SHOW(NULL, "before smmoth");

    v3_smooth(g_dim, (pixel *)g_src, (pixel *)g_dst);
    SHOW(NULL, "after smooth");

    naive_smooth(g_dim, (pixel *)g_src, (pixel *)g_dst);
    SHOW(NULL, "answer");
}