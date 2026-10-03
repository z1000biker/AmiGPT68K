#ifndef AMIGPT_JSONLITE_H
#define AMIGPT_JSONLITE_H
#include <stddef.h>
int amigpt_json_string(const char *json,const char *key,char *out,size_t cap);
int amigpt_json_bool(const char *json,const char *key,int *value);
int amigpt_json_long(const char *json,const char *key,long *value);
#endif
