#ifndef BITS_H_
#define BITS_H_

#define SET_BIT(b, n) (b |= (1 << n))
#define UNSET_BIT(b, n) (b &= ~(1 << n))
#define IS_BIT_SET(b, n) (b & (1 << n))

#endif
