#include "idtoken_verify.h"
#include "amiga_http.h"
#include "amiga_crypto.h"
#include "amigpt/jwt.h"
#include "amigpt/jwks.h"
#include "amigpt/base64url.h"
#include <proto/amissl.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/objects.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
extern struct Library *AmiSSLBase;
static int verify_rs256(const char *jwt,const char *n64,const char *e64,char *err,size_t cap){const char *d1,*d2,*sig64;static unsigned char nbuf[1024],ebuf[32],sig[1024],digest[32];size_t nn=0,en=0,sn=0,signed_len;BIGNUM *bn_n=0,*bn_e=0;RSA *rsa=0;int ok=0;d1=strchr(jwt,'.');if(!d1){snprintf(err,cap,"bad JWT");return -1;}d2=strchr(d1+1,'.');if(!d2){snprintf(err,cap,"bad JWT");return -1;}sig64=d2+1;signed_len=(size_t)(d2-jwt);if(amigpt_base64url_decode(n64,nbuf,sizeof nbuf,&nn)<0||amigpt_base64url_decode(e64,ebuf,sizeof ebuf,&en)<0||amigpt_base64url_decode(sig64,sig,sizeof sig,&sn)<0){snprintf(err,cap,"bad JWKS/JWT base64");return -1;}bn_n=BN_bin2bn(nbuf,(int)nn,0);bn_e=BN_bin2bn(ebuf,(int)en,0);rsa=RSA_new();if(!bn_n||!bn_e||!rsa){snprintf(err,cap,"RSA allocation failed");goto out;}if(RSA_set0_key(rsa,bn_n,bn_e,0)!=1){snprintf(err,cap,"RSA key setup failed");goto out;}bn_n=0;bn_e=0;if(!SHA256((const unsigned char*)jwt,signed_len,digest)){snprintf(err,cap,"SHA256 failed");goto out;}if(RSA_verify(NID_sha256,digest,32,sig,(unsigned int)sn,rsa)!=1){snprintf(err,cap,"ID token signature invalid");goto out;}ok=1;out:if(bn_n)BN_free(bn_n);if(bn_e)BN_free(bn_e);if(rsa)RSA_free(rsa);return ok?0:-1;}
int amigpt_verify_id_token(const char *jwt,const char *client_id,const char *nonce,amigpt_identity *id,char *err,size_t errcap){static amigpt_jwt_claims c;amigpt_http_response r;static char n64[2048],e64[64];long now=(long)time(0);int k;if(!jwt||!client_id||!nonce||!id){snprintf(err,errcap,"ID token validation arguments missing");return -1;}memset(id,0,sizeof *id);if(amigpt_jwt_claims_parse(jwt,&c,0)<0){snprintf(err,errcap,"cannot parse ID token");return -1;}if(strcmp(c.alg,"RS256")){snprintf(err,errcap,"unsupported ID token alg: %s",c.alg);return -1;}if(strcmp(c.iss,"https://auth.openai.com")){snprintf(err,errcap,"ID token issuer mismatch");return -1;}if(strcmp(c.aud,client_id)){snprintf(err,errcap,"ID token audience mismatch");return -1;}if(nonce&&*nonce&&strcmp(c.nonce,nonce)){snprintf(err,errcap,"ID token nonce mismatch");return -1;}if(now>0&&(c.exp<now-5||c.iat>now+5)){snprintf(err,errcap,"ID token time claims invalid");return -1;}if(amigpt_https_get("auth.openai.com","/.well-known/jwks.json","application/json",&r,err,errcap)<0)return -1;if(r.status<200||r.status>=300){snprintf(err,errcap,"JWKS HTTP status %d",r.status);amigpt_http_response_free(&r);return -1;}k=amigpt_jwks_find_rsa(r.body,c.kid,n64,sizeof n64,e64,sizeof e64);amigpt_http_response_free(&r);if(k!=1){snprintf(err,errcap,"ID token signing key not found");return -1;}if(verify_rs256(jwt,n64,e64,err,errcap)<0)return -1;snprintf(id->subject,sizeof id->subject,"%s",c.sub);snprintf(id->email,sizeof id->email,"%s",c.email);id->exp=c.exp;return 0;}
