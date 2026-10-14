// part.c
// -----------------------
// Handle open() and close()
#include "part.h"
#include "../../mmu/kmalloc.h"
#define PART_WORDS ((PART_MAX_OPEN + 31u) / 32u)
#define PART_TAIL_BITS (PART_MAX_OPEN % 32u)

static uint32_t part_bitmap[PART_WORDS];
static uint32_t part_next;

static inline uint32_t part_valid_mask(uint32_t w) {
  if (PART_TAIL_BITS && w == PART_WORDS - 1)
    return (1u << PART_TAIL_BITS) - 1u;
  return ~0u;
}

static uint32_t part_gen_handle_id(void) {
  uint32_t start_w = part_next >> 5;
  uint32_t start_b = part_next & 31u;

  for (uint32_t k = 0; k <= PART_WORDS; k++) {
    uint32_t w = start_w + k;
    if (w >= PART_WORDS)
      w -= PART_WORDS;

    uint32_t freebits = ~part_bitmap[w] & part_valid_mask(w);

    if (k == 0)
      freebits &= (~0u << start_b);
    else if (k == PART_WORDS)
      freebits &= (1u << start_b) - 1u;

    if (freebits) {
      uint32_t b = (uint32_t)__builtin_ctz(freebits);
      uint32_t idx = (w << 5) + b;

      part_bitmap[w] |= (1u << b);
      part_next = (idx + 1 >= PART_MAX_OPEN) ? 0 : idx + 1;
      return idx + 1;
    }
  }
  return 0;
}

static void part_dismember_handle_id(uint32_t id) {
  if (id == 0 || id > PART_MAX_OPEN)
    return;
  id--;
  part_bitmap[id >> 5] &= ~(1u << (id & 31u));
}

// returns if failed
int kopen(almond_filehandle_t *out, const char *path, const char *mode) {
  out->id = part_gen_handle_id();

  return 0;
}

int kclose(almond_filehandle_t *f) {
  part_dismember_handle_id(f->id);
  return 0;
}

int kread(almond_filehandle_t *fh, void *buf, uint32_t count) {
  return 0;
}
int kwrite(almond_filehandle_t *fh, const void *buf, uint32_t count) {
  return 0;
}

int kdirent_read(almond_filehandle_t *f, const int n, const uint8_t *to) {
  return 0;
}

void part_init(void) {}
