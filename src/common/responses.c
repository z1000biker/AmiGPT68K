#include "amigpt/responses.h"
#include "amigpt/jsonlite.h"
#include <stdio.h>
#include <string.h>

static int json_escape(const char *src,char *dst,size_t cap)
{
    size_t o=0;
    unsigned char c;
    if(!src||!dst||cap==0)return -1;
    while((c=(unsigned char)*src++)!=0){
        const char *esc=0;
        char tmp[7];
        if(c=='"')esc="\\\""; else if(c=='\\')esc="\\\\";
        else if(c=='\n')esc="\\n"; else if(c=='\r')esc="\\r"; else if(c=='\t')esc="\\t";
        else if(c<0x20){snprintf(tmp,sizeof tmp,"\\u%04X",(unsigned)c);esc=tmp;}
        if(esc){size_t n=strlen(esc);if(o+n>=cap)return -1;memcpy(dst+o,esc,n);o+=n;}
        else {if(o+1>=cap)return -1;dst[o++]=(char)c;}
    }
    dst[o]=0;return (int)o;
}

int amigpt_responses_body(const char *model,const char *prompt,char *out,size_t cap)
{
    char m[512],p[8192];int n;
    if(json_escape(model,m,sizeof m)<0||json_escape(prompt,p,sizeof p)<0)return -1;
    n=snprintf(out,cap,"{\"model\":\"%s\",\"input\":[{\"role\":\"user\",\"content\":\"%s\"}],\"store\":false,\"stream\":true}",m,p);
    return(n<0||(size_t)n>=cap)?-1:n;
}

int amigpt_response_delta(const char *json,char *out,size_t cap)
{
    char type[128];
    if(amigpt_json_string(json,"type",type,sizeof type)!=1)return 0;
    if(strcmp(type,"response.output_text.delta")!=0)return 0;
    return amigpt_json_string(json,"delta",out,cap)==1?1:-1;
}
