#include <stdint.h>

void sndbuf_init(void);
void sndbuf_quit(void);
void sndbuf_store(const int16_t *input, unsigned int len);
int sndbuf_retrieve(double *output);
