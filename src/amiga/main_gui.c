#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include "amiga_crypto.h"
#include "session.h"
#include "device_auth.h"
#include "models_http.h"
#include "openai_http.h"
#include "amigpt/profile.h"
#include "amigpt/models.h"
#include "amigpt/scope.h"

extern void __stkinit(void);
void *__stkinit_ref_gui = __stkinit;
unsigned long __stack = 131072UL;

struct IntuitionBase *IntuitionBase = 0;
struct Library *GadToolsBase = 0;
struct Library *AslBase = 0;
struct GfxBase *GfxBase = 0;

#define GID_MODEL    1
#define GID_PROMPT   2
#define GID_SEND     3
#define GID_CLEAR    4
#define GID_MODELS   5
#define GID_REFRESH  6
#define GID_LOGIN    7
#define GID_IMPORT   8
#define GID_HOSTID   9

static struct Window *g_win;
static struct Gadget *g_model_gad,*g_prompt_gad,*g_send_gad,*g_models_gad,*g_refresh_gad;
static char g_model[128] = "gpt-6.1-sol";
static char g_prompt[1024] = "";
static char g_text[16384] = "AmiGPT68K v0.8 GUI ready.";
static char g_auth[512] = "Auth: checking profile...";
static size_t g_text_len;
static int g_network_ready;
static int g_auth_ready;
static amigpt_profile g_profile;
static amigpt_model_list g_models;

static void append_text(const char *s)
{
    size_t n;
    if(!s)return;
    n=strlen(s);
    if(g_text_len+n+1>=sizeof g_text){
        size_t keep=sizeof(g_text)/2;
        memmove(g_text,g_text+g_text_len-keep,keep);
        g_text_len=keep;
        g_text[g_text_len]=0;
    }
    if(n>sizeof(g_text)-g_text_len-1)n=sizeof(g_text)-g_text_len-1;
    memcpy(g_text+g_text_len,s,n);g_text_len+=n;g_text[g_text_len]=0;
}

static void draw_wrapped(struct RastPort *rp,const char *text,int x,int *py,int bottom,int maxchars)
{
    const char *p=text;
    char line[128];
    if(maxchars<12)maxchars=12;if(maxchars>120)maxchars=120;
    while(*p && *py<bottom){
        const char *nl=strchr(p,'\n');
        size_t avail=nl?(size_t)(nl-p):strlen(p),n;
        if(avail==0){*py+=13;p++;continue;}
        n=avail>(size_t)maxchars?(size_t)maxchars:avail;
        if(n==(size_t)maxchars && avail>n){
            size_t k=n;while(k>8 && p[k]!=' ' && p[k]!='\t')k--;if(k>8)n=k;
        }
        if(n>=sizeof line)n=sizeof(line)-1;
        memcpy(line,p,n);line[n]=0;
        Move(rp,x,*py);Text(rp,line,(ULONG)n);*py+=13;
        p+=n;while(*p==' '||*p=='\t')p++;
        if(*p=='\n')p++;
    }
}

static void redraw_output(void)
{
    struct RastPort *rp;
    int y,maxchars,bottom;
    if(!g_win)return;
    rp=g_win->RPort;bottom=g_win->Height-52;
    SetAPen(rp,0);RectFill(rp,12,68,g_win->Width-13,bottom);
    SetAPen(rp,1);
    Move(rp,16,80);Text(rp,g_auth,(ULONG)strlen(g_auth));
    y=101;maxchars=(g_win->Width-32)/8;
    draw_wrapped(rp,g_text,16,&y,bottom,maxchars);
}

static void set_status(const char *s)
{
    g_text[0]=0;g_text_len=0;append_text(s);redraw_output();
}

static void stream_cb(const char *text,void *u)
{
    (void)u;append_text(text);redraw_output();
}

static int init_libs(void)
{
    IntuitionBase=(struct IntuitionBase*)OpenLibrary("intuition.library",37);
    GfxBase=(struct GfxBase*)OpenLibrary("graphics.library",37);
    GadToolsBase=OpenLibrary("gadtools.library",37);
    AslBase=OpenLibrary("asl.library",37);
    return IntuitionBase&&GfxBase&&GadToolsBase&&AslBase?0:-1;
}

static void close_libs(void)
{
    if(AslBase)CloseLibrary(AslBase);
    if(GadToolsBase)CloseLibrary(GadToolsBase);
    if((struct Library*)GfxBase)CloseLibrary((struct Library*)GfxBase);
    if((struct Library*)IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);
}

static void set_chat_enabled(int enabled)
{
    g_auth_ready=enabled;
    if(!g_win)return;
    if(g_send_gad)GT_SetGadgetAttrs(g_send_gad,g_win,0,GA_Disabled,!enabled,TAG_DONE);
    if(g_models_gad)GT_SetGadgetAttrs(g_models_gad,g_win,0,GA_Disabled,!enabled,TAG_DONE);
}

static void update_auth_state(void)
{
    amigpt_profile p;char err[512];int lr;
    lr=amigpt_profile_load(AMIGPT_DEFAULT_PROFILE,&p);
    if(lr!=1){snprintf(g_auth,sizeof g_auth,"Auth: NOT SIGNED IN - use Host ID + Login/Import");set_chat_enabled(0);redraw_output();return;}
    if(!strcmp(p.client_id,AMIGPT_CODEX_CLIENT_ID)){
        snprintf(g_auth,sizeof g_auth,"Auth: LEGACY DEVICELOGIN - use Login or Import SIWC profile");set_chat_enabled(0);redraw_output();return;
    }
    if(amigpt_session_load_ready(AMIGPT_DEFAULT_PROFILE,&g_profile,err,sizeof err)==0){
        if(g_profile.email[0])snprintf(g_auth,sizeof g_auth,"Auth: SIWC signed in as %s",g_profile.email);
        else snprintf(g_auth,sizeof g_auth,"Auth: SIWC profile ready");
        set_chat_enabled(1);redraw_output();return;
    }
    snprintf(g_auth,sizeof g_auth,"Auth: profile not ready - %.220s",err);
    set_chat_enabled(0);redraw_output();
}

static int ensure_network(void)
{
    char err[512],msg[640];
    if(g_network_ready)return 0;
    err[0]=0;
    if(amigpt_crypto_open(0,err,sizeof err)<0){snprintf(msg,sizeof msg,"Network init failed: %s",err[0]?err:"unknown error");set_status(msg);return -1;}
    g_network_ready=1;return 0;
}

static void do_hostid(void)
{
    char err[512],host[128],msg[256];
    if(ensure_network()<0)return;
    if(amigpt_session_prepare_host(AMIGPT_DEFAULT_PROFILE,host,sizeof host,err,sizeof err)<0){set_status(err);return;}
    snprintf(msg,sizeof msg,"Host ID created/loaded:\n%s\n\nFor PC login copy AmiGPT.profile to the PC, run tools/amigpt_login.py --profile AmiGPT.profile, then copy it back or use Import.",host);
    set_status(msg);update_auth_state();
}

static void do_login(void)
{
    char err[512],msg[640];
    if(ensure_network()<0)return;
    set_status("Starting Sign in with ChatGPT...\nAmiWebAuth must be present in PROGDIR for native Amiga login.");
    if(amigpt_session_login(AMIGPT_DEFAULT_PROFILE,err,sizeof err)<0){snprintf(msg,sizeof msg,"LOGIN failed: %s\n\nIf AmiWebAuth is not installed, use Host ID, complete login with the PC helper, then Import the returned profile.",err);set_status(msg);update_auth_state();return;}
    set_status("LOGIN OK");update_auth_state();
}

static void do_import(void)
{
    struct FileRequester *fr;
    char path[1024],msg[640];
    amigpt_profile imported,current;int have_current;
    fr=(struct FileRequester*)AllocAslRequestTags(ASL_FileRequest,ASLFR_TitleText,(ULONG)"Import SIWC AmiGPT.profile",ASLFR_InitialFile,(ULONG)"AmiGPT.profile",ASLFR_DoPatterns,FALSE,TAG_DONE);
    if(!fr){set_status("Import failed: cannot allocate file requester");return;}
    if(!AslRequestTags(fr,TAG_DONE)){FreeAslRequest(fr);return;}
    path[0]=0;strncpy(path,(const char*)fr->fr_Drawer,sizeof path-1);path[sizeof path-1]=0;
    if(!AddPart((STRPTR)path,(STRPTR)fr->fr_File,sizeof path)){FreeAslRequest(fr);set_status("Import failed: selected path is too long");return;}
    FreeAslRequest(fr);
    if(amigpt_profile_load(path,&imported)!=1){set_status("Import failed: cannot read selected profile");return;}
    if(strncmp(imported.client_id,"oaiapp_",7)!=0 || !imported.access_token[0] || !imported.refresh_token[0] || !imported.id_token[0] || !amigpt_scope_has(imported.scope,"chatgpt.tokens.use.direct")){
        set_status("Import rejected: this is not a complete SIWC profile (oaiapp client, access/refresh/ID tokens and direct-use scope required).");return;
    }
    have_current=amigpt_profile_load(AMIGPT_DEFAULT_PROFILE,&current)==1;
    if(have_current && current.host_id[0] && imported.host_id[0] && strcmp(current.host_id,imported.host_id)!=0){
        snprintf(msg,sizeof msg,"Import rejected: host ID mismatch.\nCurrent Amiga: %.180s\nImported: %.180s",current.host_id,imported.host_id);set_status(msg);return;
    }
    if(amigpt_profile_save(AMIGPT_DEFAULT_PROFILE,&imported)<0){set_status("Import failed: cannot save PROGDIR:AmiGPT.profile");return;}
    set_status("SIWC profile imported successfully. Models and Send are now enabled.");update_auth_state();
}

static void do_models(void)
{
    char err[512];size_t i;
    if(!g_auth_ready){set_status("Sign in first: use Login or Import a SIWC profile.");return;}
    if(ensure_network()<0)return;
    set_status("Loading models...");
    if(amigpt_session_load_ready(AMIGPT_DEFAULT_PROFILE,&g_profile,err,sizeof err)<0){set_status(err);update_auth_state();return;}
    if(amigpt_fetch_models(g_profile.access_token,&g_models,err,sizeof err)<0){set_status(err);return;}
    if(g_models.count){strncpy(g_model,g_models.item[0].slug,sizeof g_model-1);g_model[sizeof g_model-1]=0;GT_SetGadgetAttrs(g_model_gad,g_win,0,GTST_String,(ULONG)g_model,TAG_DONE);}
    g_text[0]=0;g_text_len=0;for(i=0;i<g_models.count && i<20;i++){append_text(g_models.item[i].slug);append_text("\n");}redraw_output();
}

static void do_refresh(void)
{
    char err[512],msg[640];
    if(ensure_network()<0)return;
    set_status("Refreshing SIWC credentials...");
    if(amigpt_session_refresh(AMIGPT_DEFAULT_PROFILE,err,sizeof err)<0){snprintf(msg,sizeof msg,"REFRESH failed: %s",err);set_status(msg);}else set_status("REFRESH OK");
    update_auth_state();
}

static void do_send(void)
{
    char err[512];struct StringInfo *si;const char *model,*prompt;
    if(!g_auth_ready){set_status("Sign in first: use Login or Import a SIWC profile.");return;}
    if(ensure_network()<0)return;
    si=(struct StringInfo*)g_model_gad->SpecialInfo;model=si&&si->Buffer?(const char*)si->Buffer:g_model;
    si=(struct StringInfo*)g_prompt_gad->SpecialInfo;prompt=si&&si->Buffer?(const char*)si->Buffer:g_prompt;
    if(!prompt||!*prompt){set_status("Enter a prompt first.");return;}
    g_text[0]=0;g_text_len=0;append_text("You: ");append_text(prompt);append_text("\n\nChatGPT: ");redraw_output();
    if(amigpt_session_load_ready(AMIGPT_DEFAULT_PROFILE,&g_profile,err,sizeof err)<0){append_text("\nERROR: ");append_text(err);redraw_output();update_auth_state();return;}
    if(amigpt_openai_stream(g_profile.access_token,model,prompt,stream_cb,0,err,sizeof err)<0){append_text("\nERROR: ");append_text(err);redraw_output();return;}
    append_text("\n");redraw_output();
}

int main(void)
{
    struct Screen *scr;void *vi=0;struct Gadget *gad=0,*last=0;struct NewGadget ng;struct IntuiMessage *im;
    ULONG cls;UWORD code;struct Gadget *ig;int running=1,winw,winh,bottom;
    struct Task *tk=FindTask(0);unsigned long have=(unsigned long)((char*)tk->tc_SPUpper-(char*)tk->tc_SPLower);
    if(have<60000UL){PutStr("AmiGPTGUI: stack too small. Run: stack 131072\n");return 20;}
    if(init_libs()<0){PutStr("AmiGPTGUI: unable to open Intuition/Graphics/GadTools/ASL\n");close_libs();return 20;}
    scr=LockPubScreen(0);if(!scr){close_libs();return 20;}
    winw=scr->Width>780?760:scr->Width-12;if(winw<600)winw=600;
    winh=scr->Height>420?400:scr->Height-12;if(winh<260)winh=260;bottom=winh-36;
    vi=GetVisualInfo(scr,TAG_DONE);if(!vi){UnlockPubScreen(0,scr);close_libs();return 20;}
    gad=CreateContext(&last);memset(&ng,0,sizeof ng);ng.ng_VisualInfo=vi;ng.ng_TextAttr=scr->Font;
    ng.ng_Flags=PLACETEXT_LEFT;ng.ng_LeftEdge=72;ng.ng_TopEdge=12;ng.ng_Width=winw-180;ng.ng_Height=18;ng.ng_GadgetText=(UBYTE*)"Model";ng.ng_GadgetID=GID_MODEL;
    g_model_gad=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)g_model,GTST_MaxChars,127,TAG_DONE);
    ng.ng_LeftEdge=72;ng.ng_TopEdge=38;ng.ng_Width=winw-175;ng.ng_GadgetText=(UBYTE*)"Prompt";ng.ng_GadgetID=GID_PROMPT;
    g_prompt_gad=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)g_prompt,GTST_MaxChars,1023,TAG_DONE);
    ng.ng_Flags=PLACETEXT_IN;ng.ng_LeftEdge=winw-92;ng.ng_TopEdge=36;ng.ng_Width=72;ng.ng_Height=20;ng.ng_GadgetText=(UBYTE*)"Send";ng.ng_GadgetID=GID_SEND;
    g_send_gad=last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_TopEdge=bottom;ng.ng_LeftEdge=12;ng.ng_Width=70;ng.ng_GadgetText=(UBYTE*)"Host ID";ng.ng_GadgetID=GID_HOSTID;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=88;ng.ng_GadgetText=(UBYTE*)"Login";ng.ng_GadgetID=GID_LOGIN;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=164;ng.ng_GadgetText=(UBYTE*)"Import";ng.ng_GadgetID=GID_IMPORT;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=240;ng.ng_Width=76;ng.ng_GadgetText=(UBYTE*)"Refresh";ng.ng_GadgetID=GID_REFRESH;g_refresh_gad=last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=322;ng.ng_Width=72;ng.ng_GadgetText=(UBYTE*)"Models";ng.ng_GadgetID=GID_MODELS;g_models_gad=last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=400;ng.ng_Width=72;ng.ng_GadgetText=(UBYTE*)"Clear";ng.ng_GadgetID=GID_CLEAR;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    g_win=OpenWindowTags(0,WA_Title,(ULONG)"AmiGPT68K v0.8 - Sign in with ChatGPT",WA_Left,6,WA_Top,6,WA_Width,winw,WA_Height,winh,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,WA_Gadgets,(ULONG)gad,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW,TAG_DONE);
    UnlockPubScreen(0,scr);
    if(!g_win){FreeGadgets(gad);FreeVisualInfo(vi);close_libs();return 20;}
    GT_RefreshWindow(g_win,0);g_text_len=strlen(g_text);update_auth_state();redraw_output();
    while(running){
        Wait(1UL<<g_win->UserPort->mp_SigBit);
        while((im=GT_GetIMsg(g_win->UserPort))!=0){cls=im->Class;code=im->Code;ig=(struct Gadget*)im->IAddress;GT_ReplyIMsg(im);(void)code;
            if(cls==IDCMP_CLOSEWINDOW)running=0;
            else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(g_win);GT_EndRefresh(g_win,TRUE);redraw_output();}
            else if(cls==IDCMP_GADGETUP&&ig){switch(ig->GadgetID){case GID_SEND:do_send();break;case GID_CLEAR:set_status("");break;case GID_MODELS:do_models();break;case GID_REFRESH:do_refresh();break;case GID_LOGIN:do_login();break;case GID_IMPORT:do_import();break;case GID_HOSTID:do_hostid();break;default:break;}}
        }
    }
    CloseWindow(g_win);g_win=0;FreeGadgets(gad);FreeVisualInfo(vi);if(g_network_ready)amigpt_crypto_close();close_libs();return 0;
}
