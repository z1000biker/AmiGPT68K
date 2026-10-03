#ifndef AMIGPT_AMIGA_HTTP_H
#define AMIGPT_AMIGA_HTTP_H
#include <stddef.h>
typedef struct {int status;char *body;size_t len;size_t cap;int oom;} amigpt_http_response;
int amigpt_https_get(const char *host,const char *path,const char *accept,amigpt_http_response *out,char *err,size_t errcap);
int amigpt_https_get_bearer(const char *host,const char *path,const char *accept,const char *token,amigpt_http_response *out,char *err,size_t errcap);
int amigpt_https_post_form(const char *host,const char *path,const char *form,amigpt_http_response *out,char *err,size_t errcap);
int amigpt_https_post_json(const char *host,const char *path,const char *json,amigpt_http_response *out,char *err,size_t errcap);
void amigpt_http_response_free(amigpt_http_response *r);
#endif
