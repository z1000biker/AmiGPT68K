#ifndef AMIGPT_IDTOKEN_VERIFY_H
#define AMIGPT_IDTOKEN_VERIFY_H
#include <stddef.h>
typedef struct {char subject[256];char email[384];long exp;} amigpt_identity;
int amigpt_verify_id_token(const char *jwt,const char *client_id,const char *nonce,amigpt_identity *id,char *err,size_t errcap);
#endif
