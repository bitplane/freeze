
#include "freeze.h"
#include "lz.h"
#include "huf.h"
#include "bitio.h"

/* for future versions ?... */

#define LENGTH_OFFSET   256
#define EncodeLiteral(l)        EncodeChar((int)l)
#define EncodeLength(l)         EncodeChar((int)l + LENGTH_OFFSET)

/*
 * Freezes stdin to stdout
 */

void 
freeze()
{
  register int i, len, s, c;
  register hash_t r;
  int     match_length;

  putchar(MAGIC1);
  putchar(MAGIC2_2);

/* Huffman-dependent part */
  write_header();
  StartHuff(N_CHAR2);
  init(Table2);
/* end of Huffman-dependent part */

  InitTree();			/* LZ dependent */
  InitIO();

  s = r = MAXDIST + 1;              /* to make zero links "too old" */

  for (len = 0; len < LOOKAHEAD && (c = getchar()) != EOF; len++) {
      text_buf[s] = c;
      if (s < LOOKAHEAD - 1)
	text_buf[s + WINSIZE] = c;
      s = (s + 1) & WINMASK;
  }

 /* check for magic header */
  if (!topipe && !force && text_buf[r] == MAGIC1 &&
    text_buf[r + 1] >= MAGIC2_1) {
    if (quiet != 1)
      fprintf(stderr, " already frozen ");
    exit_stat = 2;
    return;
  }
  in_count = len;

  /* Insert 256 "initial" spaces and the very first char into hash table */
  for (i = r - LOOKAHEAD; i < r; i++)
    text_buf[i] = ' ';
  for (i = r - LOOKAHEAD; i <= r; i++) {
	register uc_t  *key = &text_buf[i];
	register unsigned p = hashof(key);
	next[i] = hashtab[p];
	hashtab[p] = i;
  }

 /* here the main loop begins */

  while (len != 0) {
    match_length = LOOKAHEAD + get_next_match(THRESHOLD - LOOKAHEAD, r);
    if (match_length > len)
      match_length = len;

    if (match_length <= THRESHOLD) {
      match_length = 1;
      EncodeLiteral(text_buf[r & WINMASK]);
#ifdef DEBUG
      symbols_out++;
      if (verbose)
	fprintf(stderr, "'%s'\n",
	  pr_char(text_buf[r & WINMASK]));
#endif				/* DEBUG */
    } else if (match_length >= chain_length) {

/* GREEDY parsing (if match_length is big enough, put it right away) */

#if defined(GATHER_STAT) || defined (DEBUG)
      refers_out++;
#endif
      EncodeLength(match_length - THRESHOLD);
      EncodePosition((int) ((r - match_position) & WINMASK) - 1);

    } else {
      register int orig_length, orig_position;
      register us_t oldchar;

/* This fragment (delayed coding, non-greedy) is due to ideas of
	Jan Mark Wams' <jms@cs.vu.nl> COMIC:
*/
      oldchar = text_buf[r & WINMASK];
      orig_length = match_length;
      orig_position = match_position;

      Next_Char(WINSIZE, LOOKAHEAD);
      match_length = LOOKAHEAD + get_next_match(match_length - LOOKAHEAD, r);

      if (match_length > len)
	match_length = len;

      if (orig_length >= match_length) {
	EncodeLength(orig_length - THRESHOLD);
	EncodePosition((int) ((r - 1 - orig_position) & WINMASK) - 1);
#ifdef DEBUG
	match_position = orig_position;
#endif				/* DEBUG */
	match_length = orig_length - 1;
      } else {
	EncodeLiteral(oldchar);
#ifdef DEBUG
	symbols_out++;
	if (verbose)
	  fprintf(stderr, "'%s'\n",
	    pr_char(oldchar));
#endif				/* DEBUG */
	EncodeLength(match_length - THRESHOLD);
	EncodePosition((int) ((r - match_position) & WINMASK) - 1);
      }

#if defined(GATHER_STAT) || defined (DEBUG)
      refers_out++;
#endif
#ifdef DEBUG
      if (verbose) {
	register pos = match_position, leng = match_length;
	fputc('"', stderr);
	for (; leng; leng--, pos++)
	  fprintf(stderr, "%s",
	    pr_char(text_buf[pos]));
	fprintf(stderr, "\"\n");
      }
#endif				/* DEBUG */
    }

/* Process the rest of the matched sequence (insertion in the list
	only, without any matching !!!)
*/

    for (i = 0; i < match_length &&
      (c = getchar()) != EOF; i++) {
      text_buf[s] = c;
      if (s < LOOKAHEAD - 1)
	text_buf[s + WINSIZE] = c;
      s = (s + 1) & WINMASK;
      r++;
      InsertNode();
    }

    in_count += i;

    INDICATOR

      while (i++ < match_length) {
      len--;
      r++;
      InsertNode();
    }

  }

 /* to flush literals */
  EncodeLength((short) ENDOF - LENGTH_OFFSET);
#ifdef DEBUG
  symbols_out++;
#endif
  FlushTail();
  if (ferror(stdout))
	  writeerr();
  if (quiet < 0 && file_length != 0)
	  fprintf(stderr, "100%%\b\b\b\b");
 /* Print out stats on stderr */
  if (quiet != 1) {
#ifdef GATHER_STAT
    fprintf(stderr, "Average number of compares per matching: %d.%02d\n",
      (int) (node_compares / node_matches),
      (int) ((node_compares * 100 / node_matches) % 100));
    fprintf(stderr, "Average number of prolongations per reference: %d.%02d\n",
      (int) (node_prolongations / refers_out),
      (int) ((node_prolongations * 100 / refers_out) % 100));
    fprintf(stderr, "Average number of compares per prolongation: %d.%02d\n",
      (int) (node_compares / node_prolongations),
      (int) ((node_compares * 100 / node_prolongations) % 100));
    fprintf(stderr, "Prolongations/compares: %d/%d\n", node_prolongations,
      node_compares);
#endif
#ifdef DEBUG
    fprintf(stderr,
      "%ld chars in, %ld codes (%ld bytes) out, freezing factor: ",
      in_count, symbols_out + refers_out, bytes_out);
    prratio(stderr, in_count, bytes_out);
    fprintf(stderr, "\n");
    fprintf(stderr, "\tFreezing as in compact: ");
    prratio(stderr, in_count - bytes_out, in_count);
    prbits(stderr, in_count, bytes_out);
    fprintf(stderr, "\n");
    fprintf(stderr, "\tSymbols: %ld; references: %ld.\n",
      symbols_out, refers_out);
#else				/* !DEBUG */
    fprintf(stderr, " Freezing: ");
    prratio(stderr, in_count - bytes_out, in_count);
    prbits(stderr, in_count, bytes_out);
#endif				/* DEBUG */
  }
  if (bytes_out >= in_count)	/* exit(2) if no savings */
    exit_stat = 2;
  return;
}
