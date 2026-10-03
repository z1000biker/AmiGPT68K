#include "amigpt/models.h"
#include "amigpt/jsonlite.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Public SIWC /v1/models returns a "models" array whose displayable entries
   use slug/display_name and visibility="list". Preserve server ordering. */
int amigpt_models_parse(const char *json,amigpt_model_list *out)
{
    const char *p=json;
    if(!json||!out)return -1;
    memset(out,0,sizeof *out);
    while((p=strstr(p,"\"slug\""))!=0 && out->count<AMIGPT_MAX_MODELS){
        const char *b=p,*e;
        char *obj;
        size_t n;
        char slug[128],name[192],vis[32];
        while(b>json&&*b!='{')b--;
        if(*b!='{')break;
        e=strchr(p,'}');
        if(!e)break;
        n=(size_t)(e-b+1);
        obj=(char*)malloc(n+1);
        if(!obj)return -1;
        memcpy(obj,b,n);obj[n]=0;
        slug[0]=name[0]=vis[0]=0;
        amigpt_json_string(obj,"slug",slug,sizeof slug);
        amigpt_json_string(obj,"display_name",name,sizeof name);
        amigpt_json_string(obj,"visibility",vis,sizeof vis);
        if(slug[0]&&(!vis[0]||!strcmp(vis,"list"))){
            amigpt_model *m=&out->item[out->count++];
            snprintf(m->slug,sizeof m->slug,"%s",slug);
            snprintf(m->display_name,sizeof m->display_name,"%s",name[0]?name:slug);
        }
        free(obj);
        p=e+1;
    }
    return out->count?0:-1;
}
