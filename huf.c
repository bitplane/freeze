#include "freeze.h"
#include "huf.h"
#include "bitio.h"

/*----------------------------------------------------------------------*/
/*									*/
/*		HUFFMAN ENCODING					*/
/*									*/
/*----------------------------------------------------------------------*/

/* TABLES OF ENCODE/DECODE for upper 6 bits position information */

/* The contents of `Table' are used for freezing only, so we use
 * it freely when melting.
 */

#ifndef HUFVALUES
#define HUFVALUES 0,0,1,2,6,19,34,0
#endif

uc_t Table2[9] = { 0, HUFVALUES };

/* d_len is really the number of bits to read to complete literal
 * part of position information.
 */
uc_t p_len[64];        /* These arrays are built accordingly to values */
uc_t d_len[256];       /* of `Table' above which are default, from the */
		      /* command line or from the header of frozen file */
uc_t code[256];

/* use arrays of native word-size elements to improve speed */

unsigned freq[T2 + 1];          /* frequency table */
int     son[T2];                /* points to son node (son[i],son[i+1]) */
int     prnt[T2 + N_CHAR2];     /* points to parent node */

static  int t, r, chars;

/* notes :
   prnt[Tx .. Tx + N_CHARx - 1] used by
   indicates leaf position that corresponding to code.
*/

/* Initializes Huffman tree, bit I/O variables, etc.
   Static array is initialized with `table', dynamic Huffman tree
   has `n_char' leaves.
*/

void StartHuff (n_char)
	int n_char;
{
	register int i, j;
	t = n_char * 2 - 1;
	r = t - 1;
	chars = n_char;

/* A priori frequences are 1 */

	for (i = 0; i < n_char; i++) {
		freq[i] = 1;
		son[i] = i + t;
		prnt[i + t] = i;
	}
	i = 0; j = n_char;

/* Building the balanced tree */

	while (j <= r) {
		freq[j] = freq[i] + freq[i + 1];
		son[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2; j++;
	}
	freq[t] = 0xffff;
	prnt[r] = 0;
	in_count = 1;
	bytes_out = 5;
#if defined(DEBUG) || defined (GATHER_STAT)
	symbols_out = refers_out = 0;
#endif
}

/* Reconstructs tree with `chars' leaves */

void reconst ()
{
	register int i, j, k;
	register unsigned f;

#ifdef DEBUG
	if (quiet < 0)
	  fprintf(stderr,
	    "Reconstructing Huffman tree: symbols: %ld, references: %ld\n",
	    symbols_out, refers_out);
#endif

/* correct leaf node into of first half,
   and set these freqency to (freq+1)/2
*/
	j = 0;
	for (i = 0; i < t; i++) {
		if (son[i] >= t) {
			freq[j] = (freq[i] + 1) / 2;
			son[j] = son[i];
			j++;
		}
	}
/* Build tree.  Link sons first */

	for (i = 0, j = chars; j < t; i += 2, j++) {
		k = i + 1;
		f = freq[j] = freq[i] + freq[k];
		for (k = j - 1; f < freq[k]; k--);
		k++;
		{       register unsigned *p, *e;
			for (p = &freq[j], e = &freq[k]; p > e; p--)
				p[0] = p[-1];
			freq[k] = f;
		}
		{       register int *p, *e;
			for (p = &son[j], e = &son[k]; p > e; p--)
				p[0] = p[-1];
			son[k] = i;
		}
	}

/* Link parents */
	for (i = 0; i < t; i++) {
		if ((k = son[i]) >= t) {
			prnt[k] = i;
		} else {
			prnt[k] = prnt[k + 1] = i;
		}
	}
}


/* Updates given code's frequency, and updates tree */

void update (c)
	register int c;
{
	register unsigned k, *p;
	register int    i, j, l;

	if (freq[r] == MAX_FREQ) {
		reconst();
	}
	c = prnt[c + t];
	do {
		k = ++freq[c];

		/* swap nodes when become wrong frequency order. */
		if (k > freq[l = c + 1]) {
			for (p = freq+l+1; k > *p++; ) ;
			l = p - freq - 2;
			freq[c] = p[-2];
			p[-2] = k;

			i = son[c];
			prnt[i] = l;
			if (i < t) prnt[i + 1] = l;

			j = son[l];
			son[l] = i;

			prnt[j] = c;
			if (j < t) prnt[j + 1] = c;
			son[c] = j;

			c = l;
		}
	} while ((c = prnt[c]) != 0);	/* loop until reach to root */
}

/* Encodes the literal or the length information */

void EncodeChar (c)
	int c;
{
	register int k, len = 0;
	register unsigned hcode = 0;
#ifdef INT_16_BITS
	register unsigned hcode2 = 0;
#endif
	DefBits;

	k = prnt[c + t];

/* trace links from leaf node to root */
	do {
/* if node index is odd, trace larger of sons */
#ifdef INT_16_BITS
		if (len < 16) {
#endif
			hcode |= (unsigned) (k & 1) << len;
#ifdef INT_16_BITS
		} else {
			hcode2 |= (unsigned) (k & 1) << len;
		}
#endif
		len++;
	} while ((k = prnt[k]) != r);

#ifdef INT_16_BITS
	if (len > 16) {
		PutNLowBits(hcode2, len - 16);
		FlushBits();
		PutNLowBits(hcode >> 8, 8);
		FlushBits();
		hcode &= 0xFF;
		len = 8;
	} else if (len > 8) {
		PutNLowBits(hcode >> 8, len - 8);
		FlushBits();
		hcode &= 0xFF;
		len = 8;
	}
#endif
	PutNLowBits(hcode, len);
	FlushBits();
	KeepBits();
	if (ferror(stdout))
		writeerr();
	update(c);
}

/* Encodes the position information */

void EncodePosition (c)
	register int c;
{
	DefBits;
	/* output upper 6 bit from table */
	PutNLowBits(code[c >> 7], p_len[c >> 7]);
#ifdef INT_16_BITS
	FlushBits();
#endif
	/* output lower 7 bit */
	PutNLowBits(c & 0x7f, 7);
	FlushBits();
	KeepBits();
}

/* Decodes the literal or length info and returns its value.
	Returns ENDOF, if the file is corrupt.
*/

int DecodeChar ()
{
	register int c = r;
	DefBits;

	if (overrun >= sizeof(bitbuf)) {
		crpt_message();
		return ENDOF;
	}
	/* As far as MAX_FREQ == 32768, maximum length of a Huffman
	 * code cannot exceed 23 (consider Fibonacci numbers),
	 * so we don't need additional FillBits while decoding
	 * if sizeof(int) == 4.
	 */
	FillBits();
	/* trace from root to leaf,
	   got bit is 0 to small(son[]), 1 to large (son[]+1) son node */

	while ((c = son[c]) < t) {
		c += GetBit();
#ifdef INT_16_BITS
		if (loclen == 0)
			FillBits();
#endif
	}
	KeepBits();
	update(c -= t);
	return c;
}

/* Decodes the position info and returns it */

int DecodePosition ()
{
	register        i, j;
	DefBits;

	/* Upper 6 bits can be coded by a byte (8 bits) or less,
	 * plus 7 bits literally ...
	 */
	FillBits();
	/* decode upper 6 bits from the table */
	i = GetByte();
	j = (code[i] << 7) | (i << d_len[i]) & 0x7F;

	/* get lower 7 bits literally */
#ifdef INT_16_BITS
	FillBits();
#endif
	j |= GetNBits(d_len[i]);
	KeepBits();
	return j;
}

#ifdef COMPAT

uc_t Table1[9] = { 0, 0, 0, 1, 3, 8, 12, 24, 16 };

/* Old version of a routine above for handling files made by
	the 1st version of Freeze.
*/

short DecodePOld ()
{
	register        i, j;
	DefBits;

	FillBits();
	i = GetByte();
	j = (code[i] << 6) | (i << d_len[i]) & 0x3F;
#ifdef INT_16_BITS
	FillBits();
#endif
	j |= GetNBits(d_len[i]);
	KeepBits();
	return j;
}
#endif

/* Initializes static Huffman arrays */

void init(table) uc_t * table; {
	short i, j, k, num;
	num = 0;

/* There are `table[i]' `i'-bits Huffman codes */

	for(i = 1, j = 0; i <= 8; i++) {
		num += table[i] << (8 - i);
		for(k = table[i]; k; j++, k--)
			p_len[j] = i;
	}
	if (num != 256) {
		fprintf(stderr, "Invalid position table\n");
		exit(1);
	}
	num = j;
	if (do_melt == 0)

/* Freezing: building the table for encoding */

		for(i = j = 0;;) {
			code[j] = i;
			i++;
			j++;
			if (j == num) break;
			i <<= p_len[j] - p_len[j-1];
		}
	else {

/* Melting: building the table for decoding */

		for(k = j = 0; j < num; j ++)
			for(i = 1 << (8 - p_len[j]); i--;)
				code[k++] = j;

		for(k = j = 0; j < num; j ++)
			for(i = 1 << (8 - p_len[j]); i--;)
				d_len[k++] =  p_len[j] - 1
#ifdef COMPAT
				- (table == Table1)
#endif
							;
	}
}

/* Writes a 3-byte header into the frozen form of file; Table[7] and
	Table[8] aren't necessary, see `read_header'.
*/

void write_header() {
	unsigned i;

	i = Table2[5] & 0x1F; i <<= 4;
	i |= Table2[4] & 0xF; i <<= 3;
	i |= Table2[3] & 7;   i <<= 2;
	i |= Table2[2] & 3;   i <<= 1;
	i |= Table2[1] & 1;

	putchar((int)(i & 0xFF));
	putchar((int)((i >> 8)));
	putchar((int)(Table2[6] & 0x3F));
	if (ferror(stdout))
		writeerr();
}

/* Reconstructs `Table' from the header of the frozen file and checks
	its correctness. Returns 0 if OK, EOF otherwise.
*/

int read_header() {
	int i, j;
	i = getchar() & 0xFF;
	i |= (getchar() & 0xFF) << 8;
	Table2[1] = i & 1; i >>= 1;
	Table2[2] = i & 3; i >>= 2;
	Table2[3] = i & 7; i >>= 3;
	Table2[4] = i & 0xF; i >>= 4;
	Table2[5] = i & 0x1F; i >>= 5;

	if (i & 1 || (i = getchar()) & 0xC0) {
		fprintf(stderr, "Unknown header format.\n");
		crpt_message();
		return EOF;
	}

	Table2[6] = i & 0x3F;

	i = Table2[1] + Table2[2] + Table2[3] + Table2[4] +
	Table2[5] + Table2[6];

	i = 62 - i;     /* free variable length codes for 7 & 8 bits */

	j = 128 * Table2[1] + 64 * Table2[2] + 32 * Table2[3] +
	16 * Table2[4] + 8 * Table2[5] + 4 * Table2[6];

	j = 256 - j;    /* free byte images for these codes */

/*      Equation:
	    Table[7] + Table[8] = i
	2 * Table[7] + Table[8] = j
*/
	j -= i;
	if (j < 0 || i < j) {
		crpt_message();
		return EOF;
	}
	Table2[7] = j;
	Table2[8] = i - j;

#ifdef DEBUG
	fprintf(stderr, "Codes: %d %d %d %d %d %d %d %d\n",
		Table2[1], Table2[2], Table2[3], Table2[4],
		Table2[5], Table2[6], Table2[7], Table2[8]);
#endif
	return 0;
}

/* File too short or invalid header, print a message */
void crpt_message ( )
{
	fprintf ( stderr, "melt: corrupt input\n" );
}

