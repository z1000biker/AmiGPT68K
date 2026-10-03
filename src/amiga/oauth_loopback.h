#ifndef AMIGPT_OAUTH_LOOPBACK_H
#define AMIGPT_OAUTH_LOOPBACK_H
#include <stddef.h>
typedef struct {char code[2048];char state[256];char client_id[256];char error[256];} amigpt_callback;
int amigpt_loopback_open(unsigned short *port_out,char *err,size_t errcap);
int amigpt_loopback_wait(int listen_fd,amigpt_callback *cb,unsigned long timeout_seconds,char *err,size_t errcap);
void amigpt_loopback_close(int fd);
#endif
