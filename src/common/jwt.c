#include "amigpt/jwt.h"
#include "amigpt/base64url.h"
#include "amigpt/jsonlite.h"
#include <string.h>
#include <stdlib.h>

static int decode_seg(const char *p,size_t n,char *out,size_t cap)
{
    char *tmp; size_t got=0; int rc;
    if(!p||!out||cap<2)return -1;
    tmp=(char*)malloc(n+1); if(!tmp)return -1;
    memcpy(tmp,p,n); tmp[n]=0;
    rc=amigpt_base64url_decode(tmp,(unsigned char*)out,cap-1,&got);
    free(tmp); if(rc<0||got>=cap)return -1;
    out[got]=0; return 0;
}

int amigpt_jwt_decode_parts(const char *jwt,char *header,size_t hcap,char *payload,size_t pcap,const char **sig_b64)
{
    const char *a,*b;
    if(!jwt||!header||!payload)return -1;
    a=strchr(jwt,'.'); if(!a)return -1;
    b=strchr(a+1,'.'); if(!b)return -1;
    if(strchr(b+1,'.'))return -1;
    if(decode_seg(jwt,(size_t)(a-jwt),header,hcap)<0)return -1;
    if(decode_seg(a+1,(size_t)(b-a-1),payload,pcap)<0)return -1;
    if(sig_b64)*sig_b64=b+1;
    return 0;
}

int amigpt_jwt_claims_parse(const char *jwt,amigpt_jwt_claims *out,const char **sig_b64)
{
    static char h[2048],p[8192];
    if(!out)return -1;
    memset(out,0,sizeof *out);
    if(amigpt_jwt_decode_parts(jwt,h,sizeof h,p,sizeof p,sig_b64)<0)return -1;
    if(amigpt_json_string(h,"alg",out->alg,sizeof out->alg)!=1)return -1;
    if(amigpt_json_string(h,"kid",out->kid,sizeof out->kid)!=1)return -1;
    if(amigpt_json_string(p,"iss",out->iss,sizeof out->iss)!=1)return -1;
    if(amigpt_json_string(p,"aud",out->aud,sizeof out->aud)!=1)return -1;
    if(amigpt_json_string(p,"sub",out->sub,sizeof out->sub)!=1)return -1;
    amigpt_json_string(p,"nonce",out->nonce,sizeof out->nonce);
    amigpt_json_string(p,"email",out->email,sizeof out->email);
    if(amigpt_json_long(p,"exp",&out->exp)!=1)return -1;
    if(amigpt_json_long(p,"iat",&out->iat)!=1)return -1;
    return 0;
}
