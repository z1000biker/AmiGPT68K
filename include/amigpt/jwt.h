#ifndef AMIGPT_JWT_H
#define AMIGPT_JWT_H
#include <stddef.h>
typedef struct {
    char alg[32];
    char kid[192];
    char iss[256];
    char aud[256];
    char sub[256];
    char nonce[256];
    char email[384];
    long exp;
    long iat;
} amigpt_jwt_claims;
int amigpt_jwt_decode_parts(const char *jwt,char *header,size_t hcap,char *payload,size_t pcap,const char **sig_b64);
int amigpt_jwt_claims_parse(const char *jwt,amigpt_jwt_claims *out,const char **sig_b64);
#endif
