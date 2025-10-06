#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/riscv.h"

#define PGSIZE 4096
int freepages(void);


void
test_cow_vs_nocow(void)
{ 
  int npages = 50;  
  char *mem = sbrk(npages * PGSIZE);
  
  // Write to all pages
  for(int i = 0; i < npages * PGSIZE; i++){
    mem[i] = 'X';
  }
  
  int free_before = freepages();
  
  int pid = fork();
  if(pid == 0){
    int free_after = freepages();
    int used = free_before - free_after;
    
    printf("Allocated %d pages, Fork used: %d pages\n", npages, used);
    
    // With COW: ~7 pages (kernel overhead)
    // Without COW: ~57 pages (50 copied + 7 overhead)
    if(used < 15){
      printf("✓ COW IS WORKING (used %d << %d)\n", used, npages);
    } else {
      printf("✗ COW NOT WORKING (used %d, close to %d)\n", used, npages);
    }

    for(int i = 0; i < npages * PGSIZE; i += PGSIZE){
        printf("%c",mem[i]);
    }
    printf("\n");
    used = freepages() - free_after;
    printf("Allocated %d pages on read\n", used);

    for(int i = 0; i < npages * PGSIZE; i += PGSIZE){
        mem[i]='Y';
        printf("%c",mem[i]);
    }
    printf("\n");
    used = free_after - freepages();
    printf("Allocated %d pages on read\n", used);

    exit(0);
  }
  wait(0);
}

int
main(int argc, char *argv[])
{
  printf("========================================\n");
  printf("  COW Fork Test \n");
  printf("========================================\n");
  
  test_cow_vs_nocow();

  printf("\n========================================\n");
  printf("  All tests completed!\n");
  printf("========================================\n");
  
  exit(0);
}