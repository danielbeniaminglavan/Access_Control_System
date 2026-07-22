#ifndef DEFS_H
#define DEFS_H 

#include <avr/io.h>

#define setbit(data,bit_num) data |= (1 << (bit_num))
#define testbit(data,bit_num) data & (1 << (bit_num))
#define clrbit(data,bit_num) data &= (~(1 << (bit_num)))

#define F_CPU 16000000UL

#define TRUE 1U
#define FALSE 0U

#endif // DEFS_H