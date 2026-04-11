extern void StartHuff(int n_char);
extern void init(uc_t *table);
extern void write_header(void);
extern int read_header(void);
extern void EncodeChar(int c);
extern void crpt_message(void);
extern int DecodeChar(void);
extern int DecodePosition(void);
extern void EncodePosition(int c);
#define MAX_FREQ        (us_t)0x8000 /* Tree update timing */
