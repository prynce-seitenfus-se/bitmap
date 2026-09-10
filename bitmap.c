#include "bitmap.h"

/* Helper to compute the valid bitmask for the final storage word */
static BitmapWord get_last_word_mask(const Bitmap* bm)
{
    size_t remainder = bm->bit_count & BITMAP_WORD_MASK;
    if (remainder == 0U) {
        return 0xFFFFFFFFU;
    }
    return ((BitmapWord)1U << remainder) - 1U;
}

/* Hardware-accelerated or portable Count Trailing Zeros (CTZ) */
static uint32_t count_trailing_zeros(uint32_t word)
{
    if (word == 0U) {
        return 32U;
    }

#if defined(__GNUC__) && (__GNUC__ >= 4)
    return (uint32_t)__builtin_ctz(word);
#else
    uint32_t count = 0U;
    uint32_t val = word;
    if ((val & 0x0000FFFFU) == 0U) { count += 16U; val >>= 16U; }
    if ((val & 0x000000FFU) == 0U) { count += 8U;  val >>= 8U;  }
    if ((val & 0x0000000FU) == 0U) { count += 4U;  val >>= 4U;  }
    if ((val & 0x00000003U) == 0U) { count += 2U;  val >>= 2U;  }
    if ((val & 0x00000001U) == 0U) { count += 1U; }
    return count;
#endif
}

/* Hardware-accelerated or portable Count Leading Zeros (CLZ) */
static uint32_t count_leading_zeros(uint32_t word)
{
    if (word == 0U) {
        return 32U;
    }

#if defined(__GNUC__) && (__GNUC__ >= 4)
    return (uint32_t)__builtin_clz(word);
#else
    uint32_t count = 0U;
    uint32_t val = word;
    if ((val & 0xFFFF0000U) == 0U) { count += 16U; val <<= 16U; }
    if ((val & 0xFF000000U) == 0U) { count += 8U;  val <<= 8U;  }
    if ((val & 0xF0000000U) == 0U) { count += 4U;  val <<= 4U;  }
    if ((val & 0xC0000000U) == 0U) { count += 2U;  val <<= 2U;  }
    if ((val & 0x80000000U) == 0U) { count += 1U; }
    return count;
#endif
}

/* Hardware-accelerated or portable population count (Hamming weight) */
static size_t popcount_word(uint32_t word)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
    return (size_t)__builtin_popcount(word);
#else
    uint32_t v = word;
    v = v - ((v >> 1U) & 0x55555555U);
    v = (v & 0x33333333U) + ((v >> 2U) & 0x33333333U);
    v = (v + (v >> 4U)) & 0x0F0F0F0FU;
    return (size_t)((v * 0x01010101U) >> 24U);
#endif
}

/* Unified forward scanner for finding set or cleared bits */
static bool scan_forward(const Bitmap* bm, size_t* out_bit_index, bool find_zero)
{
    if ((bm == NULL) || (out_bit_index == NULL)) {
        return false;
    }

    BitmapWord xor_mask = find_zero ? 0xFFFFFFFFU : 0U;
    size_t last_idx = bm->word_count - 1U;

    for (size_t i = 0U; i < last_idx; ++i) {
        BitmapWord val = bm->words[i] ^ xor_mask;
        if (val != 0U) {
            *out_bit_index = (i << BITMAP_WORD_SHIFT) + (size_t)count_trailing_zeros(val);
            return true;
        }
    }

    BitmapWord last_val = (bm->words[last_idx] ^ xor_mask) & get_last_word_mask(bm);
    if (last_val != 0U) {
        *out_bit_index = (last_idx << BITMAP_WORD_SHIFT) + (size_t)count_trailing_zeros(last_val);
        return true;
    }

    return false;
}

/* Unified backward scanner for finding set or cleared bits */
static bool scan_backward(const Bitmap* bm, size_t* out_bit_index, bool find_zero)
{
    if ((bm == NULL) || (out_bit_index == NULL)) {
        return false;
    }

    BitmapWord xor_mask = find_zero ? 0xFFFFFFFFU : 0U;
    size_t last_idx = bm->word_count - 1U;

    BitmapWord last_val = (bm->words[last_idx] ^ xor_mask) & get_last_word_mask(bm);
    if (last_val != 0U) {
        uint32_t clz = count_leading_zeros(last_val);
        *out_bit_index = (last_idx << BITMAP_WORD_SHIFT) + (size_t)(31U - clz);
        return true;
    }

    size_t i = last_idx;
    while (i > 0U) {
        i--;
        BitmapWord val = bm->words[i] ^ xor_mask;
        if (val != 0U) {
            uint32_t clz = count_leading_zeros(val);
            *out_bit_index = (i << BITMAP_WORD_SHIFT) + (size_t)(31U - clz);
            return true;
        }
    }

    return false;
}

bool bitmap_init(Bitmap* bm, BitmapWord* storage, size_t bit_count)
{
    if ((bm == NULL) || (storage == NULL) || (bit_count == 0U)) {
        return false;
    }

    bm->words = storage;
    bm->bit_count = bit_count;
    bm->word_count = BITMAP_BITS_TO_WORDS(bit_count);

    bitmap_clear_all(bm);

    return true;
}

void bitmap_set_all(Bitmap* bm)
{
    if (bm == NULL) {
        return;
    }

    for (size_t i = 0U; i < bm->word_count; ++i) {
        bm->words[i] = 0xFFFFFFFFU;
    }

    bm->words[bm->word_count - 1U] &= get_last_word_mask(bm);
}

void bitmap_clear_all(Bitmap* bm)
{
    if (bm == NULL) {
        return;
    }

    for (size_t i = 0U; i < bm->word_count; ++i) {
        bm->words[i] = 0U;
    }
}

bool bitmap_is_empty(const Bitmap* bm)
{
    size_t dummy = 0U;
    return (bm == NULL) || (!scan_forward(bm, &dummy, false));
}

bool bitmap_is_full(const Bitmap* bm)
{
    size_t dummy = 0U;
    return (bm != NULL) && (!scan_forward(bm, &dummy, true));
}

size_t bitmap_count_set(const Bitmap* bm)
{
    if (bm == NULL) {
        return 0U;
    }

    size_t total = 0U;
    size_t last_idx = bm->word_count - 1U;
    for (size_t i = 0U; i < last_idx; ++i) {
        total += popcount_word(bm->words[i]);
    }

    total += popcount_word(bm->words[last_idx] & get_last_word_mask(bm));
    return total;
}

bool bitmap_find_first_set(const Bitmap* bm, size_t* out_bit_index)
{
    return scan_forward(bm, out_bit_index, false);
}

bool bitmap_find_last_set(const Bitmap* bm, size_t* out_bit_index)
{
    return scan_backward(bm, out_bit_index, false);
}

bool bitmap_find_first_zero(const Bitmap* bm, size_t* out_bit_index)
{
    return scan_forward(bm, out_bit_index, true);
}

bool bitmap_find_last_zero(const Bitmap* bm, size_t* out_bit_index)
{
    return scan_backward(bm, out_bit_index, true);
}
