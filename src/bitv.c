#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "bitv.h"

BitVector createBitvec(int nBits) {
  int size = sizeof(BitVector) * ceil(nBits/8.0);

  BitVector vec = {
    .size = size,
    .bits = (char *)malloc(size)
  };

  memset(vec.bits, 0, sizeof(char) * size);

  return vec;
}

void destroyBitvec(BitVector *vec) {
  free(vec->bits);
}

void setBit(BitVector *vec, int index, int bit) {
  int byte = index >> 3;
  int n = sizeof(index)*8-3;
  int offset = ((unsigned) index << n) >> n;
  if (bit) {
    vec->bits[byte] |= 1 << (7-offset);
  }
  else {
    vec->bits[byte] &= ~(1 << (7-offset));
  }
}

int getBit(const BitVector *vec, int index) {
  int byte = index >> 3;
  int n = sizeof(index)*8-3;
  int offset = ((unsigned) index << n) >> n;
  if (vec->bits[byte] & (1 << (7- offset))) {
    return 1;
  }
  else {
    return 0;
  }
}
