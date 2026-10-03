#include "amiga_crypto.h"
#include "amiga_tls.h"
#include "amigpt/base64url.h"
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <libraries/amisslmaster.h>
#include <amissl/tags.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <errno.h>

struct Library *SocketBase=0;
struct Library *AmiSSLMasterBase=0;
struct Library *AmiSSLBase=0;
struct Library *AmiSSLExtBase=0;

int amigpt_crypto_is_open(void)
{
    return SocketBase && AmiSSLMasterBase && AmiSSLBase;
}

int amigpt_crypto_open(struct Library *socket_base,char *err,size_t errcap)
{
    (void)socket_base;
    if(amigpt_crypto_is_open())return 0;
    if(SocketBase || AmiSSLMasterBase || AmiSSLBase || AmiSSLExtBase)amigpt_crypto_close();
    SocketBase=OpenLibrary((STRPTR)"bsdsocket.library",4);
    if(!SocketBase){if(err&&errcap)snprintf(err,errcap,"bsdsocket.library v4 not found");return -1;}
    AmiSSLMasterBase=OpenLibrary((STRPTR)"amisslmaster.library",5);
    if(!AmiSSLMasterBase){if(err&&errcap)snprintf(err,errcap,"AmiSSL 5 not found");CloseLibrary(SocketBase);SocketBase=0;return -1;}
    if(OpenAmiSSLTags(AMISSL_CURRENT_VERSION,AmiSSL_UsesOpenSSLStructs,FALSE,AmiSSL_GetAmiSSLBase,(ULONG)&AmiSSLBase,AmiSSL_GetAmiSSLExtBase,(ULONG)&AmiSSLExtBase,AmiSSL_SocketBase,(ULONG)SocketBase,AmiSSL_ErrNoPtr,(ULONG)&errno,TAG_DONE)!=0 || !AmiSSLBase){
        if(err&&errcap)snprintf(err,errcap,"OpenAmiSSLTags failed");
        if(AmiSSLBase){CloseAmiSSL();AmiSSLBase=0;AmiSSLExtBase=0;}
        CloseLibrary(AmiSSLMasterBase);AmiSSLMasterBase=0;CloseLibrary(SocketBase);SocketBase=0;return -1;
    }
    return 0;
}

void amigpt_crypto_close(void)
{
    amigpt_tls_global_cleanup();
    if(AmiSSLBase){CloseAmiSSL();AmiSSLBase=0;AmiSSLExtBase=0;}
    if(AmiSSLMasterBase){CloseLibrary(AmiSSLMasterBase);AmiSSLMasterBase=0;}
    if(SocketBase){CloseLibrary(SocketBase);SocketBase=0;}
}

int amigpt_amiga_sha256(const unsigned char *d,size_t n,unsigned char out[32]){return SHA256(d,n,out)?0:-1;}
int amigpt_amiga_random(unsigned char *o,size_t n){return RAND_bytes(o,(int)n)==1?0:-1;}
int amigpt_make_verifier(char *out,size_t cap){unsigned char r[32];if(amigpt_amiga_random(r,sizeof r)<0)return -1;return amigpt_base64url_encode(r,sizeof r,out,cap);}
int amigpt_make_nonce(char *out,size_t cap){unsigned char r[24];if(amigpt_amiga_random(r,sizeof r)<0)return -1;return amigpt_base64url_encode(r,sizeof r,out,cap);}
int amigpt_make_uuid_urn(char *out,size_t cap){unsigned char r[16];int n;if(amigpt_amiga_random(r,sizeof r)<0)return -1;r[6]=(unsigned char)((r[6]&0x0f)|0x40);r[8]=(unsigned char)((r[8]&0x3f)|0x80);n=snprintf(out,cap,"urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",r[0],r[1],r[2],r[3],r[4],r[5],r[6],r[7],r[8],r[9],r[10],r[11],r[12],r[13],r[14],r[15]);return(n<0||(size_t)n>=cap)?-1:n;}
