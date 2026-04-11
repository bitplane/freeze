#include <stdio.h>
#include "config.h"

#ifdef HAVE_SYS_STDTYPES_H
# include <sys/stdtypes.h>
#endif

#ifndef __EMX__
#ifndef getc
# ifdef m88k                   /* Green Hill C library bug. */
#  define getc(p)         (--(p)->_cnt < 0 ? __filbuf(p) : (int) *(p)->_ptr++)
# else
#  define getc(p)         (--(p)->_cnt < 0 ? _filbuf(p) : (int) *(p)->_ptr++)
# endif
#endif
#ifndef putc
# ifdef m88k                   /* Green Hill C library bug. */
#  define putc(x, p)      (--(p)->_cnt < 0 ? __flsbuf((unsigned char) (x), (p)) : (int) (*(p)->_ptr++ = (unsigned char) (x)))
# else
#  define putc(x, p)      (--(p)->_cnt < 0 ? _flsbuf((unsigned char) (x), (p)) : (int) (*(p)->_ptr++ = (unsigned char) (x)))
# endif
#endif
#endif

#if !defined(MSDOS) && defined(__MSDOS__)
# define MSDOS
#endif

#ifdef MSDOS
# define DOS
# include <fcntl.h>
#endif  /* MSDOS */

#ifdef TOS
# define DOS
# define O_TEXT          0x01
# define O_BINARY        0x02
#endif

#include <ctype.h>
#include <signal.h>

#ifndef RETSIGTYPE
# define RETSIGTYPE void
#endif /* RETSIGTYPE */

#ifndef TOS
# include <sys/types.h>
# include <sys/stat.h>
#else
# include <tos.h>
# include <types.h>
#endif

#ifdef SYSTIME
# include <sys/time.h>
# define UTIMES
#else
# ifdef UTIME
#  include <utime.h>
# else
#  ifdef SYSUTIME
#   include <sys/utime.h>
#  else
#   ifdef unix
/*      UNIX without any declaration of utimbuf .... Strange! */
struct utimbuf {
	time_t actime;
	time_t modtime;
};
extern int utime();
#   else
#    ifndef DOS
#     define BITS no_utimbuf_definition_on_unknown_system
#    endif
#   endif
#  endif
# endif
#endif

/* for MAXNAMLEN only !!! */
#ifdef DIRENT
# include <dirent.h>
#else
# ifdef SYSNDIR
#  include <sys/ndir.h>
# else
#  ifdef SYSDIR
#   include <sys/dir.h>
#  endif
# endif
#endif

#ifndef MAXNAMLEN
# define MAXNAMLEN       255
#endif

#if MAXNAMLEN < 255
# undef MAXNAMLEN
# define MAXNAMLEN       255
#endif

#ifdef DEBUG
# include <assert.h>
#endif  /* DEBUG */

#ifdef DOS
# include <stdlib.h>
#endif  /* DOS */

#ifdef __TURBOC__
# ifdef MSDOS
#  include <io.h>
#  include <alloc.h>
# else /* TOS */
#  include <ext.h>
# endif  /* MSDOS */
#endif /* __TURBOC__ */

typedef unsigned short us_t;
typedef unsigned char uc_t;
typedef unsigned long ul_t;

#define LOOKAHEAD       256     /* pre-sence buffer size */
#define MAXDIST         7936
#define WINSIZE         (MAXDIST + LOOKAHEAD)   /* must be a power of 2 */
#define WINMASK         (WINSIZE - 1)

#define THRESHOLD	2

#define N_CHAR2         (256 - THRESHOLD + LOOKAHEAD + 1) /* code: 0 .. N_CHARi - 1 */
#define T2              (N_CHAR2 * 2 - 1)       /* size of table */

#define ENDOF           256                     /* pseudo-literal */

extern uc_t    Table2[];

extern long     in_count;
extern off_t    file_length;

extern uc_t     text_buf[];

extern long     indc_threshold, indc_count;

extern int      do_melt, topipe, greedy, quiet, force;  /* useful flags */

#define MAGIC1          ((uc_t)'\037')
#define MAGIC2_1        ((uc_t)'\236')          /* freeze vers. 1.X */
#define MAGIC2_2        ((uc_t)'\237')

extern int exit_stat;

#ifdef DEBUG
extern int debug, verbose;
extern char * pr_char();
#endif /* DEBUG */

#if defined(GATHER_STAT) || defined(DEBUG)
extern long refers_out, symbols_out;
#endif

extern void melt2(), (*meltfunc)(), writeerr(), prratio(), prbits(), freeze();

#ifdef COMPAT
#include "compat.h"
#endif

#define INDICATOR \
if (quiet < 0 && (in_count > indc_count)) {\
	if (ferror(stdout))\
		writeerr();\
	if (file_length) {\
		static int percents, old_percents = -1;\
		if ((percents = ftell(stdin) * 100 / file_length) !=\
			old_percents) {\
			fprintf(stderr, " %2d%%\b\b\b\b", percents);\
			old_percents = percents;\
		}\
		indc_count += indc_threshold;\
	} else {\
		fprintf(stderr, " %5ldK\b\b\b\b\b\b\b", in_count / 1024);\
		indc_count += indc_threshold;\
		indc_threshold += 1024;\
	}\
	fflush (stderr);\
}

#ifdef HAVE_RINDEX
#define strchr index
#define strrchr rindex
#endif

extern char *strchr(), *strrchr();

