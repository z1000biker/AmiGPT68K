#ifndef AMIGPT_PKCE_H
#define AMIGPT_PKCE_H
#include <stddef.h>
typedef int (*amigpt_sha256_fn)(const unsigned char *data,size_t len,unsigned char out32[32]);
int amigpt_pkce_challenge(const char *verifier,amigpt_sha256_fn sha256,char *out,size_t cap);
#endif
