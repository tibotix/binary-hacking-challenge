#ifndef KMALLOC_H
#define KMALLOC_H

#include "math.h"
#include "stl.h"

#define HEAP_SIZE 0x10000
#define BUCKET_COUNT 8
#define BUCKET_SIZE_FROM_ID(id) ((0x20 << id) + sizeof(chunk_hdr))
#define BUCKET_ID_FROM_SIZE(size) (log2(align_to_next_pow2(size)) - 5)
#define CHUNK_HDR_FROM_CHUNK(chunk) ((chunk_hdr *)(chunk)-1)

typedef struct __attribute__((aligned(8))) {
  u64 bucket_id;
} chunk_hdr;

typedef struct free_chunk free_chunk;
struct free_chunk {
  free_chunk *next;
};

static u8 wilderness[HEAP_SIZE];
static u8 *brk = wilderness + sizeof(chunk_hdr);
static free_chunk *buckets[BUCKET_COUNT] = {nullptr};

// PUBLIC API:
void *kmalloc(u64 size);
void kfree(void *ptr);

// PRIVATE API:
free_chunk *free_chunk_from_bucket(u8 bucket_id);

#endif // KMALLOC_H
