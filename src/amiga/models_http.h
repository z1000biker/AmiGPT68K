#ifndef AMIGPT_MODELS_HTTP_H
#define AMIGPT_MODELS_HTTP_H
#include <stddef.h>
#include "amigpt/models.h"
int amigpt_fetch_models(const char *access_token,amigpt_model_list *models,char *err,size_t errcap);
#endif
