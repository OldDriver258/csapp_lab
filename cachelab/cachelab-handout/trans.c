/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
#define BOR(x, y)  *((i != M - 8) ? (&B[x][y] + 8) :  (&B[x][y] - 8))
#define BLOCK_TRANSPOSE_64X64(i, j)                                                              \
do {                                                                                             \
    /* �ǶԽ����ȶ��ϰ벿�ֽ��д��� */                                                              \
    for (y = 0; y < 4; y++) {                                                                    \
        /* ���ϰಿ�ֺ����һ�� cache_line, �ضϳ������ֲ��ҷŵ� B �ϵ��ϰ벿�� */                    \
        for (x = 0; x < 8; x++) {                                                                \
            cache_buf[x] = A[i + y][j + x];                                                      \
        }                                                                                        \
        for (x = 0; x < 8; x++) {                                                                \
            if (x < 4) {                                                                         \
                B[j + x][i + y] = cache_buf[x];                                                  \
            } else {                                                                             \
                B[j + x - 4][i + y + 4] = cache_buf[x];                                          \
            }                                                                                    \
        }                                                                                        \
    }                                                                                            \
                                                                                                 \
    /* �°벿�ֵ����, �����ȡ 4 ������, �滻�� B ���ϰ벿����ȷת�õ�λ��, */                       \
    /* �����滻����һ����������İ���, �ŵ� B ���°벿��                     */                      \
    for (x = 0; x < 4; x++) {                                                                    \
        for (y = 0; y < 4; y++) {                                                                \
            cache_buf[y] = A[i + y + 4][j + x];                                                  \
        }                                                                                        \
        for (y = 0; y < 4; y++) {                                                                \
            cache_buf[y + 4] = B[j + x][i + y + 4];                                              \
        }                                                                                        \
        for (y = 0; y < 4; y++) {                                                                \
            B[j + x][i + y + 4] = cache_buf[y];                                                  \
        }                                                                                        \
        for (y = 0; y < 4; y++) {                                                                \
            B[j + x + 4][i + y] = cache_buf[y + 4];                                              \
        }                                                                                        \
    }                                                                                            \
                                                                                                 \
    /* ֻʣ������½� 4x4 �Ŀ�, ���Ҷ������� cache_line ��, ֱ�ӽ���ת�� */                          \
    for (y = 4; y < 8; y++) {                                                                    \
        for (x = 4; x < 8; x++) {                                                                \
            cache_buf[x] = A[i + y][j + x];                                                      \
        }                                                                                        \
        for (x = 4; x < 8; x++) {                                                                \
            B[j + x][i + y] = cache_buf[x];                                                      \
        }                                                                                        \
    }                                                                                            \
}while(0);

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 8 && N == 8) {
        int cache_buf[8];
        int x, y;

        for (y = 0; y < M; y++) {
            for (x = 0; x < 8; x++) {
                cache_buf[x] = A[y][x];
            }
            for (x = 0; x < 8; x++) {
                B[x][y] = cache_buf[x];
            }
        }
    }

    if (M == 32 && N == 32) {
        int cache_buf[8];
        int i, j, x, y;

        for (j = 0; j < N; j += 8) {
            for (i = 0; i < M; i += 8) {
                if (i != j) {
                    /* �ǶԽ��ߵĿ�ֱ�ӽ���ת�� */
                    for (y = 0; y < 8; y++) {
                        for (x = 0; x < 8; x++) {
                            cache_buf[x] = A[i + y][j + x];
                        }
                        for (x = 0; x < 8; x++) {
                            B[j + x][i + y] = cache_buf[x];
                        }
                    }
                } else {
                    /*  �Խ��ߵĿ��Ƚ��а��� */
                    for (y = 0; y < 8; y++) {
                        for (x = 0; x < 8; x++) {
                            cache_buf[x] = A[i + y][j + x];
                        }
                        for (x = 0; x < 8; x++) {
                            B[i + y][j + x] = cache_buf[x];
                        }
                    }
                    /*  �԰��˹�����С�����ת�� */
                    for (y = 0; y < 8; y++) {
                        for (x = y; x < 8; x++) {
                            cache_buf[0] = B[i + y][j + x];
                            cache_buf[1] = B[j + x][i + y];
                            B[i + y][j + x] = cache_buf[1];
                            B[j + x][i + y] = cache_buf[0];
}
                    }
                }
            }
        }
    }

    if (M == 64 && N == 64) {
        int cache_buf[8];
        int i, j, x, y;

        /* ���ȴ����Խ��ߵĿ�, ѡ�� (i, i + 1) ����Ϊ����� */
        for (i = 0; i < M; i += 8) {
            for (y = 4; y < 8; y++) {
                for (x = 0; x < 8; x++) {
                    cache_buf[x] = A[i + y][i + x];
                }
                for (x = 0; x < 8; x++) {
                    BOR(i + y - 4,i + x) = cache_buf[x];
                }
            }

            for (y = 0; y < 4; y++) {
                for (x = 0; x < 8; x++) {
                    cache_buf[x] = A[i + y][i + x];
                }
                for (x = 0; x < 8; x++) {
                    B[i + y][i + x] = cache_buf[x];
                }
            }

            for (y = 0; y < 4; y++) {
                for (x = y; x < 4; x++) {
                    cache_buf[0] = B[i + y][i + x];
                    cache_buf[1] = B[i + x][i + y];
                    B[i + y][i + x] = cache_buf[1];
                    B[i + x][i + y] = cache_buf[0];
                }
            }
            for (y = 0; y < 4; y++) {
                for (x = y + 4; x < 8; x++) {
                    cache_buf[0] = B[i + y][i + x];
                    cache_buf[1] = B[i + x - 4][i + y + 4];
                    B[i + y][i + x] = cache_buf[1];
                    B[i + x - 4][i + y + 4] = cache_buf[0];
                }
            }
            for (y = 0; y < 4; y++) {
                for (x = y; x < 4; x++) {
                    cache_buf[0] = BOR(i + y, i + x);
                    cache_buf[1] = BOR(i + x, i + y);
                    BOR(i + y, i + x) = cache_buf[1];
                    BOR(i + x, i + y) = cache_buf[0];
                }
            }
            for (y = 0; y < 4; y++) {
                for (x = y + 4; x < 8; x++) {
                    cache_buf[0] = BOR(i + y, i + x);
                    cache_buf[1] = BOR(i + x - 4, i + y + 4);
                    BOR(i + y, i + x) = cache_buf[1];
                    BOR(i + x - 4, i + y + 4) = cache_buf[0];
                }
            }

            for (y = 0; y < 4; y++) {
                for (x = 0; x < 4; x++) {
                    cache_buf[0] = BOR(i + y, i + x);
                    cache_buf[1] = B[i + y][i + x + 4];
                    B[i + y][i + x + 4] = cache_buf[0];
                    BOR(i + y, i + x) = cache_buf[1];
                }
            }

            for (y = 0; y < 4; y++) {
                for (x = 0; x < 8; x++) {
                    cache_buf[x] = BOR(i + y, i + x);
                }
                for (x = 0; x < 8; x++) {
                    B[i + y + 4][i + x] = cache_buf[x];
                }
            }

            /* ���ÿ�� 4 ���Ѿ������� cache �У� ���ȴ����� */
            if (i != M - 8) {
                BLOCK_TRANSPOSE_64X64((i + 8), (i));
            } else {
                BLOCK_TRANSPOSE_64X64((i - 8), (i));
                
            }
        }

        /* ����ʣ�µĿ� */
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                if (!((i == j) || 
                      (i == j + 8) || 
                      ((i == M - 16) && (j == N - 8)))) {
                    BLOCK_TRANSPOSE_64X64(i, j);
                }
            }
        }
    }

    if (M == 61 && N == 67) {
        int cache_buf[8];
        int i, j, x;

        for (j = 0; j < (M / 8 * 8); j += 8){
            for (i = 0; i < (N / 8 * 8); ++i) {
                for(x = 0; x < 8; x++) {
                    cache_buf[x] = A[i][j + x];
                }
                for(x = 0; x < 8; x++) {
                    B[j + x][i] = cache_buf[x];
                }
            }
        }
        for (i = (N / 8 * 8); i < N; ++i) {
			for (j = (M / 8 * 8); j < M; ++j)
			{
				cache_buf[0] = A[i][j];
				B[j][i] = cache_buf[0];
			}
        }
		for (i = 0; i < N; ++i) {
			for (j = (M / 8 * 8); j < M; ++j)
			{
				cache_buf[0] = A[i][j];
				B[j][i] = cache_buf[0];
			}
        }
		for (i = (N / 8 * 8); i < N; ++i) {
			for (j = 0; j < M; ++j)
			{
				cache_buf[0] = A[i][j];
				B[j][i] = cache_buf[0];
			}
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}


/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

