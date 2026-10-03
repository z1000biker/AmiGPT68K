#include "amigpt/httpdec.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void amigpt_httpdec_init(amigpt_httpdec *h){memset(h,0,sizeof(*h));h->state=H_HEADER;}
static int ci_contains(const char *s,const char *needle){size_t n=strlen(needle);for(;*s;s++){size_t i;for(i=0;i<n;i++){if(!s[i]||tolower((unsigned char)s[i])!=tolower((unsigned char)needle[i]))break;}if(i==n)return 1;}return 0;}
static int parse_headers(amigpt_httpdec*h){char *sp;if(strncmp(h->header,"HTTP/",5))return -1;sp=strchr(h->header,' ');if(!sp)return -1;h->status=atoi(sp+1);h->chunked=ci_contains(h->header,"transfer-encoding: chunked");h->state=h->chunked?H_CHUNK_SIZE:H_BODY_CLOSE;return 0;}
static int chunk_size(amigpt_httpdec*h){char *end;unsigned long v;h->line[h->lused]=0;v=strtoul(h->line,&end,16);if(end==h->line)return -1;h->lused=0;h->chunk_left=v;h->state=v?H_CHUNK_DATA:H_DONE;return 0;}
int amigpt_httpdec_feed(amigpt_httpdec*h,const char*b,size_t n,amigpt_http_body_cb cb,void*u){size_t i=0;if(!h||(!b&&n))return -1;while(i<n){
 if(h->state==H_HEADER){char c=b[i++];if(h->hused+1>=sizeof(h->header)){h->state=H_ERROR;return -1;}h->header[h->hused++]=c;h->header[h->hused]=0;if(h->hused>=4&&!memcmp(h->header+h->hused-4,"\r\n\r\n",4)){if(parse_headers(h)<0){h->state=H_ERROR;return -1;}}}
 else if(h->state==H_BODY_CLOSE){if(cb)cb(b+i,n-i,u);i=n;}
 else if(h->state==H_CHUNK_SIZE){char c=b[i++];if(c=='\r')continue;if(c=='\n'){if(chunk_size(h)<0){h->state=H_ERROR;return -1;}}else{if(h->lused+1>=sizeof(h->line)){h->state=H_ERROR;return -1;}h->line[h->lused++]=c;}}
 else if(h->state==H_CHUNK_DATA){size_t take=n-i;if(take>h->chunk_left)take=(size_t)h->chunk_left;if(take&&cb)cb(b+i,take,u);i+=take;h->chunk_left-=(unsigned long)take;if(h->chunk_left==0)h->state=H_CHUNK_CRLF;}
 else if(h->state==H_CHUNK_CRLF){char c=b[i++];if(c=='\n')h->state=H_CHUNK_SIZE;}
 else if(h->state==H_DONE){return 1;}else return -1;
 }return h->state==H_DONE?1:0;}
