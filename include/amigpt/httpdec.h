#ifndef AMIGPT_HTTPDEC_H
#define AMIGPT_HTTPDEC_H
#include <stddef.h>
typedef void (*amigpt_http_body_cb)(const char *data,size_t len,void *user);
typedef enum { H_HEADER=0,H_BODY_CLOSE,H_CHUNK_SIZE,H_CHUNK_DATA,H_CHUNK_CRLF,H_DONE,H_ERROR } amigpt_http_state;
typedef struct {
 amigpt_http_state state;
 char header[16384]; size_t hused;
 char line[64]; size_t lused;
 unsigned long chunk_left;
 int status;
 int chunked;
} amigpt_httpdec;
void amigpt_httpdec_init(amigpt_httpdec *h);
int amigpt_httpdec_feed(amigpt_httpdec *h,const char *buf,size_t len,amigpt_http_body_cb cb,void *user);
#endif
