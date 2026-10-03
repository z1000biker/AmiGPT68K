#ifndef AMIGPT_PROFILE_H
#define AMIGPT_PROFILE_H
#include <stddef.h>
typedef struct {
    char host_id[128];
    char client_id[256];
    char subject[256];
    char email[384];
    char account_id[256];
    char id_token[12288];
    char access_token[8192];
    char refresh_token[8192];
    char scope[2048];
    long expires_in;
    long earliest_refresh_at;
    long saved_at;
} amigpt_profile;
void amigpt_profile_init(amigpt_profile *p);
int amigpt_profile_load(const char *path,amigpt_profile *p);
int amigpt_profile_save(const char *path,const amigpt_profile *p);
void amigpt_profile_clear_tokens(amigpt_profile *p);
#endif
