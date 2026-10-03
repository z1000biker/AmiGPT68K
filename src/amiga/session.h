#ifndef AMIGPT_SESSION_H
#define AMIGPT_SESSION_H
#include <stddef.h>
#include "amigpt/profile.h"
#define AMIGPT_DEFAULT_PROFILE "PROGDIR:AmiGPT.profile"
int amigpt_session_prepare_host(const char *profile_path,char *host_id,size_t hostcap,char *err,size_t errcap);
int amigpt_session_login(const char *profile_path,char *err,size_t errcap);
int amigpt_session_device_login(const char *profile_path,char *err,size_t errcap);
int amigpt_session_refresh(const char *profile_path,char *err,size_t errcap);
int amigpt_session_load_ready(const char *profile_path,amigpt_profile *profile,char *err,size_t errcap);
#endif
