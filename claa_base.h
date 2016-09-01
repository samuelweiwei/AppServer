#ifndef _CLAA_BASE_H_
#define _CLAA_BASE_H_

#include <limits.h>

/*
#if USHRT_MAX == 0xffff
	#define INT16TYPE short
#else
	#error Unable to create a 16 bit variable
#endif

#if UINT_MAX == 0xffffffffUL
	#define INT32TYPE int
#elif ULONG_MAX == 0xffffffffUL
	#define INT32TYPE long
#else
	#error Unable to create a 32 bit variable
#endif

#if ULLONG_MAX == 0xffffffffffffffffULL
	#define INT64TYPE long long
#else
	#error Unable to create a 64 bit variable
#endif
*/
#define INT16TYPE short
#define INT32TYPE int
#define INT64TYPE long long

typedef signed   char       schar;
typedef signed   char       sint8;
typedef unsigned char       uint8;
typedef signed   INT16TYPE  sint16;
typedef unsigned INT16TYPE  uint16;
typedef signed   INT32TYPE  sint32;
typedef unsigned INT32TYPE  uint32;
typedef signed   INT64TYPE  sint64;
typedef unsigned INT64TYPE  uint64;

#endif

