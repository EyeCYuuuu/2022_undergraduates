#include <profile.h>
#define TASK_PRI 1

#define NUM_TEB_SORT  4
#define NUM_TEB_AR  11
#define NUM_TEB_FFT  8
#define NUM_TEB_RSA  15
#define NUM_TEB_DIJKSTRA 5
#define NUM_TEB_CEM 12
#define NUM_TEB_CRC  4
#define NUM_TEB_CUCKOO  16
#define NUM_TEB_BC  10

void _benchmark_ar_init();
void _benchmark_sort_init();
void _benchmark_rsa_init();
void _benchmark_fft_init();
void _benchmark_dijkstra_init();
void _benchmark_cuckoo_init();
void _benchmark_crc_init();
void _benchmark_bc_init();
void _benchmark_cem_init();
