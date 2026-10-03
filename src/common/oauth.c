#include "amigpt/oauth.h"
#include "amigpt/urlcodec.h"
#include <stdio.h>
#include <string.h>

static int enc(const char*s,char*d,size_t n){return amigpt_urlencode(s?s:"",d,n);}
static int addq(char*out,size_t cap,size_t *used,const char *name,const char *val)
{
    static char encv[4096]; int n;
    if(!val||!*val)return 0;
    if(enc(val,encv,sizeof encv)<0)return -1;
    n=snprintf(out+*used,cap-*used,"&%s=%s",name,encv);
    if(n<0||(size_t)n>=cap-*used)return -1;
    *used+=(size_t)n; return 0;
}

int amigpt_oauth_authorize_url(const amigpt_oauth_params*p,char*out,size_t cap)
{
    char ci[256],hi[384],ru[512],st[256],no[256],ch[256],sc[512],re[256];
    int n; size_t used;
    if(!p||!out||!p->client_id||!p->host_id||!p->redirect_uri||!p->state||!p->nonce||!p->challenge)return -1;
    if(enc(p->client_id,ci,sizeof ci)<0||enc(p->host_id,hi,sizeof hi)<0||enc(p->redirect_uri,ru,sizeof ru)<0||enc(p->state,st,sizeof st)<0||enc(p->nonce,no,sizeof no)<0||enc(p->challenge,ch,sizeof ch)<0||enc(AMIGPT_SCOPES,sc,sizeof sc)<0||enc(AMIGPT_RESOURCE,re,sizeof re)<0)return -1;
    n=snprintf(out,cap,"%s?response_type=code&client_id=%s&redirect_uri=%s&scope=%s&resource=%s&code_challenge=%s&code_challenge_method=S256&state=%s&nonce=%s&ext_agent_host_id=%s",AMIGPT_AUTH_ENDPOINT,ci,ru,sc,re,ch,st,no,hi);
    if(n<0||(size_t)n>=cap)return -1;
    used=(size_t)n;
    if(p->agent_name&&*p->agent_name){ if(addq(out,cap,&used,"agent_name_hint",p->agent_name)<0)return -1; }
    if(p->id_token_hint&&*p->id_token_hint){ if(addq(out,cap,&used,"id_token_hint",p->id_token_hint)<0)return -1; }
    if(p->login_hint&&*p->login_hint){ if(addq(out,cap,&used,"login_hint",p->login_hint)<0)return -1; }
    return (int)used;
}

int amigpt_oauth_code_form(const char*ci,const char*code,const char*ver,const char*ru,char*out,size_t cap)
{
    char a[256],b[2048],c[512],d[512],e[256];int n;
    if(enc(ci,a,sizeof a)<0||enc(code,b,sizeof b)<0||enc(ver,c,sizeof c)<0||enc(ru,d,sizeof d)<0||enc(AMIGPT_RESOURCE,e,sizeof e)<0)return -1;
    n=snprintf(out,cap,"grant_type=authorization_code&client_id=%s&code=%s&code_verifier=%s&redirect_uri=%s&resource=%s",a,b,c,d,e);
    return(n<0||(size_t)n>=cap)?-1:n;
}
int amigpt_oauth_refresh_form(const char*ci,const char*rt,char*out,size_t cap)
{
    char a[256],b[4096],e[256];int n;
    if(enc(ci,a,sizeof a)<0||enc(rt,b,sizeof b)<0||enc(AMIGPT_RESOURCE,e,sizeof e)<0)return -1;
    n=snprintf(out,cap,"grant_type=refresh_token&client_id=%s&refresh_token=%s&resource=%s",a,b,e);
    return(n<0||(size_t)n>=cap)?-1:n;
}
