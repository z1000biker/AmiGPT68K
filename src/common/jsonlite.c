#include "amigpt/jsonlite.h"
#include <string.h>
#include <ctype.h>

static const char *find_key(const char *j,const char *key)
{
    size_t k=strlen(key); const char *p=j;
    while((p=strchr(p,'"'))!=0){
        p++;
        if(!strncmp(p,key,k)&&p[k]=='"'){
            const char *q=p+k+1;
            while(*q&&isspace((unsigned char)*q)) q++;
            if(*q==':') return q+1;
        }
        p++;
    }
    return 0;
}

int amigpt_json_string(const char *j,const char *key,char *out,size_t cap)
{
    const char *p; size_t o=0;
    if(!j||!key||!out||cap<1) return -1;
    p=find_key(j,key); if(!p) return 0;
    while(*p&&isspace((unsigned char)*p)) p++;
    if(*p++!='"') return -1;
    while(*p&&*p!='"'){
        unsigned char c=(unsigned char)*p++;
        if(c=='\\'){
            c=(unsigned char)*p++; if(!c) return -1;
            if(c=='n')c='\n'; else if(c=='r')c='\r'; else if(c=='t')c='\t';
            else if(c=='b')c='\b'; else if(c=='f')c='\f';
            else if(c=='u') return -2;
        }
        if(o+1>=cap) return -1;
        out[o++]=(char)c;
    }
    if(*p!='"') return -1;
    out[o]=0; return 1;
}

int amigpt_json_bool(const char *j,const char *key,int *v)
{
    const char *p;
    if(!j||!key||!v) return -1;
    p=find_key(j,key); if(!p) return 0;
    while(*p&&isspace((unsigned char)*p)) p++;
    if(!strncmp(p,"true",4)){*v=1;return 1;}
    if(!strncmp(p,"false",5)){*v=0;return 1;}
    return -1;
}

int amigpt_json_long(const char *j,const char *key,long *v)
{
    const char *p,*e; long n=0; int neg=0,any=0;
    if(!j||!key||!v) return -1;
    p=find_key(j,key); if(!p) return 0;
    while(*p&&isspace((unsigned char)*p)) p++;
    if(*p=='-'){neg=1;p++;}
    e=p;
    while(*e>='0'&&*e<='9'){
        any=1;
        if(n>214748364L) return -1;
        n=n*10+(*e-'0'); e++;
    }
    if(!any) return -1;
    *v=neg?-n:n; return 1;
}
