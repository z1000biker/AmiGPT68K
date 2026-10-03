#ifndef AMIGPT_AMIGA_CRYPTO_H
#define AMIGPT_AMIGA_CRYPTO_H
#include <stddef.h>
struct Library;
int amigpt_crypto_open(struct Library *socket_base,char *err,size_t errcap);
void amigpt_crypto_close(void);
int amigpt_crypto_is_open(void);
extern struct Library *SocketBase;
extern struct Library *AmiSSLMasterBase;
extern struct Library *AmiSSLBase;
extern struct Library *AmiSSLExtBase;
int amigpt_amiga_sha256(const unsigned char *data,size_t len,unsigned char out32[32]);
int amigpt_amiga_random(unsigned char *out,size_t len);
int amigpt_make_verifier(char *out,size_t cap);
int amigpt_make_nonce(char *out,size_t cap);
int amigpt_make_uuid_urn(char *out,size_t cap);
#endif
