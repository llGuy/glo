#ifndef _BIT_V_H_
#define _BIT_V_H_

/* Was too lazy. Thanks -> https://gist.github.com/codescv/6146378 for code */
typedef struct BitVector {
    char *bits;
    int size;
} BitVector;

BitVector createBitvec(int nBits);
void destroyBitvec(BitVector *vec);
void setBit(BitVector *vec, int index, int bit);
int getBit(BitVector *vec, int index);

#endif
