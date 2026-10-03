#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include "amiga_crypto.h"
#include "session.h"
#include "models_http.h"
#include "openai_http.h"
#include "amigpt/profile.h"
#include "amigpt/models.h"

extern void __stkinit(void);
void *__stkinit_ref = __stkinit;
unsigned long __stack = 131072UL;

static amigpt_profile cli_profile;
static amigpt_model_list cli_models;
static char cli_prompt[8192];
static char cli_host_id[128];

static void usage(void)
{
    puts("AmiGPT68K v0.8-dev (public SIWC endpoints)");
    puts("Usage:");
    puts("  AmiGPT HOSTID [profile]        create/show this Amiga host ID");
    puts("  AmiGPT DEVICELOGIN [profile]   legacy Codex device-auth diagnostic only");
    puts("  AmiGPT LOGIN [profile]         native browser SIWC flow (AmiWebAuth; experimental)");
    puts("  AmiGPT REFRESH [profile]");
    puts("  AmiGPT MODELS [profile]");
    puts("  AmiGPT CHAT <model> <prompt...> [--profile <path>]");
}

static void out_delta(const char *text,void *u){(void)u;fputs(text,stdout);fflush(stdout);}
static int join_prompt(int argc,char **argv,int first,int last,char *out,size_t cap){int i;size_t used=0;if(first>=last||!out||cap<2)return -1;out[0]=0;for(i=first;i<last;i++){size_t n=strlen(argv[i]);if(used+(used?1:0)+n+1>cap)return -1;if(used)out[used++]=' ';memcpy(out+used,argv[i],n);used+=n;out[used]=0;}return 0;}

int main(int argc,char **argv)
{
    char err[512];const char *profile=AMIGPT_DEFAULT_PROFILE;int rc=5;size_t i;const char *model;int prompt_end;
    if(argc<2){usage();return 5;}
    {struct Task *tk=FindTask(0);unsigned long have=(unsigned long)((char*)tk->tc_SPUpper-(char*)tk->tc_SPLower);if(have<60000UL){fprintf(stderr,"AmiGPT: stack is only %lu bytes (need >= 60000). Run: stack 131072\n",have);return 20;}}
    if(amigpt_crypto_open(0,err,sizeof err)<0){fprintf(stderr,"INIT failed: %s\n",err);return 20;}
    if(!strcmp(argv[1],"HOSTID")){if(argc>=3)profile=argv[2];if(amigpt_session_prepare_host(profile,cli_host_id,sizeof cli_host_id,err,sizeof err)<0){fprintf(stderr,"HOSTID failed: %s\n",err);rc=20;goto done;}printf("%s\n",cli_host_id);rc=0;goto done;}
    if(!strcmp(argv[1],"DEVICELOGIN")){if(argc>=3)profile=argv[2];if(amigpt_session_device_login(profile,err,sizeof err)<0){fprintf(stderr,"DEVICELOGIN failed: %s\n",err);rc=20;goto done;}puts("DEVICELOGIN OK (legacy diagnostic profile; not valid for v0.8 MODELS/CHAT)");rc=0;goto done;}
    if(!strcmp(argv[1],"LOGIN")){if(argc>=3)profile=argv[2];if(amigpt_session_login(profile,err,sizeof err)<0){fprintf(stderr,"LOGIN failed: %s\n",err);rc=20;goto done;}puts("LOGIN OK");rc=0;goto done;}
    if(!strcmp(argv[1],"REFRESH")){if(argc>=3)profile=argv[2];if(amigpt_session_refresh(profile,err,sizeof err)<0){fprintf(stderr,"REFRESH failed: %s\n",err);rc=20;goto done;}puts("REFRESH OK");rc=0;goto done;}
    if(!strcmp(argv[1],"MODELS")){if(argc>=3)profile=argv[2];if(amigpt_session_load_ready(profile,&cli_profile,err,sizeof err)<0){fprintf(stderr,"MODELS failed: %s\n",err);rc=20;goto done;}if(amigpt_fetch_models(cli_profile.access_token,&cli_models,err,sizeof err)<0){fprintf(stderr,"MODELS failed: %s\n",err);rc=20;goto done;}for(i=0;i<cli_models.count;i++)printf("%s\t%s\n",cli_models.item[i].slug,cli_models.item[i].display_name);rc=0;goto done;}
    if(!strcmp(argv[1],"CHAT")){if(argc<4){usage();rc=5;goto done;}model=argv[2];prompt_end=argc;if(argc>=6&&!strcmp(argv[argc-2],"--profile")){profile=argv[argc-1];prompt_end=argc-2;}if(join_prompt(argc,argv,3,prompt_end,cli_prompt,sizeof cli_prompt)<0){fprintf(stderr,"CHAT failed: prompt too large or missing\n");rc=20;goto done;}if(amigpt_session_load_ready(profile,&cli_profile,err,sizeof err)<0){fprintf(stderr,"CHAT failed: %s\n",err);rc=20;goto done;}if(amigpt_openai_stream(cli_profile.access_token,model,cli_prompt,out_delta,0,err,sizeof err)<0){fprintf(stderr,"\nCHAT failed: %s\n",err);rc=20;goto done;}putchar('\n');rc=0;goto done;}
    usage();rc=5;
done:amigpt_crypto_close();return rc;
}
