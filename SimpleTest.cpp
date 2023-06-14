#include "PhysicalMemory.h"
#include "VirtualMemory.h"

#include <cassert>
#include <cstdio>

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

int
main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  word_t val;

  (void)val;
  (void)print_tree;

  VMinitialize();
  //    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
  //        printf("writing to %llu\n", (long long int) i);
  //        VMwrite(5 * i * PAGE_SIZE, i);
  //    }
  //
  //    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
  //        word_t value;
  //        VMread(5 * i * PAGE_SIZE, &value);
  //        printf("reading from %llu %d\n", (long long int) i, value);
  //        assert(uint64_t(value) == i);
  //    }

  printf("OFFSET_WIDTH %u\n", OFFSET_WIDTH);
  printf("PAGE_SIZE %llu\n", PAGE_SIZE);
  printf("PHYSICAL_ADDRESS_WIDTH %u\n", PHYSICAL_ADDRESS_WIDTH);
  printf("RAM_SIZE %llu\n", RAM_SIZE);
  printf("VIRTUAL_ADDRESS_WIDTH %u\n", VIRTUAL_ADDRESS_WIDTH);
  printf("VIRTUAL_MEMORY_SIZE %llu\n", VIRTUAL_MEMORY_SIZE);
  printf("NUM_FRAMES %llu\n", NUM_FRAMES);
  printf("NUM_PAGES %llu\n", NUM_PAGES);
  printf("TABLES_DEPTH %u\n", TABLES_DEPTH);

  VMwrite(13, 3);
  //    printf("after VMwrite\n");
  //    print_tree(0, 0);
  VMread(13, &val);
  printf("val: %d\n", val);
  VMread(6, &val);
  //    printf("val: %d\n", val);
  print_tree(0, 0);
  VMread(31, &val);
  printf("val: %d\n", val);
  print_tree(0, 0);

  printf("success\n");

  return 0;
}
