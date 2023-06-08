//
// Created by roeey on 6/7/23.
//

#include <cstdint>

#include "VirtualMemory.h"
#include "MemoryConstants.h"
#include "PhysicalMemory.h"
#include "bits.hpp"

/*
 * translate virtual address to physical address
 */
static uint64_t translate(uint64_t virtual_addr);

/*
 * clear table
 */
static void clear_table(uint64_t frame_index);

/*
 *
 */
static bool is_table_empty(uint64_t frame_index);

/*
 * traverse the hierarchical page-tables tree in a DFS-like manner.
 *
 * find a candidate frame for swap.
 *
 * @return: frame index of the frame to swap.
 */
static uint64_t traverse(uint64_t frame_index = 0, uint64_t depth = 0, uint64_t max_frame_idx = 1);

void VMinitialize()
{
    /*
     * TODO: root table may have different size than all the outher tables.
     */
    clear_table(0);
}

int VMread(uint64_t virtualAddress, word_t* value)
{
    uint64_t physical_addr = translate(virtualAddress);

    if (0 == physical_addr) {
        /*
         * error.
         */
        return 0;
    }

    PMread(physical_addr, value);
    return 1;
}

int VMwrite(uint64_t virtualAddress, word_t value)
{
    uint64_t physical_addr = translate(virtualAddress);

    if (0 == physical_addr) {
        /*
         * error.
         */
        return 0;
    }

    PMwrite(physical_addr, value);
    return 1;
}

uint64_t translate(uint64_t virtual_addr)
{
    uint64_t physical_addr = 0;
    /* filled in reverse order, i.e. the physical offset is first & root table index is last. */
    uint64_t indices[TABLES_DEPTH + 1] = { 0 };
    uint64_t start = 0;
    uint64_t end = 0;
    word_t addr = 0;

    for (uint64_t i = 0; i < TABLES_DEPTH; i++) {
        start = i * OFFSET_WIDTH;
        end = (i + 1) * OFFSET_WIDTH;
        indices[i] = extract_bits(virtual_addr, start, end);
    }
    indices[TABLES_DEPTH] = extract_bits(virtual_addr,
                                         TABLES_DEPTH * OFFSET_WIDTH,
                                         VIRTUAL_ADDRESS_WIDTH - 1);

    addr = 0;
    for (uint64_t i = 0; i < TABLES_DEPTH; i++) {
        PMread(addr * PAGE_SIZE + indices[i], &addr);
        if (0 == addr) {
            /*
             * need to swap pages.
             */
        }
    }

    physical_addr = (uint64_t)addr;

    return physical_addr;
}

void clear_table(uint64_t frame_index)
{
    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        PMwrite(frame_index * PAGE_SIZE + i, 0);
    }
}

bool is_table_empty(uint64_t frame_index)
{
    word_t data;

    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        PMread(frame_index * PAGE_SIZE + i, &data);
        if (data != 0) {
            return false;
        }
    }

    return true;
}

uint64_t traverse(uint64_t frame_index, uint64_t depth, uint64_t max_frame_idx)
{
    if ()
}
