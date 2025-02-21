#ifndef FIFO_H
#define FIFO_H

#include "atomic.h"
#include "kmalloc.h"
#include "stl.h"

/*
 * This is a lockless atomic FIFO.
 * It uses the compare-and-swap primitive to make transactional commits to the
 * FIFO metadata, thus ensuring it's integrity even in multi-threaded
 * environments.
 */

typedef union __attribute__((packed, aligned(8))) {
  struct {
    u16 head;
    u16 tail;
    bool full;
  } c;
  u64 value;
} InplaceFIFOMeta;

typedef struct {
  char *data;
  int cap;
  InplaceFIFOMeta meta;
} InplaceFIFO;

// Public API:
InplaceFIFO *fifo_new(char *data, int capacity);
void fifo_free(InplaceFIFO *fifo);
bool fifo_try_enqueue(InplaceFIFO *fifo, char value);
bool fifo_take_first(InplaceFIFO *fifo, char *dest);
bool fifo_clear(InplaceFIFO *);
u64 fifo_size(InplaceFIFO *);
bool fifo_is_empty(InplaceFIFO *);
bool fifo_is_full(InplaceFIFO *);
u64 fifo_capacity_left(InplaceFIFO *);

// Private API:
// InplaceFIFOMeta fifo_meta_peek(InplaceFIFO*);
void fifo_meta_peek(InplaceFIFO *, InplaceFIFOMeta *);
/**
 * Commits the [new_meta] to [fifo], if [fifo]->meta == [old_meta].
 * Return true if the commit was successful, false otherwise.
 */
bool fifo_meta_commit(InplaceFIFO *fifo, InplaceFIFOMeta *old_meta,
                      InplaceFIFOMeta *new_meta);
void fifo_meta_clear(InplaceFIFOMeta *);
u64 fifo_meta_size(InplaceFIFOMeta *, int);
bool fifo_meta_is_empty(InplaceFIFOMeta *, int);
bool fifo_meta_is_full(InplaceFIFOMeta *, int);
u64 fifo_meta_capacity_left(InplaceFIFOMeta *, int);
u64 fifo_meta_inc_head(InplaceFIFOMeta *, int);
u64 fifo_meta_inc_tail(InplaceFIFOMeta *, int);

#endif // FIFO_H
