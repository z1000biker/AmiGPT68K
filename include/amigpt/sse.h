#ifndef AMIGPT_SSE_H
#define AMIGPT_SSE_H
#include <stddef.h>
typedef void (*amigpt_sse_cb)(const char *event,const char *data,void *user);
typedef struct { char line[65536]; size_t used; char event[128]; char data[65536]; size_t data_used; } amigpt_sse;
void amigpt_sse_init(amigpt_sse *s);
int amigpt_sse_feed(amigpt_sse *s,const char *buf,size_t n,amigpt_sse_cb cb,void *user);
#endif
