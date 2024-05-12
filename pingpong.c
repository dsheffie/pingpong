#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <stdint.h>

#define ITERS (1<<24)

typedef struct {
  int tid;
  int cid;
  double lat;
} arg_t;

typedef struct {
  int x;
  int pad_[15];
} cacheline_t;

pthread_t threads[2];
arg_t args[2];

int waiters __attribute__((aligned(64)));
cacheline_t cl[2] __attribute__((aligned(64)));

static inline uint64_t get_cycles() {
  return __builtin_ia32_rdtsc();
}

void* worker(void *a) {
  arg_t *arg = (arg_t*)a;
  cpu_set_t cpuset;
  int i = ITERS;
  uint64_t t;
  volatile int *v_waiters = (volatile int*)&waiters;
  int tid = arg->tid;
  int otid = tid ^ 1;
#ifdef TWO_CACHELINES
  volatile int *v_cl_m = (volatile int*)&(cl[tid].x);
  volatile int *v_cl_o = (volatile int*)&(cl[otid].x);
#else
  volatile int *v_cl = (volatile int*)&(cl[0].x);
#endif
  /* bind thread to specific core */
  CPU_ZERO(&cpuset);
  CPU_SET(arg->cid, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  __sync_fetch_and_sub(&waiters, 1);
  /* wait for other cpu */
  while(*v_waiters != 0) {}

  t = get_cycles();
  while(i) {
#ifdef TWO_CACHELINES
    while(*v_cl_m == 0) {};
    *v_cl_m = 0;
    __sync_synchronize();
    *v_cl_o = 1;
#else
    /* while not this tid, spin */
    while(*v_cl != tid) {};
    *v_cl = otid;
#endif
    
    --i;
  }
  t = get_cycles()-t;
  arg->lat = ((double)t) / ITERS;
  return NULL;
}

int main() {
  int n_cores = 4;
  
  for(int i = 0; i < n_cores; i++) {
    args[0].cid = i;
    args[0].tid = 0;
    for(int j = i+1; j < n_cores; j++) {
      args[1].cid = j;
      args[1].tid = 1;
      cl[0].x = 0;
      cl[1].x = 1;
      waiters = 2;
      for(int k = 0; k < 2; k++) {
	pthread_create(threads+k,NULL,worker,args+k);
      }
      for(int k = 0; k < 2; k++) {
	pthread_join(threads[k], NULL);
      }
      printf("%d,%d,%g,%g\n", i, j, args[0].lat, args[1].lat);
    }
  }
  return 0;
};
