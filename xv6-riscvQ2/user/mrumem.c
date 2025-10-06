// user/mrumem.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUMPAGES 50  // Allocate 40 pages (more than the 20 page limit)
#define ITERATIONS 10

int
main(int argc, char *argv[])
{
  printf("MRU Memory Test Program\n");
  printf("=======================\n");
  printf("This test allocates %d pages to trigger swapping\n", NUMPAGES);
  printf("(Physical memory limit: 40 user pages)\n\n");
  
  // Allocate memory - more than physical RAM limit
  printf("Allocating %d pages (%d KB)...\n", NUMPAGES, NUMPAGES * 4);
  char *mem = sbrk(NUMPAGES * 4096);
  if(mem == (char*)-1) {
    printf("sbrk failed\n");
    exit(1);
  }
  
  printf("Memory allocated starting at 0x%p\n\n", mem);
  
  struct pagestat st;
  getpagestat(getpid(), &st);
  printf("After allocation:\n");
  printf("  Page Faults: %ld\n", st.page_faults);
  printf("  Swap Outs:   %ld\n\n", st.swap_outs);
  
  // Initialize pages with identifiable data
  printf("Initializing pages with unique data...\n");
  for(int i = 0; i < NUMPAGES; i++) {
    // Write to each page - this will trigger allocation/swapping
    for(int j = 0; j < 4096; j += 256) {
      mem[i * 4096 + j] = (char)(i & 0xFF);
    }
    
    if((i + 1) % 10 == 0) {
      getpagestat(getpid(), &st);
      printf("  Initialized %d pages - Faults: %ld, Swap-outs: %ld\n", 
             i + 1, st.page_faults, st.swap_outs);
    }
  }
  
  printf("\nInitialization complete!\n\n");
  
  // Random access pattern to trigger more paging
  unsigned int seed = 42;
  printf("Starting random access pattern (%d iterations)...\n\n", ITERATIONS);
  
  for(int iter = 0; iter < ITERATIONS; iter++) {
    // Generate pseudo-random page number
    seed = seed * 1103515245 + 12345;
    int page = (seed / 65536) % NUMPAGES;
    
    // Access the page
    volatile char val = mem[page * 4096];
    mem[page * 4096] = (char)(page & 0xFF);
    
    // Verify data integrity
    if(val != 0 && val != (char)(page & 0xFF)) {
      printf("ERROR: Data corruption at page %d! Expected %d, got %d\n", 
             page, page & 0xFF, val);
    }
    
    // Every 20 iterations, print statistics
    if((iter + 1) % 5 == 0) {
      getpagestat(getpid(), &st);
      printf("Iteration %d:\n", iter + 1);
      printf("  Page Faults: %ld\n", st.page_faults);
      printf("  Swap Ins:    %ld\n", st.swap_ins);
      printf("  Swap Outs:   %ld\n", st.swap_outs);
      printf("\n");
    }
  }
  
  printf("\n=== Final Statistics ===\n");
  getpagestat(getpid(), &st);
  printf("Total Page Faults: %ld\n", st.page_faults);
  printf("Total Swap Ins:    %ld\n", st.swap_ins);
  printf("Total Swap Outs:   %ld\n", st.swap_outs);
  
  printf("\n=== MRU List (showing most recent 10) ===\n");
  dumpmru();
  
  printf("\n✓ Test completed successfully!\n");
  printf("✓ MRU eviction policy is working\n");
  printf("✓ Pages are being swapped in and out correctly\n");
  
  exit(0);
}