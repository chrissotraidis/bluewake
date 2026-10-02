#include "native_work_pool.h"
#if defined(_WIN32)
// The optional POSIX worker implementation is currently exercised on macOS.
// Keep the portable serial path available on Windows until its worker backend
// has the same unload and exact-output coverage.
void bluewake_parallel_range(size_t count,size_t grain,BluewakeRangeWork work,void* context) {
    (void)grain;work(context,0,count);
}
unsigned long long bluewake_parallel_batches(void) { return 0; }
#else
#include <pthread.h>
#include <stdlib.h>
#include <fenv.h>

// One guest execution thread owns submission. Workers only see immutable
// input views and disjoint output ranges; every batch joins before return.
enum { MAX_WORKERS = 8 };
static struct {
    pthread_mutex_t mutex;
    pthread_cond_t ready, finished;
    pthread_t threads[MAX_WORKERS];
    unsigned ids[MAX_WORKERS];
    unsigned workers, remaining;
    unsigned long long generation, batches;
    int stop;
    size_t count;
    BluewakeRangeWork work;
    void* context;
} pool = {.mutex=PTHREAD_MUTEX_INITIALIZER,
          .ready=PTHREAD_COND_INITIALIZER, .finished=PTHREAD_COND_INITIALIZER};
static pthread_once_t once = PTHREAD_ONCE_INIT;

static void* worker(void* argument) {
    fesetround(FE_TONEAREST);
    unsigned id=*(unsigned*)argument;
    unsigned long long seen=0;
    pthread_mutex_lock(&pool.mutex);
    for (;;) {
        while (!pool.stop && seen==pool.generation)
            pthread_cond_wait(&pool.ready,&pool.mutex);
        if (pool.stop) break;
        seen=pool.generation;
        size_t first=pool.count*id/(pool.workers+1);
        size_t end=pool.count*(id+1)/(pool.workers+1);
        BluewakeRangeWork work=pool.work;
        void* context=pool.context;
        pthread_mutex_unlock(&pool.mutex);
        work(context,first,end);
        pthread_mutex_lock(&pool.mutex);
        if (--pool.remaining==0) pthread_cond_signal(&pool.finished);
    }
    pthread_mutex_unlock(&pool.mutex);
    return NULL;
}

static void initialize(void) {
    const char* option=getenv("BLUEWAKE_NATIVE_WORKERS");
    char* end=NULL;
    long requested=option?strtol(option,&end,10):0;
    if (!option || end==option || *end || requested<1) return;
    if (requested>MAX_WORKERS) requested=MAX_WORKERS;
    for (unsigned i=0;i<(unsigned)requested;++i) {
        pool.ids[i]=i;
        if (pthread_create(&pool.threads[i],NULL,worker,&pool.ids[i])!=0) break;
        ++pool.workers;
    }
}

// A module may be unloaded by an oracle test before process exit. Join before
// its code disappears; an atexit-only worker teardown would be too late.
__attribute__((destructor)) static void shutdown_pool(void) {
    pthread_mutex_lock(&pool.mutex);
    pool.stop=1;
    pthread_cond_broadcast(&pool.ready);
    pthread_mutex_unlock(&pool.mutex);
    for (unsigned i=0;i<pool.workers;++i) pthread_join(pool.threads[i],NULL);
}

void bluewake_parallel_range(size_t count,size_t minimum_grain,
                             BluewakeRangeWork work,void* context) {
    pthread_once(&once,initialize);
    if (!pool.workers || count/(pool.workers+1)<minimum_grain) {
        work(context,0,count);
        return;
    }
    pthread_mutex_lock(&pool.mutex);
    pool.count=count;pool.context=context;pool.work=work;
    pool.remaining=pool.workers;
    ++pool.generation;++pool.batches;
    pthread_cond_broadcast(&pool.ready);
    pthread_mutex_unlock(&pool.mutex);
    work(context,count*pool.workers/(pool.workers+1),count);
    pthread_mutex_lock(&pool.mutex);
    while (pool.remaining) pthread_cond_wait(&pool.finished,&pool.mutex);
    pthread_mutex_unlock(&pool.mutex);
}

unsigned long long bluewake_parallel_batches(void) { return pool.batches; }
#endif
