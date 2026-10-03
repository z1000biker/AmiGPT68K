#include "amigpt/pkce.h"
#include "amigpt/base64url.h"
#include <string.h>
int amigpt_pkce_challenge(const char*v,amigpt_sha256_fn f,char*out,size_t cap){unsigned char d[32];if(!v||!f)return -1;if(f((const unsigned char*)v,strlen(v),d)!=0)return -1;return amigpt_base64url_encode(d,32,out,cap);}
