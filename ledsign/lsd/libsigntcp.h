#ifndef _LIBSIGNTCP_H_
#define _LIBSIGNTCP_H_

#include "../../signd/client/sdclient.h"

int sc_nop(void);
int sc_clear(unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_full(unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_hnprint(char *msg, unsigned l, unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_snprint(char *msg, unsigned l, unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_gnprint(char *msg, unsigned l, unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_hprint(char *msg, unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_sprint(char *msg, unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_gprint(char *msg, unsigned long flags, unsigned long transtm, unsigned long gntm);
int sc_dl(char *buf);
int sc_ul(char *buf, unsigned long flags, unsigned long transtm, unsigned long gntm);

#endif /* ifndef _LIBSIGNTCP_H_ */
