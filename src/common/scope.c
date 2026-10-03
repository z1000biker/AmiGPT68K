#include "amigpt/scope.h"
#include <string.h>
#include <ctype.h>
int amigpt_scope_has(const char *s,const char *w)
{
    size_t n; const char *p;
    if(!s||!w||!*w)return 0;
    n=strlen(w); p=s;
    while(*p){
        const char *b,*e;
        while(*p&&isspace((unsigned char)*p))p++;
        b=p; while(*p&&!isspace((unsigned char)*p))p++; e=p;
        if((size_t)(e-b)==n&&!memcmp(b,w,n))return 1;
    }
    return 0;
}
