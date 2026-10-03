#ifndef AMIGPT_DEVICE_AUTH_H
#define AMIGPT_DEVICE_AUTH_H
#include <stddef.h>
#include "oauth_exchange.h"
#define AMIGPT_CODEX_CLIENT_ID "app_EMoamEEZ73f0CkXaXp7hrann"
#define AMIGPT_DEVICE_VERIFY_URL "https://auth.openai.com/codex/device"
int amigpt_device_login(amigpt_tokens *tokens,char *account_id,size_t account_cap,char *email,size_t email_cap,char *err,size_t errcap);
#endif
