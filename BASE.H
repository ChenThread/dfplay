#ifndef _BASE_H_
#define _BASE_H_

typedef unsigned char uint8_t;
typedef char int8_t;

#ifdef __WATCOMC__
// Define Borland C aliases:
#define getvect _dos_getvect
#define setvect _dos_setvect
#define outportb outp
#define inportb inp
#endif

#endif
