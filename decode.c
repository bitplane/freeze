#include "freeze.h"
#include "huf.h"
#include "bitio.h"

/*
 * Melt stdin to stdout.
 */

void melt2 ()
{
	register short    i, j, k, r, c;

/* Huffman-dependent part */
	if(read_header() == EOF)
		return;
	StartHuff(N_CHAR2);
	init(Table2);
/* end of Huffman-dependent part */

	InitIO();
	for (i = WINSIZE - LOOKAHEAD; i < WINSIZE; i++)
		text_buf[i] = ' ';
	r = 0;
	for (in_count = 0;; ) {
		c = DecodeChar();

		if (c == ENDOF)
			break;
		if (c < 256) {
#ifdef DEBUG
			if (debug)
				fprintf(stderr, "'%s'\n", pr_char((uc_t)c));
#endif /* DEBUG */
			text_buf[r++] = c;
			in_count++;
			r &= WINMASK;
#ifdef DEBUG
			if (!debug)
#endif
			if (r == 0) {
				fwrite(text_buf, WINSIZE, 1, stdout);
				if (ferror(stdout))
					writeerr();
			}
		} else {
			i = (r - DecodePosition() - 1) & WINMASK;
			j = c - 256 + THRESHOLD;
#ifdef DEBUG
			if (debug)
				fputc('"', stderr);
#endif
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & WINMASK];
#ifdef DEBUG
				if (debug)
					fprintf(stderr, "%s", pr_char((uc_t)c));
#endif
				text_buf[r++] = c;
				in_count++;
				r &= WINMASK;
#ifdef DEBUG
				if (!debug)
#endif
				if (r == 0) {
					fwrite(text_buf, WINSIZE, 1, stdout);
					if (ferror(stdout))
						writeerr();
				}
			}
#ifdef DEBUG
			if (debug)
				fprintf(stderr, "\"\n");
#endif
		}
		INDICATOR
	}
	if (r) {
		fwrite(text_buf, r, 1, stdout);
		if (ferror(stdout))
			writeerr();
	}
	if (quiet < 0 && file_length != 0)
		fprintf(stderr, "100%%\b\b\b\b");
}

#ifdef COMPAT
void melt1 ()
{
	register short    i, j, k, r, c;

	StartHuff(N_CHAR1);
	init(Table1);
	InitIO();
	for (i = N1 - F1; i < N1; i++)
		text_buf[i] = ' ';
	r = 0;
	for (in_count = 0;; ) {
		c =  DecodeChar();

		if (c == ENDOF)
			break;

		if (c < 256) {
#ifdef DEBUG
			if (debug)
				fprintf(stderr, "'%s'\n", pr_char((uc_t)c));
#endif /* DEBUG */
			text_buf[r++] = c;
			in_count++;
			r &= (N1 - 1);
#ifdef DEBUG
			if (!debug)
#endif
			if (r == 0) {
				fwrite(text_buf, N1, 1, stdout);
				if (ferror(stdout))
					writeerr();
			}
		} else {
			i = (r - DecodePOld() - 1) & (N1 - 1);
			j = c - 256 + THRESHOLD;
#ifdef DEBUG
			if (debug)
				fputc('"', stderr);
#endif
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & (N1 - 1)];
#ifdef DEBUG
				if (debug)
					fprintf(stderr, "%s", pr_char((uc_t)c));
#endif
				text_buf[r++] = c;
				in_count++;
				r &= (N1 - 1);
#ifdef DEBUG
				if (!debug)
#endif
				if (r == 0) {
					fwrite(text_buf, N1, 1, stdout);
					if (ferror(stdout))
						writeerr();
				}
			}
#ifdef DEBUG
			if (debug)
				fprintf(stderr, "\"\n");
#endif
		}
		INDICATOR
	}
	if (r) {
		fwrite(text_buf, r, 1, stdout);
		if (ferror(stdout))
			writeerr();
	}
}
#endif
