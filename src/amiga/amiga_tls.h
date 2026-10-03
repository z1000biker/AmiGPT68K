#ifndef AMIGPT_AMIGA_TLS_H
#define AMIGPT_AMIGA_TLS_H
#include <stddef.h>
typedef struct amigpt_tls amigpt_tls;
amigpt_tls *amigpt_tls_connect(const char *host,unsigned short port,char *err,size_t errcap);
int amigpt_tls_write_all(amigpt_tls *c,const void *buf,size_t len,char *err,size_t errcap);
int amigpt_tls_read(amigpt_tls *c,void *buf,size_t cap,char *err,size_t errcap);
void amigpt_tls_close(amigpt_tls *c);
void amigpt_tls_global_cleanup(void);
#endif
