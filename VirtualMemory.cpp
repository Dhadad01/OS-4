//
// Created by roeey on 6/7/23.
//

#include <cstdint>

#include "MemoryConstants.h"
#include "PhysicalMemory.h"
#include "VirtualMemory.h"
#include "bits.hpp"

#ifndef MAX
#define MAX(a, b) ((a > b) ? a : b)
#endif

#ifndef MIN
#define MIN(a, b) ((a < b) ? a : b)
#endif

#ifndef ABS
#define ABS(x) ((x > 0) ? x : -x)
#endif

enum evict_frame_case
{
  UNDEFINED,
  CASE_1,
  CASE_2,
  CASE_3,
};

/*
 * translate virtual address to physical address
 */
static uint64_t
translate(uint64_t virtual_addr);

/*
 * clear table
 */
static void
clear_table(uint64_t frame_index);

/*
 *
 */
static bool
is_table_empty(uint64_t frame_index);

/*
 * traverse the hierarchical page-tables tree in a DFS-like manner.
 *
 * find a candidate frame for swap.
 *
 * @return: frame index of the frame to swap.
 */
static uint64_t
traverse(uint64_t& max_frame_idx,
         enum evict_frame_case& evict_frame,
         uint64_t& max_cyclic_distance,
         uint64_t frame_index = 0,
         uint64_t depth = 0,
         uint64_t page_num = 0,
         uint64_t current_path = 0);

static uint64_t
cyclic_distance(uint64_t swapped_in, uint64_t page_num);

void
VMinitialize()
{
  /*
   * TODO: root table may have different size than all the outher tables.
   */
  clear_table(0);
}

int
VMread(uint64_t virtualAddress, word_t* value)
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

int
VMwrite(uint64_t virtualAddress, word_t value)
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

uint64_t
translate(uint64_t virtual_addr)
{
  uint64_t physical_addr = 0;
  /* filled in reverse order, i.e. the physical offset is first & root table
   * index is last. */
  uint64_t indices[TABLES_DEPTH + 1] = { 0 };
  uint64_t start = 0;
  uint64_t end = 0;
  word_t addr = 0;
  word_t addr1 = 0;

  for (uint64_t i = 0; i < TABLES_DEPTH; i++) {
    start = i * OFFSET_WIDTH;
    end = (i + 1) * OFFSET_WIDTH;
    indices[i] = extract_bits(virtual_addr, start, end);
  }
  indices[TABLES_DEPTH] = extract_bits(
    virtual_addr, TABLES_DEPTH * OFFSET_WIDTH, VIRTUAL_ADDRESS_WIDTH - 1);

  addr = 0;
  for (uint64_t depth = 0; depth < TABLES_DEPTH; depth++) {
    PMread(addr * PAGE_SIZE + indices[depth], &addr1);
    if (0 == addr1) {
      /*
       * need to swap pages.
       */
      uint64_t max_frame_idx = 0;
      uint64_t max_cyclic_distance = 0;
      uint64_t frame = 0;
      enum evict_frame_case evict_frame = UNDEFINED;
      uint64_t page_num = virtual_addr >> OFFSET_WIDTH;
      frame = traverse(max_frame_idx,
                       evict_frame,
                       max_cyclic_distance,
                       addr,
                       depth,
                       page_num,
                       0);
      if (evict_frame == CASE_1) {
        f1 = frame;
      } else {
        if (max_frame_idx + 1 < NUM_FRAMES) {
          evict_frame = CASE_2;
          f1 = max_frame_idx + 1;
        } else {
          evict_frame = CASE_3;
          f1 = max_cyclic_distance;
        }
      }

      if (depth + 1 != TABLES_DEPTH) {
        clear_table(f1);
      }
      PMwrite(addr * PAGE_SIZE + indices[depth], f1);
      addr1 = f1;
    }
    addr = addr1;
  }

  physical_addr = (uint64_t)addr;

  return physical_addr;
}

void
clear_table(uint64_t frame_index)
{
  for (uint64_t i = 0; i < PAGE_SIZE; i++) {
    PMwrite(frame_index * PAGE_SIZE + i, 0);
  }
}

bool
is_table_empty(uint64_t frame_index)
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

uint64_t
traverse(uint64_t& max_frame_idx,
         enum evict_frame_case& evict_frame,
         uint64_t& max_cyclic_distance,
         uint64_t frame_index,
         uint64_t depth,
         uint64_t swapped_in,
         uint64_t current_path)
{
  max_frame_idx = MAX(frame_index, max_frame_idx);
  if (depth > TABLES_DEPTH) {
    return 0;
  }

  if (is_table_empty(frame_index) and depth < TABLES_DEPTH) {
    /*
     * is an empty table, i.e. not a leaf.
     *
     * case 1 in the pdf.
     */
    evict_frame = CASE_1;
    return frame_index;
  }

  /*
   * the recursive DFS traversing of the tree.
   */
  uint64_t frame;
  word_t next_frame;
  for (uint64_t i = 0; i < PAGE_SIZE; i++) {
    PMread(frame_index * PAGE_SIZE + i, &next_frame);
    if (next_frame == 0) {
      continue;
    }

    frame = traverse(max_frame_idx,
                     evict_frame,
                     max_cyclic_distance,
                     next_frame,
                     depth + 1,
                     swapped_in,
                     current_path << OFFSET_WIDTH + i);
    if (frame != 0) {
      return frame;
    }
  }

  return 0;
}

uint64_t
cyclic_distance(uint64_t swapped_in, uint64_t page_num)
{
  return MIN(NUM_PAGES - ABS(swapped_in - page_num),
             ABS(swapped_in - page_num));
}
