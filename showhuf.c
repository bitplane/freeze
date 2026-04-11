#include "freeze.h"
#include "huf.h"

uc_t Table2[9];

/* prints out Huffman information from a frozen file, just for fun
 * and testing purposes.
 */

int main(argc, argv)
int argc; char ** argv;
{
	if (argc != 2) {
		fprintf(stderr, "Usage: showhuf frozen_file\n");
		return 1;
	}

	if (freopen(argv[1], "r", stdin) == NULL) {
		fprintf(stderr, "showhuf: can't open file %s", argv[1]);
		perror(" ");
		return 1;
	}
	if (getchar() != MAGIC1)
		goto reject;

	switch (getchar()) {
	case MAGIC2_1:
		printf("%s: no Huffman table (old-style)\n", argv[1]);
		return 0;

	case MAGIC2_2:
		break;

	default: reject:
		printf("%s is not in frozen format\n", argv[1]);
		return 0;
	}

	/* Now the real work begins... */

	printf("%s: ", argv[1]);

	if (read_header() == EOF)
		return 1;

	printf("%d,%d,%d,%d,%d,%d,%d,%d\n",
		Table2[1], Table2[2], Table2[3], Table2[4],
		Table2[5], Table2[6], Table2[7], Table2[8]);

	/* ... and ends */

	return 0;
}

/* Reconstructs `Table' from the header of the frozen file and checks
	its correctness. Returns 0 if OK, EOF otherwise.
*/

int read_header() {
	short i, j;
	i = getchar() & 0xFF;
	i |= (getchar() & 0xFF) << 8;
	Table2[1] = i & 1; i >>= 1;
	Table2[2] = i & 3; i >>= 2;
	Table2[3] = i & 7; i >>= 3;
	Table2[4] = i & 0xF; i >>= 4;
	Table2[5] = i & 0x1F; i >>= 5;

	if (i & 1 || (i = getchar()) & 0xC0) {
		fprintf(stderr, "Unknown header format.\n");
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
		printf("Warning - bad Huffman table");
		return 0;
	}
	Table2[7] = j;
	Table2[8] = i - j;

	return 0;
}
