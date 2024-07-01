/* "Since glibc 2.19, defining _GNU_SOURCE also has the effect
 *  of implicitly defining _DEFAULT_SOURCE." per feature_test_macros(7)
 *  Keep the both to avoid relying on knowing that bit of trivia.
 */

#define _GNU_SOURCE     // for CPU_SET(3).
#define _DEFAULT_SOURCE // for gethostname(2).

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <sched.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

static constexpr const uint8_t tsc_sz = 64; // size in bits
static constexpr const uint64_t reps = 10'000'000'000ULL;
static uint64_t log2_tsc_delta[CPU_SETSIZE][tsc_sz];
static uint64_t log2_tscp_delta[CPU_SETSIZE][tsc_sz];

void *
rdtsc( void *v){
    for( uint64_t i = 0; i < reps; i++ ){
        uint64_t t1 = __builtin_ia32_rdtsc();
        uint64_t t2 = __builtin_ia32_rdtsc();
        ((uint64_t *)v)[  __builtin_clz( t2 - t1 ) ]++;
    }
    return NULL;
}

void *
rdtscp( void *v){
    static unsigned int tsc_aux;
    for( uint64_t i = 0; i < reps; i++ ){
        uint64_t t1 = __builtin_ia32_rdtscp(&tsc_aux);
        uint64_t t2 = __builtin_ia32_rdtscp(&tsc_aux);
        ((uint64_t *)v)[  __builtin_clz( t2 - t1 ) ]++;
    }
    return NULL;
}

void
print_header( cpu_set_t *set ){
    static bool run_once = 0;
    if( !run_once ){
        run_once = 1;
        char hn[1024] = {'\0'};
        assert( 0 == gethostname( hn, 1023 ) );
        printf("# hostname = %s\n", hn);
        printf("# nCPU     = %d\n",        CPU_COUNT( set ) );
        printf("# tsc_sz   = %"PRIu8"\n",  tsc_sz);
        printf("# reps     = %"PRIu64"\n", reps);
        printf("#-------------------------------------\n");
        printf("engine");
        printf(" clz");
        for( int c = 0; c < CPU_SETSIZE; c++ ){
            if( CPU_ISSET( c, set ) ){
                printf(" cpu%04d",c);
            }
        }
        printf("\n");
    }
}

void
print_data( char const * const engine_name, cpu_set_t *set, uint64_t a[CPU_SETSIZE][tsc_sz] ){
    for( uint8_t i = 1; i < tsc_sz; i++ ){
        printf("%s", engine_name);
        printf(" %"PRIu8, i);
        for( int c = 0; c < CPU_SETSIZE; c++ ){
            if( CPU_ISSET( c, set ) ){
                printf(" %"PRIu64, a[c][i]);
            }
        }
        printf("\n");
    }
}

int
thread_engine( char const * const engine_name, void*(*start_routine)(void*),
        uint64_t log2_delta[CPU_SETSIZE][tsc_sz]){
    // Rather than go through the pain of figuring out how many CPUs we can
    // run on and allocating that much memory, just burn some more memory and allocate
    // for the maximum possible number of CPUs supported by cpu_set_t (aka CPU_SETSIZE).
    static pthread_t thrd[CPU_SETSIZE];
    static pthread_attr_t attrib[CPU_SETSIZE];

    cpu_set_t parent_set, child_set;

    CPU_ZERO( &parent_set );
    sched_getaffinity( getpid(), sizeof(cpu_set_t), &parent_set );

    // Set up threads and affinity
    for( int c = 0; c < CPU_SETSIZE; c++ ){
        if( CPU_ISSET( c, &parent_set ) ){
            CPU_ZERO( &child_set );
            CPU_SET( c, &child_set );
            assert( 0 == pthread_attr_init( &attrib[c] ) );
            assert( 0 == pthread_attr_setaffinity_np( &(attrib[c]), sizeof(cpu_set_t), &child_set ) );
        }
    }

    // Launch threads
    for( int c = 0; c < CPU_SETSIZE; c++ ){
        if( CPU_ISSET( c, &parent_set ) ){
            assert( 0 == pthread_create( &thrd[c], &attrib[c], start_routine, log2_delta[c] ) );
        }
    }

    // Cleanup
    for( int c = 0; c < CPU_SETSIZE; c++ ){
        if( CPU_ISSET( c, &parent_set ) ){
            assert( 0 == pthread_join( thrd[c], NULL ) );
            assert( 0 == pthread_attr_destroy( &attrib[c] ) );
        }
    }
    print_header( &parent_set );
    print_data( engine_name, &parent_set, log2_delta );

    return 0;
}

int
main(){
    thread_engine( "rdtsc",  rdtsc,  log2_tsc_delta );
    thread_engine( "rdtscp", rdtscp, log2_tscp_delta );
    return 0;
}
