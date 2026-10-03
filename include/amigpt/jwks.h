#ifndef AMIGPT_JWKS_H
#define AMIGPT_JWKS_H
#include <stddef.h>
int amigpt_jwks_find_rsa(const char *jwks,const char *kid,char *n_b64,size_t ncap,char *e_b64,size_t ecap);
#endif
