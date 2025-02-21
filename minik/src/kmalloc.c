#include <minik/kmalloc.h>

// PUBLIC API:
void *kmalloc(u64 size) {
  u8 bucket_id = BUCKET_ID_FROM_SIZE(size);
  u64 bucket_size = BUCKET_SIZE_FROM_ID(bucket_id);
  assert(HEAP_SIZE - (brk - wilderness) >= bucket_size &&
         "kmalloc: out-of-memory");
  chunk_hdr *hdr = (chunk_hdr *)(brk);
  brk += bucket_size;
  hdr->bucket_id = bucket_id;
  return (void *)(hdr + 1);
}
void kfree(void *ptr) {
  chunk_hdr *hdr = CHUNK_HDR_FROM_CHUNK(ptr);
  assert(hdr->bucket_id < BUCKET_COUNT);
  free_chunk *bucket_head = buckets[hdr->bucket_id];

  free_chunk *chunk = (free_chunk *)ptr;
  chunk->next = bucket_head;
  buckets[hdr->bucket_id] = chunk;
}

// PRIVATE API:
free_chunk *free_chunk_from_bucket(u8 bucket_id) {
  assert(bucket_id < BUCKET_COUNT);
  free_chunk *bucket_head = buckets[bucket_id];
  if (bucket_head == nullptr)
    return bucket_head;
  buckets[bucket_id] = bucket_head->next;
  return bucket_head;
}
