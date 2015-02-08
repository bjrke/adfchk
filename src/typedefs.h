#ifndef TYPEDEFS_H
#define TYPEDEFS_H

/* one-byte integer 0x00..0xFF */
typedef unsigned char uint8;
typedef signed char int8;

typedef unsigned short uint16;
typedef signed short int16;

typedef unsigned int uint32;
typedef signed int int32;

typedef unsigned long uint64;
typedef signed long int64;

#ifndef bool
typedef enum { false, true } bool;
#endif

#define MINMENU 1
#define MAXMENU 5


#endif 
