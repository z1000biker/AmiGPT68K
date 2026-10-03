#include "amigpt/profile.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void amigpt_profile_init(amigpt_profile *p){if(p)memset(p,0,sizeof *p);}
void amigpt_profile_clear_tokens(amigpt_profile *p){if(!p)return;p->id_token[0]=0;p->access_token[0]=0;p->refresh_token[0]=0;p->scope[0]=0;p->expires_in=0;p->earliest_refresh_at=0;p->saved_at=0;}
static void copyv(char *dst,size_t cap,const char *v){size_t n=strcspn(v,"\r\n");if(n>=cap)n=cap-1;memcpy(dst,v,n);dst[n]=0;}
static int setv(amigpt_profile*p,const char*k,const char*v){
#define S(name) if(!strcmp(k,#name)){copyv(p->name,sizeof p->name,v);return 1;}
    S(host_id) S(client_id) S(subject) S(email) S(account_id) S(id_token) S(access_token) S(refresh_token) S(scope)
#undef S
    if(!strcmp(k,"expires_in")){p->expires_in=strtol(v,0,10);return 1;}
    if(!strcmp(k,"earliest_refresh_at")){p->earliest_refresh_at=strtol(v,0,10);return 1;}
    if(!strcmp(k,"saved_at")){p->saved_at=strtol(v,0,10);return 1;}
    return 0;
}
int amigpt_profile_load(const char *path,amigpt_profile *p){FILE*f;static char line[16384];if(!path||!p)return -1;amigpt_profile_init(p);f=fopen(path,"rb");if(!f)return 0;while(fgets(line,sizeof line,f)){char*eq;if(line[0]=='#'||line[0]==';'||line[0]=='\r'||line[0]=='\n')continue;eq=strchr(line,'=');if(!eq)continue;*eq++=0;setv(p,line,eq);}fclose(f);return 1;}
static int w(FILE*f,const char*k,const char*v){return fprintf(f,"%s=%s\n",k,v?v:"")<0?-1:0;}
int amigpt_profile_save(const char *path,const amigpt_profile *p){char tmp[1024];FILE*f;int ok=0;if(!path||!p)return -1;if(snprintf(tmp,sizeof tmp,"%s.tmp",path)<0||(strlen(path)+4)>=sizeof tmp)return -1;f=fopen(tmp,"wb");if(!f)return -1;if(fputs("# AmiGPT profile v1\n",f)==EOF)goto out;
#define W(name) if(w(f,#name,p->name)<0)goto out
 W(host_id);W(client_id);W(subject);W(email);W(account_id);W(id_token);W(access_token);W(refresh_token);W(scope);
#undef W
 if(fprintf(f,"expires_in=%ld\nearliest_refresh_at=%ld\nsaved_at=%ld\n",p->expires_in,p->earliest_refresh_at,p->saved_at)<0)goto out;
 if(fflush(f)!=0)goto out;
 ok=1;
out: if(fclose(f)!=0)ok=0;if(!ok){remove(tmp);return -1;}remove(path);if(rename(tmp,path)!=0){remove(tmp);return -1;}return 0;}
