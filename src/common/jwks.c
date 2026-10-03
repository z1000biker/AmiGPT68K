#include "amigpt/jwks.h"
#include "amigpt/jsonlite.h"
#include <string.h>
#include <stdlib.h>

int amigpt_jwks_find_rsa(const char *jwks,const char *kid,char *n_b64,size_t ncap,char *e_b64,size_t ecap)
{
    const char *p=jwks;
    if(!jwks||!kid||!n_b64||!e_b64)return -1;
    while((p=strstr(p,"\"kid\""))!=0){
        const char *b=p,*e; char *obj; size_t len; char k[192],kty[32],alg[32];
        while(b>jwks&&*b!='{')b--;
        if(*b!='{')return -1;
        e=strchr(p,'}'); if(!e)return -1;
        len=(size_t)(e-b+1); obj=(char*)malloc(len+1); if(!obj)return -1;
        memcpy(obj,b,len);obj[len]=0;
        if(amigpt_json_string(obj,"kid",k,sizeof k)==1 && !strcmp(k,kid) &&
           amigpt_json_string(obj,"kty",kty,sizeof kty)==1 && !strcmp(kty,"RSA") &&
           amigpt_json_string(obj,"alg",alg,sizeof alg)==1 && !strcmp(alg,"RS256") &&
           amigpt_json_string(obj,"n",n_b64,ncap)==1 &&
           amigpt_json_string(obj,"e",e_b64,ecap)==1){free(obj);return 1;}
        free(obj); p=e+1;
    }
    return 0;
}
