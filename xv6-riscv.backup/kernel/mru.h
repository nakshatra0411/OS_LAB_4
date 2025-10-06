// kernel/mru.h
struct mru_entry {
  struct mru_entry *prev, *next;
  int pid;          // owner process id
  uint64 vpn;       // virtual page number (VA / PGSIZE)
  char *kaddr;      // kernel virtual address of the physical page (page frame)
};
void mru_init(void);
void mru_touch(int pid, uint64 va);    // mark page as MRU
void mru_add(struct mru_entry *e);
void mru_remove(struct mru_entry *e);
struct mru_entry* mru_evict(void);    // removes and returns MRU (head)
