

#ifndef _HAVE_BYTEORDER_H_
#define _HAVE_BYTEORDER_H_



#ifndef __BYTE_ORDER__
#if (__ARM_EABI__ == 1)
#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__ 1234
#endif /* __ORDER_LITTLE_ENDIAN__ */
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif /*  __ARM_EABI__ */
#endif /* __BYTE_ORDER__ */

#ifndef ntohl
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ntohl(x) (((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | ((x << 24) & 0xff000000))
#else
#define ntohl(x) (x)
#endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ )*/
#endif /* ntohl */


#endif /* _HAVE_BYTEORDER_H_ */
