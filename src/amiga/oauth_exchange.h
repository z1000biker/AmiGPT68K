#ifndef AMIGPT_OAUTH_EXCHANGE_H
#define AMIGPT_OAUTH_EXCHANGE_H
#include <stddef.h>
typedef struct {char access_token[8192];char refresh_token[8192];char id_token[12288];char scope[2048];long expires_in;long earliest_refresh_at;} amigpt_tokens;
int amigpt_exchange_code(const char *client_id,const char *code,const char *verifier,const char *redirect_uri,amigpt_tokens *t,char *err,size_t errcap);
int amigpt_refresh_tokens(const char *client_id,const char *refresh_token,amigpt_tokens *t,char *err,size_t errcap);
int amigpt_exchange_device_code(const char *client_id,const char *code,const char *verifier,const char *redirect_uri,amigpt_tokens *t,char *err,size_t errcap);
int amigpt_refresh_device_tokens(const char *client_id,const char *refresh_token,amigpt_tokens *t,char *err,size_t errcap);
#endif
