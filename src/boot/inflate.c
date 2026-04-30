#include "common.h"

// raw DEFLATE inflater derived from gzip 1.2.4 inflate.c

typedef struct Huft_t Huft_t;

typedef struct Huft_t {
    /* 0x00 */ u8 extraBits; // number of extra bits or operation
    /* 0x01 */ u8 bitCount; // number of bits in this code or subcode
    union {
    /* 0x04 */ u16 literalValue; // literal, length base, or distance base
    /* 0x04 */ Huft_t* table; // pointer to next-level table
    } value;
} Huft; /* sizeof = 0x08 */

// Order of the bit-length code lengths
u32 bitLengthOrder[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2,
    14, 1, 15,
};

// Copy lengths for literal codes 257->285
u16 literalLengthBases[31] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0,
};

// Extra bits for literal codes 257->285. A value of 99 marks an invalid code
u16 literalLengthExtraBits[31] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99,
};

// Copy offsets for distance codes 0->29
u16 distanceBases[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
};

// Extra bits for distance codes
u16 distanceExtraBits[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
};

// Low-bit masks used after NEEDBITS()
u16 bitMaskTable[17] = {
    0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF,
    0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
};

// First-level lookup width for literal/length and distance tables
u32 literalTableLookupBits = 9;
u32 distanceTableLookupBits = 6;

static u8* sInflateInput;
static u8* sInflateOutput;
static u32 sBitBuffer;
static u32 sBitCount;
static s32 sHuftArenaStart;
static s32 sHuftArenaOffset;
static s32 sHuftArenaSize;
static u8 sUnusedBss_8020199C[0x2004];
static u8 sUnusedBss_802039A0[0x6000];

void huft_arena_init(s32 arena_start, s32 arena_size);
int inflate(void);
int huft_build(unsigned* bit_lengths, unsigned code_count, unsigned simple_count, u16* base_values, u16* extra_bits, Huft** table, int* lookup_bits);
int inflate_codes(Huft* literal_table, Huft* distance_table, int literal_bits, int distance_bits);
int inflate_stored(void);
int inflate_fixed(void);
int inflate_dynamic(void);
int inflate_block(int* last_block);

void huft_arena_init(s32 arena_start, s32 arena_size) {
    sHuftArenaStart = arena_start;
    sHuftArenaSize = arena_size;
    sHuftArenaOffset = 0;
}

s32 huft_alloc(s32 byte_count) {
    s32 new_offset = sHuftArenaOffset + byte_count;
    sHuftArenaOffset = new_offset;
    return sHuftArenaStart + (new_offset - byte_count);
}

#define BMAX 16
#define N_MAX 288

// Build one or more Huffman lookup tables from a set of bit lengths
// Returns zero on success, one for an incomplete set, two for invalid lengths, and three if the fixed boot arena is exhausted
int huft_build(unsigned* bit_lengths, unsigned code_count, unsigned simple_count, u16* base_values, u16* extra_bits, Huft** table, int* lookup_bits) {
    unsigned codes_at_current_length;
    unsigned bit_length_counts[BMAX + 1];
    unsigned table_entry_stride;
    int max_code_length;
    int table_level;
    unsigned code_index;
    unsigned table_index;
    int current_bit_length;
    int root_lookup_bits;
    unsigned* code_ptr;
    Huft* current_table;
    Huft table_entry;
    Huft* table_stack[BMAX];
    unsigned sorted_codes[N_MAX];
    int table_bit_offset;
    unsigned offsets[BMAX + 1];
    unsigned* offset_ptr;
    int remaining_codes;
    unsigned table_size;
    unsigned* clear_ptr;
    int clear_count;

    // Count the number of codes for each bit length
    clear_ptr = bit_length_counts;
    clear_count = BMAX;
    do {
        *clear_ptr++ = 0;
    } while (--clear_count != -1);

    code_ptr = bit_lengths;
    code_index = code_count;
    do {
        bit_length_counts[*code_ptr]++;
        code_ptr++;
    } while (--code_index);

    if (bit_length_counts[0] == code_count) {
        *table = NULL;
        *lookup_bits = 0;
        return 0;
    }

    // Find the minimum and maximum code lengths, then clamp the requested root lookup width to that range
    root_lookup_bits = *lookup_bits;
    for (table_index = 1; table_index <= BMAX; table_index++) {
        if (bit_length_counts[table_index] != 0) {
            break;
        }
    }
    current_bit_length = table_index;
    if ((unsigned)root_lookup_bits < table_index) {
        root_lookup_bits = table_index;
    }

    for (code_index = BMAX; code_index != 0; code_index--) {
        if (bit_length_counts[code_index] != 0) {
            break;
        }
    }
    max_code_length = code_index;
    if ((unsigned)root_lookup_bits > code_index) {
        root_lookup_bits = code_index;
    }
    *lookup_bits = root_lookup_bits;

    // Check for oversubscribed lengths and pad incomplete sets with dummy codes
    for (remaining_codes = 1 << table_index; table_index < code_index; table_index++, remaining_codes <<= 1) {
        if ((remaining_codes -= bit_length_counts[table_index]) < 0) {
            return 2;
        }
    }
    if ((remaining_codes -= bit_length_counts[code_index]) < 0) {
        return 2;
    }
    bit_length_counts[code_index] += remaining_codes;

    offsets[1] = table_index = 0;
    code_ptr = bit_length_counts + 1;
    offset_ptr = offsets + 2;
    while (--code_index) {
        *offset_ptr++ = table_index += *code_ptr++;
    }

    // Sort symbols by bit length
    code_ptr = bit_lengths;
    code_index = 0;
    do {
        if ((table_index = *code_ptr++) != 0) {
            sorted_codes[offsets[table_index]++] = code_index;
        }
    } while (++code_index < code_count);

    offsets[0] = code_index = 0;
    code_ptr = sorted_codes;
    table_level = -1;
    table_bit_offset = -root_lookup_bits;
    table_stack[0] = NULL;
    current_table = NULL;
    table_size = 0;

    for (; current_bit_length <= max_code_length; current_bit_length++) {
        codes_at_current_length = bit_length_counts[current_bit_length];
        while (codes_at_current_length--) {
            while (current_bit_length > table_bit_offset + root_lookup_bits) {
                table_level++;
                table_bit_offset += root_lookup_bits;

                table_size = (table_size = max_code_length - table_bit_offset) > (unsigned)root_lookup_bits ? root_lookup_bits : table_size;
                if ((table_entry_stride = 1 << (table_index = current_bit_length - table_bit_offset)) > codes_at_current_length + 1) {
                    table_entry_stride -= codes_at_current_length + 1;
                    offset_ptr = bit_length_counts + current_bit_length;
                    while (++table_index < table_size) {
                        if ((table_entry_stride <<= 1) <= *++offset_ptr) {
                            break;
                        }
                        table_entry_stride -= *offset_ptr;
                    }
                }
                table_size = 1 << table_index;

                current_table = (Huft*)huft_alloc((table_size + 1) * sizeof(Huft));
                if (current_table == NULL) {
                    return 3;
                }

                *table = current_table + 1;
                *(table = &(current_table->value.table)) = NULL;
                table_stack[table_level] = ++current_table;

                if (table_level != 0) {
                    offsets[table_level] = code_index;
                    table_entry.bitCount = root_lookup_bits;
                    table_entry.extraBits = 16 + table_index;
                    table_entry.value.table = current_table;
                    table_index = code_index >> (table_bit_offset - root_lookup_bits);
                    table_stack[table_level - 1][table_index] = table_entry;
                }
            }

            table_entry.bitCount = current_bit_length - table_bit_offset;
            if (code_ptr >= sorted_codes + code_count) {
                table_entry.extraBits = 99;
            }
            else if (*code_ptr < simple_count) {
                table_entry.extraBits = *code_ptr < 256 ? 16 : 15;
                table_entry.value.literalValue = *code_ptr;
                code_ptr++;
            }
            else {
                table_entry.extraBits = extra_bits[*code_ptr - simple_count];
                table_entry.value.literalValue = base_values[*code_ptr++ - simple_count];
            }

            table_entry_stride = 1 << (current_bit_length - table_bit_offset);
            for (table_index = code_index >> table_bit_offset; table_index < table_size; table_index += table_entry_stride) {
                current_table[table_index] = table_entry;
            }

            for (table_index = 1 << (current_bit_length - 1); code_index & table_index; table_index >>= 1) {
                code_index ^= table_index;
            }
            code_index ^= table_index;

            while ((code_index & ((1 << table_bit_offset) - 1)) != offsets[table_level]) {
                table_level--;
                table_bit_offset -= root_lookup_bits;
            }
        }
    }

    return remaining_codes != 0 && max_code_length != 1;
}

// The bit macros use local variables named bit_buffer and bit_count
// NEEDBITS pulls only enough bytes to satisfy the request, while DUMPBITS consumes the low bits that were just decoded
#define NEXTBYTE() (*sInflateInput++)
#define NEEDBITS(needed_bits)                     \
    while (bit_count < (unsigned)(needed_bits)) { \
        bit_buffer |= (unsigned long)NEXTBYTE() << bit_count; \
        bit_count += 8;                 \
    }
#define DUMPBITS(consumed_bits)      \
    bit_buffer >>= (consumed_bits);  \
    bit_count -= (consumed_bits)

// Decode literal/length and distance codes until an end-of-block code is hit
int inflate_codes(Huft* literal_table, Huft* distance_table, int literal_bits, int distance_bits) {
    unsigned entry_extra_bits;
    unsigned copy_length;
    unsigned copy_distance;
    unsigned copy_source_address;
    Huft* table_entry;
    unsigned literal_mask;
    unsigned distance_mask;
    unsigned long bit_buffer;
    unsigned bit_count;

    bit_buffer = sBitBuffer;
    bit_count = sBitCount;
    literal_mask = bitMaskTable[literal_bits];
    distance_mask = bitMaskTable[distance_bits];

    while (1) {
        NEEDBITS((unsigned)literal_bits);
        if ((entry_extra_bits = (table_entry = literal_table + (bit_buffer & literal_mask))->extraBits) > 16) {
            do {
                if (entry_extra_bits == 99) {
                    return 1;
                }
                DUMPBITS(table_entry->bitCount);
                entry_extra_bits -= 16;
                NEEDBITS(entry_extra_bits);
            } while ((entry_extra_bits = (table_entry = table_entry->value.table + (bit_buffer & bitMaskTable[entry_extra_bits]))->extraBits) > 16);
        }

        DUMPBITS(table_entry->bitCount);
        if (entry_extra_bits == 16) {
            *sInflateOutput++ = table_entry->value.literalValue;
        }
        else {
            if (entry_extra_bits == 15) {
                break;
            }

            NEEDBITS(entry_extra_bits);
            copy_length = table_entry->value.literalValue + (bit_buffer & bitMaskTable[entry_extra_bits]);
            DUMPBITS(entry_extra_bits);

            NEEDBITS((unsigned)distance_bits);
            if ((entry_extra_bits = (table_entry = distance_table + (bit_buffer & distance_mask))->extraBits) > 16) {
                do {
                    if (entry_extra_bits == 99) {
                        return 1;
                    }
                    DUMPBITS(table_entry->bitCount);
                    entry_extra_bits -= 16;
                    NEEDBITS(entry_extra_bits);
                } while ((entry_extra_bits = (table_entry = table_entry->value.table + (bit_buffer & bitMaskTable[entry_extra_bits]))->extraBits) > 16);
            }

            DUMPBITS(table_entry->bitCount);
            if (bit_count < entry_extra_bits) {
                do {
                    bit_buffer |= (unsigned long)NEXTBYTE() << bit_count;
                    bit_count += 8;
                } while (bit_count < entry_extra_bits);
                bit_count -= entry_extra_bits;
            }
            else {
                bit_count -= entry_extra_bits;
            }
            copy_distance = bit_buffer & bitMaskTable[entry_extra_bits];
            bit_buffer >>= entry_extra_bits;
            copy_source_address = (u32)sInflateOutput + (-table_entry->value.literalValue - copy_distance);

            while (--copy_length != -1) {
                *sInflateOutput++ = *(u8*)copy_source_address++;
            }
        }
    }

    sBitBuffer = bit_buffer;
    sBitCount = bit_count;
    return 0;
}

// Stored blocks are byte-aligned and contain raw bytes plus a one's-complement length check
int inflate_stored(void) {
    u32 stored_length;
    u32 bit_buffer;
    u32 bit_count;
    u8* output;
    u8* input;

    bit_count = sBitCount;
    bit_buffer = sBitBuffer;
    stored_length = bit_count & 7;
    bit_count -= stored_length;
    bit_buffer >>= stored_length;

    while (bit_count < 16U) {
        input = sInflateInput;
        sInflateInput = input + 1;
        bit_buffer |= *input << bit_count;
        bit_count += 8;
    }
    stored_length = bit_buffer & 0xFFFF;
    bit_count -= 16;
    bit_buffer >>= 16;

    while (bit_count < 16U) {
        input = sInflateInput;
        sInflateInput = input + 1;
        bit_buffer |= *input << bit_count;
        bit_count += 8;
    }
    if (stored_length != (~bit_buffer & 0xFFFF)) {
        return 1;
    }

    bit_buffer >>= 16;
    stored_length--;
    bit_count -= 16;

    while (stored_length != -1U) {
        while (bit_count < 8U) {
            input = sInflateInput;
            sInflateInput = input + 1;
            bit_buffer |= *input << bit_count;
            bit_count += 8;
        }
        output = sInflateOutput;
        bit_count -= 8;
        stored_length--;
        sInflateOutput = output + 1;
        *output = bit_buffer;
        bit_buffer >>= 8;
    }

    sBitBuffer = bit_buffer;
    sBitCount = bit_count;
    return 0;
}

// Decode a block using the fixed literal/length and distance code layout
int inflate_fixed(void) {
    int result_or_index;
    Huft* literal_table;
    Huft* distance_table;
    int literal_bits;
    int distance_bits;
    unsigned bit_lengths[288];

    // Set up literal table: 0->143 use 8 bits, 144->255 use 9 bits, 256->279 use 7 bits, and 280->287 complete the tree with 8 bits
    for (result_or_index = 0; result_or_index < 144; result_or_index++) {
        bit_lengths[result_or_index] = 8;
    }
    for (; result_or_index < 256; result_or_index++) {
        bit_lengths[result_or_index] = 9;
    }
    for (; result_or_index < 280; result_or_index++) {
        bit_lengths[result_or_index] = 7;
    }
    for (; result_or_index < 288; result_or_index++) {
        bit_lengths[result_or_index] = 8;
    }

    literal_bits = 7;
    if ((result_or_index = huft_build(bit_lengths, 288, 257, literalLengthBases, literalLengthExtraBits, &literal_table, &literal_bits)) != 0) {
        return result_or_index;
    }

    // Set up an incomplete distance table
    for (result_or_index = 0; result_or_index < 30; result_or_index++) {
        bit_lengths[result_or_index] = 5;
    }

    distance_bits = 5;
    if ((result_or_index = huft_build(bit_lengths, 30, 0, distanceBases, distanceExtraBits, &distance_table, &distance_bits)) > 1) {
        sHuftArenaOffset = 0;
        return result_or_index;
    }

    result_or_index = inflate_codes(literal_table, distance_table, literal_bits, distance_bits);
    if (result_or_index != 0) {
        return result_or_index >= 0 ? 1 : result_or_index;
    }

    sHuftArenaOffset = 0;
    return 0;
}

// Decode a block with dynamic Huffman tables encoded in the stream
int inflate_dynamic(void) {
    int result_or_index;
    unsigned code_index;
    unsigned previous_length;
    unsigned bit_length_mask;
    unsigned total_lengths;
    Huft* literal_table;
    Huft* distance_table;
    int literal_bits;
    int distance_bits;
    unsigned bit_length_count;
    unsigned literal_count;
    unsigned distance_count;
    unsigned bit_lengths[286 + 30];
    unsigned long bit_buffer;
    unsigned bit_count;

    bit_buffer = sBitBuffer;
    bit_count = sBitCount;

    NEEDBITS(5);
    literal_count = 257 + (bit_buffer & 0x1F);
    DUMPBITS(5);
    NEEDBITS(5);
    distance_count = 1 + (bit_buffer & 0x1F);
    DUMPBITS(5);
    NEEDBITS(4);
    bit_length_count = 4 + (bit_buffer & 0xF);
    DUMPBITS(4);

    if (literal_count > 286 || distance_count > 30) {
        goto invalid_block;
    }

    // Read the code-length-code lengths in the prescribed order
    for (code_index = 0; code_index < bit_length_count; code_index++) {
        NEEDBITS(3);
        bit_lengths[bitLengthOrder[code_index]] = bit_buffer & 7;
        DUMPBITS(3);
    }
    for (; code_index < 19; code_index++) {
        bit_lengths[bitLengthOrder[code_index]] = 0;
    }

    literal_bits = 7;
    if ((result_or_index = huft_build(bit_lengths, 19, 19, NULL, NULL, &literal_table, &literal_bits)) != 0) {
        if (result_or_index == 1) {
            sHuftArenaOffset = 0;
        }
        return result_or_index;
    }

    // Use that table to read literal/length and distance code lengths
    total_lengths = literal_count + distance_count;
    bit_length_mask = bitMaskTable[literal_bits];
    result_or_index = previous_length = 0;
    while ((unsigned)result_or_index < total_lengths) {
        NEEDBITS(literal_bits);
        distance_table = literal_table + (bit_buffer & bit_length_mask);
        code_index = distance_table->bitCount;
        DUMPBITS(code_index);
        code_index = distance_table->value.literalValue;
        if (code_index < 16) {
            bit_lengths[result_or_index++] = previous_length = code_index;
        }
        else if (code_index == 16) {
            NEEDBITS(2);
            code_index = 3 + (bit_buffer & 3);
            DUMPBITS(2);
            if ((unsigned)result_or_index + code_index > total_lengths) {
                goto invalid_block;
            }
            while (code_index--) {
                bit_lengths[result_or_index++] = previous_length;
            }
        }
        else if (code_index == 17) {
            NEEDBITS(3);
            code_index = 3 + (bit_buffer & 7);
            DUMPBITS(3);
            if ((unsigned)result_or_index + code_index > total_lengths) {
                goto invalid_block;
            }
            while (code_index--) {
                bit_lengths[result_or_index++] = 0;
            }
            previous_length = 0;
        }
        else {
            NEEDBITS(7);
            code_index = 11 + (bit_buffer & 0x7F);
            DUMPBITS(7);
            if ((unsigned)result_or_index + code_index > total_lengths) {
                goto invalid_block;
            }
            while (code_index--) {
                bit_lengths[result_or_index++] = 0;
            }
            previous_length = 0;
        }
    }

    sHuftArenaOffset = 0;
    sBitBuffer = bit_buffer;
    sBitCount = bit_count;

    // Build the real literal/length and distance decode tables
    literal_bits = literalTableLookupBits;
    if ((result_or_index = huft_build(bit_lengths, literal_count, 257, literalLengthBases, literalLengthExtraBits, &literal_table, &literal_bits)) != 0) {
        sHuftArenaOffset = 0;
        return result_or_index;
    }

    distance_bits = distanceTableLookupBits;
    if ((result_or_index = huft_build(bit_lengths + literal_count, distance_count, 0, distanceBases, distanceExtraBits, &distance_table, &distance_bits)) != 0) {
        sHuftArenaOffset = 0;
        return result_or_index;
    }

    result_or_index = inflate_codes(literal_table, distance_table, literal_bits, distance_bits);
    if (result_or_index != 0) {
        return result_or_index < 0 ? result_or_index : 1;
    }
    goto complete_block;

invalid_block:
    return 1;

complete_block:
    sHuftArenaOffset = 0;
    return 0;
}

// Read a deflate block header and dispatch to the matching block decoder
int inflate_block(int* last_block) {
    u32 block_type;
    u32 bit_buffer;
    u32 bit_count;
    u8* input;

    bit_count = sBitCount;
    bit_buffer = sBitBuffer;

    if (bit_count == 0) {
        do {
            input = sInflateInput;
            sInflateInput = input + 1;
            bit_buffer |= *input << bit_count;
            bit_count += 8;
        } while (bit_count == 0);
    }

    *last_block = bit_buffer & 1;
    bit_count--;
    bit_buffer >>= 1;

    if (bit_count < 2U) {
        do {
            input = sInflateInput;
            sInflateInput = input + 1;
            bit_buffer |= *input << bit_count;
            bit_count += 8;
        } while (bit_count < 2U);
    }

    block_type = bit_buffer & 3;
    bit_buffer >>= 2;
    bit_count -= 2;

    sBitBuffer = bit_buffer;
    sBitCount = bit_count;

    if (block_type == 2) {
        return inflate_dynamic();
    }

    if (block_type == 0) {
        return inflate_stored();
    }

    if (block_type == 1) {
        return inflate_fixed();
    }

    return 2;
}

// Decompress deflate blocks until the final-block bit is seen
int inflate(void) {
    int last_block;
    int result;

    sBitCount = 0;
    sBitBuffer = 0;

    do {
        result = inflate_block(&last_block);
        if (result != 0) {
            return result;
        }
    } while (last_block == 0);

    return 0;
}

u32 inflate_decompress(u8* input, u8* output) {
    sInflateOutput = output;
    sInflateInput = input;
    huft_arena_init(0x80220000, 0x5000);
    inflate();
    return (u32)sInflateInput - (u32)input;
}
