extern void InitTree();

#ifndef SEGMENTED
# define MAXBITS 16
#else
# ifdef INT_16_BITS
#  define MAXBITS 15
# else
#  define MAXBITS 14
# endif
#endif

#ifdef SEGMENTED
# ifdef TINY
#  undef MAXBITS
#  define MAXBITS 13
# endif
#endif

#ifndef BITS
# define BITS    MAXBITS
#endif

#if BITS < 13
# undef BITS
# define BITS    13      /* 1:1 hash */
#endif

#if BITS > MAXBITS
# undef BITS
# define BITS    MAXBITS
#endif

/* The following hash-function isn't optimal but it is very fast:

	HASH =      ((first + (second << LEN0) +
		    (third << LEN1)) & ((1L << BITS) - 1);

  The difference of LENs is no more than one bit.
*/

#define LEN0    ((BITS-8)/2)
#define LEN1    (BITS-8)

#define hash_size      (1L << BITS)

/* Use native size hash unless required otherwise */
#if defined(SMALL) || defined(TINY)
typedef us_t hash_t;
#else
typedef unsigned hash_t;
#endif  /* SMALL || TINY */

extern int       match_position, chain_length;

extern hash_t hashtab[], next[];

/* Some defines to eliminate function-call overhead */

/* Hash function (no more than 16 bits, so we don't need longs */

#define hash(p)\
	((unsigned)(p)[0] + ((unsigned)(p)[1] << LEN0) +\
	((unsigned)(p)[2] << LEN1))

#ifdef FASTHASH
#define hashof(p)\
	(((p)[0] != (p)[1] ? hash(p) : hash(p) + hash((p) + 3)) &\
	((1L << BITS) - 1))
#else
#define hashof(p)\
	(hash(p) & ((1L << BITS) - 1))
#endif

/* Inserting of a node `r' into hashed linked list: `r' becomes
	the head of list.
*/

#define InsertNode()\
{\
	register uc_t  *key = &text_buf[r & WINMASK];\
	register unsigned p = hashof(key);\
	if (r < MAXDIST) /* wraparound occured */\
		r = rehash(r);\
	next[r & WINMASK] = hashtab[p];\
	hashtab[p] = r;\
}

/* This routine inputs the char from stdin and does some other
	actions depending of this char's presence.
*/

#define Next_Char(N,F)\
{\
	if ((c = getchar()) != EOF) {\
		text_buf[s] = c;\
		if (s < F - 1)\
			text_buf[s + N] = c;\
		s = (s + 1) & (N - 1);\
		in_count++;\
	} else\
		len--;\
	r++;\
	InsertNode();\
}

#if defined(__GNUC__)
#if defined(__i386__)
/* Optimizer cannot allocate these registers correctly :( (v1.39) */
#define FIX_SI  asm("si")
#define FIX_DI  asm("di")
#else

/* GNU-style register allocations for other processors are welcome! */

#define FIX_SI
#define FIX_DI
#endif
#else

/* Dummy defines for non-GNU compilers */

#define FIX_SI
#define FIX_DI
#endif

/* some heuristic to avoid necessity of "-ggg..." */
#define CHAIN_THRESHOLD (LOOKAHEAD / 2)

extern int get_next_match();
extern hash_t rehash();

#ifdef GATHER_STAT
extern long node_matches, node_compares, node_prolongations;
#endif

#ifdef ALLOW_MISALIGN
#define FETCH(array,index) *(us_t*)(&array[index]-1)
#else
#define FETCH(array,index) array[index]
#endif
