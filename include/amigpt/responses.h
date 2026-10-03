#ifndef AMIGPT_RESPONSES_H
#define AMIGPT_RESPONSES_H
#include <stddef.h>
int amigpt_responses_body(const char *model,const char *prompt,char *out,size_t cap);
int amigpt_response_delta(const char *json,char *out,size_t cap);
#endif
