#include "amigpt/urlcodec.h"
#include <ctype.h>
static int unreserved(unsigned char c){return isalnum(c)||c=='-'||c=='.'||c=='_'||c=='~';}
int amigpt_urlencode(const char *src,char *dst,size_t cap){
 static const char h[]="0123456789ABCDEF"; size_t o=0; unsigned char c;
 if(!src||!dst||cap==0)return -1;
 while((c=(unsigned char)*src++)!=0){
  if(unreserved(c)){if(o+1>=cap)return -1;dst[o++]=(char)c;}
  else {if(o+3>=cap)return -1;dst[o++]='%';dst[o++]=h[c>>4];dst[o++]=h[c&15];}
 }
 dst[o]=0; return (int)o;
}
