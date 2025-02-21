#include <minik/fifo.h>

InplaceFIFO *fifo_new(char *data, int capacity) {
  assert(capacity > 0);
  InplaceFIFO *fifo = (InplaceFIFO *)kmalloc(sizeof(InplaceFIFO));
  fifo->data = data;
  fifo->cap = capacity;
  fifo->meta.c.head = 0;
  fifo->meta.c.tail = 0;
  fifo->meta.c.full = false;
  return fifo;
}
void fifo_free(InplaceFIFO *fifo) { kfree(fifo); }
bool fifo_try_enqueue(InplaceFIFO *fifo, char value) {
  InplaceFIFOMeta old_meta, meta;
  fifo_meta_peek(fifo, &old_meta);
  meta.value = old_meta.value;
  if (fifo_meta_is_full(&meta, fifo->cap))
    return false;
  char *src = &fifo->data[fifo_meta_inc_tail(&meta, fifo->cap)];
  char old_value = *src;
  if (!fifo_meta_commit(fifo, &old_meta, &meta)) {
    return false;
  }
  // only swap if old_value has not changed
  return compare_and_swap8((u8 *)src, old_value, value);
}
bool fifo_take_first(InplaceFIFO *fifo, char *dest) {
  InplaceFIFOMeta old_meta, meta;
  fifo_meta_peek(fifo, &old_meta);
  meta.value = old_meta.value;
  if (fifo_meta_is_empty(&meta, fifo->cap))
    return false;
  char c = fifo->data[fifo_meta_inc_head(&meta, fifo->cap)];
  if (!fifo_meta_commit(fifo, &old_meta, &meta)) {
    return false;
  }
  *dest = c;
  return true;
}
bool fifo_clear(InplaceFIFO *fifo) {
  InplaceFIFOMeta old_meta, meta;
  fifo_meta_peek(fifo, &old_meta);
  meta.value = old_meta.value;
  fifo_meta_clear(&meta);
  return fifo_meta_commit(fifo, &old_meta, &meta);
}
u64 fifo_size(InplaceFIFO *fifo) {
  InplaceFIFOMeta meta;
  fifo_meta_peek(fifo, &meta);
  return fifo_meta_size(&meta, fifo->cap);
}
bool fifo_is_empty(InplaceFIFO *fifo) {
  InplaceFIFOMeta meta;
  fifo_meta_peek(fifo, &meta);
  return fifo_meta_is_empty(&meta, fifo->cap);
}
bool fifo_is_full(InplaceFIFO *fifo) {
  InplaceFIFOMeta meta;
  fifo_meta_peek(fifo, &meta);
  return fifo_meta_is_full(&meta, fifo->cap);
}
u64 fifo_capacity_left(InplaceFIFO *fifo) {
  InplaceFIFOMeta meta;
  fifo_meta_peek(fifo, &meta);
  return fifo_meta_capacity_left(&meta, fifo->cap);
}

// Private API:

void fifo_meta_peek(InplaceFIFO *fifo, InplaceFIFOMeta *meta) {
  meta->value = atomic_read64(&fifo->meta.value);
}
u8 fifo_meta_commit(InplaceFIFO *fifo, InplaceFIFOMeta *old_meta,
                    InplaceFIFOMeta *new_meta) {
  return compare_and_swap64(&fifo->meta.value, old_meta->value,
                            new_meta->value);
}
void fifo_meta_clear(InplaceFIFOMeta *meta) {
  meta->c.head = meta->c.tail = 0;
  meta->c.full = false;
};
u64 fifo_meta_size(InplaceFIFOMeta *meta, int cap) {
  i64 diff = (i64)(meta->c.tail) - meta->c.head;
  return (((diff % cap) + cap) % cap) + cap * meta->c.full;
}
bool fifo_meta_is_empty(InplaceFIFOMeta *meta, int cap) {
  return fifo_meta_size(meta, cap) == 0;
};
bool fifo_meta_is_full(InplaceFIFOMeta *meta, int cap) {
  return fifo_meta_size(meta, cap) == cap;
};
u64 fifo_meta_capacity_left(InplaceFIFOMeta *meta, int cap) {
  return cap - fifo_meta_size(meta, cap);
};
u64 fifo_meta_inc_head(InplaceFIFOMeta *meta, int cap) {
  auto temp = meta->c.head;
  meta->c.head = (meta->c.head + 1) % cap;
  meta->c.full = false;
  return temp;
}
u64 fifo_meta_inc_tail(InplaceFIFOMeta *meta, int cap) {
  auto temp = meta->c.tail;
  meta->c.tail = (meta->c.tail + 1) % cap;
  meta->c.full = meta->c.tail == meta->c.head;
  return temp;
}
