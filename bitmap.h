#ifndef BITMAP_H
#define BITMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Fundamental word type backing the bitmap storage.
 */
typedef uint32_t BitmapWord;

/**
 * @brief Bits per BitmapWord storage unit.
 */
#define BITMAP_WORD_BITS          (32U)

/**
 * @brief Bitmask for index calculation within a single word.
 */
#define BITMAP_WORD_MASK          (BITMAP_WORD_BITS - 1U)

/**
 * @brief Bit-shift factor for word index calculation (2^5 = 32).
 */
#define BITMAP_WORD_SHIFT         (5U)

/**
 * @brief Calculates the number of BitmapWord elements required to hold a specified number of bits.
 *
 * @param bits Total number of bits required.
 */
#define BITMAP_BITS_TO_WORDS(bits) \
    (((bits) + (BITMAP_WORD_BITS - 1U)) / BITMAP_WORD_BITS)

/**
 * @brief Bitmap control structure.
 *
 * Zero-allocation container operating over a caller-allocated array of BitmapWord.
 */
typedef struct Bitmap {
    BitmapWord* words;      /**< Pointer to caller-allocated word storage buffer. */
    size_t bit_count;       /**< Total number of manageable bits. */
    size_t word_count;      /**< Number of BitmapWord elements in storage. */
} Bitmap;

/**
 * @brief Initializes the bitmap instance.
 *
 * Validates parameters and clears all bits to zero.
 *
 * @param bm Pointer to the Bitmap structure to initialize.
 * @param storage Pointer to caller-allocated BitmapWord storage array.
 * @param bit_count Total number of bits to manage (must be > 0).
 * @return true if initialization succeeded, false if parameters are invalid.
 */
bool bitmap_init(Bitmap* bm, BitmapWord* storage, size_t bit_count);

/**
 * @brief Sets a specific bit to 1.
 *
 * Inlines directly at the call site for zero function call overhead.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param bit_index 0-based index of the bit to set.
 * @return true if set successfully, false if bm is NULL or bit_index is out of range.
 */
static inline __attribute__((no_instrument_function)) bool bitmap_set_bit(Bitmap* bm, size_t bit_index)
{
    if ((bm == NULL) || (bit_index >= bm->bit_count)) {
        return false;
    }

    bm->words[bit_index >> BITMAP_WORD_SHIFT] |= ((BitmapWord)1U << (bit_index & BITMAP_WORD_MASK));
    return true;
}

/**
 * @brief Clears a specific bit to 0.
 *
 * Inlines directly at the call site for zero function call overhead.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param bit_index 0-based index of the bit to clear.
 * @return true if cleared successfully, false if bm is NULL or bit_index is out of range.
 */
static inline __attribute__((no_instrument_function)) bool bitmap_clear_bit(Bitmap* bm, size_t bit_index)
{
    if ((bm == NULL) || (bit_index >= bm->bit_count)) {
        return false;
    }

    bm->words[bit_index >> BITMAP_WORD_SHIFT] &= ~((BitmapWord)1U << (bit_index & BITMAP_WORD_MASK));
    return true;
}

/**
 * @brief Toggles (inverts) a specific bit.
 *
 * Inlines directly at the call site for zero function call overhead.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param bit_index 0-based index of the bit to toggle.
 * @return true if toggled successfully, false if bm is NULL or bit_index is out of range.
 */
static inline __attribute__((no_instrument_function)) bool bitmap_toggle_bit(Bitmap* bm, size_t bit_index)
{
    if ((bm == NULL) || (bit_index >= bm->bit_count)) {
        return false;
    }

    bm->words[bit_index >> BITMAP_WORD_SHIFT] ^= ((BitmapWord)1U << (bit_index & BITMAP_WORD_MASK));
    return true;
}

/**
 * @brief Tests the value of a specific bit.
 *
 * Inlines directly at the call site for zero function call overhead.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param bit_index 0-based index of the bit to inspect.
 * @return true if bit is set (1), false if bit is 0 or parameters are invalid.
 */
static inline __attribute__((no_instrument_function)) bool bitmap_test_bit(const Bitmap* bm, size_t bit_index)
{
    if ((bm == NULL) || (bit_index >= bm->bit_count)) {
        return false;
    }

    return ((bm->words[bit_index >> BITMAP_WORD_SHIFT] & ((BitmapWord)1U << (bit_index & BITMAP_WORD_MASK))) != 0U);
}

/**
 * @brief Returns the total bit capacity of the bitmap.
 *
 * @param bm Pointer to the Bitmap instance.
 * @return Total manageable bit count, or 0 if bm is NULL.
 */
static inline __attribute__((no_instrument_function)) size_t bitmap_bit_count(const Bitmap* bm)
{
    if (bm == NULL) {
        return 0U;
    }

    return bm->bit_count;
}

/**
 * @brief Sets all valid bits in the bitmap to 1.
 *
 * Out-of-range trailing bits in the last storage word remain cleared to 0.
 *
 * @param bm Pointer to the Bitmap instance.
 */
void bitmap_set_all(Bitmap* bm);

/**
 * @brief Clears all bits in the bitmap to 0.
 *
 * @param bm Pointer to the Bitmap instance.
 */
void bitmap_clear_all(Bitmap* bm);

/**
 * @brief Checks if all manageable bits in the bitmap are 0.
 *
 * @param bm Pointer to the Bitmap instance.
 * @return true if all bits are 0 or bm is NULL, false if any bit is set.
 */
bool bitmap_is_empty(const Bitmap* bm);

/**
 * @brief Checks if all manageable bits in the bitmap are set to 1.
 *
 * @param bm Pointer to the Bitmap instance.
 * @return true if all bits are 1, false if any bit is 0 or bm is NULL.
 */
bool bitmap_is_full(const Bitmap* bm);

/**
 * @brief Counts the total number of set bits (population count / Hamming weight).
 *
 * @param bm Pointer to the Bitmap instance.
 * @return Number of bits set to 1, or 0 if bm is NULL.
 */
size_t bitmap_count_set(const Bitmap* bm);

/**
 * @brief Finds the index of the first (lowest / least significant) bit set to 1.
 *
 * Utilizes hardware-accelerated Count Trailing Zeros (CTZ) where available.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param out_bit_index Pointer to variable where the lowest set bit index is written.
 * @return true if a set bit was found, false if no bits are set or parameters are invalid.
 */
bool bitmap_find_first_set(const Bitmap* bm, size_t* out_bit_index);

/**
 * @brief Finds the index of the last (highest / most significant) bit set to 1.
 *
 * Utilizes hardware-accelerated Count Leading Zeros (CLZ) where available.
 * This function provides O(1) highest-priority task selection in RTOS schedulers.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param out_bit_index Pointer to variable where the highest set bit index is written.
 * @return true if a set bit was found, false if no bits are set or parameters are invalid.
 */
bool bitmap_find_last_set(const Bitmap* bm, size_t* out_bit_index);

/**
 * @brief Finds the index of the first (lowest / least significant) bit that is 0.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param out_bit_index Pointer to variable where the lowest cleared bit index is written.
 * @return true if a zero bit was found, false if all bits are 1 or parameters are invalid.
 */
bool bitmap_find_first_zero(const Bitmap* bm, size_t* out_bit_index);

/**
 * @brief Finds the index of the last (highest / most significant) bit that is 0.
 *
 * @param bm Pointer to the Bitmap instance.
 * @param out_bit_index Pointer to variable where the highest cleared bit index is written.
 * @return true if a zero bit was found, false if all bits are 1 or parameters are invalid.
 */
bool bitmap_find_last_zero(const Bitmap* bm, size_t* out_bit_index);

#endif /* BITMAP_H */
