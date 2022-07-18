#include <stdio.h>

const static int M = 24, N = 24;
static int A[256][256];
static int B[256][256];

void show_B(int M, int N, int line);

#define BOR(x, y)  *((i != M - 8) ? (&B[x][y] + 8) :  (&B[x][y] - 8))

void BLOCK_TRANSPOSE_64X64(int i, int j) {
    int cache_buf[8];
    int x, y;

    /* �ǶԽ����ȶ��ϰ벿�ֽ��д��� */                                                              
    for (y = 0; y < 4; y++) {                                                                    
        /* ���ϰಿ�ֺ����һ�� cache_line, �ضϳ������ֲ��ҷŵ� B �ϵ��ϰ벿�� */                    
        for (x = 0; x < 8; x++) {                                                                
            cache_buf[x] = A[i + y][j + x];                                                      
        }                                                                                        
        for (x = 0; x < 8; x++) {                                                                
            if (x < 4) {                                                                         
                B[j + x][i + y] = cache_buf[x];                                                  
            } else {                                                                             
                B[j + x - 4][i + y + 4] = cache_buf[x];                                          
            }                                                                                    
        }                                                                                       
    }        
    show_B(M, N, __LINE__);                                                                                      

                                                                                                 
    /* �°벿�ֵ����, �����ȡ 4 ������, �滻�� B ���ϰ벿����ȷת�õ�λ��, */                       
    /* �����滻����һ����������İ���, �ŵ� B ���°벿��                     */                      
    for (x = 0; x < 4; x++) {                                                                    
        for (y = 0; y < 4; y++) {                                                                
            cache_buf[y] = A[i + y + 4][j + x];                                                  
        }                                                                                        
        for (y = 0; y < 4; y++) {                                                                
            cache_buf[y + 4] = B[j + x][i + y + 4];                                              
        }                                                                                        
        for (y = 0; y < 4; y++) {                                                                
            B[j + x][i + y + 4] = cache_buf[y];                                                  
        }                                                                                        
        for (y = 0; y < 4; y++) {                                                                
            B[j + x + 4][i + y] = cache_buf[y + 4];                                              
        }                                                                                        
    }                                                                                            
    show_B(M, N, __LINE__); 
                                                                                                 
    /* ֻʣ������½� 4x4 �Ŀ�, ���Ҷ������� cache_line ��, ֱ�ӽ���ת�� */                          
    for (y = 4; y < 8; y++) {                                                                    
        for (x = 4; x < 8; x++) {                                                                
            cache_buf[x] = A[i + y][j + x];                                                      
        }                                                                                        
        for (x = 4; x < 8; x++) {                                                                
            B[j + x][i + y] = cache_buf[x];                                                      
        }                                                                                        
    }                                                                                            
    show_B(M, N, __LINE__); 
}

void fill_A(int M, int N)
{
    int x, y;

    for (y = 0; y < N; y++) {
        for (x = 0; x < M; x++) {
            A[y][x] = x;
        }
    }
}

void show_B(int M, int N, int line)
{
    int x, y;

    printf("matrix B at line(%d) is: \n", line);
    for (y = 0; y < N; y++) {
        for (x = 0; x < M; x++) {
            printf("%2d ", B[y][x]);
        }
        printf("    ");
        for (x = 0; x < M; x++) {
            printf("%2d ", B[y][x + 8]);
        }
        printf("\n");
    }
}

void show_A(int M, int N)
{
    int x, y;

    printf("matrix A is: \n");
    for (y = 0; y < N; y++) {
        for (x = 0; x < M; x++) {
            printf("%2d ", A[y][x]);
        }
        printf("\n");
    }
}

void transpose_submit(int M, int N)
{
    if ((M == 64 && N == 64) || 1) {
        int cache_buf[8];
        int i, j, x, y;

        /* ���ȴ���Խ��ߵĿ�, ѡ�� (i, i + 1) ����Ϊ����� */
        for (i = 0; i < M; i += 8) {
            for (y = 4; y < 8; y++) {
                for (x = 0; x < 8; x++) {
                    cache_buf[x] = A[i + y][i + x];
                }
                for (x = 0; x < 8; x++) {
                    BOR(i + y - 4,i + x) = cache_buf[x];
                }
                show_B(M, N, __LINE__);
            }


            for (y = 0; y < 4; y++) {
                for (x = 0; x < 8; x++) {
                    cache_buf[x] = A[i + y][i + x];
                }
                for (x = 0; x < 8; x++) {
                    B[i + y][i + x] = cache_buf[x];
                }
                show_B(M, N, __LINE__);
            }

            for (y = 0; y < 4; y++) {
                for (x = y; x < 4; x++) {
                    cache_buf[0] = B[i + y][i + x];
                    cache_buf[1] = B[i + x][i + y];
                    B[i + y][i + x] = cache_buf[1];
                    B[i + x][i + y] = cache_buf[0];
                }
            }
            show_B(M, N, __LINE__);
            for (y = 0; y < 4; y++) {
                for (x = y + 4; x < 8; x++) {
                    cache_buf[0] = B[i + y][i + x];
                    cache_buf[1] = B[i + x - 4][i + y + 4];
                    B[i + y][i + x] = cache_buf[1];
                    B[i + x - 4][i + y + 4] = cache_buf[0];
                }
            }
            show_B(M, N, __LINE__);
            for (y = 0; y < 4; y++) {
                for (x = y; x < 4; x++) {
                    cache_buf[0] = BOR(i + y, i + x);
                    cache_buf[1] = BOR(i + x, i + y);
                    BOR(i + y, i + x) = cache_buf[1];
                    BOR(i + x, i + y) = cache_buf[0];
                }
            }
            show_B(M, N, __LINE__);
            for (y = 0; y < 4; y++) {
                for (x = y + 4; x < 8; x++) {
                    cache_buf[0] = BOR(i + y, i + x);
                    cache_buf[1] = BOR(i + x - 4, i + y + 4);
                    BOR(i + y, i + x) = cache_buf[1];
                    BOR(i + x - 4, i + y + 4) = cache_buf[0];
                }
            }
            show_B(M, N, __LINE__);

            for (y = 0; y < 4; y++) {
                for (x = 0; x < 4; x++) {
                    cache_buf[0] = BOR(i + y, i + x);
                    cache_buf[1] = B[i + y][i + x + 4];
                    B[i + y][i + x + 4] = cache_buf[0];
                    BOR(i + y, i + x) = cache_buf[1];
                }
            }
            show_B(M, N, __LINE__);

            for (y = 0; y < 4; y++) {
                for (x = 0; x < 8; x++) {
                    cache_buf[x] = BOR(i + y, i + x);
                }
                for (x = 0; x < 8; x++) {
                    B[i + y + 4][i + x] = cache_buf[x];
                }
            }
            show_B(M, N, __LINE__);

            /* ���ÿ�� 4 ���Ѿ������� cache �У� ���ȴ���� */
            if (i != M - 8) {
                BLOCK_TRANSPOSE_64X64((i + 8), (i));
            } else {
                BLOCK_TRANSPOSE_64X64((i - 8), (i));
                
            }
            show_B(M, N, __LINE__);
        }

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
}

int main (void)
{
    fill_A(M, N);
    show_A(M, N);

    transpose_submit(M, N);

    show_B(M, N, __LINE__);

    return 0;
}