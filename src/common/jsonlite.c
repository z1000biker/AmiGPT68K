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

static int hex4(const char *p,unsigned long *v)
{
    int i;unsigned long n=0;
    for(i=0;i<4;i++){
        unsigned char c=(unsigned char)p[i];unsigned long d;
        if(c>='0'&&c<='9')d=(unsigned long)(c-'0');
        else if(c>='a'&&c<='f')d=(unsigned long)(c-'a'+10);
        else if(c>='A'&&c<='F')d=(unsigned long)(c-'A'+10);
        else return -1;
        n=(n<<4)|d;
    }
    *v=n;return 0;
}

static int emit_utf8(unsigned long cp,char *out,size_t cap,size_t *o)
{
    if(cp<=0x7f){if(*o+1>=cap)return -1;out[(*o)++]=(char)cp;return 0;}
    if(cp<=0x7ff){if(*o+2>=cap)return -1;out[(*o)++]=(char)(0xc0|(cp>>6));out[(*o)++]=(char)(0x80|(cp&0x3f));return 0;}
    if(cp<=0xffff){if(*o+3>=cap)return -1;out[(*o)++]=(char)(0xe0|(cp>>12));out[(*o)++]=(char)(0x80|((cp>>6)&0x3f));out[(*o)++]=(char)(0x80|(cp&0x3f));return 0;}
    if(cp<=0x10ffff){if(*o+4>=cap)return -1;out[(*o)++]=(char)(0xf0|(cp>>18));out[(*o)++]=(char)(0x80|((cp>>12)&0x3f));out[(*o)++]=(char)(0x80|((cp>>6)&0x3f));out[(*o)++]=(char)(0x80|(cp&0x3f));return 0;}
    return -1;
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
            else if(c=='u'){
                unsigned long cp,lo;
                if(hex4(p,&cp)<0)return -1;p+=4;
                if(cp>=0xd800&&cp<=0xdbff){
                    if(p[0]!='\\'||p[1]!='u'||hex4(p+2,&lo)<0||lo<0xdc00||lo>0xdfff)return -1;
                    p+=6;cp=0x10000+((cp-0xd800)<<10)+(lo-0xdc00);
                }else if(cp>=0xdc00&&cp<=0xdfff)return -1;
                if(emit_utf8(cp,out,cap,&o)<0)return -1;
                continue;
            }
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
