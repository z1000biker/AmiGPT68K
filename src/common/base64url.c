#include "amigpt/base64url.h"
#include <string.h>

static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int amigpt_base64url_encode(const unsigned char *src, size_t n, char *dst, size_t cap)
{
    size_t i=0,o=0;
    unsigned v;
    if (!dst || (!src && n)) return -1;
    while (i + 3 <= n) {
        if (o + 4 >= cap) return -1;
        v=((unsigned)src[i]<<16)|((unsigned)src[i+1]<<8)|src[i+2]; i+=3;
        dst[o++]=b64[(v>>18)&63]; dst[o++]=b64[(v>>12)&63];
        dst[o++]=b64[(v>>6)&63]; dst[o++]=b64[v&63];
    }
    if (i<n) {
        size_t rem=n-i;
        unsigned a=src[i++], bb=(i<n)?src[i++]:0;
        v=(a<<16)|(bb<<8);
        if (o + (rem==2?3:2) >= cap) return -1;
        dst[o++]=b64[(v>>18)&63]; dst[o++]=b64[(v>>12)&63];
        if (rem==2) dst[o++]=b64[(v>>6)&63];
    }
    if (o>=cap) return -1;
    dst[o]='\0';
    for (i=0;i<o;i++) { if (dst[i]=='+') dst[i]='-'; else if (dst[i]=='/') dst[i]='_'; }
    return (int)o;
}

static int d64(int c)
{
    if(c>='A'&&c<='Z')return c-'A';
    if(c>='a'&&c<='z')return c-'a'+26;
    if(c>='0'&&c<='9')return c-'0'+52;
    if(c=='-'||c=='+')return 62;
    if(c=='_'||c=='/')return 63;
    return -1;
}

int amigpt_base64url_decode(const char *src, unsigned char *dst, size_t cap, size_t *out_len)
{
    size_t n,i=0,o=0;
    unsigned acc=0,bits=0;
    if(!src||!dst)return -1;
    n=strlen(src);
    while(i<n){
        int v;
        unsigned char c=(unsigned char)src[i++];
        if(c=='=')break;
        v=d64(c); if(v<0)return -1;
        acc=(acc<<6)|(unsigned)v; bits+=6;
        if(bits>=8){
            bits-=8;
            if(o>=cap)return -1;
            dst[o++]=(unsigned char)((acc>>bits)&0xffu);
        }
    }
    if(out_len)*out_len=o;
    return 0;
}
