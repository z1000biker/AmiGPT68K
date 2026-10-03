#ifndef AMIGPT_OPENAI_HTTP_H
#define AMIGPT_OPENAI_HTTP_H
#include <stddef.h>
typedef void (*amigpt_delta_cb)(const char *text,void *user);
int amigpt_openai_stream(const char *access_token,const char *model,const char *prompt,amigpt_delta_cb cb,void *user,char *err,size_t errcap);
#endif
