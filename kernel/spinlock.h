// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

extern char end[];
struct refcount {
  struct spinlock lk;
  uint count[128*1024*1024 / 4096];
};