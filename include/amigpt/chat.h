#ifndef AMIGPT_CHAT_H
#define AMIGPT_CHAT_H
#include <stddef.h>
#define AMIGPT_CHAT_MAX 64
typedef struct {char *role;char *text;} amigpt_message;
typedef struct {amigpt_message msg[AMIGPT_CHAT_MAX];size_t count;} amigpt_chat;
void amigpt_chat_init(amigpt_chat *c);
void amigpt_chat_free(amigpt_chat *c);
int amigpt_chat_add(amigpt_chat *c,const char *role,const char *text);
int amigpt_chat_request(const amigpt_chat *c,const char *model,char *out,size_t cap);
char *amigpt_chat_request_alloc(const amigpt_chat *c,const char *model,size_t *len_out);
#endif
