#ifndef AMIGPT_OAUTH_H
#define AMIGPT_OAUTH_H
#include <stddef.h>
#define AMIGPT_AUTH_ENDPOINT "https://auth.openai.com/api/accounts/authorize"
#define AMIGPT_TOKEN_ENDPOINT "https://auth.openai.com/api/accounts/oauth/token"
#define AMIGPT_RESOURCE "https://api.openai.com/v1"
#define AMIGPT_DYNAMIC_CLIENT "dynamic_agent_client"
#define AMIGPT_SCOPES "openid profile email offline_access resource.invoke chatgpt.tokens.use.direct"
typedef struct {
    const char *client_id;
    const char *host_id;
    const char *agent_name;
    const char *redirect_uri;
    const char *state;
    const char *nonce;
    const char *challenge;
    const char *id_token_hint;
    const char *login_hint;
} amigpt_oauth_params;
int amigpt_oauth_authorize_url(const amigpt_oauth_params *p,char *out,size_t cap);
int amigpt_oauth_code_form(const char *client_id,const char *code,const char *verifier,const char *redirect_uri,char *out,size_t cap);
int amigpt_oauth_refresh_form(const char *client_id,const char *refresh_token,char *out,size_t cap);
#endif
