# Bitmap

A high-performance, zero-allocation bit array and bit-search library in strict C99, engineered for real-time operating system (RTOS) priority scheduling, event flag synchronization, and embedded resource tracking.

---

## Key Features

* **Deterministic $O(1)$ Priority Lookup**: Features hardware-accelerated `bitmap_find_last_set` (using `CLZ` / Count Leading Zeros) and `bitmap_find_first_set` (using `CTZ` / Count Trailing Zeros) to find active priority levels in constant time.
* **Unified Branch-Free Scanners**: Set/zero forward and backward search algorithms are unified using branchless bit-inversion masks (`val ^ xor_mask`), minimizing flash footprint and branch mispredictions.
* **Zero Dynamic Allocation**: Operates entirely over a caller-allocated `BitmapWord` (`uint32_t`) storage array. Fully complies with MISRA C:2012 Rule 21.3.
* **Inlined Fast Bit Operations**: `set`, `clear`, `toggle`, `test`, and `bit_count` are implemented as `static inline` functions in [`bitmap.h`](bitmap.h) for zero function-call overhead.
* **Arbitrary Bit Capacities**: Supports any bit width—from single-word 32-bit ready sets to multi-word bit arrays (e.g. 64, 256, 1024 bits)—with automatic masking of non-aligned trailing bits in the final word.
* **Hardware-Agnostic with GCC Built-Ins**: Utilizes compiler intrinsics (`__builtin_clz`, `__builtin_ctz`, `__builtin_popcount`) that map directly to single-cycle assembly instructions on ARM Cortex-M (`CLZ`, `RBIT`) and RISC-V, while including branchless portable C fallbacks.
* **Defensive Parameter Validation**: Strictly validates pointers against `NULL` and checks bit indices against the configured boundary.
* **MISRA C:2012 Compliant**: Adheres to MISRA C:2012 guidelines (C-style block comments exclusively, explicit typing, explicit integer conversions).

---

## Data Structures

```c
typedef uint32_t BitmapWord;

#define BITMAP_WORD_BITS (32U)
#define BITMAP_BITS_TO_WORDS(bits) (((bits) + (BITMAP_WORD_BITS - 1U)) / BITMAP_WORD_BITS)

typedef struct Bitmap {
    BitmapWord* words;      /**< Pointer to caller-allocated word storage buffer. */
    size_t bit_count;       /**< Total number of manageable bits. */
    size_t word_count;      /**< Number of BitmapWord elements in storage. */
} Bitmap;
```

---

## API Reference

Declared in [`bitmap.h`](bitmap.h):

```c
/* Initialization & Inspection */
bool bitmap_init(Bitmap* bm, BitmapWord* storage, size_t bit_count);
bool bitmap_is_empty(const Bitmap* bm);
bool bitmap_is_full(const Bitmap* bm);
size_t bitmap_count_set(const Bitmap* bm);
static inline size_t bitmap_bit_count(const Bitmap* bm);

/* Inlined Single-Bit Manipulation (Zero Call Overhead) */
static inline bool bitmap_set_bit(Bitmap* bm, size_t bit_index);
static inline bool bitmap_clear_bit(Bitmap* bm, size_t bit_index);
static inline bool bitmap_toggle_bit(Bitmap* bm, size_t bit_index);
static inline bool bitmap_test_bit(const Bitmap* bm, size_t bit_index);

/* Bulk Operations */
void bitmap_set_all(Bitmap* bm);
void bitmap_clear_all(Bitmap* bm);

/* High-Performance O(1) Search Primitives */
bool bitmap_find_first_set(const Bitmap* bm, size_t* out_bit_index);
bool bitmap_find_last_set(const Bitmap* bm, size_t* out_bit_index);
bool bitmap_find_first_zero(const Bitmap* bm, size_t* out_bit_index);
bool bitmap_find_last_zero(const Bitmap* bm, size_t* out_bit_index);
```

---

## Usage Example: RTOS Preemptive Priority Scheduler

```c
#include "bitmap.h"
#include <stdint.h>
#include <stdio.h>

#define MAX_PRIORITIES (32U)

static Bitmap ready_bitmap;
static BitmapWord ready_storage[BITMAP_BITS_TO_WORDS(MAX_PRIORITIES)];

void scheduler_example(void)
{
    /* Initialize 32-priority ready bitmask */
    bitmap_init(&ready_bitmap, ready_storage, MAX_PRIORITIES);

    /* Mark tasks ready at priority 3 and priority 12 */
    bitmap_set_bit(&ready_bitmap, 3U);
    bitmap_set_bit(&ready_bitmap, 12U);

    /* Find the highest priority ready task in O(1) single-cycle time */
    size_t highest_prio = 0U;
    if (bitmap_find_last_set(&ready_bitmap, &highest_prio)) {
        /* highest_prio will be 12 */
        printf("Scheduling task with highest priority: %zu\n", highest_prio);
    }
}
```

---

## Standards Compliance

* **Language Standard**: ISO/IEC 9899:1999 (C99).
* **MISRA C:2012**: Strictly adheres to all Mandatory and Required rules.
* **Workspace Guidelines**: 100% compliant with [`GEMINI.md`](../GEMINI.md).

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.