#include "amigpt/sse.h"
#include <string.h>
static int dispatch(amigpt_sse *s,amigpt_sse_cb cb,void *u)
{
    if(s->data_used&&cb){
        if(s->data[s->data_used-1]=='\n') s->data[--s->data_used]=0;
        cb(s->event[0]?s->event:"message",s->data,u);
    }
    s->event[0]=0;s->data[0]=0;s->data_used=0;return 0;
}
static int process_line(amigpt_sse *s,char *p,amigpt_sse_cb cb,void *u)
{
    char *c;
    if(!*p) return dispatch(s,cb,u);
    if(*p==':') return 0;
    c=strchr(p,':'); if(c){*c++=0;if(*c==' ')c++;}else c="";
    if(!strcmp(p,"event")){
        strncpy(s->event,c,sizeof(s->event)-1);s->event[sizeof(s->event)-1]=0;
    }else if(!strcmp(p,"data")){
        size_t n=strlen(c);
        if(s->data_used+n+2>sizeof(s->data)) return -1;
        memcpy(s->data+s->data_used,c,n);s->data_used+=n;
        s->data[s->data_used++]='\n';s->data[s->data_used]=0;
    }
    return 0;
}
void amigpt_sse_init(amigpt_sse *s){memset(s,0,sizeof(*s));}
int amigpt_sse_feed(amigpt_sse *s,const char *b,size_t n,amigpt_sse_cb cb,void *u)
{
    size_t i;
    if(!s||(!b&&n)) return -1;
    for(i=0;i<n;i++){
        char c=b[i]; if(c=='\r') continue;
        if(c=='\n'){
            s->line[s->used]=0;
            if(process_line(s,s->line,cb,u)<0) return -1;
            s->used=0;
        }else{
            if(s->used+1>=sizeof(s->line)) return -1;
            s->line[s->used++]=c;
        }
    }
    return 0;
}
