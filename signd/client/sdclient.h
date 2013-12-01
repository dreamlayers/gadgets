#ifndef _SDCLIENT_H_

#include "../sdprotocol.h"

#ifdef IN_SDCLIENT

/* Port to listen on */
extern const int tcpport;

/* Protocol version */
extern const char protver[];

int sc_xdatac(char c, char *b, unsigned l, unsigned long flags,
              unsigned long transtm, unsigned long gntm);

int sc_xsimplec(char c, unsigned long flags,
                unsigned long transtm, unsigned long gntm);

#endif /* IN_SDCLIENT */

int sc_open(void);

void sc_close(void);

#endif /* !_SDCLIENT_H_ */
