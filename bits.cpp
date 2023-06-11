//
// Created by roeey on 6/7/23.
//

#include "bits.hpp"

#include <stdint.h>

uint64_t
extract_bits(uint64_t number, uint64_t start, uint64_t end)
{
  uint64_t mask;

  mask = ((1 << (end + 1)) - 1) - ((1 << start) - 1);

  return (number & mask) >> start;
}
