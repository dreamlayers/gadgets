/* Header file for VFD gadget analog logging. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#ifndef _ANALOGLOG_H_
#define _ANALOGLOG_H_
void anbuf_flush(void);
void anbuf_add(unsigned int cts, unsigned const char *data, unsigned int l);
#endif
