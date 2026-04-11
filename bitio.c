unsigned bitbuf = 0;    /* use native word size, it's faster */
int     bitlen = 0, overrun = 0;
long    bytes_out;      /* counter of bytes processed by PutBits */
