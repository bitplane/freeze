/* Some definitions for faster bit-level I/O */
/* Assumptions: local variables "loclen" and "locbuf" are defined
 * via "DefBits";
 * AvailBits() is called before all bit input operations, the
 * maximum allowed argument for AvailBits() is bits(bitbuf) -7;
 * FlushBits() is called often enough while bit output operations;
 * KeepBits() is called as opposite to DefBits.
 */

extern unsigned bitbuf; /* Bit I/O buffers */
extern int  bitlen;     /* Number of bits actually in `bitbuf' */
extern int  overrun;    /* at least as many bytes were read as EOF */
extern long bytes_out;  /* we use it, we declare it */

#define bits(x) ((int)sizeof(x)*8)
#define BYSH  (bits(bitbuf)-8)
#define BISH  (bits(bitbuf)-1)

#define InitIO()        { overrun = bitlen = 0; bitbuf = 0; }

#define DefBits         register unsigned locbuf = bitbuf;\
register int loclen = bitlen

#define FillBits()   if (loclen <= bits(bitbuf) - 8) {\
	do {\
		locbuf |= (unsigned)(getchar() & 0xFF) << (BYSH - loclen);\
		loclen += 8;\
	} while (loclen <= bits(bitbuf) - 8);\
if (feof(stdin)) overrun++;\
}

#define FlushBits() if (loclen >= 8) do {\
	putchar ((int)(locbuf >> BYSH));\
	bytes_out++;\
	locbuf <<= 8;\
	loclen -= 8;\
} while (loclen >= 8)

/* FlushTail works with global variables !! */
#define FlushTail() if (bitlen) {\
	putchar((int)(bitbuf >> BYSH));\
	bytes_out++;\
}

#define KeepBits()      bitbuf = locbuf, bitlen = loclen

/* GetX() macros may be used only in "var op= GetX();" statements !! */

#define GetBit()  /* var op= */locbuf >> BISH, locbuf <<= 1, loclen--

#define GetByte() /* var op= */locbuf >> BYSH, locbuf <<= 8, loclen -= 8

/* NB! `n' is used more than once here! */
#define GetNBits(n) /* var op= */ locbuf >> (bits(bitbuf) - (n)),\
	locbuf <<= (n), loclen -= (n)

/* Puts n MSBs of var to the stream, assume other bits == 0 */
#define PutNBits(var, n) locbuf |= (unsigned)(var) >> loclen, loclen += (n)

/* Puts n LSBs of var to the stream, assume other bits == 0 */
#define PutNLowBits(var, n) locbuf |= (unsigned)(var) << (bits(bitbuf) -\
(n) - loclen), loclen += (n)

/* Puts LSB (!) of var to the stream, isn't used now */
#define PutBit(var) locbuf |= (unsigned)((var) & 1) << (BISH - loclen), loclen++
