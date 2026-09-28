#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned char bool_t;
// 64-bit things; will be emulated by compiler
typedef long long int64_t;
typedef unsigned long long uint64_t;
// boolean values
#define FALSE 0
#define TRUE 1
#define false FALSE
#define true TRUE
#endif

