/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following team struct 
 */
team_t team = {
    "bovik",              /* Team name */

    "Harry Q. Bovik",     /* First member full name */
    "bovik@nowhere.edu",  /* First member email address */

    "",                   /* Second member full name (leave blank if none) */
    ""                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

char v1_rotate_descr[] = "v1_rotate: Reduce the calc times of dim-1-j";
void v1_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (j = 0; j < dim; j++) {
        int diff = dim - j - 1;
        for (i = 0; i < dim; i++) {
            dst[RIDX(diff, i, dim)] = src[RIDX(i, j, dim)];
        }
    }

}

char v2_rotate_descr[] = "v2_rotate: Divide whole matrix to 32*32 small block";
void v2_rotate(int dim, pixel *src, pixel *dst)
{
    int row, col;
    int row32, col32;
    int diff;

    for (row = 0; row < dim; row += 32) {
        for (col = 0; col < dim; col += 32) {
            for (row32 = row; row32 < (row + 32); row32++) {
                diff = dim - row32 - 1;
                for (col32 = col; col32 < (col + 32); col32++) {
                    dst[RIDX(diff, col32, dim)] = src[RIDX(col32, row32, dim)];
                }
            }
        }
    }
}

#define REPEAT32(x) {x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x; x;}

char v3_rotate_descr[] = "v3_rotate: Don`t use RIDX macro, loop unwind 32*32 small block";
void v3_rotate(int dim, pixel *src, pixel *dst)
{
    int row, col;
    int row32;
    int diff;
    pixel *src_start = src;
    pixel *dst_start = dst;

    for (row = 0; row < dim; row += 32) {
        for (col = 0; col < dim; col += 32) {
            diff = dim - row - 1;
            src  = src_start + ((col  * dim) + row);
            dst  = dst_start + ((diff * dim) + col);
            for (row32 = row; row32 < (row + 32); row32++) {
                REPEAT32({
                    *dst = *src;
                    dst++;
                    src += dim;
                })

                dst = dst - 32 - dim;
                src = src - (32 * dim) + 1;
            }

        }
    }
}

char v4_rotate_descr[] = "v4_rotate: Replace use of src_start and dst_start";
void v4_rotate(int dim, pixel *src, pixel *dst)
{
    int row, col;
    int row32;

    dst += (dim - 1) * dim;

    for (row = 0; row < dim; row += 32) {
        for (col = 0; col < dim; col += 32) {
            for (row32 = row; row32 < (row + 32); row32++) {
                REPEAT32({
                    *dst = *src;
                    dst++;
                    src += dim;
                })

                dst = dst - 32 - dim;
                src = src - (32 * dim) + 1;
            }
        src = src + (32 * dim) - 32;
        dst = dst + 32 + (32 * dim);
        }
    src = src - (dim * dim) + 32;
    dst = dst - (dim * 32)  - dim;
    }
}

char v5_rotate_descr[] = "v5_rotate: Change traversal order, Reduce point operation";
void v5_rotate(int dim, pixel *src, pixel *dst)
{
    int row, col;
    int row32;

    dst += (dim - 1) * dim;

    for (col = 0; col < dim; col += 32) {
        for (row = 0; row < dim; row += 32) {
            for (row32 = row; row32 < (row + 32); row32++) {
                REPEAT32({
                    *dst = *src;
                    dst++;
                    src += dim;
                })

                dst = dst - 32 - dim;
                src = src - (32 * dim) + 1;
            }
        }
        dst = dst + 32 + (dim * dim);
        src = src + (32 * dim) - dim;
    }
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char *rotate_descr = v5_rotate_descr;
void rotate(int dim, pixel *src, pixel *dst) 
{
    v5_rotate(dim, src, dst);
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() 
{
    add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
    /* ... Register additional test functions here */
    add_rotate_function(&v1_rotate, v1_rotate_descr);  
    add_rotate_function(&v2_rotate, v2_rotate_descr); 
    add_rotate_function(&v3_rotate, v3_rotate_descr);
    add_rotate_function(&v4_rotate, v4_rotate_descr);
    add_rotate_function(&v5_rotate, v5_rotate_descr);
}


/***************
 * SMOOTH KERNEL
 **************/

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

char v1_smooth_descr[] = "v1_smooth: Avoid calling funciton";
void v1_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++) {
        for (j = 0; j < dim; j++) {
            int ii, jj;
            pixel_sum sum;
            pixel current_pixel;

            sum.red = sum.green = sum.blue = 0;
            sum.num = 0;
            for(ii = max(i - 1, 0); ii <= min(i + 1, dim - 1); ii++) {
                for(jj = max(j - 1, 0); jj <= min(j + 1, dim - 1); jj++) {
                    sum.red   += (int) src[RIDX(ii, jj, dim)].red;
                    sum.green += (int) src[RIDX(ii, jj, dim)].green;
                    sum.blue  += (int) src[RIDX(ii, jj, dim)].blue;
                    sum.num++;
                }   
            }

            current_pixel.red   = (unsigned short) (sum.red   / sum.num);
            current_pixel.green = (unsigned short) (sum.green / sum.num);
            current_pixel.blue  = (unsigned short) (sum.blue  / sum.num);

            dst[RIDX(i, j, dim)] = current_pixel;
        }
    }
	
}


typedef struct {
   unsigned int red;
   unsigned int green;
   unsigned int blue;
} pixel_sum_t;

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
    int j = 0;;
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_2_pixel(src, src + dim);
    sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 + dim);

    // ���Ͻ�
    temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    // �ϱ���
    for (j = 1; j < dim - 1; j++) {
        sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 + dim);
        temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
        *dst = pixel_div(&temp, 6);
        dst++;
        src++;
    }

    // ���Ͻ�
    temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    *ppSrc = src;
    *ppDst = dst;
}

static inline 
void process_last_line(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j = 0;
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_2_pixel(src, src - dim);
    sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 - dim);

    // ���½�
    temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    // �±���
    for (j = 1; j < dim - 1; j++) {
        sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 - dim);
        temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
        *dst = pixel_div(&temp, 6);
        dst++;
        src++;
    }

    // ���½�
    temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    *ppSrc = src;
    *ppDst = dst;
}

static inline
void process_middle_line(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j = 0;
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_3_pixel(src, src - dim, src + dim);
    sum_temp[(j + 1) % 3] = add_3_pixel(src + 1, src + 1 - dim, src + 1 + dim);

    // ����һ��
    temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
    *dst = pixel_div(&temp, 6);
    dst++;
    src++;

    // �м�
    for (j = 1; j < dim - 1; j++) {  
        sum_temp[(j + 1) % 3] = add_3_pixel(src + 1, src + 1 - dim, src + 1 + dim);
        temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
        *dst = pixel_div(&temp, 9);
        dst++;
        src++;
    }

    // �Ҳ��һ��
    temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
    *dst = pixel_div(&temp, 6);
    dst++;
    src++;
    
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

    // ����
    process_first_line(dim, ppSrc, ppDst);

    // �м���
    for (i = 0; i < dim - 2; i++) {
        process_middle_line(dim, ppSrc, ppDst);
    }

    // β��
    process_last_line(dim, ppSrc, ppDst);
}

#define UNWIND_WAY      6
#define REPEAT          REPEAT_6

#define REPEAT_3(x)     {x; x; x;}
#define REPEAT_6(x)     {REPEAT_3(x);  REPEAT_3(x);}
#define REPEAT_9(x)     {REPEAT_6(x);  REPEAT_3(x);}
#define REPEAT_12(x)    {REPEAT_9(x);  REPEAT_3(x);}
#define REPEAT_15(x)    {REPEAT_12(x); REPEAT_3(x);}
#define REPEAT_18(x)    {REPEAT_15(x); REPEAT_3(x);}
#define REPEAT_21(x)    {REPEAT_18(x); REPEAT_3(x);}
#define REPEAT_24(x)    {REPEAT_21(x); REPEAT_3(x);}
#define REPEAT_27(x)    {REPEAT_24(x); REPEAT_3(x);}
#define REPEAT_30(x)    {REPEAT_27(x); REPEAT_3(x);}

static inline 
void process_first_line_unwind(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j = 0;
    int length = dim - 1;
    int limit  = length - (UNWIND_WAY - 1);
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_2_pixel(src, src + dim);
    sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 + dim);

    // ���Ͻ�
    temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    // �ϱ���
    for (j = 1; j < limit;) {
        REPEAT({
            sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 + dim);
            temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
            *dst = pixel_div(&temp, 6);
            dst++;
            src++;
            j++;
        })
    }

    for (; j < length; j++) {
        sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 + dim);
        temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
        *dst = pixel_div(&temp, 6);
        dst++;
        src++;
    }

    // ���Ͻ�
    temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    *ppSrc = src;
    *ppDst = dst;
}

static inline 
void process_last_line_unwind(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j = 0;
    int length = dim - 1;
    int limit  = length - (UNWIND_WAY - 1);
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_2_pixel(src, src - dim);
    sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 - dim);

    // ���½�
    temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    // �±���
    for (j = 1; j < limit;) {
        REPEAT({
            sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 - dim);
            temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
            *dst = pixel_div(&temp, 6);
            dst++;
            src++;
            j++;
        })
    }

    for (; j < length; j++) {
        sum_temp[(j + 1) % 3] = add_2_pixel(src + 1, src + 1 - dim);
        temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
        *dst = pixel_div(&temp, 6);
        dst++;
        src++;
    }

    // ���½�
    temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
    *dst = pixel_div(&temp, 4);
    dst++;
    src++;

    *ppSrc = src;
    *ppDst = dst;
}

static inline
void process_middle_line_unwind(int dim, pixel **ppSrc, pixel **ppDst)
{
    int j = 0;
    int length = dim - 1;
    int limit  = length - (UNWIND_WAY - 1);
    pixel *src, *dst;
    pixel_sum_t sum_temp[3];
    pixel_sum_t temp;

    src = *ppSrc;
    dst = *ppDst;
    sum_temp[0] = add_3_pixel(src, src - dim, src + dim);
    sum_temp[(j + 1) % 3] = add_3_pixel(src + 1, src + 1 - dim, src + 1 + dim);

    // ����һ��
    temp = add_2_sum(&sum_temp[0], &sum_temp[1]);
    *dst = pixel_div(&temp, 6);
    dst++;
    src++;

    // �м�
    for (j = 1; j < limit;) {
        REPEAT({ 
            sum_temp[(j + 1) % 3] = add_3_pixel(src + 1, src + 1 - dim, src + 1 + dim);
            temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
            *dst = pixel_div(&temp, 9);
            dst++;
            src++;
            j++;
        })
    }

    for (; j < length; j++) {
        sum_temp[(j + 1) % 3] = add_3_pixel(src + 1, src + 1 - dim, src + 1 + dim);
        temp = add_3_sum(&sum_temp[0], &sum_temp[1], &sum_temp[2]);
        *dst = pixel_div(&temp, 9);
        dst++;
        src++;
    }

    // �Ҳ��һ��
    temp = add_2_sum(&sum_temp[(dim - 1) % 3], &sum_temp[(dim - 2) % 3]);
    *dst = pixel_div(&temp, 6);
    dst++;
    src++;
    
    *ppSrc = src;
    *ppDst = dst;
}

char v3_smooth_descr[] = "v3_smooth: Loop unwind middle traversal";
void v3_smooth(int dim, pixel *src, pixel *dst)
{
    pixel **ppSrc, **ppDst;
    int i;
    int length = dim - 2;
    int limit  = length - (UNWIND_WAY - 1);

    ppSrc = &src;
    ppDst = &dst;

    // ����
    process_first_line_unwind(dim, ppSrc, ppDst);

    // �м���
    for (i = 0; i < limit; i += UNWIND_WAY) {
        REPEAT({
            process_middle_line_unwind(dim, ppSrc, ppDst);
        });
    }

    for (; i < length; i++) {
        process_middle_line_unwind(dim, ppSrc, ppDst);
    }

    // β��
    process_last_line_unwind(dim, ppSrc, ppDst);
}

/*
 * smooth - Your current working version of smooth. 
 * IMPORTANT: This is the version you will be graded on
 */
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) 
{
    naive_smooth(dim, src, dst);
}


/********************************************************************* 
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_smooth_functions() {
    add_smooth_function(&smooth, smooth_descr);
    add_smooth_function(&naive_smooth, naive_smooth_descr);
    /* ... Register additional test functions here */
    add_smooth_function(&v1_smooth, v1_smooth_descr);
    add_smooth_function(&v2_smooth, v2_smooth_descr);
    add_smooth_function(&v3_smooth, v3_smooth_descr);
}

