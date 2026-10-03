#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include "amiga_crypto.h"
#include "session.h"
#include "models_http.h"
#include "openai_http.h"
#include "amigpt/profile.h"
#include "amigpt/models.h"

extern void __stkinit(void);
void *__stkinit_ref_gui = __stkinit;
unsigned long __stack = 131072UL;

struct Library *IntuitionBase = 0;
struct Library *GadToolsBase = 0;
struct GfxBase *GfxBase = 0;

#define GID_MODEL   1
#define GID_PROMPT  2
#define GID_SEND    3
#define GID_CLEAR   4
#define GID_MODELS  5
#define GID_REFRESH 6

static struct Window *g_win;
static struct Gadget *g_model_gad,*g_prompt_gad;
static char g_model[128] = "gpt-6.1-sol";
static char g_prompt[1024] = "";
static char g_text[16384] = "AmiGPT68K v0.8 GUI ready.";
static size_t g_text_len;
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

static void redraw_output(void)
{
    struct RastPort *rp;
    char line[100];
    const char *p=g_text,*e;
    int y=86,lines=0;
    if(!g_win)return;
    rp=g_win->RPort;
    SetAPen(rp,0);RectFill(rp,12,78,g_win->Width-13,g_win->Height-48);
    SetAPen(rp,1);
    while(*p && lines<10){
        size_t n;
        e=strchr(p,'\n');
        if(!e)e=p+strlen(p);
        n=(size_t)(e-p);if(n>95)n=95;
        memcpy(line,p,n);line[n]=0;
        Move(rp,16,y);Text(rp,line,(ULONG)n);
        y+=13;lines++;
        p=(*e=='\n')?e+1:e;
    }
}

static void set_status(const char *s)
{
    g_text[0]=0;g_text_len=0;append_text(s);redraw_output();
}

static void stream_cb(const char *text,void *u)
{
    (void)u;
    append_text(text);
    redraw_output();
}

static int init_libs(void)
{
    IntuitionBase=OpenLibrary("intuition.library",37);
    GfxBase=(struct GfxBase*)OpenLibrary("graphics.library",37);
    GadToolsBase=OpenLibrary("gadtools.library",37);
    return IntuitionBase&&GfxBase&&GadToolsBase?0:-1;
}

static void close_libs(void)
{
    if(GadToolsBase)CloseLibrary(GadToolsBase);
    if((struct Library*)GfxBase)CloseLibrary((struct Library*)GfxBase);
    if(IntuitionBase)CloseLibrary(IntuitionBase);
}

static void do_models(void)
{
    char err[512];size_t i;
    set_status("Loading models...");
    if(amigpt_session_load_ready(AMIGPT_DEFAULT_PROFILE,&g_profile,err,sizeof err)<0){set_status(err);return;}
    if(amigpt_fetch_models(g_profile.access_token,&g_models,err,sizeof err)<0){set_status(err);return;}
    if(g_models.count){
        strncpy(g_model,g_models.item[0].slug,sizeof g_model-1);g_model[sizeof g_model-1]=0;
        GT_SetGadgetAttrs(g_model_gad,g_win,0,GTST_String,(ULONG)g_model,TAG_DONE);
    }
    g_text[0]=0;g_text_len=0;
    for(i=0;i<g_models.count && i<10;i++){
        append_text(g_models.item[i].slug);append_text("\n");
    }
    redraw_output();
}

static void do_refresh(void)
{
    char err[512];
    set_status("Refreshing credentials...");
    if(amigpt_session_refresh(AMIGPT_DEFAULT_PROFILE,err,sizeof err)<0)set_status(err);
    else set_status("REFRESH OK");
}

static void do_send(void)
{
    char err[512];
    struct StringInfo *si;
    const char *model,*prompt;
    si=(struct StringInfo*)g_model_gad->SpecialInfo;model=si&&si->Buffer?si->Buffer:g_model;
    si=(struct StringInfo*)g_prompt_gad->SpecialInfo;prompt=si&&si->Buffer?si->Buffer:g_prompt;
    g_text[0]=0;g_text_len=0;append_text("You: ");append_text(prompt);append_text("\n\nChatGPT: ");redraw_output();
    if(amigpt_session_load_ready(AMIGPT_DEFAULT_PROFILE,&g_profile,err,sizeof err)<0){append_text("\nERROR: ");append_text(err);redraw_output();return;}
    if(amigpt_openai_stream(g_profile.access_token,model,prompt,stream_cb,0,err,sizeof err)<0){append_text("\nERROR: ");append_text(err);redraw_output();return;}
    append_text("\n");redraw_output();
}

int main(void)
{
    struct Screen *scr;
    void *vi=0;
    struct Gadget *gad=0,*last=0;
    struct NewGadget ng;
    struct IntuiMessage *im;
    ULONG cls;UWORD code;struct Gadget *ig;
    char err[256];int running=1;
    struct Task *tk=FindTask(0);
    unsigned long have=(unsigned long)((char*)tk->tc_SPUpper-(char*)tk->tc_SPLower);
    if(have<60000UL){PutStr("AmiGPTGUI: stack too small. Run: stack 131072\n");return 20;}
    if(init_libs()<0){PutStr("AmiGPTGUI: unable to open Intuition/Graphics/GadTools\n");close_libs();return 20;}
    if(amigpt_crypto_open(0,err,sizeof err)<0){PutStr("AmiGPTGUI: network init failed\n");close_libs();return 20;}
    scr=LockPubScreen(0);if(!scr){amigpt_crypto_close();close_libs();return 20;}
    vi=GetVisualInfo(scr,TAG_DONE);if(!vi){UnlockPubScreen(0,scr);amigpt_crypto_close();close_libs();return 20;}
    gad=CreateContext(&last);
    memset(&ng,0,sizeof ng);ng.ng_VisualInfo=vi;ng.ng_TextAttr=scr->Font;ng.ng_Flags=PLACETEXT_LEFT;
    ng.ng_LeftEdge=72;ng.ng_TopEdge=12;ng.ng_Width=360;ng.ng_Height=16;ng.ng_GadgetText="Model";ng.ng_GadgetID=GID_MODEL;
    g_model_gad=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)g_model,GTST_MaxChars,127,TAG_DONE);
    ng.ng_LeftEdge=72;ng.ng_TopEdge=36;ng.ng_Width=500;ng.ng_GadgetText="Prompt";ng.ng_GadgetID=GID_PROMPT;
    g_prompt_gad=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)g_prompt,GTST_MaxChars,1023,TAG_DONE);
    ng.ng_LeftEdge=580;ng.ng_TopEdge=34;ng.ng_Width=52;ng.ng_GadgetText="Send";ng.ng_GadgetID=GID_SEND;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=12;ng.ng_TopEdge=218;ng.ng_Width=70;ng.ng_GadgetText="Models";ng.ng_GadgetID=GID_MODELS;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=90;ng.ng_Width=70;ng.ng_GadgetText="Refresh";ng.ng_GadgetID=GID_REFRESH;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    ng.ng_LeftEdge=168;ng.ng_Width=70;ng.ng_GadgetText="Clear";ng.ng_GadgetID=GID_CLEAR;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);
    g_win=OpenWindowTags(0,WA_Title,(ULONG)"AmiGPT68K v0.8",WA_Left,20,WA_Top,20,WA_Width,650,WA_Height,260,WA_MinWidth,500,WA_MinHeight,220,WA_MaxWidth,~0,WA_MaxHeight,~0,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_SizeGadget,TRUE,WA_Activate,TRUE,WA_Gadgets,(ULONG)gad,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW,TAG_DONE);
    UnlockPubScreen(0,scr);
    if(!g_win){FreeGadgets(gad);FreeVisualInfo(vi);amigpt_crypto_close();close_libs();return 20;}
    GT_RefreshWindow(g_win,0);g_text_len=strlen(g_text);redraw_output();
    while(running){
        Wait(1UL<<g_win->UserPort->mp_SigBit);
        while((im=GT_GetIMsg(g_win->UserPort))!=0){cls=im->Class;code=im->Code;ig=(struct Gadget*)im->IAddress;GT_ReplyIMsg(im);(void)code;
            if(cls==IDCMP_CLOSEWINDOW)running=0;
            else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(g_win);GT_EndRefresh(g_win,TRUE);redraw_output();}
            else if(cls==IDCMP_GADGETUP&&ig){
                switch(ig->GadgetID){case GID_SEND:do_send();break;case GID_CLEAR:set_status("");break;case GID_MODELS:do_models();break;case GID_REFRESH:do_refresh();break;default:break;}
            }
        }
    }
    CloseWindow(g_win);g_win=0;FreeGadgets(gad);FreeVisualInfo(vi);amigpt_crypto_close();close_libs();return 0;
}
