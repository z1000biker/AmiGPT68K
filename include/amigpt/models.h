#ifndef AMIGPT_MODELS_H
#define AMIGPT_MODELS_H
#include <stddef.h>
#define AMIGPT_MAX_MODELS 32
typedef struct {char slug[128];char display_name[192];} amigpt_model;
typedef struct {amigpt_model item[AMIGPT_MAX_MODELS];size_t count;} amigpt_model_list;
int amigpt_models_parse(const char *json,amigpt_model_list *out);
#endif
