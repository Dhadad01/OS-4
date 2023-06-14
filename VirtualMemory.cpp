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

#define ROOT_TABLE_SIZE                                                        \
  (1 << (VIRTUAL_ADDRESS_WIDTH - (TABLES_DEPTH * OFFSET_WIDTH)))
#define ROOT_TABLE_FRAME_INDEX 0

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
clear_table(word_t frame_index);

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
         uint64_t& max_cyclic_distance_page_num,
         uint64_t& max_cyclic_distance_frame_num,
         uint64_t& max_cyclic_distance_frame_padre,
         uint64_t frame_padre = 0,
         uint64_t frame_index = 0,
         uint64_t depth = 0,
         uint64_t page_num = 0,
         uint64_t current_path = 0,
         uint64_t skip_table = 0);

static uint64_t
cyclic_distance(uint64_t swapped_in, uint64_t page_num);

void
VMinitialize()
{
  //  for (uint64_t i = 0; i < ROOT_TABLE_SIZE; i++) {
  //    PMwrite(i, 0);
  //  }
  for (uint64_t i = 0; i < PAGE_SIZE; i++) {
    PMwrite(i, 0);
  }
}

#include <cstdio>

int
VMread(uint64_t virtualAddress, word_t* value)
{
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
    /*
     * out of bounds.
     */
    return 0;
  }
  uint64_t physical_addr = translate(virtualAddress);
  printf("%s: virtual address %lu is mapped to physical address is: %lu\n",
         __FUNCTION__,
         virtualAddress,
         physical_addr);
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
  if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
    /*
     * out of bounds.
     */
    return 0;
  }

  uint64_t physical_addr = translate(virtualAddress);
  printf("%s: virtual address %lu is mapped to physical address is: %lu\n",
         __FUNCTION__,
         virtualAddress,
         physical_addr);
  printf("%d was written\n", value);
  if (0 == physical_addr) {
    /*
     * error.
     */
    return 0;
  }

  PMwrite(physical_addr, value);
  return 1;
}

//**************************************************
#include <iostream>
#include <vector>
static void
print_tree(uint64_t frame, int depth)
{
  std::vector<word_t> values;
  word_t val = 0;
  std::cout << "******FRAME " << frame
            << " is leaf: " << ((depth == TABLES_DEPTH) ? "yes" : "no")
            << "******\n";
  for (int i = 0; i < PAGE_SIZE; ++i) {
    PMread((PAGE_SIZE * frame) + i, &val);
    //    std::cout << "index: " << i << " value: " << val << " ";
    std::cout << " " << val;
    values.push_back(val);
  }
  std::cout << std::endl;
  for (const auto& value : values) {
    if (value != 0 && depth != TABLES_DEPTH) {
      print_tree(value, depth + 1);
    }
  }
}
//**************************************************

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
  word_t f1 = 0;

  /* the indices (p0 | p1 | ... | pn | offset) are stored in reverse order. */
  for (uint64_t i = 0; i < TABLES_DEPTH; i++) {
    start = i * OFFSET_WIDTH;
    end = ((i + 1) * OFFSET_WIDTH) - 1;
    indices[i] = extract_bits(virtual_addr, start, end);
  }
  indices[TABLES_DEPTH] = extract_bits(
    virtual_addr, TABLES_DEPTH * OFFSET_WIDTH, VIRTUAL_ADDRESS_WIDTH - 1);

  addr = 0;
  for (uint64_t depth = 0; depth < TABLES_DEPTH; depth++) {
    PMread(addr * PAGE_SIZE + indices[TABLES_DEPTH - depth], &addr1);
    if (0 == addr1) {
      /*
       * need to swap pages.
       */
      uint64_t max_frame_idx = 0;
      uint64_t max_cyclic_distance = 0;
      uint64_t max_cyclic_distance_page_num = 0;
      uint64_t max_cyclic_distance_frame_num = 0;
      uint64_t max_cyclic_distance_frame_padre = 0;
      uint64_t frame = 0;
      enum evict_frame_case evict_frame = UNDEFINED;
      uint64_t page_num = virtual_addr >> OFFSET_WIDTH;
      frame = traverse(
        max_frame_idx,
        evict_frame,
        max_cyclic_distance,
        max_cyclic_distance_page_num,
        max_cyclic_distance_frame_num,
        max_cyclic_distance_frame_padre,
        0,
        //                       addr,   // replaced with the root - which is 0
        ROOT_TABLE_FRAME_INDEX,
        //                       depth,  // replaced with the root's depth -
        //                       which is 0
        0,
        page_num,
        0,
        addr);
      if (evict_frame == CASE_1) {
        f1 = frame;
        // unlink empty table from the previous table.
        PMwrite(max_cyclic_distance_frame_padre, 0);
      } else {
        if (max_frame_idx + 1 < NUM_FRAMES) {
          evict_frame = CASE_2;
          if (depth != TABLES_DEPTH) {
            f1 = max_frame_idx + 1;
          }
        } else {
          evict_frame = CASE_3;
          PMevict(max_cyclic_distance_frame_num, max_cyclic_distance_page_num);
          // unlink evicted page from previous table.
          PMwrite(max_cyclic_distance_frame_padre, 0);
          f1 = max_cyclic_distance_frame_num;
        }
      }

      if (depth + 1 != TABLES_DEPTH) {
        clear_table(f1);
        PMwrite(addr * PAGE_SIZE + indices[TABLES_DEPTH - depth], f1);
      } else {
        PMrestore(f1, page_num);
        PMwrite(addr * PAGE_SIZE + indices[TABLES_DEPTH - depth], f1);
      }

      addr1 = f1;
    }
    addr = addr1;
    //    printf("DEBUG IN TRANSLATE\n");
    //    print_tree(0, 0);
    //    printf("\n\n\n\n\n");
  }

  /* addr is the frame, indices[0] is the offset. */
  physical_addr = (addr << OFFSET_WIDTH) + indices[0];

  return physical_addr;
}

void
clear_table(word_t frame_index)
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
         uint64_t& max_cyclic_distance_page_num,
         uint64_t& max_cyclic_distance_frame_num,
         uint64_t& max_cyclic_distance_frame_padre,
         uint64_t frame_padre,
         uint64_t frame_index,
         uint64_t depth,
         uint64_t swapped_in,
         uint64_t current_path,
         uint64_t skip_table)
{
  max_frame_idx = MAX(frame_index, max_frame_idx);
  if (depth > TABLES_DEPTH) {
    return 0;
  }
  // TODO index +-1
  if (depth == TABLES_DEPTH) {
    // case 3
    if (cyclic_distance(swapped_in, current_path) > max_cyclic_distance) {
      max_cyclic_distance_page_num = current_path;
      max_cyclic_distance_frame_num = frame_index;
      max_cyclic_distance_frame_padre = frame_padre;
    }

    max_cyclic_distance =
      MAX(max_cyclic_distance, cyclic_distance(swapped_in, current_path));
    return 0;
  }

  if ((is_table_empty(frame_index) and depth < TABLES_DEPTH) and
      frame_index != skip_table) {
    /*
     * is an empty table, i.e. not a leaf.
     *
     * case 1 in the pdf.
     */
    evict_frame = CASE_1;
    /*
     * abuse of notation...
     * use max_cyclic_distance_frame_padre just for passing back to the caller
     * from which table entry to unlink the current frame.
     */
    max_cyclic_distance_frame_padre = frame_padre;
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
                     max_cyclic_distance_page_num,
                     max_cyclic_distance_frame_num,
                     max_cyclic_distance_frame_padre,
                     frame_index * PAGE_SIZE + i,
                     next_frame,
                     depth + 1,
                     swapped_in,
                     (current_path << OFFSET_WIDTH) + i,
                     skip_table);
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
