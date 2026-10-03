#ifndef AMIGPT_LOGIN_FLOW_H
#define AMIGPT_LOGIN_FLOW_H
#include <stddef.h>
#include "amigpt/profile.h"
#include "oauth_exchange.h"
#include "idtoken_verify.h"
int amigpt_browser_login(const amigpt_profile *existing,amigpt_tokens *tokens,amigpt_identity *identity,char *issued_client,size_t clientcap,char *err,size_t errcap);
int amigpt_first_login(const char *host_id,amigpt_tokens *tokens,char *issued_client,size_t clientcap,char *err,size_t errcap);
#endif
