#include "openai_http.h"
#include "amiga_tls.h"
#include "amigpt/responses.h"
#include "amigpt/sse.h"
#include "amigpt/httpdec.h"
#include "amigpt/jsonlite.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    amigpt_delta_cb cb;
    void *user;
    amigpt_sse s;
    amigpt_httpdec *http;
    int parse_error;
    int completed;
    int failed;
    int incomplete;
    char terminal_code[128];
    char error_body[512];
    size_t error_used;
    char assembled[65536];
    size_t assembled_used;
    char final_text[65536];
    size_t final_used;
} stream_ctx;

static int append_buf(char *dst,size_t cap,size_t *used,const char *src)
{
    size_t n;
    if(!src)return 0;
    n=strlen(src);
    if(*used+n+1>cap)return -1;
    memcpy(dst+*used,src,n);
    *used+=n;
    dst[*used]=0;
    return 0;
}

static void on_sse(const char *event,const char *data,void *u)
{
    stream_ctx *c=(stream_ctx*)u;
    static char delta[65536],type[128],code[128],full[65536];
    int r;
    (void)event;
    type[0]=0;code[0]=0;
    if(amigpt_json_string(data,"type",type,sizeof type)!=1)return;
    if(!strcmp(type,"response.output_text.delta")){
        r=amigpt_response_delta(data,delta,sizeof delta);
        if(r==1){
            if(append_buf(c->assembled,sizeof c->assembled,&c->assembled_used,delta)<0)c->parse_error=1;
        }else if(r<0)c->parse_error=1;
    }else if(!strcmp(type,"response.output_text.done")){
        full[0]=0;
        r=amigpt_json_string(data,"text",full,sizeof full);
        if(r==1){
            if(append_buf(c->final_text,sizeof c->final_text,&c->final_used,full)<0)c->parse_error=1;
        }else if(r<0)c->parse_error=1;
    }else if(!strcmp(type,"response.completed")){
        c->completed=1;
    }else if(!strcmp(type,"response.failed")){
        c->failed=1;
        if(amigpt_json_string(data,"code",code,sizeof code)==1)snprintf(c->terminal_code,sizeof c->terminal_code,"%s",code);
    }else if(!strcmp(type,"response.incomplete")){
        c->incomplete=1;
    }else if(!strcmp(type,"error")){
        c->failed=1;
        if(amigpt_json_string(data,"message",code,sizeof code)==1)snprintf(c->terminal_code,sizeof c->terminal_code,"%s",code);
    }
}

static void on_body(const char *data,size_t len,void *u)
{
    stream_ctx *c=(stream_ctx*)u;
    if(c->http&&(c->http->status<200||c->http->status>=300)){
        size_t room=sizeof(c->error_body)-1-c->error_used;
        if(len>room)len=room;
        if(len){memcpy(c->error_body+c->error_used,data,len);c->error_used+=len;c->error_body[c->error_used]=0;}
        return;
    }
    if(amigpt_sse_feed(&c->s,data,len,on_sse,c)<0)c->parse_error=1;
}

int amigpt_openai_stream(const char *tok,const char *model,const char *prompt,amigpt_delta_cb cb,void *user,char *err,size_t errcap)
{
    amigpt_tls *t;
    static char body[16384],hdr[12288],buf[4096];
    static stream_ctx x;
    int bl,hn,n,rc;
    amigpt_httpdec h;
    const char *answer;
    if(!tok||!*tok){snprintf(err,errcap,"missing OAuth access token");return -1;}
    memset(&x,0,sizeof x);x.cb=cb;x.user=user;x.http=&h;
    amigpt_sse_init(&x.s);amigpt_httpdec_init(&h);
    bl=amigpt_responses_body(model,prompt,body,sizeof body);
    if(bl<0){snprintf(err,errcap,"request body too large");return -1;}
    hn=snprintf(hdr,sizeof hdr,"POST /v1/responses HTTP/1.1\r\nHost: api.openai.com\r\nAuthorization: Bearer %s\r\nUser-Agent: AmiGPT68K/0.8\r\nContent-Type: application/json\r\nAccept: text/event-stream\r\nConnection: close\r\nContent-Length: %d\r\n\r\n",tok,bl);
    if(hn<0||(size_t)hn>=sizeof hdr){snprintf(err,errcap,"request headers too large");return -1;}
    t=amigpt_tls_connect("api.openai.com",443,err,errcap);if(!t)return -1;
    if(amigpt_tls_write_all(t,hdr,(size_t)hn,err,errcap)<0||amigpt_tls_write_all(t,body,(size_t)bl,err,errcap)<0){amigpt_tls_close(t);return -1;}
    while((n=amigpt_tls_read(t,buf,sizeof buf,err,errcap))>0){
        rc=amigpt_httpdec_feed(&h,buf,(size_t)n,on_body,&x);
        if(rc<0||x.parse_error){snprintf(err,errcap,"HTTP/SSE parse error");amigpt_tls_close(t);return -1;}
    }
    amigpt_tls_close(t);if(n<0)return -1;
    if(h.status<200||h.status>=300){snprintf(err,errcap,"Responses HTTP status %d: %.200s",h.status,x.error_body[0]?x.error_body:"(empty body)");return -1;}
    if(x.failed){snprintf(err,errcap,"response.failed%s%s",x.terminal_code[0]?": ":"",x.terminal_code);return -1;}
    if(x.incomplete){snprintf(err,errcap,"response.incomplete");return -1;}
    if(!x.completed){snprintf(err,errcap,"stream ended without response.completed");return -1;}
    answer=x.final_used?x.final_text:x.assembled;
    if(!answer[0]){snprintf(err,errcap,"completed response contained no output text");return -1;}
    if(cb)cb(answer,user);
    return 0;
}
