#include "amigpt/models.h"
#include "amigpt/jsonlite.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Public /v1/models returns entries with an "id" field. */
int amigpt_models_parse(const char *json,amigpt_model_list *out)
{
    const char *p=json;
    if(!json||!out)return -1;
    memset(out,0,sizeof *out);
    while((p=strstr(p,"\"id\""))!=0 && out->count<AMIGPT_MAX_MODELS){
        const char *b=p,*e;char *obj;size_t n;char id[128];
        while(b>json&&*b!='{')b--;
        if(*b!='{')break;
        e=strchr(p,'}');if(!e)break;
        n=(size_t)(e-b+1);obj=(char*)malloc(n+1);if(!obj)return -1;
        memcpy(obj,b,n);obj[n]=0;id[0]=0;
        if(amigpt_json_string(obj,"id",id,sizeof id)==1&&id[0]){
            amigpt_model *m=&out->item[out->count++];
            snprintf(m->slug,sizeof m->slug,"%s",id);
            snprintf(m->display_name,sizeof m->display_name,"%s",id);
        }
        free(obj);p=e+1;
    }
    return out->count?0:-1;
}
