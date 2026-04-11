#include "freeze.h"
#include "lz.h"

/*----------------------------------------------------------------------*/
/*                                                                      */
/*                          LZSS ENCODING                               */
/*                                                                      */
/*----------------------------------------------------------------------*/

uc_t    text_buf[WINSIZE + LOOKAHEAD - 1];/* cyclic buffer with an overlay */
int     match_position;                 /* current position of
						matched pattern */
int     chain_length;                   /* max_chain_length ==
						CHAIN_THRESHOLD >> greedy */

/*      next[N+1..] is used as hash table,
	the rest of next is a link down,
*/

hash_t hashtab[hash_size];      /* a VERY large array :-) */
hash_t next[WINSIZE];

#ifdef GATHER_STAT
long node_matches, node_compares, node_prolongations;
#endif  /* GATHER_STAT */

/* Initialize the data structures and allocate memory, if needed.
	Although there is no more trees in the LZ algorithm
	implementation, routine name is kept intact :-)
*/

void InitTree ()
{

	unsigned i = 0;
#ifdef GATHER_STAT
	node_matches = node_compares = node_prolongations = 0;
#endif  /* GATHER_STAT */

	do {
		hashtab[i] = 0;
	} while (i++ != hash_size - 1);

	if (greedy >= 0)
		chain_length = ((CHAIN_THRESHOLD - 1) >> greedy) + 1;
	else
		chain_length = LOOKAHEAD * 2;
}

/* Get the longest (longer than `match_length' when entering in function)
	nearest match of the string beginning in text_buf[r]
	to the cyclic buffer. Result (length & position) is returned
	as the result and in global variable
	`match_position'). Unchanged `match_length' denotes failure and
	`match_position' contains garbage !!
	In order to achieve faster operation, `match_length' is shifted
	down to LOOKAHEAD. Ideas of Andrew Cadach <kadach@isi.itfs.nsk.su>
	have been used (lastbyte).
*/

int get_next_match (match_length, r)
	register hash_t r; int match_length;
{
	register hash_t p = r & WINMASK;
	register int m;
#ifdef ALLOW_MISALIGN
	register us_t lastbyte;
#else
	register uc_t lastbyte;
#endif
	register uc_t  *key FIX_SI, *pattern FIX_DI;
	int     chain_count = chain_length;

#ifdef GATHER_STAT
	node_matches++;
#endif
	key = text_buf + (r & WINMASK) + LOOKAHEAD;
	r -= MAXDIST;           /* `r' is now a "barrier value" */

	for(;;) {
		lastbyte = FETCH(key, match_length);
		do {
			if(chain_count <= 0)
				/* chain length exceeded, simple return */
				return match_length;

			pattern = text_buf + match_length + LOOKAHEAD;

			do {
				if ((p = next[p]) < r)
					return match_length;
			} while (FETCH(pattern, p &= WINMASK) != lastbyte);

			chain_count--;  /* successful lastbyte match, cost = 1 */
			pattern = text_buf + p + LOOKAHEAD;

#ifdef GATHER_STAT
		node_compares++;
#endif

#ifdef ALLOW_MISALIGN
			for (m = -LOOKAHEAD;
			*(unsigned*)&key[m] == *(unsigned*)&pattern[m] &&
				(m += sizeof(unsigned)) < 0;);
#ifndef INT_16_BITS
		/* Hope that sizeof(int) > 2 ==> sizeof(int) > sizeof(short) */
			if (m < 0 && *(us_t*)&key[m] == *(us_t*)&pattern[m])
				m += sizeof(us_t);
#endif
#ifdef BIGSHORTS
			while
#else
			if
#endif
				(m < 0 && key[m] == pattern[m])
					++m;
#else
			for (m = -LOOKAHEAD; key[m] == pattern[m] && ++m < 0;);
#endif
		} while (m < match_length);

		match_position = p;     /* remember new results */
		if (m == 0)
			return 0;
		match_length = m;

#ifdef GATHER_STAT
		node_prolongations++;
#endif
		chain_count -= 2;       /* yet another match found, cost = 2 */
	}
}

hash_t
rehash(r)
hash_t r;
{
  unsigned i = 0;
  r += WINSIZE;
  do {
    /* process links; zero must remain zero */
    if (next[i] && (next[i] += WINSIZE) > r) {
	    next[i] = 0;
    }
  } while(i++ != WINSIZE - 1);
  i = 0;
  do {
    /* process the hash table itself; zero must remain zero */
    if (hashtab[i] && (hashtab[i] += WINSIZE) > r)
	    hashtab[i] = 0;
  } while (i++ != hash_size - 1);
  return r;
}
