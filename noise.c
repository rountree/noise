/* "Since glibc 2.19, defining _GNU_SOURCE also has the effect
 *  of implicitly defining _DEFAULT_SOURCE." per feature_test_macros(7)
 *  Keep the both to avoid relying on that bit of trivia.
 */

#define _GNU_SOURCE // per man CPU_SET
#define _DEFAULT_SOURCE // per man gethostname.

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <sched.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

static constexpr const uint8_t tsc_sz = 32; // Fixed at 32 bits on Intel
static constexpr const uint64_t reps = 10'000'000'000ULL;
static uint64_t a[CPU_SETSIZE][tsc_sz];

void *
spin( void *v){
    for( uint64_t i = 0; i < reps; i++ ){
        uint32_t t1 = __builtin_ia32_rdtsc();
        uint32_t t2 = __builtin_ia32_rdtsc();
        ((uint64_t *)v)[  __builtin_clz( t2 - t1 ) ]++;
    }
    return NULL;
}

void
print_header( cpu_set_t *set ){
    char hn[1024] = {'\0'};
    assert( 0 == gethostname( hn, 1023 ) );
    printf("# hostname = %s\n", hn);
    printf("# nCPU     = %d\n",        CPU_COUNT( set ) );
    printf("# tsc_sz   = %"PRIu8"\n",  tsc_sz);
    printf("# reps     = %"PRIu64"\n", reps);
    printf("#-------------------------------------\n");
    printf("clz");
    for( int c = 0; c < CPU_SETSIZE; c++ ){
        if( CPU_ISSET( c, set ) ){
            printf(" cpu%04d",c);
        }
    }
    printf("\n");
}

void
print_data( cpu_set_t *set ){
    for( uint8_t i = 1; i < tsc_sz; i++ ){
        printf("%"PRIu8, i);
        for( int c = 0; c < CPU_SETSIZE; c++ ){
            if( CPU_ISSET( c, set ) ){
                printf(" %"PRIu64, a[c][i]);
            }
        }
        printf("\n");
    }
}

int
main(){
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
            assert( 0 == pthread_create( &thrd[c], &attrib[c], spin, (void *)a[c] ) );
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
    print_data( &parent_set );

    return 0;
}
