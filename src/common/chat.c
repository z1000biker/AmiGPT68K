#include "amigpt/chat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static char *dupstr(const char*s){size_t n=strlen(s)+1;char*p=(char*)malloc(n);if(p)memcpy(p,s,n);return p;}
static int put(char*out,size_t cap,size_t*o,const char*s){size_t n=strlen(s);if(*o+n>=cap)return -1;memcpy(out+*o,s,n);*o+=n;out[*o]=0;return 0;}
static int esc(char*out,size_t cap,size_t*o,const char*s){unsigned char c;char x[7];while((c=(unsigned char)*s++)){const char*e=0;if(c=='"')e="\\\"";else if(c=='\\')e="\\\\";else if(c=='\n')e="\\n";else if(c=='\r')e="\\r";else if(c=='\t')e="\\t";else if(c<0x20){snprintf(x,sizeof x,"\\u%04X",(unsigned)c);e=x;}if(e){if(put(out,cap,o,e)<0)return -1;}else{if(*o+1>=cap)return -1;out[(*o)++]=(char)c;out[*o]=0;}}return 0;}
void amigpt_chat_init(amigpt_chat*c){memset(c,0,sizeof *c);}
void amigpt_chat_free(amigpt_chat*c){size_t i;for(i=0;i<c->count;i++){free(c->msg[i].role);free(c->msg[i].text);}c->count=0;}
int amigpt_chat_add(amigpt_chat*c,const char*r,const char*t){if(!c||!r||!t||c->count>=AMIGPT_CHAT_MAX)return -1;c->msg[c->count].role=dupstr(r);c->msg[c->count].text=dupstr(t);if(!c->msg[c->count].role||!c->msg[c->count].text){free(c->msg[c->count].role);free(c->msg[c->count].text);return -1;}c->count++;return 0;}
int amigpt_chat_request(const amigpt_chat*c,const char*model,char*out,size_t cap){size_t i,o=0;if(!c||!model||!out||cap<2)return -1;out[0]=0;if(put(out,cap,&o,"{\"model\":\"")<0||esc(out,cap,&o,model)<0||put(out,cap,&o,"\",\"input\":[")<0)return -1;for(i=0;i<c->count;i++){if(i&&put(out,cap,&o,",")<0)return -1;if(put(out,cap,&o,"{\"role\":\"")<0||esc(out,cap,&o,c->msg[i].role)<0||put(out,cap,&o,"\",\"content\":\"")<0||esc(out,cap,&o,c->msg[i].text)<0||put(out,cap,&o,"\"}")<0)return -1;}if(put(out,cap,&o,"],\"store\":false,\"stream\":true}")<0)return -1;return (int)o;}
char *amigpt_chat_request_alloc(const amigpt_chat*c,const char*model,size_t*len_out){size_t i,need=4096;char*out;int n;if(!c||!model)return 0;need+=strlen(model)*6;for(i=0;i<c->count;i++){need+=(strlen(c->msg[i].role)+strlen(c->msg[i].text))*6+128;}out=(char*)malloc(need);if(!out)return 0;n=amigpt_chat_request(c,model,out,need);if(n<0){free(out);return 0;}if(len_out)*len_out=(size_t)n;return out;}
